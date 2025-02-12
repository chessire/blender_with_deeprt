/*
 * Copyright 2011-2013 Blender Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "render/colorspace.h"
#include "render/image.h"
#include "render/image_vdb.h"
#include "render/object.h"
#include "render/volume.h"

#include "blender/blender_sync.h"
#include "blender/blender_util.h"

#ifdef WITH_OPENVDB
#  include <openvdb/openvdb.h>
openvdb::GridBase::ConstPtr BKE_volume_grid_openvdb_for_read(const struct Volume *volume,
                                                             struct VolumeGrid *grid);
#endif

CCL_NAMESPACE_BEGIN

/* TODO: verify this is not loading unnecessary attributes. */
class BlenderSmokeLoader : public ImageLoader {
 public:
  BlenderSmokeLoader(BL::Object &b_ob, AttributeStandard attribute)
      : b_domain(object_fluid_gas_domain_find(b_ob)), attribute(attribute)
  {
    BL::Mesh b_mesh(b_ob.data());
    mesh_texture_space(b_mesh, texspace_loc, texspace_size);
  }

  bool load_metadata(const ImageDeviceFeatures &, ImageMetaData &metadata) override
  {
    if (!b_domain) {
      return false;
    }

    if (attribute == ATTR_STD_VOLUME_DENSITY || attribute == ATTR_STD_VOLUME_FLAME ||
        attribute == ATTR_STD_VOLUME_HEAT || attribute == ATTR_STD_VOLUME_TEMPERATURE) {
      metadata.type = IMAGE_DATA_TYPE_FLOAT;
      metadata.channels = 1;
    }
    else if (attribute == ATTR_STD_VOLUME_COLOR) {
      metadata.type = IMAGE_DATA_TYPE_FLOAT4;
      metadata.channels = 4;
    }
    else if (attribute == ATTR_STD_VOLUME_VELOCITY) {
      metadata.type = IMAGE_DATA_TYPE_FLOAT4;
      metadata.channels = 3;
    }
    else {
      return false;
    }

    int3 resolution = get_int3(b_domain.domain_resolution());
    int amplify = (b_domain.use_noise()) ? b_domain.noise_scale() : 1;

    /* Velocity and heat data is always low-resolution. */
    if (attribute == ATTR_STD_VOLUME_VELOCITY || attribute == ATTR_STD_VOLUME_HEAT) {
      amplify = 1;
    }

    metadata.width = resolution.x * amplify;
    metadata.height = resolution.y * amplify;
    metadata.depth = resolution.z * amplify;

    /* Create a matrix to transform from object space to mesh texture space.
     * This does not work with deformations but that can probably only be done
     * well with a volume grid mapping of coordinates. */
    metadata.transform_3d = transform_translate(-texspace_loc) * transform_scale(texspace_size);
    metadata.use_transform_3d = true;

    return true;
  }

