/****************************************************************************
 *
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
 *
 * Description:
 * Display Factory
 */

#ifndef VP_DISPLAY_FACTORY_H
#define VP_DISPLAY_FACTORY_H

#include <visp3/core/vpConfig.h>
#include <visp3/core/vpDisplay.h>
#include <visp3/gui/vpDisplayD3D.h>
#include <visp3/gui/vpDisplayGDI.h>
#include <visp3/gui/vpDisplayGTK.h>
#include <visp3/gui/vpDisplayOpenCV.h>
#include <visp3/gui/vpDisplayX.h>

#if (VISP_CXX_STANDARD >= VISP_CXX_STANDARD_11)
#include <memory>
#endif

BEGIN_VISP_NAMESPACE
/**
 * \ingroup group_gui_display
*/
namespace vpDisplayFactory
{
/**
 * \brief Return a newly allocated vpDisplay specialization
 * if a GUI library is available or nullptr otherwise.
 *
 * \return A newly allocated vpDisplay specialization
 * if a GUI library is available or nullptr otherwise.
 */
inline vpDisplay *allocateDisplay()
{
#if defined(VISP_HAVE_DISPLAY)
#ifdef VISP_HAVE_X11
  return new vpDisplayX();
#elif defined(VISP_HAVE_D3D9)
  return new vpDisplayD3D();
#elif defined(VISP_HAVE_GDI)
  return new vpDisplayGDI();
#elif defined(VISP_HAVE_GTK)
  return new vpDisplayGTK();
#elif defined(HAVE_OPENCV_HIGHGUI)
  return new vpDisplayOpenCV();
#endif
#else
  return nullptr;
#endif
}

/**
 * \brief Return a newly allocated vpDisplay specialization initialized with \b I
 * if a GUI library is available or nullptr otherwise.
 *
 * \tparam T : Any type that an image can handle and that can be displayed.
 * \param[in] I : The image the display must be initialized with.
 * \param[in] winx : The horizontal position of the display on the screen.
 * \param[in] winy : The vertical position of the display on the screen.
 * \param[in] title : The title of the display.
 * \param[in] scaleType : If this parameter is set to:
 * - vpDisplay::SCALE_AUTO, the display size is adapted to ensure the image is fully displayed in the screen;
 * - vpDisplay::SCALE_DEFAULT or vpDisplay::SCALE_1, the display size is the same than the image size.
 * - vpDisplay::SCALE_2, the display size is down scaled by 2 along the lines and the columns.
 * - vpDisplay::SCALE_3, the display size is down scaled by 3 along the lines and the columns.
 * - vpDisplay::SCALE_4, the display size is down scaled by 4 along the lines and the columns.
 * - vpDisplay::SCALE_5, the display size is down scaled by 5 along the lines and the columns.
 *
 * \return A newly allocated vpDisplay specialization initialized with \b I
 * if a GUI library is available or nullptr otherwise.
 * \warning The user must free the memory when the display is not used anymore.
 */
template<typename T>
vpDisplay *allocateDisplay(vpImage<T> &I, const int winx = -1, const int winy = -1, const std::string &title = "",
                           const vpDisplay::vpScaleType &scaleType = vpDisplay::SCALE_DEFAULT)
{
#if defined(VISP_HAVE_DISPLAY)
#ifdef VISP_HAVE_X11
  return new vpDisplayX(I, winx, winy, title, scaleType);
#elif defined(VISP_HAVE_GDI)
  return new vpDisplayGDI(I, winx, winy, title, scaleType);
#elif defined(HAVE_OPENCV_HIGHGUI)
  return new vpDisplayOpenCV(I, winx, winy, title, scaleType);
#elif defined(VISP_HAVE_GTK)
  return new vpDisplayGTK(I, winx, winy, title, scaleType);
#elif defined(VISP_HAVE_D3D9)
  return new vpDisplayD3D(I, winx, winy, title, scaleType);
#endif
#else
  (void)I;
  (void)winx;
  (void)winy;
  (void)title;
  (void)scaleType;
  return nullptr;
#endif
}

#if (VISP_CXX_STANDARD >= VISP_CXX_STANDARD_11)
/**
 * \brief Return a smart pointer vpDisplay specialization
 * if a GUI library is available or nullptr otherwise.
 *
 * \return A smart pointer pointing to a vpDisplay specialization
 * if a GUI library is available or nullptr otherwise.
 */
inline std::shared_ptr<vpDisplay> createDisplay()
{
#if defined(VISP_HAVE_DISPLAY)
#ifdef VISP_HAVE_X11
  return std::make_shared<vpDisplayX>();
#elif defined(VISP_HAVE_GDI)
  return std::make_shared<vpDisplayGDI>();
#elif defined(HAVE_OPENCV_HIGHGUI)
  return std::make_shared<vpDisplayOpenCV>();
#elif defined(VISP_HAVE_GTK)
  return std::make_shared<vpDisplayGTK>();
#elif defined(VISP_HAVE_D3D9)
  return std::make_shared<vpDisplayD3D>();
#endif
#else
  return std::shared_ptr<vpDisplay>(nullptr);
#endif
}

/**
 * \brief Return a smart pointer vpDisplay specialization initialized with \b I
 * if a GUI library is available or nullptr otherwise.
 *
 * \tparam T : Any type that an image can handle and that can be displayed.
 * \param[in] I : The image the display must be initialized with.
 * \param[in] winx : The horizontal position of the display on the screen.
 * \param[in] winy : The vertical position of the display on the screen.
 * \param[in] title : The title of the display.
 * \param[in] scaleType : If this parameter is set to:
 * - vpDisplay::SCALE_AUTO, the display size is adapted to ensure the image is fully displayed in the screen;
 * - vpDisplay::SCALE_DEFAULT or vpDisplay::SCALE_1, the display size is the same than the image size.
 * - vpDisplay::SCALE_2, the display size is down scaled by 2 along the lines and the columns.
 * - vpDisplay::SCALE_3, the display size is down scaled by 3 along the lines and the columns.
 * - vpDisplay::SCALE_4, the display size is down scaled by 4 along the lines and the columns.
 * - vpDisplay::SCALE_5, the display size is down scaled by 5 along the lines and the columns.
 *
 * \return A smart pointer pointing to a vpDisplay specialization initialized with \b I
 * if a GUI library is available or nullptr otherwise.
 */
template<typename T>
std::shared_ptr<vpDisplay> createDisplay(vpImage<T> &I, const int winx = -1, const int winy = -1,
                                         const std::string &title = "", const vpDisplay::vpScaleType &scaleType = vpDisplay::SCALE_DEFAULT)
{
#if defined(VISP_HAVE_DISPLAY)
#ifdef VISP_HAVE_X11
  return std::make_shared<vpDisplayX>(I, winx, winy, title, scaleType);
#elif defined(VISP_HAVE_GDI)
  return std::make_shared<vpDisplayGDI>(I, winx, winy, title, scaleType);
#elif defined(HAVE_OPENCV_HIGHGUI)
  return std::make_shared<vpDisplayOpenCV>(I, winx, winy, title, scaleType);
#elif defined(VISP_HAVE_GTK)
  return std::make_shared<vpDisplayGTK>(I, winx, winy, title, scaleType);
#elif defined(VISP_HAVE_D3D9)
  return std::make_shared<vpDisplayD3D>(I, winx, winy, title, scaleType);
#endif
#else
  (void)I;
  (void)winx;
  (void)winy;
  (void)title;
  (void)scaleType;
  return nullptr;
  return std::shared_ptr<vpDisplay>(nullptr);
#endif
}
#endif

}
END_VISP_NAMESPACE
#endif
