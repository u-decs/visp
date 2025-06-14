/*
 * ViSP, open source Visual Servoing Platform software.
 * Copyright (C) 2005 - 2025 by Inria. All rights reserved.
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
 *
 * Description:
 * windows 32 display using D3D
 */

/*!
  \file vpDisplayD3D.cpp
  \brief windows 32 display using D3D
*/

#include <visp3/core/vpConfig.h>
#if (defined(_WIN32) & defined(VISP_HAVE_D3D9))

#include <visp3/gui/vpD3DRenderer.h>
#include <visp3/gui/vpDisplayD3D.h>

BEGIN_VISP_NAMESPACE
/*!
  \brief Basic constructor.
*/
vpDisplayD3D::vpDisplayD3D() : vpDisplayWin32(new vpD3DRenderer()) { }

/*!
  \brief Constructor : Initialize a display.

  \param[in] winx : Horizontal position of the upper-left window's corner in the screen.
  \param[in] winy : Vertical position of the upper-left window's corner in the screen.
  \param[in] title : Window's title.

*/
vpDisplayD3D::vpDisplayD3D(int winx, int winy, const std::string &title) : vpDisplayWin32(new vpD3DRenderer())
{
  m_windowXPosition = winx;
  m_windowYPosition = winy;

  if (!title.empty())
    m_title = title;
  else
    m_title = std::string(" ");
}

/*!

  \brief Constructor : initialize a display to visualize a RGBa image (32 bits).

  \param[in] I : Image to be displayed (note that image has to be initialized).
  \param[in] winx : Horizontal position of the upper-left window's corner in the screen.
  \param[in] winy : Vertical position of the upper-left window's corner in the screen.
  \param title : Window's title.
  \param[in] scaleType : If this parameter is set to:
  - vpDisplay::SCALE_AUTO, the display size is adapted to ensure the image is fully displayed in the screen;
  - vpDisplay::SCALE_DEFAULT or vpDisplay::SCALE_1, the display size is the same than the image size.
  - vpDisplay::SCALE_2, the display size is down scaled by 2 along the lines and the columns.
  - vpDisplay::SCALE_3, the display size is down scaled by 3 along the lines and the columns.
  - vpDisplay::SCALE_4, the display size is down scaled by 4 along the lines and the columns.
  - vpDisplay::SCALE_5, the display size is down scaled by 5 along the lines and the columns.
*/
vpDisplayD3D::vpDisplayD3D(vpImage<vpRGBa> &I, vpScaleType scaleType) : vpDisplayWin32(new vpD3DRenderer())
{
  setScale(scaleType, I.getWidth(), I.getHeight());
  init(I);
}

/*!
  \brief Constructor : initialize a display to visualize a RGBa image (32 bits).

  \param[in] I : Image to be displayed (note that image has to be initialized).
  \param[in] winx : Horizontal position of the upper-left window's corner in the screen.
  \param[in] winy : Vertical position of the upper-left window's corner in the screen.
  \param[in] title : Window's title.
  \param[in] scaleType : If this parameter is set to:
  - vpDisplay::SCALE_AUTO, the display size is adapted to ensure the image is fully displayed in the screen;
  - vpDisplay::SCALE_DEFAULT or vpDisplay::SCALE_1, the display size is the same than the image size.
  - vpDisplay::SCALE_2, the display size is down scaled by 2 along the lines and the columns.
  - vpDisplay::SCALE_3, the display size is down scaled by 3 along the lines and the columns.
  - vpDisplay::SCALE_4, the display size is down scaled by 4 along the lines and the columns.
  - vpDisplay::SCALE_5, the display size is down scaled by 5 along the lines and the columns.
*/
vpDisplayD3D::vpDisplayD3D(vpImage<vpRGBa> &I, int winx, int winy, const std::string &title, vpScaleType scaleType)
  : vpDisplayWin32(new vpD3DRenderer())
{
  setScale(scaleType, I.getWidth(), I.getHeight());
  init(I, winx, winy, title);
}

/*!
  \brief Constructor : initialize a display to visualize a grayscale image
  (8 bits).

  \param[in] I  Image to be displayed (note that image has to be initialized).
  \param[in] scaleType : If this parameter is set to:
  - vpDisplay::SCALE_AUTO, the display size is adapted to ensure the image is fully displayed in the screen;
  - vpDisplay::SCALE_DEFAULT or vpDisplay::SCALE_1, the display size is the same than the image size.
  - vpDisplay::SCALE_2, the display size is down scaled by 2 along the lines and the columns.
  - vpDisplay::SCALE_3, the display size is down scaled by 3 along the lines and the columns.
  - vpDisplay::SCALE_4, the display size is down scaled by 4 along the lines and the columns.
  - vpDisplay::SCALE_5, the display size is down scaled by 5 along the lines and the columns.
*/
vpDisplayD3D::vpDisplayD3D(vpImage<unsigned char> &I, vpScaleType scaleType) : vpDisplayWin32(new vpD3DRenderer())
{
  setScale(scaleType, I.getWidth(), I.getHeight());
  init(I);
}

/*!
  \brief Constructor : initialize a display to visualize a grayscale image
  (8 bits).

  \param[in] I  Image to be displayed (note that image has to be initialized).
  \param[in] winx : Horizontal position of the upper-left window's corner in the screen.
  \param[in] winy : Vertical position of the upper-left window's corner in the screen.
  \param[in] title  Window's title.
  \param[in] scaleType : If this parameter is set to:
  - vpDisplay::SCALE_AUTO, the display size is adapted to ensure the image is fully displayed in the screen;
  - vpDisplay::SCALE_DEFAULT or vpDisplay::SCALE_1, the display size is the same than the image size.
  - vpDisplay::SCALE_2, the display size is down scaled by 2 along the lines and the columns.
  - vpDisplay::SCALE_3, the display size is down scaled by 3 along the lines and the columns.
  - vpDisplay::SCALE_4, the display size is down scaled by 4 along the lines and the columns.
  - vpDisplay::SCALE_5, the display size is down scaled by 5 along the lines and the columns.
*/
vpDisplayD3D::vpDisplayD3D(vpImage<unsigned char> &I, int winx, int winy, const std::string &title,
                           vpScaleType scaleType)
  : vpDisplayWin32(new vpD3DRenderer())
{
  setScale(scaleType, I.getWidth(), I.getHeight());
  init(I, winx, winy, title);
}

END_VISP_NAMESPACE

#elif !defined(VISP_BUILD_SHARED_LIBS)
// Work around to avoid warning: libvisp_gui.a(vpDisplayD3D.cpp.o) has no symbols
void dummy_vpDisplayD3D() { }
#endif
