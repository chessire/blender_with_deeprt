/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2020 Blender Foundation
 * All rights reserved.
 */

/** \file
 * \ingroup bgpencil
 */

#include "BLI_math_vector.h"

#include "DNA_gpencil_types.h"
#include "DNA_material_types.h"
#include "DNA_object_types.h"
#include "DNA_scene_types.h"
#include "DNA_screen_types.h"
#include "DNA_view3d_types.h"

#include "BKE_context.h"
#include "BKE_gpencil.h"
#include "BKE_gpencil_geom.h"
#include "BKE_main.h"
#include "BKE_material.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_query.h"

#include "ED_gpencil.h"
#include "ED_view3d.h"

#ifdef WIN32
#  include "utfconv.h"
#endif

#include "UI_view2d.h"

#include "gpencil_io.h"
#include "gpencil_io_export_pdf.hh"

namespace blender ::io ::gpencil {

static void error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void *UNUSED(user_data))
{
  printf("ERROR: error_no=%04X, detail_no=%u\n", (HPDF_UINT)error_no, (HPDF_UINT)detail_no);
}

/* Constructor. */
GpencilExporterPDF::GpencilExporterPDF(const char *filename, const GpencilIOParams *iparams)
    : GpencilExporter(iparams)
{
  filename_set(filename);

  invert_axis_[0] = false;
  invert_axis_[1] = false;

  pdf_ = nullptr;
  page_ = nullptr;
  gstate_ = nullptr;
}

bool GpencilExporterPDF::new_document()
{
  return create_document();
}

bool GpencilExporterPDF::add_newpage()
{
  return add_page();
}

bool GpencilExporterPDF::add_body()
{
  export_gpencil_layers();
  return true;
}

bool GpencilExporterPDF::write()
{
  /* Support unicode character paths on Windows. */
  HPDF_STATUS res = 0;
  /* TODO: It looks libharu does not support unicode. */
  //#ifdef WIN32
  //  char filename_cstr[FILE_MAX];
  //  BLI_strncpy(filename_cstr, filename_, FILE_MAX);
  //
  //  UTF16_ENCODE(filename_cstr);
  //  std::wstring wstr(filename_cstr_16);
  //  res = HPDF_SaveToFile(pdf_, wstr.c_str());
  //
  //  UTF16_UN_ENCODE(filename_cstr);
  //#else
  res = HPDF_SaveToFile(pdf_, filename_);
  //#endif

  return (res == 0) ? true : false;
}

/* Create pdf document. */
bool GpencilExporterPDF::create_document()
{
  pdf_ = HPDF_New(error_handler, nullptr);
  if (!pdf_) {
    std::cout << "error: cannot create PdfDoc object\n";
    return false;
  }
  return true;
}

/* Add page. */
bool GpencilExporterPDF::add_page()
{
  /* Add a new page object. */
  page_ = HPDF_AddPage(pdf_);
  if (!pdf_) {
    std::cout << "error: cannot create PdfPage\n";
    return false;
  }

  HPDF_Page_SetWidth(page_, render_x_);
  HPDF_Page_SetHeight(page_, render_y_);

  return true;
}

/* Main layer loop. */
void GpencilExporterPDF::export_gpencil_layers()
{
  /* If is doing a set of frames, the list of objects can change for each frame. */
  create_object_list();

  const bool is_normalized = ((params_.flag & GP_EXPORT_NORM_THICKNESS) != 0);

  for (ObjectZ &obz : ob_list_) {
    Object *ob = obz.ob;

    /* Use evaluated version to get strokes with modifiers. */
    Object *ob_eval_ = (Object *)DEG_get_evaluated_id(depsgraph_, &ob->id);
    bGPdata *gpd_eval = (bGPdata *)ob_eval_->data;

    LISTBASE_FOREACH (bGPDlayer *, gpl, &gpd_eval->layers) {
      if (gpl->flag & GP_LAYER_HIDE) {
        continue;
      }
      prepare_layer_export_matrix(ob, gpl);

      bGPDframe *gpf = gpl->actframe;
      if ((gpf == nullptr) || (gpf->strokes.first == nullptr)) {
        continue;
      }

      LISTBASE_FOREACH (bGPDstroke *, gps, &gpf->strokes) {
        if (gps->totpoints < 2) {
          continue;
        }
        if (!ED_gpencil_stroke_material_visible(ob, gps)) {
          continue;
        }
        /* Duplicate the stroke to apply any layer thickness change. */
        bGPDstroke *gps_duplicate = BKE_gpencil_stroke_duplicate(gps, true, false);
        MaterialGPencilStyle *gp_style = BKE_gpencil_material_settings(ob,
                                                                       gps_duplicate->mat_nr + 1);

        const bool is_stroke = ((gp_style->flag & GP_MATERIAL_STROKE_SHOW) &&
                                (gp_style->stroke_rgba[3] > GPENCIL_ALPHA_OPACITY_THRESH));
        const bool is_fill = ((gp_style->flag & GP_MATERIAL_FILL_SHOW) &&
                              (gp_style->fill_rgba[3] > GPENCIL_ALPHA_OPACITY_THRESH));
        prepare_stroke_export_colors(ob, gps_duplicate);

        /* Apply layer thickness change. */
        gps_duplicate->thickness += gpl->line_change;
        /* Apply object scale to thickness. */
        gps_duplicate->thickness *= mat4_to_scale(ob->obmat);
        CLAMP_MIN(gps_duplicate->thickness, 1.0f);
        /* Fill. */
        if ((is_fill) && (params_.flag & GP_EXPORT_FILL)) {
          /* Fill is exported as polygon for fill and stroke in a different shape. */
          export_stroke_to_polyline(gpl, gps_duplicate, is_stroke, true, false);
        }

        /* Stroke. */
        if (is_stroke) {
          if (is_normalized) {
            export_stroke_to_polyline(gpl, gps_duplicate, is_stroke, false, true);
          }
          else {
            bGPDstroke *gps_perimeter = BKE_gpencil_stroke_perimeter_from_view(
                rv3d_, gpd_, gpl, gps_duplicate, 3, diff_mat_.values);

            /* Sample stroke. */
            if (params_.stroke_sample > 0.0f) {
              BKE_gpencil_stroke_sample(gpd_eval, gps_perimeter, params_.stroke_sample, false);
            }

            export_stroke_to_polyline(gpl, gps_perimeter, is_stroke, false, false);

            BKE_gpencil_free_stroke(gps_perimeter);
          }
        }
        BKE_gpencil_free_stroke(gps_duplicate);
      }
    }
  }
}