  bool load_pixels(const ImageMetaData &, void *pixels, const size_t, const bool) override
  {
    if (!b_domain) {
      return false;
    }
#ifdef WITH_FLUID
    int3 resolution = get_int3(b_domain.domain_resolution());
    int length, amplify = (b_domain.use_noise()) ? b_domain.noise_scale() : 1;

    /* Velocity and heat data is always low-resolution. */
    if (attribute == ATTR_STD_VOLUME_VELOCITY || attribute == ATTR_STD_VOLUME_HEAT) {
      amplify = 1;
    }

    const int width = resolution.x * amplify;
    const int height = resolution.y * amplify;
    const int depth = resolution.z * amplify;
    const size_t num_pixels = ((size_t)width) * height * depth;

    float *fpixels = (float *)pixels;

    if (attribute == ATTR_STD_VOLUME_DENSITY) {
      FluidDomainSettings_density_grid_get_length(&b_domain.ptr, &length);
      if (length == num_pixels) {
        FluidDomainSettings_density_grid_get(&b_domain.ptr, fpixels);
        return true;
      }
    }
    else if (attribute == ATTR_STD_VOLUME_FLAME) {
      /* this is in range 0..1, and interpreted by the OpenGL smoke viewer
       * as 1500..3000 K with the first part faded to zero density */
      FluidDomainSettings_flame_grid_get_length(&b_domain.ptr, &length);
      if (length == num_pixels) {
        FluidDomainSettings_flame_grid_get(&b_domain.ptr, fpixels);
        return true;
      }
    }
    else if (attribute == ATTR_STD_VOLUME_COLOR) {
      /* the RGB is "premultiplied" by density for better interpolation results */
      FluidDomainSettings_color_grid_get_length(&b_domain.ptr, &length);
      if (length == num_pixels * 4) {
        FluidDomainSettings_color_grid_get(&b_domain.ptr, fpixels);
        return true;
      }
    }
    else if (attribute == ATTR_STD_VOLUME_VELOCITY) {
      FluidDomainSettings_velocity_grid_get_length(&b_domain.ptr, &length);
      if (length == num_pixels * 3) {
        FluidDomainSettings_velocity_grid_get(&b_domain.ptr, fpixels);
        return true;
      }
    }
    else if (attribute == ATTR_STD_VOLUME_HEAT) {
      FluidDomainSettings_heat_grid_get_length(&b_domain.ptr, &length);
      if (length == num_pixels) {
        FluidDomainSettings_heat_grid_get(&b_domain.ptr, fpixels);
        return true;
      }
    }
    else if (attribute == ATTR_STD_VOLUME_TEMPERATURE) {
      FluidDomainSettings_temperature_grid_get_length(&b_domain.ptr, &length);
      if (length == num_pixels) {
        FluidDomainSettings_temperature_grid_get(&b_domain.ptr, fpixels);
        return true;
      }
    }
    else {
      fprintf(stderr,
              "Cycles error: unknown volume attribute %s, skipping\n",
              Attribute::standard_name(attribute));
      fpixels[0] = 0.0f;
      return false;
    }
#else
    (void)pixels;
#endif
    fprintf(stderr, "Cycles error: unexpected smoke volume resolution, skipping\n");
    return false;
  }

  string name() const override
  {
    return Attribute::standard_name(attribute);
  }

  bool equals(const ImageLoader &other) const override
  {
    const BlenderSmokeLoader &other_loader = (const BlenderSmokeLoader &)other;
    return b_domain == other_loader.b_domain && attribute == other_loader.attribute;
  }

  BL::FluidDomainSettings b_domain;
  float3 texspace_loc, texspace_size;
  AttributeStandard attribute;
};

static void sync_smoke_volume(Scene *scene, BL::Object &b_ob, Volume *volume, float frame)
{
  BL::FluidDomainSettings b_domain = object_fluid_gas_domain_find(b_ob);
  if (!b_domain) {
    return;
  }

  AttributeStandard attributes[] = {ATTR_STD_VOLUME_DENSITY,
                                    ATTR_STD_VOLUME_COLOR,
                                    ATTR_STD_VOLUME_FLAME,
                                    ATTR_STD_VOLUME_HEAT,
                                    ATTR_STD_VOLUME_TEMPERATURE,
                                    ATTR_STD_VOLUME_VELOCITY,
                                    ATTR_STD_NONE};

  for (int i = 0; attributes[i] != ATTR_STD_NONE; i++) {
    AttributeStandard std = attributes[i];
    if (!volume->need_attribute(scene, std)) {
      continue;
    }

    volume->set_clipping(b_domain.clipping());

    Attribute *attr = volume->attributes.add(std);

    ImageLoader *loader = new BlenderSmokeLoader(b_ob, std);
    ImageParams params;
    params.frame = frame;

    attr->data_voxel() = scene->image_manager->add_image(loader, params);
  }
}

