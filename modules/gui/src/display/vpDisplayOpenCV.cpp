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
 * Image display.
 */

/*!
  \file vpDisplayOpenCV.cpp
  \brief Define the OpenCV console to display images.
*/

#include <visp3/core/vpConfig.h>

#if defined(HAVE_OPENCV_HIGHGUI)

#include <cmath> // std::fabs
#include <iostream>
#include <limits> // numeric_limits
#include <stdio.h>
#include <stdlib.h>

// Display stuff
#include <visp3/core/vpDisplay.h>
#include <visp3/core/vpImageTools.h>
#include <visp3/core/vpIoTools.h>
#include <visp3/core/vpMath.h>
#include <visp3/gui/vpDisplayOpenCV.h>

// debug / exception
#include <visp3/core/vpDisplayException.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#if (VISP_HAVE_OPENCV_VERSION < 0x050000)
#include <opencv2/core/core_c.h> // for CV_FILLED versus cv::FILLED
#endif
#if defined(HAVE_OPENCV_IMGPROC)
#include <opencv2/imgproc/imgproc.hpp>
#endif

#ifndef CV_RGB
#define CV_RGB(r, g, b) cv::Scalar((b), (g), (r), 0)
#endif

#ifdef VISP_HAVE_X11
#include <visp3/gui/vpDisplayX.h> // to get screen resolution
#elif defined(_WIN32)

// Mute warning with clang-cl
// warning : non-portable path to file '<Windows.h>'; specified path differs in case from file name on disk [-Wnonportable-system-include-path]
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wnonportable-system-include-path"
#endif

#include <windows.h>

#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

#endif

BEGIN_VISP_NAMESPACE

#ifndef DOXYGEN_SHOULD_SKIP_THIS
class vpDisplayOpenCV::Impl
{
private:
  cv::Mat m_background;
  cv::Scalar *col;
  cv::Scalar cvcolor;
  int font;
  double fontScale;
  static std::vector<std::string> m_listTitles;
  static unsigned int m_nbWindows;
  int fontHeight;
  int x_move;
  int y_move;
  bool move;
  int x_lbuttondown;
  int y_lbuttondown;
  bool lbuttondown;
  int x_mbuttondown;
  int y_mbuttondown;
  bool mbuttondown;
  int x_rbuttondown;
  int y_rbuttondown;
  bool rbuttondown;
  int x_lbuttonup;
  int y_lbuttonup;
  bool lbuttonup;
  int x_mbuttonup;
  int y_mbuttonup;
  bool mbuttonup;
  int x_rbuttonup;
  int y_rbuttonup;
  bool rbuttonup;

  friend class vpDisplayOpenCV;

public:
  Impl() :
    m_background(), col(nullptr), cvcolor(), font(cv::FONT_HERSHEY_PLAIN), fontScale(0.8),
    fontHeight(10), x_move(0), y_move(0), move(false), x_lbuttondown(0), y_lbuttondown(0), lbuttondown(false),
    x_mbuttondown(0), y_mbuttondown(0), mbuttondown(false), x_rbuttondown(0), y_rbuttondown(0), rbuttondown(false),
    x_lbuttonup(0), y_lbuttonup(0), lbuttonup(false), x_mbuttonup(0), y_mbuttonup(0), mbuttonup(false), x_rbuttonup(0),
    y_rbuttonup(0), rbuttonup(false)
  { }

  virtual ~Impl()
  {
    if (col != nullptr) {
      delete[] col;
      col = nullptr;
    }
  }

  void getImage(vpImage<vpRGBa> &I)
  {
    // Should be optimized
    vpImageConvert::convert(m_background, I);
  }

  std::string init(const std::string &title, const int &windowXPosition, const int &windowYPosition)
  {
    int flags = cv::WINDOW_AUTOSIZE;
    std::string outTitle = title;
    if (outTitle.empty()) {
      std::ostringstream s;
      s << m_nbWindows++;
      outTitle = std::string("Window ") + s.str();
    }

    bool isInList;
    do {
      isInList = false;
      size_t i = 0;
      while (i < m_listTitles.size() && !isInList) {
        if (m_listTitles[i] == outTitle) {
          std::ostringstream s;
          s << m_nbWindows++;
          outTitle = std::string("Window ") + s.str();
          isInList = true;
        }
        i++;
      }
    } while (isInList);

    m_listTitles.push_back(outTitle);


    /* Create the window*/
    cv::namedWindow(outTitle, flags);
    cv::moveWindow(outTitle.c_str(), windowXPosition, windowYPosition);

    move = false;
    lbuttondown = false;
    mbuttondown = false;
    rbuttondown = false;
    lbuttonup = false;
    mbuttonup = false;
    rbuttonup = false;

    cv::setMouseCallback(outTitle, on_mouse, this);
    col = new cv::Scalar[vpColor::id_unknown];

    /* Create color */
    vpColor pcolor; // Predefined colors
    pcolor = vpColor::lightBlue;
    col[vpColor::id_lightBlue] = CV_RGB(pcolor.R, pcolor.G, pcolor.B);
    pcolor = vpColor::blue;
    col[vpColor::id_blue] = CV_RGB(pcolor.R, pcolor.G, pcolor.B);
    pcolor = vpColor::darkBlue;
    col[vpColor::id_darkBlue] = CV_RGB(pcolor.R, pcolor.G, pcolor.B);
    pcolor = vpColor::lightRed;
    col[vpColor::id_lightRed] = CV_RGB(pcolor.R, pcolor.G, pcolor.B);
    pcolor = vpColor::red;
    col[vpColor::id_red] = CV_RGB(pcolor.R, pcolor.G, pcolor.B);
    pcolor = vpColor::darkRed;
    col[vpColor::id_darkRed] = CV_RGB(pcolor.R, pcolor.G, pcolor.B);
    pcolor = vpColor::lightGreen;
    col[vpColor::id_lightGreen] = CV_RGB(pcolor.R, pcolor.G, pcolor.B);
    pcolor = vpColor::green;
    col[vpColor::id_green] = CV_RGB(pcolor.R, pcolor.G, pcolor.B);
    pcolor = vpColor::darkGreen;
    col[vpColor::id_darkGreen] = CV_RGB(pcolor.R, pcolor.G, pcolor.B);
    pcolor = vpColor::yellow;
    col[vpColor::id_yellow] = CV_RGB(pcolor.R, pcolor.G, pcolor.B);
    pcolor = vpColor::cyan;
    col[vpColor::id_cyan] = CV_RGB(pcolor.R, pcolor.G, pcolor.B);
    pcolor = vpColor::orange;
    col[vpColor::id_orange] = CV_RGB(pcolor.R, pcolor.G, pcolor.B);
    pcolor = vpColor::purple;
    col[vpColor::id_purple] = CV_RGB(pcolor.R, pcolor.G, pcolor.B);
    pcolor = vpColor::white;
    col[vpColor::id_white] = CV_RGB(pcolor.R, pcolor.G, pcolor.B);
    pcolor = vpColor::black;
    col[vpColor::id_black] = CV_RGB(pcolor.R, pcolor.G, pcolor.B);
    pcolor = vpColor::lightGray;
    col[vpColor::id_lightGray] = CV_RGB(pcolor.R, pcolor.G, pcolor.B);
    pcolor = vpColor::gray;
    col[vpColor::id_gray] = CV_RGB(pcolor.R, pcolor.G, pcolor.B);
    pcolor = vpColor::darkGray;
    col[vpColor::id_darkGray] = CV_RGB(pcolor.R, pcolor.G, pcolor.B);

    int thickness = 1;
    cv::Size fontSize;
    int baseline;
    fontSize = cv::getTextSize("A", font, fontScale, thickness, &baseline);

    fontHeight = fontSize.height + baseline;
    return outTitle;
  }

