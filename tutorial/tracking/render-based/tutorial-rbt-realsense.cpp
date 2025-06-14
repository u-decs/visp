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
//! \example tutorial-rbt-realsense.cpp
#include <iostream>
#include <visp3/core/vpConfig.h>

#ifdef ENABLE_VISP_NAMESPACE
using namespace VISP_NAMESPACE_NAME;
#endif

#ifndef VISP_HAVE_REALSENSE2

int main()
{
  std::cerr << "To run this tutorial, recompile ViSP with the Realsense third party library" << std::endl;
  return EXIT_SUCCESS;
}

#else
#include <visp3/sensor/vpRealSense2.h>
#include <visp3/io/vpParseArgv.h>

#include <visp3/ar/vpPanda3DFrameworkManager.h>

#include <visp3/rbt/vpRBTracker.h>

#include "render-based-tutorial-utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
/**
 * \brief Realsense specific arguments
*/
struct CmdArguments
{
  CmdArguments() : height(480), width(848), fps(60)
  { }

  void registerArguments(vpJsonArgumentParser &parser)
  {
    parser.addArgument("--height", height, false, "Realsense requested image height")
      .addArgument("--width", width, false, "Realsense requested image width")
      .addArgument("--fps", fps, false, "Realsense requested framerate");
  }
  unsigned int height, width, fps;
};
#endif

/**
 * \brief Convert from a raw uint16_t depth map to a float depth map in meters and a display uint8_t depth map
 *
 * \param depthRaw raw depth map from the realsense
 * \param depthScale uint16_t to meters scale factor
 * \param maxZDisplay Max Z value when displaying (used for color scaling)
 * \param depth output depth map in meters
 * \param IdepthDisplay Displayable output depth map
*/
void updateDepth(const vpImage<uint16_t> &depthRaw, float depthScale, float maxZDisplay, vpImage<float> &depth, vpImage<unsigned char> &IdepthDisplay)
{
  depth.resize(depthRaw.getHeight(), depthRaw.getWidth());
#ifdef VISP_HAVE_OPENMP
#pragma omp parallel for
#endif
  for (int i = 0; i < static_cast<int>(depthRaw.getSize()); ++i) {
    depth.bitmap[i] = depthScale * static_cast<float>(depthRaw.bitmap[i]);
    IdepthDisplay.bitmap[i] = depth.bitmap[i] > maxZDisplay ? 0 : static_cast<unsigned int>((depth.bitmap[i] / maxZDisplay) * 255.f);
  }
}