class BlenderVolumeLoader : public VDBImageLoader {
 public:
  BlenderVolumeLoader(BL::BlendData &b_data, BL::Volume &b_volume, const string &grid_name)
      : VDBImageLoader(grid_name), b_volume(b_volume)
  {
    b_volume.grids.load(b_data.ptr.data);

#ifdef WITH_OPENVDB
    for (BL::VolumeGrid &b_volume_grid : b_volume.grids) {
      if (b_volume_grid.name() == grid_name) {
        const bool unload = !b_volume_grid.is_loaded();

        ::Volume *volume = (::Volume *)b_volume.ptr.data;
        VolumeGrid *volume_grid = (VolumeGrid *)b_volume_grid.ptr.data;
        grid = BKE_volume_grid_openvdb_for_read(volume, volume_grid);

        if (unload) {
          b_volume_grid.unload();
        }

        break;
      }
    }
#endif
  }

  BL::Volume b_volume;
};

static void sync_volume_object(BL::BlendData &b_data,
                               BL::Object &b_ob,
                               Scene *scene,
                               Volume *volume)
{
  BL::Volume b_volume(b_ob.data());
  b_volume.grids.load(b_data.ptr.data);

  BL::VolumeRender b_render(b_volume.render());

  volume->set_clipping(b_render.clipping());
  volume->set_step_size(b_render.step_size());
  volume->set_object_space((b_render.space() == BL::VolumeRender::space_OBJECT));

  /* Find grid with matching name. */
  for (BL::VolumeGrid &b_grid : b_volume.grids) {
    ustring name = ustring(b_grid.name());
    AttributeStandard std = ATTR_STD_NONE;

    if (name == Attribute::standard_name(ATTR_STD_VOLUME_DENSITY)) {
      std = ATTR_STD_VOLUME_DENSITY;
    }
    else if (name == Attribute::standard_name(ATTR_STD_VOLUME_COLOR)) {
      std = ATTR_STD_VOLUME_COLOR;
    }
    else if (name == Attribute::standard_name(ATTR_STD_VOLUME_FLAME)) {
      std = ATTR_STD_VOLUME_FLAME;
    }
    else if (name == Attribute::standard_name(ATTR_STD_VOLUME_HEAT)) {
      std = ATTR_STD_VOLUME_HEAT;
    }
    else if (name == Attribute::standard_name(ATTR_STD_VOLUME_TEMPERATURE)) {
      std = ATTR_STD_VOLUME_TEMPERATURE;
    }
    else if (name == Attribute::standard_name(ATTR_STD_VOLUME_VELOCITY)) {
      std = ATTR_STD_VOLUME_VELOCITY;
    }

    if ((std != ATTR_STD_NONE && volume->need_attribute(scene, std)) ||
        volume->need_attribute(scene, name)) {
      Attribute *attr = (std != ATTR_STD_NONE) ?
                            volume->attributes.add(std) :
                            volume->attributes.add(name, TypeDesc::TypeFloat, ATTR_ELEMENT_VOXEL);

      ImageLoader *loader = new BlenderVolumeLoader(b_data, b_volume, name.string());
      ImageParams params;
      params.frame = b_volume.grids.frame();

      attr->data_voxel() = scene->image_manager->add_image(loader, params, false);
    }
  }
}

void BlenderSync::sync_volume(BL::Object &b_ob, Volume *volume)
{
  volume->clear(true);

  if (view_layer.use_volumes) {
    if (b_ob.type() == BL::Object::type_VOLUME) {
      /* Volume object. Create only attributes, bounding mesh will then
       * be automatically generated later. */
      sync_volume_object(b_data, b_ob, scene, volume);
    }
    else {
      /* Smoke domain. */
      sync_smoke_volume(scene, b_ob, volume, b_scene.frame_current());
    }
  }

  /* Tag update. */
  volume->tag_update(scene, true);
}

CCL_NAMESPACE_END
