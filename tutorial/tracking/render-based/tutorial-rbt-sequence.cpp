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
//! \example tutorial-rbt-sequence.cpp
#include <iostream>
#include <visp3/core/vpConfig.h>

#if defined(VISP_HAVE_NLOHMANN_JSON)

#include <visp3/core/vpException.h>
#include <visp3/core/vpImageException.h>
#include <visp3/core/vpIoTools.h>
#include <visp3/core/vpRGBa.h>

#include <visp3/io/vpVideoReader.h>
#include <visp3/io/vpVideoWriter.h>

#include <visp3/ar/vpPanda3DFrameworkManager.h>

#include <visp3/rbt/vpRBTracker.h>

#ifdef ENABLE_VISP_NAMESPACE
using namespace VISP_NAMESPACE_NAME;
#endif

#include "render-based-tutorial-utils.h"

struct CmdArguments
{
  CmdArguments() : startFrame(-1), frameStep(1), stepByStep(false)
  { }

  void registerArguments(vpJsonArgumentParser &parser)
  {
    parser
      .addArgument("--color", colorSequence, true, "The color sequence (in video reader format, eg., /path/to/I\%04d.png)")
      .addArgument("--depth", depthFolder, false, "The depth images associated to the color sequence. Frames should be aligned")
      .addArgument("--start", startFrame, false, "The first frame of the sequence")
      .addArgument("--step", frameStep, false, "How many frames should be read between calls to the tracker")
      .addFlag("--step-by-step", stepByStep, "Go through the sequence interactively, frame by frame");
  }

  void postProcessArguments()
  {
    if (colorSequence.empty()) {
      throw vpException(vpException::badValue, "Input sequence should not be empty");
    }
  }

  std::string colorSequence;
  std::string depthFolder;
  int startFrame;
  unsigned int frameStep;
  bool stepByStep;
};

