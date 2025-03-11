/*
 * ViSP, open source Visual Servoing Platform software.
 * Copyright (C) 2005 - 2024 by Inria. All rights reserved.
 *
 * This software is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file LICENSE.txt at the root directory of this source
 * distribution for additional information about the GNU GPL.
 *
 * For using ViSP with software that can not be combined with the GNU
 * GPL, please contact Inria about acquiring a ViSP Professional
 * Edition License.
 *
 * See https://visp.inria.fr for more information.
 *
 * This software was developed at:
 * Inria Rennes - Bretagne Atlantique
 * Campus Universitaire de Beaulieu
 * 35042 Rennes Cedex
 * France
 *
 * If you have questions regarding the use of this file, please contact
 * Inria at visp@inria.fr
 *
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <visp3/ar/vpPanda3DCommonFilters.h>
#include <visp3/ar/vpPanda3DRGBRenderer.h>

#if defined(VISP_HAVE_PANDA3D)

#include "windowFramework.h"
#include "graphicsOutput.h"


BEGIN_VISP_NAMESPACE
const std::string vpPanda3DLuminanceFilter::FRAGMENT_SHADER =
"#version 330\n"
"in vec2 texcoords;\n"
"uniform sampler2D p3d_Texture0;\n"
"out vec4 p3d_FragData;\n"
"void main() {\n"
"  vec4 v = texture(p3d_Texture0, texcoords);\n"
"  p3d_FragData.b = 0.299 * v.r + 0.587 * v.g + 0.114 * v.b;\n"
"}\n";

vpPanda3DLuminanceFilter::vpPanda3DLuminanceFilter(const std::string &name, std::shared_ptr<vpPanda3DRGBRenderer> inputRenderer, bool isOutput)
  : vpPanda3DPostProcessFilter(name, inputRenderer, isOutput, std::string(vpPanda3DLuminanceFilter::FRAGMENT_SHADER))
{ }
FrameBufferProperties vpPanda3DLuminanceFilter::getBufferProperties() const
{
  FrameBufferProperties fbp;
  fbp.set_depth_bits(0);
  fbp.set_rgba_bits(0, 0, 8, 0);
  fbp.set_float_color(false);
  return fbp;
}
void vpPanda3DLuminanceFilter::getRender(vpImage<unsigned char> &I) const
{
  vpPanda3DPostProcessFilter::getRenderBasic(I);
}

const std::string  vpPanda3DGaussianBlur::FRAGMENT_SHADER =
"#version 330\n"
"in vec2 texcoords;\n"
"uniform sampler2D p3d_Texture0;\n"
"uniform vec2 dp; // 1 divided by number of pixels\n"
"const float kernel[25] = float[25](\n"
"  2, 4, 5, 4, 2,\n"
"  4, 9, 12, 9, 4,\n"
"  5, 12, 15, 12, 5,\n"
"  4, 9, 12, 9, 4,\n"
"  2, 4, 5, 4, 2\n"
");\n"
"const float normalize = 1 / 159.0;\n"
"vec2 offset[25] = vec2[25](\n"
"  vec2(-2*dp.x,-2*dp.y),  vec2(-dp.x,-2*dp.y),  vec2(0,-2*dp.y),    vec2(dp.x,-2*dp.y), vec2(2*dp.x,-2*dp.y),\n"
"  vec2(-2*dp.x,-dp.y),    vec2(-dp.x, -dp.y),   vec2(0.0, -dp.y),   vec2(dp.x, -dp.y),  vec2(2*dp.x,-dp.y),\n"
"  vec2(-2*dp.x,0.0),      vec2(-dp.x, 0.0),     vec2(0.0, 0.0),     vec2(dp.x, 0.0),    vec2(2*dp.x,0.0),\n"
"  vec2(-2*dp.x, dp.y),    vec2(-dp.x, dp.y),    vec2(0.0, dp.y),    vec2(dp.x, dp.y),   vec2(2*dp.x, dp.y),\n"
"  vec2(-2*dp.x, 2*dp.y),  vec2(-dp.x, 2*dp.y),  vec2(0.0, 2*dp.y),  vec2(dp.x, 2*dp.y), vec2(2*dp.x, 2*dp.y)\n"
");\n"
"out vec4 p3d_FragData;\n"
"void main() {\n"
"  float v = 0.f;\n"
"  for(int i = 0; i < 25; ++i) {\n"
"    v += kernel[i] * texture(p3d_Texture0, texcoords + offset[i]).b;\n"
"  }\n"
"  p3d_FragData.b = v * normalize;\n"
"}\n";

vpPanda3DGaussianBlur::vpPanda3DGaussianBlur(const std::string &name, std::shared_ptr<vpPanda3DBaseRenderer> inputRenderer, bool isOutput)
  : vpPanda3DPostProcessFilter(name, inputRenderer, isOutput, vpPanda3DGaussianBlur::FRAGMENT_SHADER)
{ }

FrameBufferProperties vpPanda3DGaussianBlur::getBufferProperties() const
{
  FrameBufferProperties fbp;
  fbp.set_depth_bits(0);
  fbp.set_rgba_bits(0, 0, 8, 0);
  fbp.set_float_color(false);
  return fbp;
}

void vpPanda3DGaussianBlur::getRender(vpImage<unsigned char> &I) const
{
  vpPanda3DPostProcessFilter::getRenderBasic(I);
}

const std::string vpPanda3DCanny::FRAGMENT_SHADER =
"#version 330\n"
"in vec2 texcoords;\n"
"uniform sampler2D p3d_Texture0;\n"
"uniform vec2 dp; // 1 divided by number of pixels\n"
"uniform float edgeThreshold;\n"
"const float kernel[9] = float[9](\n"
"  0.0, 1.0, 0.0,\n"
"  1.0,-4.0, 1.0,\n"
"  0.0, 1.0, 0.0\n"
");\n"
"const float kernel_h[9] = float[9](\n"
"  -1.0, 0.0, 1.0,\n"
"  -2.0, 0.0, 2.0,\n"
"  -1.0, 0.0, 1.0\n"
");\n"
"const float kernel_v[9] = float[9](\n"
"  -1.0, -2.0, -1.0,\n"
"  0.0, 0.0, 0.0,\n"
"  1.0, 2.0, 1.0\n"
");\n"
"vec2 offset[9] = vec2[9](\n"
"  vec2(-dp.x, -dp.y), vec2(0.0, -dp.y), vec2(dp.x, -dp.y),\n"
"  vec2(-dp.x, 0.0),   vec2(0.0, 0.0),   vec2(dp.x, 0.0),\n"
"  vec2(-dp.x, dp.y),  vec2(0.0, dp.y),  vec2(dp.x, dp.y)\n"
");\n"
"out vec4 p3d_FragData;\n"
"void main() {\n"
"  float sum = 0.f;\n"
"  for(int i = 0; i < 9; ++i) {\n"
"    float pix = texture(p3d_Texture0, texcoords + offset[i]).b;\n"
"    sum += pix * kernel[i];\n"
"  }\n"
"  if(abs(sum * 255.f) > edgeThreshold) {\n"
"    float sum_h = 0.f;\n"
"    float sum_v = 0.f;\n"
"    for(int i = 0; i < 9; ++i) {\n"
"      float pix = texture(p3d_Texture0, texcoords + offset[i]).b;\n"
"      sum_h += pix * kernel_h[i];\n"
"      sum_v += pix * kernel_v[i];\n"
"    }\n"
"    vec2 orientationAndValid = sum_h * sum_h + sum_v *  sum_v > 0 ? vec2(-atan(sum_v/sum_h), 1.f) : vec2(0.f, 0.f);\n"
"    p3d_FragData = vec4(sum_h, sum_v, orientationAndValid.x, orientationAndValid.y);\n"
"  } else {\n"
"    p3d_FragData = vec4(0.f, 0.f, 0.f, 0.f);\n"
"  }\n"
"}\n";

vpPanda3DCanny::vpPanda3DCanny(const std::string &name, std::shared_ptr<vpPanda3DBaseRenderer> inputRenderer, bool isOutput, float edgeThreshold)
  : vpPanda3DPostProcessFilter(name, inputRenderer, isOutput, vpPanda3DCanny::FRAGMENT_SHADER), m_edgeThreshold(edgeThreshold)
{ }

void vpPanda3DCanny::setupScene()
{
  vpPanda3DPostProcessFilter::setupScene();
  m_renderRoot.set_shader_input("edgeThreshold", LVector2f(m_edgeThreshold));
}

void vpPanda3DCanny::setEdgeThreshold(float edgeThreshold)
{
  m_edgeThreshold = edgeThreshold;
  m_renderRoot.set_shader_input("edgeThreshold", LVector2f(m_edgeThreshold));
}

FrameBufferProperties vpPanda3DCanny::getBufferProperties() const
{
  FrameBufferProperties fbp;
  fbp.set_depth_bits(0);
  fbp.set_rgba_bits(32, 32, 32, 32);
  fbp.set_float_color(true);
  return fbp;
}

void vpPanda3DCanny::getRender(vpImage<vpRGBf> &I) const
{
  vpPanda3DPostProcessFilter::getRenderBasic(I);
}

END_VISP_NAMESPACE

#elif !defined(VISP_BUILD_SHARED_LIBS)
// Work around to avoid warning: libvisp_ar.a(vpPanda3DCanny.cpp.o) has no symbols
void dummy_vpPanda3DCanny() { };

#endif
