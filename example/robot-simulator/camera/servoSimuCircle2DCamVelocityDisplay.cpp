/****************************************************************************
 *
 * ViSP, open source Visual Servoing Platform software.
 * Copyright (C) 2005 - 2023 by Inria. All rights reserved.
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
 * Simulation of a 2D visual servoing on a circle.
 *
*****************************************************************************/

/*!
  \example servoSimuCircle2DCamVelocityDisplay.cpp
  \brief Servo a circle:

  Servo a circle:
  - eye-in-hand control law,
  - velocity computed in the camera frame,
  - display the camera view.
*/

#include <visp3/core/vpConfig.h>
#include <visp3/core/vpDebug.h>

#if defined(VISP_HAVE_DISPLAY) &&       \
    (defined(VISP_HAVE_LAPACK) || defined(VISP_HAVE_EIGEN3) || defined(VISP_HAVE_OPENCV))

#include <stdio.h>
#include <stdlib.h>

#include <visp3/core/vpCameraParameters.h>
#include <visp3/core/vpCircle.h>
#include <visp3/core/vpHomogeneousMatrix.h>
#include <visp3/core/vpImage.h>
#include <visp3/core/vpMath.h>
#include <visp3/gui/vpDisplayFactory.h>
#include <visp3/io/vpParseArgv.h>
#include <visp3/robot/vpSimulatorCamera.h>
#include <visp3/visual_features/vpFeatureBuilder.h>
#include <visp3/visual_features/vpFeatureLine.h>
#include <visp3/vs/vpServo.h>
#include <visp3/vs/vpServoDisplay.h>

// List of allowed command line options
#define GETOPTARGS "cdh"

#ifdef ENABLE_VISP_NAMESPACE
using namespace VISP_NAMESPACE_NAME;
#endif

void usage(const char *name, const char *badparam);
bool getOptions(int argc, const char **argv, bool &click_allowed, bool &display);

/*!

  Print the program options.

  \param name : Program name.
  \param badparam : Bad parameter name.

*/
void usage(const char *name, const char *badparam)
{
  fprintf(stdout, "\n\
Simulation of a 2D visual servoing on a circle:\n\
- eye-in-hand control law,\n\
- velocity computed in the camera frame,\n\
- display the camera view.\n\
          \n\
SYNOPSIS\n\
  %s [-c] [-d] [-h]\n",
          name);

  fprintf(stdout, "\n\
OPTIONS:                                               Default\n\
                  \n\
  -c\n\
     Disable the mouse click. Useful to automate the \n\
     execution of this program without human intervention.\n\
                  \n\
  -d \n\
     Turn off the display.\n\
                  \n\
  -h\n\
     Print the help.\n");

  if (badparam) {
    fprintf(stderr, "ERROR: \n");
    fprintf(stderr, "\nBad parameter [%s]\n", badparam);
  }
}

/*!

  Set the program options.

  \param argc : Command line number of parameters.
  \param argv : Array of command line parameters.
  \param click_allowed : false if mouse click is not allowed.
  \param display : false if the display is to turn off.

  \return false if the program has to be stopped, true otherwise.

*/
bool getOptions(int argc, const char **argv, bool &click_allowed, bool &display)
{
  const char *optarg_;
  int c;
  while ((c = vpParseArgv::parse(argc, argv, GETOPTARGS, &optarg_)) > 1) {

    switch (c) {
    case 'c':
      click_allowed = false;
      break;
    case 'd':
      display = false;
      break;
    case 'h':
      usage(argv[0], nullptr);
      return false;

    default:
      usage(argv[0], optarg_);
      return false;
    }
  }

  if ((c == 1) || (c == -1)) {
    // standalone param or error
    usage(argv[0], nullptr);
    std::cerr << "ERROR: " << std::endl;
    std::cerr << "  Bad argument " << optarg_ << std::endl << std::endl;
    return false;
  }

  return true;
}