  void closeDisplay(const std::string &title)
  {
    if (col != nullptr) {
      delete[] col;
      col = nullptr;
    }

    cv::destroyWindow(title);
    for (size_t i = 0; i < m_listTitles.size(); i++) {
      if (title == m_listTitles[i]) {
        m_listTitles.erase(m_listTitles.begin() + static_cast<long int>(i));
        break;
      }
    }
  }

  void displayArrow(const vpImagePoint &ip1, const vpImagePoint &ip2, const vpColor &color,
                    const unsigned int &w, const unsigned int &h, const unsigned int &thickness, const unsigned int &scale)
  {
    double a = ip2.get_i() - ip1.get_i();
    double b = ip2.get_j() - ip1.get_j();
    double lg = sqrt(vpMath::sqr(a) + vpMath::sqr(b));

    if ((std::fabs(a) <= std::numeric_limits<double>::epsilon()) &&
        (std::fabs(b) <= std::numeric_limits<double>::epsilon())) {
    }
    else {
      a /= lg;
      b /= lg;

      vpImagePoint ip3;
      ip3.set_i(ip2.get_i() - w * a);
      ip3.set_j(ip2.get_j() - w * b);

      vpImagePoint ip4;
      ip4.set_i(ip3.get_i() - b * h);
      ip4.set_j(ip3.get_j() + a * h);

      if (lg > 2 * vpImagePoint::distance(ip2, ip4))
        displayLine(ip2, ip4, color, thickness, scale);

      ip4.set_i(ip3.get_i() + b * h);
      ip4.set_j(ip3.get_j() - a * h);

      if (lg > 2 * vpImagePoint::distance(ip2, ip4))
        displayLine(ip2, ip4, color, thickness, scale);

      displayLine(ip1, ip2, color, thickness, scale);
    }
  }

  void displayCircle(const vpImagePoint &center, const unsigned int &radius, const vpColor &color, const bool &fill,
                     const unsigned int &thickness, const unsigned int &scale)
  {
    int x = vpMath::round(center.get_u() / scale);
    int y = vpMath::round(center.get_v() / scale);
    int r = static_cast<int>(radius / scale);
    cv::Scalar cv_color;
    if ((color.id < vpColor::id_black) || (color.id >= vpColor::id_unknown)) {
      cv_color = CV_RGB(color.R, color.G, color.B);
    }
    else {
      cv_color = col[color.id];
    }

    if (fill == false) {
      int cv_thickness = static_cast<int>(thickness);
      cv::circle(m_background, cv::Point(x, y), r, cv_color, cv_thickness);
    }
    else {
#if VISP_HAVE_OPENCV_VERSION >= 0x030000
      int filled = cv::FILLED;
#else
      int filled = CV_FILLED;
#endif
      double opacity = static_cast<double>(color.A) / 255.0;
      overlay([x, y, r, cv_color, filled](cv::Mat image) { cv::circle(image, cv::Point(x, y), r, cv_color, filled); },
              opacity);
    }
  }

  void displayCross(const vpImagePoint &ip, const unsigned int &size, const vpColor &color, const unsigned int &thickness, const unsigned int &scale)
  {
    vpImagePoint top, bottom, left, right;
    top.set_i(ip.get_i() - size / 2);
    top.set_j(ip.get_j());
    bottom.set_i(ip.get_i() + size / 2);
    bottom.set_j(ip.get_j());
    left.set_i(ip.get_i());
    left.set_j(ip.get_j() - size / 2);
    right.set_i(ip.get_i());
    right.set_j(ip.get_j() + size / 2);
    displayLine(top, bottom, color, thickness, scale);
    displayLine(left, right, color, thickness, scale);
  }

  void displayDotLine(const vpImagePoint &ip1, const vpImagePoint &ip2, const vpColor &color,
                      const unsigned int &thickness, const unsigned int &scale)
  {
    vpImagePoint ip1_ = ip1;
    vpImagePoint ip2_ = ip2;

    double size = 10. * scale;
    double length = sqrt(vpMath::sqr(ip2_.get_i() - ip1_.get_i()) + vpMath::sqr(ip2_.get_j() - ip1_.get_j()));
    bool vertical_line = static_cast<int>(ip2_.get_j()) == static_cast<int>(ip1_.get_j());
    if (vertical_line) {
      if (ip2_.get_i() < ip1_.get_i()) {
        std::swap(ip1_, ip2_);
      }
    }
    else if (ip2_.get_j() < ip1_.get_j()) {
      std::swap(ip1_, ip2_);
    }

    double diff_j = vertical_line ? 1 : ip2_.get_j() - ip1_.get_j();
    double deltaj = size / length * diff_j;
    double deltai = size / length * (ip2_.get_i() - ip1_.get_i());
    double slope = (ip2_.get_i() - ip1_.get_i()) / diff_j;
    double orig = ip1_.get_i() - slope * ip1_.get_j();

    if (vertical_line) {
      for (unsigned int i = static_cast<unsigned int>(ip1_.get_i()); i < ip2_.get_i(); i += static_cast<unsigned int>(2 * deltai)) {
        double j = ip1_.get_j();
        displayLine(vpImagePoint(i, j), vpImagePoint(i + deltai, j), color, thickness, scale);
      }
    }
    else {
      for (unsigned int j = static_cast<unsigned int>(ip1_.get_j()); j < ip2_.get_j(); j += static_cast<unsigned int>(2 * deltaj)) {
        double i = slope * j + orig;
        displayLine(vpImagePoint(i, j), vpImagePoint(i + deltai, j + deltaj), color, thickness, scale);
      }
    }
  }

  void displayImage(const vpImage<unsigned char> &I, const unsigned int &scale, const unsigned int &width, const unsigned int &height)
  {
    int depth = CV_8U;
    int channels = 3;
    cv::Size size(static_cast<int>(width), static_cast<int>(height));
    if (m_background.channels() != channels || m_background.depth() != depth || m_background.rows != static_cast<int>(height) ||
        m_background.cols != static_cast<int>(width)) {
      m_background = cv::Mat(size, CV_MAKETYPE(depth, channels));
    }

    if (scale == 1) {
      for (unsigned int i = 0; i < height; i++) {
        unsigned char *dst_24 = static_cast<unsigned char *>(m_background.data) + static_cast<int>(i * 3 * width);
        for (unsigned int j = 0; j < width; j++) {
          unsigned char val = I[i][j];
          *(dst_24++) = val;
          *(dst_24++) = val;
          *(dst_24++) = val;
        }
      }
    }
    else {
      for (unsigned int i = 0; i < height; i++) {
        unsigned char *dst_24 = static_cast<unsigned char *>(m_background.data) + static_cast<int>(i * 3 * width);
        for (unsigned int j = 0; j < width; j++) {
          unsigned char val = I[i * scale][j * scale];
          *(dst_24++) = val;
          *(dst_24++) = val;
          *(dst_24++) = val;
        }
      }
    }
  }

  void displayImage(const vpImage<vpRGBa> &I, const unsigned int &scale, const unsigned int &width, const unsigned int &height)
  {
    int depth = CV_8U;
    int channels = 3;
    cv::Size size(static_cast<int>(width), static_cast<int>(height));
    if (m_background.channels() != channels || m_background.depth() != depth || m_background.rows != static_cast<int>(height) ||
        m_background.cols != static_cast<int>(width)) {
      m_background = cv::Mat(size, CV_MAKETYPE(depth, channels));
    }

    if (scale == 1) {
      for (unsigned int i = 0; i < height; i++) {
        unsigned char *dst_24 = static_cast<unsigned char *>(m_background.data) + static_cast<int>(i * 3 * width);
        for (unsigned int j = 0; j < width; j++) {
          vpRGBa val = I[i][j];
          *(dst_24++) = val.B;
          *(dst_24++) = val.G;
          *(dst_24++) = val.R;
        }
      }
    }
    else {
      for (unsigned int i = 0; i < height; i++) {
        unsigned char *dst_24 = static_cast<unsigned char *>(m_background.data) + static_cast<int>(i * 3 * width);
        for (unsigned int j = 0; j < width; j++) {
          vpRGBa val = I[i * scale][j * scale];
          *(dst_24++) = val.B;
          *(dst_24++) = val.G;
          *(dst_24++) = val.R;
        }
      }
    }
  }

