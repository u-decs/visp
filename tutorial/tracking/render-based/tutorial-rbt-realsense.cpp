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

#include <visp3/rbt/vpRBTracker.h>

#include "render-based-tutorial-utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
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

void updateDepth(const vpImage<uint16_t> &depthRaw, float depthScale, float maxZDisplay, vpImage<float> &depth, vpImage<unsigned char> &IdepthDisplay)
{
  depth.resize(depthRaw.getHeight(), depthRaw.getWidth());
#ifdef VISP_HAVE_OPENMP
#pragma omp parallel for
#endif
  for (unsigned int i = 0; i < depthRaw.getSize(); ++i) {
    depth.bitmap[i] = depthScale * static_cast<float>(depthRaw.bitmap[i]);
    IdepthDisplay.bitmap[i] = depth.bitmap[i] > maxZDisplay ? 0 : static_cast<unsigned int>((depth.bitmap[i] / maxZDisplay) * 255.f);
  }
}

int main(int argc, const char **argv)
{
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

  if (baseArgs.enableRenderProfiling) {
    vpRBTrackerTutorial::enableRendererProfiling();
  }

  std::cout << "Loading tracker: " << baseArgs.trackerConfiguration << std::endl;
  vpRBTracker tracker;
  tracker.loadConfigurationFile(baseArgs.trackerConfiguration);
  tracker.startTracking();
  const unsigned int width = realsenseArgs.width, height = realsenseArgs.height;
  const unsigned fps = realsenseArgs.fps;

  vpImage<unsigned char> Id(height, width);
  vpImage<vpRGBa> Icol(height, width);
  vpImage<uint16_t> depthRaw(height, width);
  vpImage<float> depth(height, width);
  vpImage<unsigned char> IdepthDisplay(height, width);
  vpImage<unsigned char> IProbaDisplay(height, width);
  vpImage<unsigned char> cannyDisplay(height, width);
  vpImage<vpRGBa> InormDisplay(height, width);

  vpRealSense2 realsense;

  std::cout << "Opening realsense with " << width << "x" << height << " @ " << fps << "fps" << std::endl;
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
    return EXIT_SUCCESS;
  }

  float depthScale = realsense.getDepthScale();
  //camera warmup
  for (int i = 0; i < 10; ++i) {
    realsense.acquire(Icol);
  }
  vpImageConvert::convert(Icol, Id);

  vpCameraParameters cam = realsense.getCameraParameters(RS2_STREAM_COLOR, vpCameraParameters::perspectiveProjWithoutDistortion);
  tracker.setCameraParameters(cam, height, width);

  std::cout << "Creating displays" << std::endl;;
  std::vector<std::shared_ptr<vpDisplay>> displays, displaysDebug;

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

  updateDepth(depthRaw, depthScale, baseArgs.maxDepthDisplay, depth, IdepthDisplay);

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
    throw vpException(vpException::notImplementedError, "Cannot initalize tracking: no auto init function provided");
  }

  std::cout << "Starting pose: " << vpPoseVector(cMo).t() << std::endl;

  if (baseArgs.display) {
    vpDisplay::flush(Id);
  }

//vpRBTrackerFilter &ukfm = tracker.getFilter();
  logger.startLog();
  unsigned int iter = 1;
  // Main tracking loop
  double expStart = vpTime::measureTimeMs();
  while (true) {
    double frameStart = vpTime::measureTimeMs();
    // Acquire images
    realsense.acquire((unsigned char *)Icol.bitmap, (unsigned char *)depthRaw.bitmap, nullptr, nullptr, &align_to);
    updateDepth(depthRaw, depthScale, baseArgs.maxDepthDisplay, depth, IdepthDisplay);
    vpImageConvert::convert(Icol, Id);

    // Pose tracking
    double trackingStart = vpTime::measureTimeMs();
    tracker.track(Id, Icol, depth);
    double trackingEnd = vpTime::measureTimeMs();
    tracker.getPose(cMo);
    double displayStart = vpTime::measureTimeMs();
    if (baseArgs.display) {
      if (baseArgs.debugDisplay) {
        const vpRBFeatureTrackerInput &lastFrame = tracker.getMostRecentFrame();

        vpRBTrackerTutorial::displayCanny(lastFrame.renders.silhouetteCanny, cannyDisplay, lastFrame.renders.isSilhouette);
      }

      vpDisplay::display(IdepthDisplay);
      vpDisplay::display(Id);
      vpDisplay::display(Icol);
      tracker.display(Id, Icol, IdepthDisplay);
      vpDisplay::displayFrame(Icol, cMo, cam, 0.05, vpColor::none, 2);
      vpDisplay::displayText(Id, 20, 5, "Right click to exit", vpColor::red);
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

    logger.logFrame(tracker, iter, Id, Icol, IdepthDisplay, IProbaDisplay);
    const double displayEnd = vpTime::measureTimeMs();

    // ukfm.filter(cMo, 0.05);
    // const vpHomogeneousMatrix cMoFiltered = ukfm.getFilteredPose();
    // vpDisplay::displayFrame(Icol, cMoFiltered, cam, 0.05, vpColor::yellow, 2);

    const double frameEnd = vpTime::measureTimeMs();
    std::cout << "Iter " << iter << ": " << round(frameEnd - frameStart) << "ms" << std::endl;
    std::cout << "- Tracking:  " << round(trackingEnd - trackingStart) << "ms" << std::endl;
    std::cout << "- Display: " << round(displayEnd - displayStart) << "ms" << std::endl;
    plotter.plot(tracker, (frameEnd - expStart) / 1000.0);
    iter++;
  }

  logger.close();
  return EXIT_SUCCESS;
}
#endif
