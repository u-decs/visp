/*! \example tutorial-klt-tracker-live.cpp */
#include <iostream>

#include <visp3/core/vpConfig.h>

//! [Undef grabber]
// Comment / uncomment following lines to use the specific 3rd party compatible with your camera
// #undef VISP_HAVE_V4L2
// #undef HAVE_OPENCV_HIGHGUI
// #undef HAVE_OPENCV_VIDEOIO
//! [Undef grabber]

#if defined(HAVE_OPENCV_HIGHGUI) && defined(HAVE_OPENCV_IMGPROC) && defined(HAVE_OPENCV_VIDEO) && \
  (defined(VISP_HAVE_V4L2) || ((VISP_HAVE_OPENCV_VERSION < 0x030000) && defined(HAVE_OPENCV_HIGHGUI)) || \
                              ((VISP_HAVE_OPENCV_VERSION >= 0x030000) && defined(HAVE_OPENCV_VIDEOIO)))

#ifdef VISP_HAVE_MODULE_SENSOR
#include <visp3/sensor/vpV4l2Grabber.h>
#endif
#include <visp3/core/vpImageConvert.h>
#include <visp3/gui/vpDisplayOpenCV.h>
#include <visp3/io/vpVideoReader.h>
#include <visp3/klt/vpKltOpencv.h>

#if (VISP_HAVE_OPENCV_VERSION < 0x030000) && defined(HAVE_OPENCV_HIGHGUI)
#include <opencv2/highgui/highgui.hpp> // for cv::VideoCapture
#elif (VISP_HAVE_OPENCV_VERSION >= 0x030000) && defined(HAVE_OPENCV_VIDEOIO)
#include <opencv2/videoio/videoio.hpp>
#endif

int main(int argc, const char *argv[])
{
#ifdef ENABLE_VISP_NAMESPACE
  using namespace VISP_NAMESPACE_NAME;
#endif

  try {
    bool opt_init_by_click = false;
    int opt_device = 0;

    for (int i = 1; i < argc; i++) {
      if (std::string(argv[i]) == "--init-by-click") {
        opt_init_by_click = true;
      }
      else if (std::string(argv[i]) == "--device" && i + 1 < argc) {
        opt_device = atoi(argv[++i]);
      }
      else if (std::string(argv[i]) == "--help") {
        std::cout << "Usage: " << argv[0] << " [--init-by-click] [--device <camera device>] [--help]" << std::endl;
        return EXIT_SUCCESS;
      }
    }

    vpImage<unsigned char> I;

#if defined(VISP_HAVE_V4L2)
    std::cout << "Use v4l2 grabber..." << std::endl;
    vpV4l2Grabber g;
    std::ostringstream device;
    device << "/dev/video" << opt_device;
    g.setDevice(device.str());
    g.open(I);
    g.acquire(I);
#elif ((VISP_HAVE_OPENCV_VERSION < 0x030000) && defined(HAVE_OPENCV_HIGHGUI)) || ((VISP_HAVE_OPENCV_VERSION >= 0x030000) && defined(HAVE_OPENCV_VIDEOIO))
    std::cout << "Use OpenCV grabber..." << std::endl;
    cv::VideoCapture g(opt_device);
    if (!g.isOpened()) { // check if we succeeded
      std::cout << "Failed to open the camera" << std::endl;
      return EXIT_FAILURE;
    }
    cv::Mat frame;
    g >> frame; // get a new frame from camera
    vpImageConvert::convert(frame, I);
#endif

    cv::Mat cvI;
    vpImageConvert::convert(I, cvI);

    // Display initialisation
    vpDisplayOpenCV d(I, 0, 0, "Klt tracking");
    vpDisplay::display(I);
    vpDisplay::flush(I);

    vpKltOpencv tracker;
    // Set tracker parameters
    tracker.setMaxFeatures(200);
    tracker.setWindowSize(10);
    tracker.setQuality(0.01);
    tracker.setMinDistance(15);
    tracker.setHarrisFreeParameter(0.04);
    tracker.setBlockSize(9);
    tracker.setUseHarris(1);
    tracker.setPyramidLevels(3);

    // Initialise the tracking
    if (opt_init_by_click) {
      vpMouseButton::vpMouseButtonType button;
      std::vector<cv::Point2f> guess;
      vpImagePoint ip;
      do {
        vpDisplay::displayText(I, 10, 10, "Left click to select a point, right to start tracking", vpColor::red);
        if (vpDisplay::getClick(I, ip, button, false)) {
          if (button == vpMouseButton::button1) {
            guess.push_back(cv::Point2f((float)ip.get_u(), (float)ip.get_v()));
            vpDisplay::displayText(I, 10, 10, "Left click to select a point, right to start tracking", vpColor::red);
            vpDisplay::displayCross(I, ip, 12, vpColor::green);
          }
        }
        vpDisplay::flush(I);
        vpTime::wait(20);
      } while (button != vpMouseButton::button3);
      tracker.initTracking(cvI, guess);
    }
    else {
      tracker.initTracking(cvI);
    }

    while (1) {
#if defined(VISP_HAVE_V4L2)
      g.acquire(I);
#elif ((VISP_HAVE_OPENCV_VERSION < 0x030000) && defined(HAVE_OPENCV_HIGHGUI)) || ((VISP_HAVE_OPENCV_VERSION >= 0x030000) && defined(HAVE_OPENCV_VIDEOIO))
      g >> frame;
      vpImageConvert::convert(frame, I);
#endif
      vpDisplay::display(I);

      vpImageConvert::convert(I, cvI);
      tracker.track(cvI);

      tracker.display(I, vpColor::red);
      vpDisplay::displayText(I, 10, 10, "Click to quit", vpColor::red);
      vpDisplay::flush(I);
      if (vpDisplay::getClick(I, false))
        break;
    }
  }
  catch (const vpException &e) {
    std::cout << "Catch an exception: " << e << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

#else

int main()
{
#if !defined(HAVE_OPENCV_HIGHGUI)
  std::cout << "This tutorial needs OpenCV highgui module that is missing." << std::endl;
#endif
#if !defined(HAVE_OPENCV_IMGPROC)
  std::cout << "This tutorial needs OpenCV imgproc module that is missing." << std::endl;
#endif
#if !defined(HAVE_OPENCV_VIDEO)
  std::cout << "This tutorial needs OpenCV video module that is missing." << std::endl;
#endif
#if !(defined(VISP_HAVE_V4L2) || ((VISP_HAVE_OPENCV_VERSION < 0x030000) && defined(HAVE_OPENCV_HIGHGUI)) || \
                                 ((VISP_HAVE_OPENCV_VERSION >= 0x030000) && defined(HAVE_OPENCV_VIDEOIO)))
  std::cout << "This tutorial needs V4l2 or OpenCV grabber capabilities." << std::endl;
#endif
}

#endif