  void displayImageROI(const vpImage<unsigned char> &I, const vpImagePoint &iP, const unsigned int &w,
                       const unsigned int &h, const unsigned int &imgWidth, const unsigned int &imgHeight, const unsigned int &scale)
  {
    int depth = CV_8U;
    int channels = 3;
    cv::Size size(static_cast<int>(imgWidth), static_cast<int>(imgHeight));
    if (m_background.channels() != channels || m_background.depth() != depth || m_background.rows != static_cast<int>(imgHeight) ||
        m_background.cols != static_cast<int>(imgWidth)) {
      m_background = cv::Mat(size, CV_MAKETYPE(depth, channels));
    }

    if (scale == 1) {
      unsigned int i_min = static_cast<unsigned int>(iP.get_i());
      unsigned int j_min = static_cast<unsigned int>(iP.get_j());
      unsigned int i_max = std::min<unsigned int>(i_min + h, imgHeight);
      unsigned int j_max = std::min<unsigned int>(j_min + w, imgWidth);
      for (unsigned int i = i_min; i < i_max; i++) {
        unsigned char *dst_24 = static_cast<unsigned char *>(m_background.data) + static_cast<int>(i * 3u * imgWidth + j_min * 3u);
        for (unsigned int j = j_min; j < j_max; j++) {
          unsigned char val = I[i][j];
          *(dst_24++) = val;
          *(dst_24++) = val;
          *(dst_24++) = val;
        }
      }
    }
    else {
      unsigned int i_min = std::max<unsigned int>(static_cast<unsigned int>(ceil(iP.get_i() / scale)), 0);
      unsigned int j_min = std::max<unsigned int>(static_cast<unsigned int>(ceil(iP.get_j() / scale)), 0);
      unsigned int i_max = std::min<unsigned int>(static_cast<unsigned int>(ceil((iP.get_i() + h) / scale)), imgHeight);
      unsigned int j_max = std::min<unsigned int>(static_cast<unsigned int>(ceil((iP.get_j() + w) / scale)), imgWidth);
      for (unsigned int i = i_min; i < i_max; i++) {
        unsigned char *dst_24 = static_cast<unsigned char *>(m_background.data) + static_cast<int>(i * 3u * imgWidth + j_min * 3u);
        for (unsigned int j = j_min; j < j_max; j++) {
          unsigned char val = I[i * scale][j * scale];
          *(dst_24++) = val;
          *(dst_24++) = val;
          *(dst_24++) = val;
        }
      }
    }
  }

  void displayImageROI(const vpImage<vpRGBa> &I, const vpImagePoint &iP, const unsigned int &w, const unsigned int &h, const unsigned int &imgWidth, const unsigned int &imgHeight, const unsigned int &scale)
  {
    int depth = CV_8U;
    int channels = 3;
    cv::Size size(static_cast<int>(imgWidth), static_cast<int>(imgHeight));
    if (m_background.channels() != channels || m_background.depth() != depth || m_background.rows != static_cast<int>(imgHeight) ||
        m_background.cols != static_cast<int>(imgWidth)) {
      m_background = cv::Mat(size, CV_MAKETYPE(depth, channels));
    }

    if (scale == 1) {
      unsigned int i_min = static_cast<unsigned int>(iP.get_i());
      unsigned int j_min = static_cast<unsigned int>(iP.get_j());
      unsigned int i_max = std::min<unsigned int>(i_min + h, imgHeight);
      unsigned int j_max = std::min<unsigned int>(j_min + w, imgWidth);
      for (unsigned int i = i_min; i < i_max; i++) {
        unsigned char *dst_24 = static_cast<unsigned char *>(m_background.data) + static_cast<int>(i * 3 * imgWidth + j_min * 3);
        for (unsigned int j = j_min; j < j_max; j++) {
          vpRGBa val = I[i][j];
          *(dst_24++) = val.B;
          *(dst_24++) = val.G;
          *(dst_24++) = val.R;
        }
      }
    }
    else {
      unsigned int i_min = std::max<unsigned int>(static_cast<unsigned int>(ceil(iP.get_i() / scale)), 0);
      unsigned int j_min = std::max<unsigned int>(static_cast<unsigned int>(ceil(iP.get_j() / scale)), 0);
      unsigned int i_max = std::min<unsigned int>(static_cast<unsigned int>(ceil((iP.get_i() + h) / scale)), imgHeight);
      unsigned int j_max = std::min<unsigned int>(static_cast<unsigned int>(ceil((iP.get_j() + w) / scale)), imgWidth);
      for (unsigned int i = i_min; i < i_max; i++) {
        unsigned char *dst_24 = static_cast<unsigned char *>(m_background.data) + static_cast<int>(i * 3 * imgWidth + j_min * 3);
        for (unsigned int j = j_min; j < j_max; j++) {
          vpRGBa val = I[i * scale][j * scale];
          *(dst_24++) = val.B;
          *(dst_24++) = val.G;
          *(dst_24++) = val.R;
        }
      }
    }
  }

  void displayLine(const vpImagePoint &ip1, const vpImagePoint &ip2, const vpColor &color, const unsigned int &thickness, const unsigned int &scale)
  {
    if ((color.id < vpColor::id_black) || (color.id >= vpColor::id_unknown)) {
      cvcolor = CV_RGB(color.R, color.G, color.B);
      cv::line(m_background, cv::Point(vpMath::round(ip1.get_u() / scale), vpMath::round(ip1.get_v() / scale)),
               cv::Point(vpMath::round(ip2.get_u() / scale), vpMath::round(ip2.get_v() / scale)), cvcolor,
               static_cast<int>(thickness));
    }
    else {
      cv::line(m_background, cv::Point(vpMath::round(ip1.get_u() / scale), vpMath::round(ip1.get_v() / scale)),
               cv::Point(vpMath::round(ip2.get_u() / scale), vpMath::round(ip2.get_v() / scale)), col[color.id],
               static_cast<int>(thickness));
    }
  }

  void displayPoint(const vpImagePoint &ip, const vpColor &color, const unsigned int &thickness, const unsigned int &scale)
  {
    for (unsigned int i = 0; i < thickness; i++) {
      if ((color.id < vpColor::id_black) || (color.id >= vpColor::id_unknown)) {
        cvcolor = CV_RGB(color.R, color.G, color.B);
        cv::line(m_background, cv::Point(vpMath::round(ip.get_u() / scale), vpMath::round(ip.get_v() / scale)),
                 cv::Point(vpMath::round(ip.get_u() / scale + thickness - 1), vpMath::round(ip.get_v() / scale)),
                 cvcolor, static_cast<int>(thickness));
      }
      else {
        cv::line(m_background, cv::Point(vpMath::round(ip.get_u() / scale), vpMath::round(ip.get_v() / scale)),
                 cv::Point(vpMath::round(ip.get_u() / scale + thickness - 1), vpMath::round(ip.get_v() / scale)),
                 col[color.id], static_cast<int>(thickness));
      }
    }
  }

