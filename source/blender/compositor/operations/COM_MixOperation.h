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
 * Copyright 2011, Blender Foundation.
 */

#pragma once

#include "COM_NodeOperation.h"

namespace blender::compositor {

/**
 * All this programs converts an input color to an output value.
 * it assumes we are in sRGB color space.
 */

class MixBaseOperation : public NodeOperation {
 protected:
  /**
   * Prefetched reference to the inputProgram
   */
  SocketReader *m_inputValueOperation;
  SocketReader *m_inputColor1Operation;
  SocketReader *m_inputColor2Operation;
  bool m_valueAlphaMultiply;
  bool m_useClamp;

  inline void clampIfNeeded(float color[4])
  {
    if (m_useClamp) {
      clamp_v4(color, 0.0f, 1.0f);
    }
  }

 public:
  /**
   * Default constructor
   */
  MixBaseOperation();

  /**
   * The inner loop of this operation.
   */
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;

  /**
   * Initialize the execution
   */
  void initExecution() override;

  /**
   * Deinitialize the execution
   */
  void deinitExecution() override;

  void determineResolution(unsigned int resolution[2],
                           unsigned int preferredResolution[2]) override;

  void setUseValueAlphaMultiply(const bool value)
  {
    this->m_valueAlphaMultiply = value;
  }
  inline bool useValueAlphaMultiply()
  {
    return this->m_valueAlphaMultiply;
  }
  void setUseClamp(bool value)
  {
    this->m_useClamp = value;
  }
};

class MixAddOperation : public MixBaseOperation {
 public:
  MixAddOperation();
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;
};

class MixBlendOperation : public MixBaseOperation {
 public:
  MixBlendOperation();
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;
};

class MixColorBurnOperation : public MixBaseOperation {
 public:
  MixColorBurnOperation();
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;
};

class MixColorOperation : public MixBaseOperation {
 public:
  MixColorOperation();
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;
};

class MixDarkenOperation : public MixBaseOperation {
 public:
  MixDarkenOperation();
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;
};

class MixDifferenceOperation : public MixBaseOperation {
 public:
  MixDifferenceOperation();
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;
};

class MixDivideOperation : public MixBaseOperation {
 public:
  MixDivideOperation();
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;
};

class MixDodgeOperation : public MixBaseOperation {
 public:
  MixDodgeOperation();
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;
};

class MixGlareOperation : public MixBaseOperation {
 public:
  MixGlareOperation();
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;
};

class MixHueOperation : public MixBaseOperation {
 public:
  MixHueOperation();
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;
};

class MixLightenOperation : public MixBaseOperation {
 public:
  MixLightenOperation();
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;
};

class MixLinearLightOperation : public MixBaseOperation {
 public:
  MixLinearLightOperation();
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;
};

class MixMultiplyOperation : public MixBaseOperation {
 public:
  MixMultiplyOperation();
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;
};

class MixOverlayOperation : public MixBaseOperation {
 public:
  MixOverlayOperation();
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;
};

class MixSaturationOperation : public MixBaseOperation {
 public:
  MixSaturationOperation();
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;
};

class MixScreenOperation : public MixBaseOperation {
 public:
  MixScreenOperation();
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;
};

class MixSoftLightOperation : public MixBaseOperation {
 public:
  MixSoftLightOperation();
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;
};

class MixSubtractOperation : public MixBaseOperation {
 public:
  MixSubtractOperation();
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;
};

class MixValueOperation : public MixBaseOperation {
 public:
  MixValueOperation();
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;
};

}  // namespace blender::compositor
