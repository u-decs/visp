//! \example tutorial-bridge-opencv-camera-param.cpp
#include <iostream>

#include <visp3/core/vpConfig.h>

#if defined(VISP_HAVE_OPENCV) && defined(HAVE_OPENCV_IMGPROC) \
  && (((VISP_HAVE_OPENCV_VERSION < 0x050000) && defined(HAVE_OPENCV_CALIB3D)) || ((VISP_HAVE_OPENCV_VERSION >= 0x050000) && defined(HAVE_OPENCV_3D)))

#include <visp3/core/vpCameraParameters.h>
#include <visp3/core/vpImageConvert.h>
#include <visp3/io/vpImageIo.h>

#if defined(HAVE_OPENCV_CALIB3D)
#include <opencv2/calib3d/calib3d.hpp>
#elif defined(HAVE_OPENCV_3D)
#include <opencv2/3d.hpp>
#endif
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

int main()
{
#ifdef ENABLE_VISP_NAMESPACE
  using namespace VISP_NAMESPACE_NAME;
#endif
  //! [Set ViSP camera parameters]
  double u0 = 326.6;
  double v0 = 215.0;
  double px = 582.7;
  double py = 580.6;
  double kud = -0.3372;
  double kdu = 0.4021;
  vpCameraParameters cam(px, py, u0, v0, kud, kdu);
  //! [Set ViSP camera parameters]

  //! [Set OpenCV camera parameters]
  cv::Mat K = (cv::Mat_<double>(3, 3) << cam.get_px(), 0, cam.get_u0(), 0, cam.get_py(), cam.get_v0(), 0, 0, 1);
  cv::Mat D = (cv::Mat_<double>(4, 1) << cam.get_kud(), 0, 0, 0);
  //! [Set OpenCV camera parameters]

  //! [Load ViSP image]
  vpImage<unsigned char> I;
  std::string image_name = "chessboard.jpeg";
  std::cout << "Read image: " << image_name << std::endl;
  vpImageIo::read(I, image_name);
  //! [Load ViSP image]

  //! [Convert ViSP 2 OpenCV image]
  cv::Mat image;
  vpImageConvert::convert(I, image);
  //! [Convert ViSP 2 OpenCV image]

  //! [Undistort OpenCV image]
  cv::Mat imageUndistorted;
  cv::undistort(image, imageUndistorted, K, D);
  //! [Undistort OpenCV image]

  //! [Convert OpenCV 2 ViSP image]
  vpImage<unsigned char> IUndistorted;
  vpImageConvert::convert(imageUndistorted, IUndistorted);
  //! [Convert OpenCV 2 ViSP image]

  //! [Save image]
  image_name = "chessboard-undistorted.jpeg";
  std::cout << "Save undistorted image: " << image_name << std::endl;
  vpImageIo::write(IUndistorted, image_name);
  //! [Save image]
}

#else
int main()
{
#if !defined(HAVE_OPENCV_IMGPROC)
  std::cout << "This tutorial requires OpenCV imgproc module." << std::endl;
#endif
#if defined(VISP_HAVE_OPENCV) && (VISP_HAVE_OPENCV_VERSION < 0x050000) && !defined(HAVE_OPENCV_CALIB3D)
  std::cout << "This tutorial requires OpenCV calib3d module." << std::endl;
#endif
#if defined(VISP_HAVE_OPENCV) && (VISP_HAVE_OPENCV_VERSION >= 0x050000) && !defined(HAVE_OPENCV_3D)
  std::cout << "This tutorial requires OpenCV 3d module." << std::endl;
#endif
  return EXIT_SUCCESS;
}
#endif