  void displayRectangle(const vpImagePoint &topLeft, const unsigned int &w, const unsigned int &h, const vpColor &color,
                        const bool &fill, const unsigned int &thickness, const unsigned int &scale)
  {
    int left = vpMath::round(topLeft.get_u() / scale);
    int top = vpMath::round(topLeft.get_v() / scale);
    int right = vpMath::round((topLeft.get_u() + w) / scale);
    int bottom = vpMath::round((topLeft.get_v() + h) / scale);
    cv::Scalar cv_color;
    if ((color.id < vpColor::id_black) || (color.id >= vpColor::id_unknown)) {
      cv_color = CV_RGB(color.R, color.G, color.B);
    }
    else {
      cv_color = col[color.id];
    }

    if (fill == false) {
      int cv_thickness = static_cast<int>(thickness);
      cv::rectangle(m_background, cv::Point(left, top), cv::Point(right, bottom), cv_color, cv_thickness);
    }
    else {
#if VISP_HAVE_OPENCV_VERSION >= 0x030000
      int filled = cv::FILLED;
#else
      int filled = CV_FILLED;
#endif
      double opacity = static_cast<double>(color.A) / 255.0;
      overlay([left, top, right, bottom, cv_color, filled](cv::Mat image) {
        cv::rectangle(image, cv::Point(left, top), cv::Point(right, bottom), cv_color, filled);
          },
          opacity);
    }
  }

  void displayText(const vpImagePoint &ip, const std::string &text, const vpColor &color, const unsigned int &scale)
  {
    if ((color.id < vpColor::id_black) || (color.id >= vpColor::id_unknown)) {
      cvcolor = CV_RGB(color.R, color.G, color.B);
      cv::putText(m_background, text,
                  cv::Point(vpMath::round(ip.get_u() / scale), vpMath::round(ip.get_v() / scale + fontHeight)), font,
                  fontScale, cvcolor);
    }
    else {
      cv::putText(m_background, text,
                  cv::Point(vpMath::round(ip.get_u() / scale), vpMath::round(ip.get_v() / scale + fontHeight)), font,
                  fontScale, col[color.id]);
    }
  }

  void flushDisplay(const std::string &title)
  {
    cv::imshow(title, m_background);
    cv::waitKey(5);
  }

  void flushDisplayROI(const std::string &title)
  {
    cv::imshow(title.c_str(), m_background);
    cv::waitKey(5);
  }

  bool getClick(vpImagePoint &ip, vpMouseButton::vpMouseButtonType &button, const bool &blocking, const std::string &title, const unsigned int &scale)
  {
    flushDisplay(title);
    bool ret = false;
    double u, v;
    if (blocking) {
      lbuttondown = false;
      mbuttondown = false;
      rbuttondown = false;
    }
    do {
      if (lbuttondown) {
        ret = true;
        u = static_cast<unsigned int>(x_lbuttondown) * scale;
        v = static_cast<unsigned int>(y_lbuttondown) * scale;
        ip.set_u(u);
        ip.set_v(v);
        button = vpMouseButton::button1;
        lbuttondown = false;
      }
      if (mbuttondown) {
        ret = true;
        u = static_cast<unsigned int>(x_mbuttondown) * scale;
        v = static_cast<unsigned int>(y_mbuttondown) * scale;
        ip.set_u(u);
        ip.set_v(v);
        button = vpMouseButton::button2;
        mbuttondown = false;
      }
      if (rbuttondown) {
        ret = true;
        u = static_cast<unsigned int>(x_rbuttondown) * scale;
        v = static_cast<unsigned int>(y_rbuttondown) * scale;
        ip.set_u(u);
        ip.set_v(v);
        button = vpMouseButton::button3;
        rbuttondown = false;
      }
      if (blocking) {
        cv::waitKey(10);
      }
    } while (ret == false && blocking == true);
    return ret;
  }

  bool getClickUp(vpImagePoint &ip, vpMouseButton::vpMouseButtonType &button, const bool &blocking, const unsigned int &scale)
  {
    bool ret = false;
    double u, v;
    if (blocking) {
      lbuttonup = false;
      mbuttonup = false;
      rbuttonup = false;
    }
    do {
      if (lbuttonup) {
        ret = true;
        u = static_cast<unsigned int>(x_lbuttonup) * scale;
        v = static_cast<unsigned int>(y_lbuttonup) * scale;
        ip.set_u(u);
        ip.set_v(v);
        button = vpMouseButton::button1;
        lbuttonup = false;
      }
      if (mbuttonup) {
        ret = true;
        u = static_cast<unsigned int>(x_mbuttonup) * scale;
        v = static_cast<unsigned int>(y_mbuttonup) * scale;
        ip.set_u(u);
        ip.set_v(v);
        button = vpMouseButton::button2;
        mbuttonup = false;
      }
      if (rbuttonup) {
        ret = true;
        u = static_cast<unsigned int>(x_rbuttonup) * scale;
        v = static_cast<unsigned int>(y_rbuttonup) * scale;
        ip.set_u(u);
        ip.set_v(v);
        button = vpMouseButton::button3;
        rbuttonup = false;
      }
      if (blocking) {
        cv::waitKey(10);
      }
    } while (ret == false && blocking == true);
    return ret;
  }

  bool getPointerMotionEvent(vpImagePoint &ip, const unsigned int &scale)
  {
    bool ret = false;
    if (move) {
      ret = true;
      double u = static_cast<unsigned int>(x_move) / scale;
      double v = static_cast<unsigned int>(y_move) / scale;
      ip.set_u(u);
      ip.set_v(v);
      move = false;
    }
    return ret;
  }

  void getPointerPosition(vpImagePoint &ip, const unsigned int &scale)
  {
    bool moved = getPointerMotionEvent(ip, scale);
    if (!moved) {
      double u, v;
      u = static_cast<unsigned int>(x_move) / scale;
      v = static_cast<unsigned int>(y_move) / scale;
      ip.set_u(u);
      ip.set_v(v);
    }
  }

  static void on_mouse(int event, int x, int y, int /*flags*/, void *display)
  {
    Impl *disp = static_cast<Impl *>(display);

    switch (event) {
    case cv::EVENT_MOUSEMOVE:
    {
      disp->move = true;
      disp->x_move = x;
      disp->y_move = y;
      break;
    }
    case cv::EVENT_LBUTTONDOWN:
    {
      disp->lbuttondown = true;
      disp->x_lbuttondown = x;
      disp->y_lbuttondown = y;
      break;
    }
    case cv::EVENT_MBUTTONDOWN:
    {
      disp->mbuttondown = true;
      disp->x_mbuttondown = x;
      disp->y_mbuttondown = y;
      break;
    }
    case cv::EVENT_RBUTTONDOWN:
    {
      disp->rbuttondown = true;
      disp->x_rbuttondown = x;
      disp->y_rbuttondown = y;
      break;
    }
    case cv::EVENT_LBUTTONUP:
    {
      disp->lbuttonup = true;
      disp->x_lbuttonup = x;
      disp->y_lbuttonup = y;
      break;
    }
    case cv::EVENT_MBUTTONUP:
    {
      disp->mbuttonup = true;
      disp->x_mbuttonup = x;
      disp->y_mbuttonup = y;
      break;
    }
    case cv::EVENT_RBUTTONUP:
    {
      disp->rbuttonup = true;
      disp->x_rbuttonup = x;
      disp->y_rbuttonup = y;
      break;
    }

    default:
      break;
    }
  }

  void overlay(std::function<void(cv::Mat &)> overlay_function, const double &opacity)
  {
    // Initialize overlay layer for transparency
    cv::Mat overlay;
    if (opacity < 1.0) {
      // Deep copy
      overlay = m_background.clone();
    }
    else {
      // Shallow copy
      overlay = m_background;
    }

    overlay_function(overlay);

    // Blend background and overlay
    if (opacity < 1.0) {
      cv::addWeighted(overlay, opacity, m_background, 1.0 - opacity, 0.0, m_background);
    }
  }
};