int main(int argc, const char **argv)
{
  //! [Command line parsing]
  // Read the command line options
  vpRBTrackerTutorial::BaseArguments baseArgs;
  CmdArguments realsenseArgs;
  vpRBTrackerTutorial::vpRBExperimentLogger logger;
  vpRBTrackerTutorial::vpRBExperimentPlotter plotter;


  vpJsonArgumentParser parser(
    "Tutorial showing the usage of the Render-Based tracker with a RealSense camera",
    "--config", "/"
  );

  baseArgs.registerArguments(parser);
  realsenseArgs.registerArguments(parser);
  logger.registerArguments(parser);
  plotter.registerArguments(parser);

  parser.parse(argc, argv);

  baseArgs.postProcessArguments();
  plotter.postProcessArguments(baseArgs.display);
  //! [Command line parsing]


  if (baseArgs.enableRenderProfiling) {
    vpRBTrackerTutorial::enableRendererProfiling();
  }

  //! [Loading config]
  std::cout << "Loading tracker: " << baseArgs.trackerConfiguration << std::endl;
  vpRBTracker tracker;
  tracker.loadConfigurationFile(baseArgs.trackerConfiguration);
  if (!baseArgs.modelPath.empty()) {
    tracker.setModelPath(baseArgs.modelPath);
  }
  //! [Loading config]


  //! [Realsense opening]
  const unsigned int width = realsenseArgs.width, height = realsenseArgs.height;
  const unsigned fps = realsenseArgs.fps;

  vpRealSense2 realsense;
  std::cout << "Opening realsense with settings: " << width << "x" << height << " @ " << fps << "fps" << std::endl;
  rs2::config config;
  config.enable_stream(RS2_STREAM_COLOR, width, height, RS2_FORMAT_RGBA8, fps);
  config.enable_stream(RS2_STREAM_DEPTH, width, height, RS2_FORMAT_Z16, fps);
  rs2::align align_to(RS2_STREAM_COLOR);
  try {
    realsense.open(config);
  }
  catch (const vpException &e) {
    std::cout << "Caught an exception: " << e.what() << std::endl;
    std::cout << "Check if the Realsense camera is connected..." << std::endl;
    return EXIT_FAILURE;
  }
  const float depthScale = realsense.getDepthScale(); // used to convert uint16_t to meters
  //! [Realsense opening]

  //! [Tracker update]
  const vpCameraParameters cam = realsense.getCameraParameters(RS2_STREAM_COLOR, vpCameraParameters::perspectiveProjWithoutDistortion);
  tracker.setCameraParameters(cam, height, width);
  tracker.startTracking();
  //! [Tracker update]

//! [Images]
  vpImage<vpRGBa> Icol(height, width); // Color image
  vpImage<unsigned char> Id(height, width); // Grayscale image, converted from Icol
  vpImage<uint16_t> depthRaw(height, width); // Raw depth map, in realsense format
  vpImage<float> depth(height, width); // Depth map, in meters

  // Display versions of raw image data
  vpImage<unsigned char> IdepthDisplay(height, width);
  vpImage<unsigned char> IProbaDisplay(height, width);
  vpImage<unsigned char> cannyDisplay(height, width);
  vpImage<vpRGBa> InormDisplay(height, width);
  //! [Images]

  //camera warmup, colors may appear washed out in the first few frames
  for (int i = 0; i < 10; ++i) {
    realsense.acquire(Icol);
  }
  vpImageConvert::convert(Icol, Id);



  std::cout << "Creating displays..." << std::endl;
  std::vector<std::shared_ptr<vpDisplay>> displays, displaysDebug;

  //! [Create displays]
  if (baseArgs.display) {
    displays = vpRBTrackerTutorial::createDisplays(Id, Icol, IdepthDisplay, IProbaDisplay);
    if (baseArgs.debugDisplay) {
      displaysDebug = vpDisplayFactory::makeDisplayGrid(1, 2,
        0, 0,
        20, 20,
        "Normals in object frame", InormDisplay,
        "Depth canny", cannyDisplay
      );
    }
    plotter.init(displays);
  }
  //! [Create displays]

  //! [Wait before init]
  if (baseArgs.display && !baseArgs.hasInlineInit()) {
    bool ready = false;
    while (!ready) {
      realsense.acquire((unsigned char *)Icol.bitmap, (unsigned char *)depthRaw.bitmap, nullptr, nullptr, &align_to);
      updateDepth(depthRaw, depthScale, baseArgs.maxDepthDisplay, depth, IdepthDisplay);
      vpImageConvert::convert(Icol, Id);
      vpDisplay::display(Icol); vpDisplay::display(Id); vpDisplay::display(IdepthDisplay);
      vpDisplay::flush(Icol); vpDisplay::flush(Id); vpDisplay::flush(IdepthDisplay);
      if (vpDisplay::getClick(Id, false)) {
        ready = true;
      }
      else {
        vpTime::wait(1000.0 / fps);
      }
    }
  }


  //! [Wait before init]

  //! [Init]
  vpHomogeneousMatrix cMo;
  // Manual initialization of the tracker
  std::cout << "Starting init" << std::endl;
  if (baseArgs.hasInlineInit()) {
    tracker.setPose(baseArgs.cMoInit);
  }
  else if (baseArgs.display) {
    tracker.initClick(Id, baseArgs.initFile, true);
    tracker.getPose(cMo);
  }
  else {
    throw vpException(vpException::notImplementedError, "Cannot initialize tracking: no initial pose provided or display to perform click initialization.");
  }

  std::cout << "Starting pose: " << vpPoseVector(cMo).t() << std::endl;
  //! [Init]

  if (baseArgs.display) {
    vpDisplay::flush(Id);
  }

  //! [Tracking loop]
  logger.startLog();
  unsigned int iter = 1;
  // Main tracking loop
  double expStart = vpTime::measureTimeMs();
  while (true) {
    double frameStart = vpTime::measureTimeMs();
    // Acquire images
    //! [Image acquisition]
    realsense.acquire((unsigned char *)Icol.bitmap, (unsigned char *)depthRaw.bitmap, nullptr, nullptr, &align_to);
    updateDepth(depthRaw, depthScale, baseArgs.maxDepthDisplay, depth, IdepthDisplay);
    vpImageConvert::convert(Icol, Id);
    //![Image acquisition]
    //! [Call to tracker]
    double trackingStart = vpTime::measureTimeMs();
    vpRBTrackingResult result = tracker.track(Id, Icol, depth);
    double trackingEnd = vpTime::measureTimeMs();
    tracker.getPose(cMo);
    //! [Call to tracker]

    //! [Result parsing]
    switch (result.getStoppingReason()) {
    case vpRBTrackingStoppingReason::EXCEPTION:
    {
      std::cout << "Encountered an exception during tracking, pose was not updated!" << std::endl;
      break;
    }
    case vpRBTrackingStoppingReason::NOT_ENOUGH_FEATURES:
    {
      std::cout << "There were not enough feature to perform tracking!" << std::endl;
      break;
    }
    case vpRBTrackingStoppingReason::OBJECT_NOT_IN_IMAGE:
    {
      std::cout << "Object is not in image!" << std::endl;
      break;
    }
    case vpRBTrackingStoppingReason::CONVERGENCE_CRITERION:
    {
      std::cout << "Convergence criterion reached:" << std::endl;
      std::cout << "- Num iterations: " << result.getNumIterations() << std::endl;
      std::cout << "- Convergence criterion: " << *(result.getConvergenceMetricValues().end() - 1) << std::endl;
      break;
    }
    case vpRBTrackingStoppingReason::MAX_ITERS:
    {
      break;
    }
    default:
    { }

    }

    const std::shared_ptr<vpRBDriftDetector> driftDetector = tracker.getDriftDetector();
    if (driftDetector) {
      if (driftDetector->getScore() < 0.25) {
        std::cout << "Drift detection has low confidence score: " << driftDetector->getScore() << std::endl;
      }
    }

    //! [Result parsing]

    double displayStart = vpTime::measureTimeMs();

    //! [Display]
    if (baseArgs.display) {
      if (baseArgs.debugDisplay) {
        const vpRBFeatureTrackerInput &lastFrame = tracker.getMostRecentFrame();

        vpRBTrackerTutorial::displayCanny(lastFrame.renders.silhouetteCanny, cannyDisplay, lastFrame.renders.isSilhouette);
      }

      vpDisplay::display(Id); vpDisplay::display(Icol); vpDisplay::display(IdepthDisplay);
      tracker.display(Id, Icol, IdepthDisplay);

      vpDisplay::displayFrame(Icol, cMo, cam, 0.05, vpColor::none, 2);
      vpDisplay::displayText(Id, 20, 5, "Right click to exit", vpColor::red);
      if (driftDetector) {
        std::stringstream ss;
        ss << "Confidence score: " << std::setprecision(2) << driftDetector->getScore() << std::endl;
        vpDisplay::displayText(Id, Id.getHeight() - 40, 5, ss.str(), vpColor::red);
      }
      vpMouseButton::vpMouseButtonType button;
      if (vpDisplay::getClick(Id, button, false)) {
        if (button == vpMouseButton::button3) {
          break;
        }
      }
      tracker.displayMask(IProbaDisplay);
      vpDisplay::display(IProbaDisplay);
      vpDisplay::flush(Id); vpDisplay::flush(Icol);
      vpDisplay::flush(IdepthDisplay); vpDisplay::flush(IProbaDisplay);
    }
    //! [Display]

    const double displayEnd = vpTime::measureTimeMs();

    const double frameEnd = vpTime::measureTimeMs();
    //! [Logging]

    logger.logFrame(tracker, iter, Id, Icol, IdepthDisplay, IProbaDisplay);
    std::cout << "Iter " << iter << ": " << round(frameEnd - frameStart) << "ms" << std::endl;
    std::cout << "- Tracking:  " << round(trackingEnd - trackingStart) << "ms" << std::endl;
    std::cout << "- Display: " << round(displayEnd - displayStart) << "ms" << std::endl;
    if (baseArgs.verbose) {
      std::cout << result.timer() << std::endl;
    }
    plotter.plot(tracker, (frameEnd - expStart) / 1000.0);
    //! [Logging]
    iter++;
  }
  //! [Tracking loop]

  //! [Cleanup]
  logger.close();
  vpPanda3DFrameworkManager::getInstance().exit();
  return EXIT_SUCCESS;
  //! [Cleanup]
}
#endif