int main(int argc, const char **argv)
{
#if (VISP_CXX_STANDARD >= VISP_CXX_STANDARD_11)
  std::shared_ptr<vpDisplay> display;
#else
  vpDisplay *display = nullptr;
#endif
  try {
    bool opt_display = true;
    bool opt_click_allowed = true;

    // Read the command line options
    if (getOptions(argc, argv, opt_click_allowed, opt_display) == false) {
      return (EXIT_FAILURE);
    }

    vpImage<unsigned char> I(512, 512, 0);

    if (opt_display) {
      // Display size is automatically defined by the image (I) size
#if (VISP_CXX_STANDARD >= VISP_CXX_STANDARD_11)
      display = vpDisplayFactory::createDisplay(I, 100, 100, "Camera view...");
#else
      display = vpDisplayFactory::allocateDisplay(I, 100, 100, "Camera view...");
#endif
      // Display the image
      // The image class has a member that specify a pointer toward
      // the display that has been initialized in the display declaration
      // therefore is is no longer necessary to make a reference to the
      // display variable.
      vpDisplay::display(I);
      vpDisplay::flush(I);
    }

    double px = 600, py = 600;
    double u0 = I.getWidth() / 2., v0 = I.getHeight() / 2.;

    vpCameraParameters cam(px, py, u0, v0);

    vpServo task;
    vpSimulatorCamera robot;

    // sets the initial camera location
    vpHomogeneousMatrix cMo(0, 0, 1, vpMath::rad(0), vpMath::rad(80), vpMath::rad(30));
    vpHomogeneousMatrix wMc, wMo;
    robot.getPosition(wMc);
    wMo = wMc * cMo; // Compute the position of the object in the world frame

    vpHomogeneousMatrix cMod(-0.1, -0.1, 0.7, vpMath::rad(40), vpMath::rad(10), vpMath::rad(30));

    // sets the circle coordinates in the world frame
    vpCircle circle;
    circle.setWorldCoordinates(0, 0, 1, 0, 0, 0, 0.1);

    // sets the desired position of the visual feature
    vpFeatureEllipse pd;
    circle.track(cMod);
    vpFeatureBuilder::create(pd, circle);

    // project : computes the circle coordinates in the camera frame and its
    // 2D coordinates sets the current position of the visual feature
    vpFeatureEllipse p;
    circle.track(cMo);
    vpFeatureBuilder::create(p, circle);

    // define the task
    // - we want an eye-in-hand control law
    // - robot is controlled in the camera frame
    task.setServo(vpServo::EYEINHAND_CAMERA);
    task.setInteractionMatrixType(vpServo::DESIRED);
    // - we want to see a circle on a circle
    task.addFeature(p, pd);
    // - set the gain
    task.setLambda(1);

    // Display task information
    task.print();

    unsigned int iter = 0;
    // loop
    while (iter++ < 200) {
      std::cout << "---------------------------------------------" << iter << std::endl;
      vpColVector v;

      // get the robot position
      robot.getPosition(wMc);
      // Compute the position of the object frame in the camera frame
      cMo = wMc.inverse() * wMo;

      // new circle position
      // retrieve x,y and Z of the vpCircle structure
      circle.track(cMo);
      vpFeatureBuilder::create(p, circle);
      circle.print();
      p.print();

      if (opt_display) {
        vpDisplay::display(I);
        vpServoDisplay::display(task, cam, I);
        vpDisplay::flush(I);
      }

      // compute the control law
      v = task.computeControlLaw();
      std::cout << "task rank: " << task.getTaskRank() << std::endl;
      // send the camera velocity to the controller
      robot.setVelocity(vpRobot::CAMERA_FRAME, v);

      std::cout << "|| s - s* || = " << (task.getError()).sumSquare() << std::endl;
    }

    // Display task information
    task.print();

    if (opt_display && opt_click_allowed) {
      vpDisplay::displayText(I, 20, 20, "Click to quit...", vpColor::white);
      vpDisplay::flush(I);
      vpDisplay::getClick(I);
    }
#if (VISP_CXX_STANDARD < VISP_CXX_STANDARD_11)
    if (display != nullptr) {
      delete display;
    }
#endif
    return EXIT_SUCCESS;
  }
  catch (const vpException &e) {
    std::cout << "Catch a ViSP exception: " << e << std::endl;
#if (VISP_CXX_STANDARD < VISP_CXX_STANDARD_11)
    if (display != nullptr) {
      delete display;
    }
#endif
    return EXIT_FAILURE;
  }
}
#elif !(defined(VISP_HAVE_LAPACK) || defined(VISP_HAVE_EIGEN3) || defined(VISP_HAVE_OPENCV))
int main()
{
  std::cout << "Cannot run this example: install Lapack, Eigen3 or OpenCV" << std::endl;
  return EXIT_SUCCESS;
}
#else
int main()
{
  std::cout << "You do not have X11, or GTK, or GDI (Graphical Device Interface) or OpenCV functionalities to display "
    "images..."
    << std::endl;
  std::cout << "Tip if you are on a unix-like system:" << std::endl;
  std::cout << "- Install X11, configure again ViSP using cmake and build again this example" << std::endl;
  std::cout << "Tip if you are on a windows-like system:" << std::endl;
  std::cout << "- Install GDI, configure again ViSP using cmake and build again this example" << std::endl;
  return EXIT_SUCCESS;
}
#endif