VP_ATTRIBUTE_NO_DESTROY std::vector<std::string> vpDisplayOpenCV::Impl::m_listTitles = std::vector<std::string>();
unsigned int vpDisplayOpenCV::Impl::m_nbWindows = 0;
#endif // DOXYGEN_SHOULD_SKIP_THIS

/*!
  Constructor. Initialize a display to visualize a gray level image (8 bits).

  \param I : Image to be displayed (not that image has to be initialized)
  \param scaleType : If this parameter is set to:
  - vpDisplay::SCALE_AUTO, the display size is adapted to ensure the image is fully displayed in the screen;
  - vpDisplay::SCALE_DEFAULT or vpDisplay::SCALE_1, the display size is the same than the image size.
  - vpDisplay::SCALE_2, the display size is down scaled by 2 along the lines and the columns.
  - vpDisplay::SCALE_3, the display size is down scaled by 3 along the lines and the columns.
  - vpDisplay::SCALE_4, the display size is down scaled by 4 along the lines and the columns.
  - vpDisplay::SCALE_5, the display size is down scaled by 5 along the lines and the columns.
*/
vpDisplayOpenCV::vpDisplayOpenCV(vpImage<unsigned char> &I, vpScaleType scaleType)
  : vpDisplay()
  , m_impl(new Impl())
{
  setScale(scaleType, I.getWidth(), I.getHeight());
  init(I);
}

/*!
  Constructor. Initialize a display to visualize a gray level image (8 bits).

  \param I : Image to be displayed (not that image has to be initialized)
  \param x : The window is set at position x (column index).
  \param y : The window is set at position y (row index).
  \param title : Window title.
  \param scaleType : If this parameter is set to:
  - vpDisplay::SCALE_AUTO, the display size is adapted to ensure the image is fully displayed in the screen;
  - vpDisplay::SCALE_DEFAULT or vpDisplay::SCALE_1, the display size is the same than the image size.
  - vpDisplay::SCALE_2, the display size is down scaled by 2 along the lines and the columns.
  - vpDisplay::SCALE_3, the display size is down scaled by 3 along the lines and the columns.
  - vpDisplay::SCALE_4, the display size is down scaled by 4 along the lines and the columns.
  - vpDisplay::SCALE_5, the display size is down scaled by 5 along the lines and the columns.
*/
vpDisplayOpenCV::vpDisplayOpenCV(vpImage<unsigned char> &I, int x, int y, const std::string &title,
                                 vpScaleType scaleType)
  : vpDisplay()
  , m_impl(new Impl())
{
  setScale(scaleType, I.getWidth(), I.getHeight());
  init(I, x, y, title);
}

/*!
  Constructor. Initialize a display to visualize a RGBa image (32 bits).

  \param I : Image to be displayed (not that image has to be initialized)
  \param scaleType : If this parameter is set to:
  - vpDisplay::SCALE_AUTO, the display size is adapted to ensure the image is fully displayed in the screen;
  - vpDisplay::SCALE_DEFAULT or vpDisplay::SCALE_1, the display size is the same than the image size.
  - vpDisplay::SCALE_2, the display size is down scaled by 2 along the lines and the columns.
  - vpDisplay::SCALE_3, the display size is down scaled by 3 along the lines and the columns.
  - vpDisplay::SCALE_4, the display size is down scaled by 4 along the lines and the columns.
  - vpDisplay::SCALE_5, the display size is down scaled by 5 along the lines and the columns.
*/
vpDisplayOpenCV::vpDisplayOpenCV(vpImage<vpRGBa> &I, vpScaleType scaleType)
  : vpDisplay()
  , m_impl(new Impl())
{
  setScale(scaleType, I.getWidth(), I.getHeight());
  init(I);
}

/*!
  Constructor. Initialize a display to visualize a RGBa image (32 bits).

  \param I : Image to be displayed (not that image has to be initialized)
  \param x : The window is set at position x (column index).
  \param y : The window is set at position y (row index).
  \param title : Window title.
  \param scaleType : If this parameter is set to:
  - vpDisplay::SCALE_AUTO, the display size is adapted to ensure the image is fully displayed in the screen;
  - vpDisplay::SCALE_DEFAULT or vpDisplay::SCALE_1, the display size is the same than the image size.
  - vpDisplay::SCALE_2, the display size is down scaled by 2 along the lines and the columns.
  - vpDisplay::SCALE_3, the display size is down scaled by 3 along the lines and the columns.
  - vpDisplay::SCALE_4, the display size is down scaled by 4 along the lines and the columns.
  - vpDisplay::SCALE_5, the display size is down scaled by 5 along the lines and the columns.
*/
vpDisplayOpenCV::vpDisplayOpenCV(vpImage<vpRGBa> &I, int x, int y, const std::string &title, vpScaleType scaleType)
  : vpDisplay()
  , m_impl(new Impl())
{
  setScale(scaleType, I.getWidth(), I.getHeight());
  init(I, x, y, title);
}

/*!

  Constructor that just initialize the display position in the screen and the display title.

  \param x : The window is set at position x (column index).
  \param y : The window is set at position y (row index).
  \param title : Window title.

  To initialize the display size, you need to call init().

  \code
  #include <visp3/core/vpImage.h>
  #include <visp3/gui/vpDisplayOpenCV.h>

  #ifdef ENABLE_VISP_NAMESPACE
  using namespace VISP_NAMESPACE_NAME;
  #endif

  int main()
  {
    vpDisplayOpenCV d(100, 200, "My display");
    vpImage<unsigned char> I(240, 384);
    d.init(I);
  }
  \endcode
*/
vpDisplayOpenCV::vpDisplayOpenCV(int x, int y, const std::string &title)
  : vpDisplay()
  , m_impl(new Impl())
{
  m_windowXPosition = x;
  m_windowYPosition = y;

  if (!title.empty()) {
    m_title = title;
  }
  else {
    std::ostringstream s;
    s << vpDisplayOpenCV::Impl::m_nbWindows++;
    m_title = std::string("Window ") + s.str();
  }

  bool isInList;
  do {
    isInList = false;
    for (size_t i = 0; i < vpDisplayOpenCV::Impl::m_listTitles.size(); i++) {
      if (vpDisplayOpenCV::Impl::m_listTitles[i] == m_title) {
        std::ostringstream s;
        s << vpDisplayOpenCV::Impl::m_nbWindows++;
        m_title = std::string("Window ") + s.str();
        isInList = true;
        break;
      }
    }
  } while (isInList);

  vpDisplayOpenCV::Impl::m_listTitles.push_back(m_title);
}

/*!
  Basic constructor.

  To initialize the window position, title and size you may call
  init(vpImage<unsigned char> &, int, int, const std::string &) or
  init(vpImage<vpRGBa> &, int, int, const std::string &).

  \code
  #include <visp3/core/vpImage.h>
  #include <visp3/gui/vpDisplayOpenCV.h>

  #ifdef ENABLE_VISP_NAMESPACE
  using namespace VISP_NAMESPACE_NAME;
  #endif

  int main()
  {
    vpDisplayOpenCV d;
    vpImage<unsigned char> I(240, 384);
    d.init(I, 100, 200, "My display");
  }
  \endcode
*/
vpDisplayOpenCV::vpDisplayOpenCV()
  : vpDisplay()
  , m_impl(new Impl())
{ }

/*!
 * Copy constructor.
 */
vpDisplayOpenCV::vpDisplayOpenCV(const vpDisplayOpenCV &display) : vpDisplay(display)
{
  *this = display;
}

/*!
 * Copy operator.
 */
vpDisplayOpenCV &vpDisplayOpenCV::operator=(const vpDisplayOpenCV &display)
{
  m_impl = display.m_impl;
  return *this;
}