/**
 * Export a stroke using polyline or polygon
 * \param do_fill: True if the stroke is only fill
 */
void GpencilExporterPDF::export_stroke_to_polyline(bGPDlayer *gpl,
                                                   bGPDstroke *gps,
                                                   const bool is_stroke,
                                                   const bool do_fill,
                                                   const bool normalize)
{
  const bool cyclic = ((gps->flag & GP_STROKE_CYCLIC) != 0);
  const float avg_pressure = BKE_gpencil_stroke_average_pressure_get(gps);

  /* Get the thickness in pixels using a simple 1 point stroke. */
  bGPDstroke *gps_temp = BKE_gpencil_stroke_duplicate(gps, false, false);
  gps_temp->totpoints = 1;
  gps_temp->points = (bGPDspoint *)MEM_callocN(sizeof(bGPDspoint), "gp_stroke_points");
  const bGPDspoint *pt_src = &gps->points[0];
  bGPDspoint *pt_dst = &gps_temp->points[0];
  copy_v3_v3(&pt_dst->x, &pt_src->x);
  pt_dst->pressure = avg_pressure;

  const float radius = stroke_point_radius_get(gpl, gps_temp);

  BKE_gpencil_free_stroke(gps_temp);

  color_set(gpl, do_fill);

  if (is_stroke && !do_fill) {
    HPDF_Page_SetLineJoin(page_, HPDF_ROUND_JOIN);
    HPDF_Page_SetLineWidth(page_, MAX2((radius * 2.0f) - gpl->line_change, 1.0f));
  }

  /* Loop all points. */
  for (const int i : IndexRange(gps->totpoints)) {
    bGPDspoint *pt = &gps->points[i];
    const float2 screen_co = gpencil_3D_point_to_2D(&pt->x);
    if (i == 0) {
      HPDF_Page_MoveTo(page_, screen_co.x, screen_co.y);
    }
    else {
      HPDF_Page_LineTo(page_, screen_co.x, screen_co.y);
    }
  }
  /* Close cyclic */
  if (cyclic) {
    HPDF_Page_ClosePath(page_);
  }

  if (do_fill || !normalize) {
    HPDF_Page_Fill(page_);
  }
  else {
    HPDF_Page_Stroke(page_);
  }

  HPDF_Page_GRestore(page_);
}

/**
 * Set color
 * @param do_fill: True if the stroke is only fill
 */
void GpencilExporterPDF::color_set(bGPDlayer *gpl, const bool do_fill)
{
  const float fill_opacity = fill_color_[3] * gpl->opacity;
  const float stroke_opacity = stroke_color_[3] * stroke_average_opacity_get() * gpl->opacity;

  HPDF_Page_GSave(page_);
  gstate_ = HPDF_CreateExtGState(pdf_);

  float col[3];
  if (do_fill) {
    interp_v3_v3v3(col, fill_color_, gpl->tintcolor, gpl->tintcolor[3]);
    linearrgb_to_srgb_v3_v3(col, col);
    CLAMP3(col, 0.0f, 1.0f);

    HPDF_ExtGState_SetAlphaFill(gstate_, clamp_f(fill_opacity, 0.0f, 1.0f));
    HPDF_Page_SetRGBFill(page_, col[0], col[1], col[2]);
  }
  else {
    interp_v3_v3v3(col, stroke_color_, gpl->tintcolor, gpl->tintcolor[3]);
    linearrgb_to_srgb_v3_v3(col, col);
    CLAMP3(col, 0.0f, 1.0f);

    HPDF_ExtGState_SetAlphaFill(gstate_, clamp_f(stroke_opacity, 0.0f, 1.0f));
    HPDF_ExtGState_SetAlphaStroke(gstate_, clamp_f(stroke_opacity, 0.0f, 1.0f));
    HPDF_Page_SetRGBFill(page_, col[0], col[1], col[2]);
    HPDF_Page_SetRGBStroke(page_, col[0], col[1], col[2]);
  }
  HPDF_Page_SetExtGState(page_, gstate_);
}
}  // namespace blender::io::gpencil