int main(int argc, const char **argv)
{
  vpRBTrackerTutorial::BaseArguments baseArgs;
  CmdArguments sequenceArgs;
  vpRBTrackerTutorial::vpRBExperimentLogger logger;
  vpRBTrackerTutorial::vpRBExperimentPlotter plotter;

  vpJsonArgumentParser parser("Tutorial showing how to use the Render-Based Tracker on an offline sequence", "--config", "/");
  baseArgs.registerArguments(parser);
  sequenceArgs.registerArguments(parser);
  logger.registerArguments(parser);
  plotter.registerArguments(parser);

  parser.parse(argc, argv);

  baseArgs.postProcessArguments();
  sequenceArgs.postProcessArguments();
  plotter.postProcessArguments(baseArgs.display);

  if (baseArgs.enableRenderProfiling) {
    vpRBTrackerTutorial::enableRendererProfiling();
  }

  baseArgs.display = true;
  // Get the option values

  logger.startLog();

  // Set tracking and rendering parameters
  vpCameraParameters cam;

  vpRBTracker tracker;
  tracker.loadConfigurationFile(baseArgs.trackerConfiguration);
  tracker.startTracking();
  cam = tracker.getCameraParameters();

  // VideoReader to read images from disk
  vpImage<vpRGBa> Icol;
  vpVideoReader readerRGB;
  std::cout << "Input video" << std::endl;
  std::cout << "  Filename   : " << sequenceArgs.colorSequence << std::endl;
  if (sequenceArgs.startFrame >= 0) {
    readerRGB.setFirstFrameIndex(sequenceArgs.startFrame);
    std::cout << "  First frame: " << sequenceArgs.startFrame << std::endl;
  }
  readerRGB.setFileName(sequenceArgs.colorSequence);
  readerRGB.open(Icol);
  readerRGB.acquire(Icol);

  if (sequenceArgs.startFrame < 0) {
    std::cout << "  First frame: " << readerRGB.getFirstFrameIndex() << std::endl;
  }

  const int width = readerRGB.getWidth();
  const int height = readerRGB.getHeight();

  std::cout << "  Image size : " << width << " x " << height << std::endl;

  vpImage<unsigned char> Id(height, width);
  vpImage<float> depth(0, 0);
  vpImage<unsigned char> depthDisplay(height, width);
  vpImage<unsigned char> IProbaDisplay(height, width);
  vpImage<vpRGBa> IRender(height, width);
  vpImage<vpRGBa> InormDisplay(height, width);
  vpImage<unsigned char> ICannyDisplay(height, width);

  vpImageConvert::convert(Icol, Id);

  // Main window creation and displaying

  std::vector<std::shared_ptr<vpDisplay>> displays, debugDisplays;

  if (baseArgs.display) {
    displays = vpRBTrackerTutorial::createDisplays(Id, Icol, depthDisplay, IProbaDisplay);
    if (baseArgs.debugDisplay) {
      debugDisplays = vpDisplayFactory::makeDisplayGrid(
        1, 3,
        0, 0,
        20, 20,
        "Normals in object frame", InormDisplay,
        "Depth canny", ICannyDisplay,
        "Color render", IRender
      );
    }
    plotter.init(displays);
  }

  vpHomogeneousMatrix cMo;

  nlohmann::json result = nlohmann::json::array();

  // Manual initialization of the tracker
  std::cout << "Starting init" << std::endl;

  if (baseArgs.hasInlineInit()) {
    tracker.setPose(baseArgs.cMoInit);
  }
  else if (baseArgs.display) {
    tracker.initClick(Id, baseArgs.initFile, true);
  }
  else {
    throw vpException(vpException::notImplementedError, "Cannot initialize tracking: no auto init function provided");
  }

  if (baseArgs.display) {
    vpDisplay::flush(Id);
  }

  int im = sequenceArgs.startFrame;
  unsigned int iter = 1;
  // Main tracking loop
  double expStart = vpTime::measureTimeMs();
  bool quit = false;
  while (!quit) {
    double frameStart = vpTime::measureTimeMs();
    // Acquire images
    for (unsigned int sp = 0; sp < sequenceArgs.frameStep; ++sp) {
      readerRGB.acquire(Icol);
      vpImageConvert::convert(Icol, Id);
      if (!sequenceArgs.depthFolder.empty()) {
        std::string depthName = vpIoTools::formatString(sequenceArgs.depthFolder + "/%06d.npy", im);
        visp::cnpy::NpyArray npz_data = visp::cnpy::npy_load(depthName);
        vpImage<uint16_t> dataArray(npz_data.data<uint16_t>(), npz_data.shape[0], npz_data.shape[1], false);
        float scale = 9.999999747378752e-05;
        depth.resize(dataArray.getHeight(), dataArray.getWidth());
        depthDisplay.resize(dataArray.getHeight(), dataArray.getWidth());
#ifdef VISP_HAVE_OPENMP
#pragma omp parallel for
#endif
        for (int i = 0; i < static_cast<int>(dataArray.getSize()); ++i) {
          float value = static_cast<float>(dataArray.bitmap[i]) * scale;
          depth.bitmap[i] = value;
          depthDisplay.bitmap[i] = value > baseArgs.maxDepthDisplay ? 0.f : static_cast<unsigned char>((depth.bitmap[i] / baseArgs.maxDepthDisplay) * 255.f);
        }
      }
    }

    // Pose tracking
    double trackingStart = vpTime::measureTimeMs();
    vpRBTrackingResult trackingResult;
    if (depth.getSize() == 0) {
      trackingResult = tracker.track(Id, Icol);
    }
    else {
      trackingResult = tracker.track(Id, Icol, depth);
    }

    switch (trackingResult.getStoppingReason()) {
    case vpRBTrackingStoppingReason::EXCEPTION:
    {
      std::cout << "Encountered an exception during tracking, pose was not updated" << std::endl;
      break;
    }
    case vpRBTrackingStoppingReason::NOT_ENOUGH_FEATURES:
    {
      std::cout << "There were not enough feature to perform tracking" << std::endl;
      break;
    }
    case vpRBTrackingStoppingReason::OBJECT_NOT_IN_IMAGE:
    {
      std::cout << "Object is not in image" << std::endl;
      break;
    }
    case vpRBTrackingStoppingReason::CONVERGENCE_CRITERION:
    {
      std::cout << "Convergence criterion reached:" << std::endl;
      std::cout << "- Num iterations: " << trackingResult.getNumIterations() << std::endl;
      std::cout << "- Convergence criterion: " << *(trackingResult.getConvergenceMetricValues().end() - 1) << std::endl;
      break;
    }
    case vpRBTrackingStoppingReason::MAX_ITERS:
    {
      break;
    }
    default:
    {
    }
    }

    std::cout << "Tracking took " << vpTime::measureTimeMs() - trackingStart << "ms" << std::endl;

    if (baseArgs.display) {
      if (baseArgs.debugDisplay) {
        const vpRBFeatureTrackerInput &lastFrame = tracker.getMostRecentFrame();

        vpRBTrackerTutorial::displayNormals(lastFrame.renders.normals, InormDisplay);

        vpRBTrackerTutorial::displayCanny(lastFrame.renders.silhouetteCanny, ICannyDisplay, lastFrame.renders.isSilhouette);
        if (lastFrame.renders.color.getSize() > 0) {
          IRender = lastFrame.renders.color;
          vpDisplay::display(IRender);
          vpDisplay::flush(IRender);
        }
      }

      vpDisplay::display(Id);
      vpDisplay::displayText(Id, 20, 20, "Click to quit", vpColor::red);
      vpDisplay::display(Icol);

      tracker.display(Id, Icol, depthDisplay);
      tracker.displayMask(IProbaDisplay);
      vpDisplay::display(IProbaDisplay);
      vpDisplay::displayFrame(Icol, cMo, cam, 0.05, vpColor::none, 2);
      vpDisplay::flush(Icol);
      vpDisplay::flush(Id);
      vpDisplay::flush(IProbaDisplay);
      if (depth.getSize() > 0) {
        vpDisplay::display(depthDisplay);
        vpDisplay::flush(depthDisplay);
      }
    }

    tracker.getPose(cMo);
    result.push_back(cMo);

    logger.logFrame(tracker, iter, Id, Icol, depthDisplay, IProbaDisplay);

    if (sequenceArgs.stepByStep && baseArgs.display) {
      vpDisplay::getClick(Id, true);
    }

    std::cout << "Iter: " << iter << std::endl;
    ++im;
    ++iter;
    if (im > readerRGB.getLastFrameIndex()) {
      quit = true;
      std::cout << "End of video reached" << std::endl;
    }

    double frameEnd = vpTime::measureTimeMs();
    std::cout << "Frame took: " << frameEnd - frameStart << "ms" << std::endl;
    plotter.plot(tracker, (frameEnd - expStart) / 1000.0);

    if (vpDisplay::getClick(Id, false)) {
      quit = true;
    }
  }

  logger.close();
  vpPanda3DFrameworkManager::getInstance().exit();

  return EXIT_SUCCESS;
}

#else
int main()
{
  std::cout << "This tutorial requires nlohmann_json 3rdparty." << std::endl;
}
#endif