/*!
  Destructor.
*/
vpDisplayOpenCV::~vpDisplayOpenCV()
{
  closeDisplay();
  delete m_impl;
}

/*!
  Initialize the display (size, position and title) of a gray level image.

  \param I : Image to be displayed (not that image has to be initialized).
  \param x : The window is set at position x (column index).
  \param y : The window is set at position y (row index).
  \param title : Window title.

*/
void vpDisplayOpenCV::init(vpImage<unsigned char> &I, int x, int y, const std::string &title)
{
  if ((I.getHeight() == 0) || (I.getWidth() == 0)) {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "Image not initialized"));
  }
  setScale(m_scaleType, I.getWidth(), I.getHeight());
  init(I.getWidth(), I.getHeight(), x, y, title);
  I.display = this;
  m_displayHasBeenInitialized = true;
}

/*!
  Initialize the display (size, position and title) of a color
  image in RGBa format.

  \param I : Image to be displayed (not that image has to be initialized).
  \param x : The window is set at position x (column index).
  \param y : The window is set at position y (row index).
  \param title : Window title.

*/
void vpDisplayOpenCV::init(vpImage<vpRGBa> &I, int x, int y, const std::string &title)
{
  if ((I.getHeight() == 0) || (I.getWidth() == 0)) {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "Image not initialized"));
  }

  setScale(m_scaleType, I.getWidth(), I.getHeight());
  init(I.getWidth(), I.getHeight(), x, y, title);
  I.display = this;
  m_displayHasBeenInitialized = true;
}

/*!
  Initialize the display size, position and title.

  \param w : Window width.
  \param h : Window height.
  \param x : The window is set at position x (column index).
  \param y : The window is set at position y (row index).
  \param title : Window title.

  \exception vpDisplayException::notInitializedError If OpenCV was not build
  with an available display device suach as Gtk, Cocoa, Carbon, Qt.
*/
void vpDisplayOpenCV::init(unsigned int w, unsigned int h, int x, int y, const std::string &title)
{
  setScale(m_scaleType, w, h);

  this->m_width = w / m_scale;
  this->m_height = h / m_scale;

  if (x != -1) {
    this->m_windowXPosition = x;
  }
  if (y != -1) {
    this->m_windowYPosition = y;
  }

  if (m_title.empty()) {
    if (!title.empty()) {
      m_title = std::string(title);
    }
  }
  m_title = m_impl->init(m_title, m_windowXPosition, m_windowYPosition);

  m_displayHasBeenInitialized = true;
}

/*!
  \warning This method is not yet implemented.

  Set the font used to display a text in overlay. The display is
  performed using displayText().

  \param font : The expected font name. The available fonts are given by
  the "xlsfonts" binary. To choose a font you can also use the
  "xfontsel" binary.

  \note Under UNIX, to know all the available fonts, use the
  "xlsfonts" binary in a terminal. You can also use the "xfontsel" binary.

  \sa displayText()
*/
void vpDisplayOpenCV::setFont(const std::string &font)
{
  // Not yet implemented
  (void)font;
}

/*!
  Set the window title.

  \warning This method is not implemented yet.

  \param title : Window title.
 */
void vpDisplayOpenCV::setTitle(const std::string &title)
{
  // Not implemented
  (void)title;
}

/*!
  Set the window position in the screen.

  \param[in] winx : Horizontal position of the upper-left window's corner in the screen.
  \param[in] winy : Vertical position of the upper-left window's corner in the screen.

  \exception vpDisplayException::notInitializedError : If the video
  device is not initialized.
*/
void vpDisplayOpenCV::setWindowPosition(int winx, int winy)
{
  if (m_displayHasBeenInitialized) {
    this->m_windowXPosition = winx;
    this->m_windowYPosition = winy;
    cv::moveWindow(this->m_title.c_str(), winx, winy);
  }
  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
}

/*!
  Display the gray level image \e I (8bits).

  \warning Display has to be initialized.

  \warning Suppress the overlay drawing.

  \param I : Image to display.

  \sa init(), closeDisplay()
*/
void vpDisplayOpenCV::displayImage(const vpImage<unsigned char> &I)
{
  if (m_displayHasBeenInitialized) {
    m_impl->displayImage(I, m_scale, m_width, m_height);
  }
  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
}

/*!
  Display a selection of the gray level image \e I (8bits).

  \warning Display has to be initialized.

  \warning Suppress the overlay drawing in the region of interest.

  \param I : Image to display.
  \param iP : Top left corner of the region of interest.
  \param w : Width of the region of interest.
  \param h : Height of the region of interest.

  \sa init(), closeDisplay()
*/
void vpDisplayOpenCV::displayImageROI(const vpImage<unsigned char> &I, const vpImagePoint &iP, unsigned int w,
                                      unsigned int h)
{
  if (m_displayHasBeenInitialized) {
    m_impl->displayImageROI(I, iP, w, h, m_width, m_height, m_scale);
  }
  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
}

/*!
  Display the color image \e I in RGBa format (32bits).

  \warning Display has to be initialized.

  \warning suppress the overlay drawing

  \param I : Image to display.

  \sa init(), closeDisplay()
*/
void vpDisplayOpenCV::displayImage(const vpImage<vpRGBa> &I)
{

  if (m_displayHasBeenInitialized) {
    m_impl->displayImage(I, m_scale, m_width, m_height);
  }
  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
}

/*!
  Display a selection of the color image \e I in RGBa format (32bits).

  \warning Display has to be initialized.

  \warning Suppress the overlay drawing in the region of interest.

  \param I : Image to display.
  \param iP : Top left corner of the region of interest.
  \param w : Width of the region of interest.
  \param h : Height of the region of interest.

  \sa init(), closeDisplay()
*/
void vpDisplayOpenCV::displayImageROI(const vpImage<vpRGBa> &I, const vpImagePoint &iP, unsigned int w, unsigned int h)
{
  if (m_displayHasBeenInitialized) {
    m_impl->displayImageROI(I, iP, w, h, m_width, m_height, m_scale);
  }
  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
}

/*!
  \warning ot implemented yet

  \sa init(), closeDisplay()
*/
void vpDisplayOpenCV::displayImage(const unsigned char * /* I */)
{
  // not implemented
}

/*!

  Close the window.

  \sa init()

*/
void vpDisplayOpenCV::closeDisplay()
{
  if (m_displayHasBeenInitialized) {
    m_impl->closeDisplay(m_title);

    m_title.clear();

    m_displayHasBeenInitialized = false;
  }
}

/*!
  Flushes the OpenCV buffer.
  It's necessary to use this function to see the results of any drawing.

*/
void vpDisplayOpenCV::flushDisplay()
{
  if (m_displayHasBeenInitialized) {
    m_impl->flushDisplay(m_title);
  }
  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
}

/*!
  Flushes the OpenCV buffer.
  It's necessary to use this function to see the results of any drawing.

*/
void vpDisplayOpenCV::flushDisplayROI(const vpImagePoint & /*iP*/, const unsigned int /*width*/,
                                      const unsigned int /*height*/)
{
  if (m_displayHasBeenInitialized) {
    m_impl->flushDisplayROI(m_title);
  }
  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
}

/*!
  \warning Not implemented yet.
*/
void vpDisplayOpenCV::clearDisplay(const vpColor & /* color */)
{
  // Not implemented
}

/*!
  Display an arrow from image point \e ip1 to image point \e ip2.
  \param ip1 : Initial image point.
  \param ip2 : Final image point.
  \param color : Arrow color.
  \param w : Arrow width.
  \param h : Arrow height.
  \param thickness : Thickness of the lines used to display the arrow.
*/
void vpDisplayOpenCV::displayArrow(const vpImagePoint &ip1, const vpImagePoint &ip2, const vpColor &color,
                                   unsigned int w, unsigned int h, unsigned int thickness)
{
  if (m_displayHasBeenInitialized) {
    m_impl->displayArrow(ip1, ip2, color, w, h, thickness, m_scale);
  }
  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
}

/*!
  Display a string at the image point \e ip location.

  To select the font used to display the string, use setFont().

  \param ip : Upper left image point location of the string in the display.
  \param text : String to display in overlay.
  \param color : String color.

  \sa setFont()
*/
void vpDisplayOpenCV::displayText(const vpImagePoint &ip, const std::string &text, const vpColor &color)
{
  if (m_displayHasBeenInitialized) {
    m_impl->displayText(ip, text, color, m_scale);
  }
  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
}

/*!
  Display a circle.
  \param center : Circle center position.
  \param radius : Circle radius.
  \param color : RGB color used to display the rectangle.
  Alpha channel in color.A is here taken into account when cxx standard is set to cxx11 or higher.
  When alpha value is set to 255 (default) the rectangle is displayed without
  transparency. Closer is the alpha value to zero, more the rectangle is transparent.
  \param fill : When set to true fill the circle.
  \param thickness : Thickness of the circle. This parameter is only useful
  when \e fill is set to false.
*/
void vpDisplayOpenCV::displayCircle(const vpImagePoint &center, unsigned int radius, const vpColor &color, bool fill,
                                    unsigned int thickness)
{
  if (m_displayHasBeenInitialized) {
    m_impl->displayCircle(center, radius, color, fill, thickness, m_scale);
  }
  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
}

/*!
  Display a cross at the image point \e ip location.
  \param ip : Cross location.
  \param size : Size (width and height) of the cross.
  \param color : Cross color.
  \param thickness : Thickness of the lines used to display the cross.
*/
void vpDisplayOpenCV::displayCross(const vpImagePoint &ip, unsigned int size, const vpColor &color,
                                   unsigned int thickness)
{
  if (m_displayHasBeenInitialized) {
    m_impl->displayCross(ip, size, color, thickness, m_scale);
  }
  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
}

/*!
  Display a dashed line from image point \e ip1 to image point \e ip2.

  \param ip1 : Initial image point.
  \param ip2 : Final image point.
  \param color : Line color.
  \param thickness : Line thickness.
*/
void vpDisplayOpenCV::displayDotLine(const vpImagePoint &ip1, const vpImagePoint &ip2, const vpColor &color,
                                     unsigned int thickness)
{
  if (m_displayHasBeenInitialized) {
    m_impl->displayDotLine(ip1, ip2, color, thickness, m_scale);
  }
  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
}

/*!
  Display a line from image point \e ip1 to image point \e ip2.
  \param ip1 : Initial image point.
  \param ip2 : Final image point.
  \param color : Line color.
  \param thickness : Line thickness.
*/
void vpDisplayOpenCV::displayLine(const vpImagePoint &ip1, const vpImagePoint &ip2, const vpColor &color,
                                  unsigned int thickness)
{
  if (m_displayHasBeenInitialized) {
    m_impl->displayLine(ip1, ip2, color, thickness, m_scale);
  }
  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
}

/*!
  Display a point at the image point \e ip location.
  \param[in] ip : Point location.
  \param[in] color : Point color.
  \param[in] thickness : point thickness.
*/
void vpDisplayOpenCV::displayPoint(const vpImagePoint &ip, const vpColor &color, unsigned int thickness)
{
  if (m_displayHasBeenInitialized) {
    m_impl->displayPoint(ip, color, thickness, m_scale);
  }
  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
}

/*!
  Display a rectangle with \e topLeft as the top-left corner and \e
  width and \e height the rectangle size.

  \param[in] topLeft : Top-left corner of the rectangle.
  \param[in] w : Rectangle width.
  \param[in] h : Rectangle height.
  \param[in] color : RGB color used to display the rectangle.
  Alpha channel in color.A is here taken into account when cxx standard is set to cxx11 or higher.
  When alpha value is set to 255 (default) the rectangle is displayed without
  transparency. Closer is the alpha value to zero, more the rectangle is transparent.
  \param[in] fill : When set to true fill the rectangle.
  \param[in] thickness : Thickness of the four lines used to display the
  rectangle. This parameter is only useful when \e fill is set to
  false.
*/
void vpDisplayOpenCV::displayRectangle(const vpImagePoint &topLeft, unsigned int w, unsigned int h,
                                       const vpColor &color, bool fill, unsigned int thickness)
{
  if (m_displayHasBeenInitialized) {
    m_impl->displayRectangle(topLeft, w, h, color, fill, thickness, m_scale);
  }
  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
}
/*!
  Display a rectangle.

  \param[in] topLeft : Top-left corner of the rectangle.
  \param[in] bottomRight : Bottom-right corner of the rectangle.
  \param[in] color : RGB color used to display the rectangle.
  Alpha channel in color.A is here taken into account when cxx standard is set to cxx11 or higher.
  When alpha value is set to 255 (default) the rectangle is displayed without
  transparency. Closer is the alpha value to zero, more the rectangle is transparent.
  \param[in] fill : When set to true fill the rectangle.
  \param[in] thickness : Thickness of the four lines used to display the
  rectangle. This parameter is only useful when \e fill is set to
  false.
*/
void vpDisplayOpenCV::displayRectangle(const vpImagePoint &topLeft, const vpImagePoint &bottomRight,
                                       const vpColor &color, bool fill, unsigned int thickness)
{
  if (m_displayHasBeenInitialized) {
    unsigned int w = static_cast<unsigned int>(bottomRight.get_u() - topLeft.get_u() + 1);
    unsigned int h = static_cast<unsigned int>(bottomRight.get_v() - topLeft.get_v() + 1);
    displayRectangle(topLeft, w, h, color, fill, thickness);
  }
  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
}

/*!
  Display a rectangle.

  \param rectangle : Rectangle characteristics.
  \param color : RGB color used to display the rectangle.
  Alpha channel in color.A is here taken into account when cxx standard is set to cxx11 or higher.
  When alpha value is set to 255 (default) the rectangle is displayed without
  transparency. Closer is the alpha value to zero, more the rectangle is transparent.
  \param fill : When set to true fill the rectangle.
  \param thickness : Thickness of the four lines used to display the
  rectangle. This parameter is only useful when \e fill is set to
  false.
*/
void vpDisplayOpenCV::displayRectangle(const vpRect &rectangle, const vpColor &color, bool fill, unsigned int thickness)
{
  if (m_displayHasBeenInitialized) {
    displayRectangle(rectangle.getTopLeft(), static_cast<unsigned int>(rectangle.getWidth()), static_cast<unsigned int>(rectangle.getHeight()), color, fill, thickness);
  }
  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
}

/*!
  Wait for a click from one of the mouse button.

  \param blocking [in] : Blocking behavior.
  - When set to true, this method waits until a mouse button is
    pressed and then returns always true.
  - When set to false, returns true only if a mouse button is
    pressed, otherwise returns false.

  \return
  - true if a button was clicked. This is always the case if blocking is set
    to \e true.
  - false if no button was clicked. This can occur if blocking is set
    to \e false.
*/
bool vpDisplayOpenCV::getClick(bool blocking)
{
  bool ret = false;
  if (m_displayHasBeenInitialized) {
    vpImagePoint ip;
    vpMouseButton::vpMouseButtonType button;
    ret = getClick(ip, button, blocking);
  }
  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
  return ret;
}

/*!
  Wait for a click from one of the mouse button and get the position
  of the clicked image point.

  \param ip [out] : The coordinates of the clicked image point.

  \param blocking [in] : true for a blocking behaviour waiting a mouse
  button click, false for a non blocking behaviour.

  \return
  - true if a button was clicked. This is always the case if blocking is set
    to \e true.
  - false if no button was clicked. This can occur if blocking is set
    to \e false.
*/
bool vpDisplayOpenCV::getClick(vpImagePoint &ip, bool blocking)
{
  bool ret = false;

  if (m_displayHasBeenInitialized) {
    vpMouseButton::vpMouseButtonType button;
    ret = getClick(ip, button, blocking);
  }
  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
  return ret;
}

/*!
  Wait for a mouse button click and get the position of the clicked
  pixel. The button used to click is also set.

  \param ip [out] : The coordinates of the clicked image point.

  \param button [out] : The button used to click.

  \param blocking [in] :
  - When set to true, this method waits until a mouse button is
    pressed and then returns always true.
  - When set to false, returns true only if a mouse button is
    pressed, otherwise returns false.

  \return true if a mouse button is pressed, false otherwise. If a
  button is pressed, the location of the mouse pointer is updated in
  \e ip.
*/
bool vpDisplayOpenCV::getClick(vpImagePoint &ip, vpMouseButton::vpMouseButtonType &button, bool blocking)
{
  bool ret = false;

  if (m_displayHasBeenInitialized) {
    ret = m_impl->getClick(ip, button, blocking, m_title, m_scale);
  }
  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
  return ret;
}

/*!
  Wait for a mouse button click release and get the position of the
  image point were the click release occurs.  The button used to click is
  also set. Same method as getClick(unsigned int&, unsigned int&,
  vpMouseButton::vpMouseButtonType &, bool).

  \param ip [out] : Position of the clicked image point.

  \param button [in] : Button used to click.

  \param blocking [in] : true for a blocking behaviour waiting a mouse
  button click, false for a non blocking behaviour.

  \return
  - true if a button was clicked. This is always the case if blocking is set
    to \e true.
  - false if no button was clicked. This can occur if blocking is set
    to \e false.

  \sa getClick(vpImagePoint &, vpMouseButton::vpMouseButtonType &, bool)
*/
bool vpDisplayOpenCV::getClickUp(vpImagePoint &ip, vpMouseButton::vpMouseButtonType &button, bool blocking)
{
  bool ret = false;
  if (m_displayHasBeenInitialized) {
    ret = m_impl->getClickUp(ip, button, blocking, m_scale);
  }
  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
  return ret;
}

/*
  Gets the displayed image (including the overlay plane)
  and returns an RGBa image.
*/
void vpDisplayOpenCV::getImage(vpImage<vpRGBa> &I)
{
  m_impl->getImage(I);
}

/*!
  Get a keyboard event.

  \param blocking [in] : Blocking behavior.
  - When set to true, this method waits until a key is
    pressed and then returns always true.
  - When set to false, returns true only if a key is
    pressed, otherwise returns false.

  \return
  - true if a key was pressed. This is always the case if blocking is set
    to \e true.
  - false if no key was pressed. This can occur if blocking is set
    to \e false.
*/
bool vpDisplayOpenCV::getKeyboardEvent(bool blocking)
{
  if (m_displayHasBeenInitialized) {
    int delay;
    flushDisplay();
    if (blocking)
      delay = 0;
    else
      delay = 10;

    int key_pressed = cv::waitKey(delay);

    if (key_pressed == -1)
      return false;
    return true;
  }
  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
  // return false; // Never reached after throw()
}

/*!
  Get a keyboard event.

  \param blocking [in] : Blocking behavior.
  - When set to true, this method waits until a key is
    pressed and then returns always true.
  - When set to false, returns true only if a key is
    pressed, otherwise returns false.

  \param key [out]: If possible, an ISO Latin-1 character
  corresponding to the keyboard key.

  \return
  - true if a key was pressed. This is always the case if blocking is set
    to \e true.
  - false if no key was pressed. This can occur if blocking is set
    to \e false.
*/
bool vpDisplayOpenCV::getKeyboardEvent(std::string &key, bool blocking)
{
  if (m_displayHasBeenInitialized) {
    int delay;
    flushDisplay();
    if (blocking)
      delay = 0;
    else
      delay = 10;

    int key_pressed = cv::waitKey(delay);
    if (key_pressed == -1)
      return false;
    else {
      // std::cout << "Key pressed: \"" << key_pressed << "\"" << std::endl;
      std::stringstream ss;
      ss << key_pressed;
      key = ss.str();
    }
    return true;
  }
  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
  // return false; // Never reached after throw()
}

/*!
  Get the coordinates of the mouse pointer.

  \param[out] ip : The coordinates of the mouse pointer.

  \return true if a pointer motion event was received, false otherwise.

  \exception vpDisplayException::notInitializedError : If the display
  was not initialized.
*/
bool vpDisplayOpenCV::getPointerMotionEvent(vpImagePoint &ip)
{
  bool ret = false;

  if (m_displayHasBeenInitialized) {
    ret = m_impl->getPointerMotionEvent(ip, m_scale);
  }

  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
  return ret;
}

/*!
  Get the coordinates of the mouse pointer.

  \param[out] ip : The coordinates of the mouse pointer.

  \return true.

  \exception vpDisplayException::notInitializedError : If the display
  was not initialized.
*/
bool vpDisplayOpenCV::getPointerPosition(vpImagePoint &ip)
{
  if (m_displayHasBeenInitialized) {
    m_impl->getPointerPosition(ip, m_scale);
    return false;
  }
  else {
    throw(vpDisplayException(vpDisplayException::notInitializedError, "OpenCV not initialized"));
  }
}

/*!
  Gets screen resolution in pixels.
  \param[out] w : Horizontal screen resolution.
  \param[out] h : Vertical screen resolution.
 */
void vpDisplayOpenCV::getScreenSize(unsigned int &w, unsigned int &h)
{
  w = h = 0;

#if defined(VISP_HAVE_X11)
  vpDisplayX d;
  d.getScreenSize(w, h);
#elif defined(VISP_HAVE_XRANDR)
  std::string command = "xrandr | grep '*'";
  FILE *fpipe = (FILE *)popen(command.c_str(), "r");
  char line[256];
  while (fgets(line, sizeof(line), fpipe)) {
    std::string str(line);
    std::size_t found = str.find("Failed");

    if (found == std::string::npos) {
      std::vector<std::string> elm;
      elm = vpIoTools::splitChain(str, " ");
      for (size_t i = 0; i < elm.size(); i++) {
        if (!elm[i].empty()) {
          std::vector<std::string> resolution = vpIoTools::splitChain(elm[i], "x");
          if (resolution.size() == 2) {
            std::istringstream sswidth(resolution[0]), ssheight(resolution[1]);
            sswidth >> w;
            ssheight >> h;
            break;
          }
        }
      }
    }
  }
  pclose(fpipe);
#elif defined(_WIN32)
#if !defined(WINRT)
  w = static_cast<unsigned int>(GetSystemMetrics(SM_CXSCREEN));
  h = static_cast<unsigned int>(GetSystemMetrics(SM_CYSCREEN));
#else
  throw(vpException(vpException::functionNotImplementedError, "The function vpDisplayOpenCV::getScreenSize() is not "
                    "implemented on winrt"));
#endif
#endif
}

/*!
  Gets the screen horizontal resolution in pixels.
 */
unsigned int vpDisplayOpenCV::getScreenWidth()
{
  unsigned int width, height;
  getScreenSize(width, height);
  return width;
}

/*!
  Gets the screen vertical resolution in pixels.
 */
unsigned int vpDisplayOpenCV::getScreenHeight()
{
  unsigned int width, height;
  getScreenSize(width, height);
  return height;
}

END_VISP_NAMESPACE

#elif !defined(VISP_BUILD_SHARED_LIBS)
// Work around to avoid warning: libvisp_gui.a(vpDisplayOpenCV.cpp.o) has no symbols
void dummy_vpDisplayOpenCV() { }
#endif
