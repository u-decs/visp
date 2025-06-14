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
 *
 * Description:
 * Common test for Pololu velocity control of one servo connected to a given channel.
 */

/*!
 * \example testPololuVelocity.cpp
 */
#include <iostream>

#include <visp3/core/vpConfig.h>

#if defined(VISP_HAVE_POLOLU) && defined(VISP_HAVE_THREADS)

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include <visp3/core/vpMath.h>
#include <visp3/robot/vpPololu.h>

#ifdef ENABLE_VISP_NAMESPACE
using namespace VISP_NAMESPACE_NAME;
#endif

void usage(const char **argv, int error, const std::string &device, int baudrate, int channel,
           unsigned short pwm_min, unsigned short pwm_max, float angle_min, float angle_max);

void usage(const char **argv, int error, const std::string &device, int baudrate, int channel,
           unsigned short pwm_min, unsigned short pwm_max, float angle_min, float angle_max)
{
  std::cout << "Synopsis" << std::endl
    << "  " << argv[0] << " [--device <name>] [--channel <number>] [--calibrate] [--range-pwm <min max> ] [--verbose, -v] [--help, -h]" << std::endl
    << std::endl;
  std::cout << "Description" << std::endl
    << "  --device <name>  Device name." << std::endl
    << "    Default: " << device << std::endl
    << std::endl
    << "  --baud <rate>  Serial link baud rate." << std::endl
    << "    Default: " << baudrate << std::endl
    << std::endl
    << "  --channel <number>  Channel to dial with." << std::endl
    << "    Default: " << channel << std::endl
    << std::endl
    << "  --range-pwm <min max>  Set PWM min and max values." << std::endl
    << "    You can use \"--calibrate\" to retrieve min and max pwm values."
    << "    Default: " << pwm_min << " " << pwm_max << std::endl
    << std::endl
    << "  --range-angles <min max>  Set angle min and max values (deg)." << std::endl
    << "    Default: " << vpMath::deg(angle_min) << " " << vpMath::deg(angle_max) << std::endl
    << std::endl
    << "  --verbose, -v  Enable verbosity." << std::endl
    << std::endl
    << "  --calibrate  Start pwm calibration determining min and max admissible values." << std::endl
    << "    Once calibration done you can use \"--range-pwm <min max>\" option to set" << std::endl
    << "    the corresponding values" << std::endl
    << std::endl
    << "  --help, -h  Print this helper message." << std::endl
    << std::endl;
  if (error) {
    std::cout << "Error" << std::endl
      << "  "
      << "Unsupported parameter " << argv[error] << std::endl;
  }
}

int main(int argc, const char **argv)
{
#ifdef _WIN32
  std::string opt_device = "COM4";
#else
  std::string opt_device = "/dev/ttyACM0";
  // Example for Mac OS, the Maestro creates two devices, use the one with the lowest number (the command port)
  //std::string opt_device = "/dev/cu.usbmodem00031501";
#endif
  int opt_channel = 0;
  int opt_baudrate = 38400;
  bool opt_verbose = false;
  bool opt_calibrate = false;
  unsigned short opt_pwm_min = 4000;
  unsigned short opt_pwm_max = 8000;
  float opt_angle_min = static_cast<float>(vpMath::rad(-45));
  float opt_angle_max = static_cast<float>(vpMath::rad(45));
  float opt_velocity = static_cast<float>(vpMath::rad(5));
  float last_angle = 0;

  for (int i = 1; i < argc; i++) {
    if (std::string(argv[i]) == "--device" && i + 1 < argc) {
      opt_device = std::string(argv[i + 1]);
      i++;
    }
    else if (std::string(argv[i]) == "--baud" && i + 1 < argc) {
      opt_baudrate = std::atoi(argv[i + 1]);
      i++;
    }
    else if (std::string(argv[i]) == "--channel" && i + 1 < argc) {
      opt_channel = std::atoi(argv[i + 1]);
      i++;
    }
    else if (std::string(argv[i]) == "--range-pwm" && i + 2 < argc) {
      opt_pwm_min = static_cast<unsigned short>(vpMath::rad(std::atoi(argv[i + 1])));
      opt_pwm_max = static_cast<unsigned short>(vpMath::rad(std::atoi(argv[i + 2])));
      i += 2;
    }
    else if (std::string(argv[i]) == "--range-angles" && i + 2 < argc) {
      opt_angle_min = static_cast<float>(std::atof(argv[i + 1]));
      opt_angle_max = static_cast<float>(std::atof(argv[i + 2]));
      i += 2;
    }
    else if (std::string(argv[i]) == "--calibrate") {
      opt_calibrate = true;
    }
    else if (std::string(argv[i]) == "--verbose" || std::string(argv[i]) == "-v") {
      opt_verbose = true;
    }
    else if (std::string(argv[i]) == "--help" || std::string(argv[i]) == "-h") {
      usage(argv, 0, opt_device, opt_baudrate, opt_channel, opt_pwm_min, opt_pwm_max, opt_angle_min, opt_angle_max);
      return EXIT_SUCCESS;
    }
    else {
      usage(argv, i, opt_device, opt_baudrate, opt_channel, opt_pwm_min, opt_pwm_max, opt_angle_min, opt_angle_max);
      return EXIT_FAILURE;
    }
  }

  std::chrono::seconds sec(1);

  try {
    // Creating the servo object on channel 0
    vpPololu servo(opt_verbose);

    servo.connect(opt_device, opt_baudrate, opt_channel);

    if (opt_calibrate) {
      std::cout << "Proceed to calibration to determine pwm min and max values..." << std::endl;
      std::cout << "WARNING: Calibration will move the servo at channel " << opt_channel << "!" << std::endl;
      std::cout << "Press Enter to move to min and max pwm positions..." << std::endl;
      std::cin.ignore();

      unsigned short pwm_min, pwm_max;
      servo.calibrate(pwm_min, pwm_max);
      std::cout << "Servo on channel " << opt_channel << " has pwm range [" << pwm_min << ", " << pwm_max << "]" << std::endl;
      return EXIT_SUCCESS;
    }

    servo.setPwmRange(opt_pwm_min, opt_pwm_max);
    servo.setAngularRange(opt_angle_min, opt_angle_max);

    // Servo will move to 0 angle at a max velocity in rad/s
    std::cout << "Move to zero position (deg): " << vpMath::deg(0) << " at max velocity" << std::endl;
    servo.setAngularPosition(0, 0);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    last_angle = servo.getAngularPosition();
    std::cout << "Servo reached position (deg): " << vpMath::deg(last_angle) << std::endl;

    if (1) {
        // Servo will first move in one direction at a velocity of 10 for 3 sec and move back in the other direction for 3 sec
      short vel_pwm = 10;
      std::cout << "Move at velocity (pwm): " << vel_pwm << " for 3 sec" << std::endl;
      servo.setPwmVelocity(vel_pwm);
      std::this_thread::sleep_for(3 * sec);
      std::cout << "Servo reached position (pwm): " << servo.getPwmPosition() << std::endl;

      vel_pwm = -10;
      std::cout << "Move at velocity (pwm): " << vel_pwm << " for 3 sec" << std::endl;
      servo.setPwmVelocity(vel_pwm);
      std::this_thread::sleep_for(3 * sec);
      std::cout << "Servo reached position (pwm): " << servo.getPwmPosition() << std::endl;
      std::cout << "End of velocity motion" << std::endl;
    }

    // Servo will first move in one direction at a velocity of 10 for 3 sec and move back in the other direction for 3 sec
    std::cout << "Move at velocity (deg/s): " << vpMath::deg(opt_velocity) << " for 3 sec" << std::endl;
    servo.setAngularVelocity(opt_velocity);
    std::this_thread::sleep_for(3 * sec);
    std::cout << "Servo reached position (deg): " << servo.getPwmPosition() << std::endl;

    std::cout << "Move at velocity (deg/s): " << vpMath::deg(-opt_velocity) << " for 3 sec" << std::endl;
    servo.setAngularVelocity(-opt_velocity);
    std::this_thread::sleep_for(3 * sec);
    std::cout << "Servo reached position (deg): " << servo.getPwmPosition() << std::endl;

    // Stopping the velocity command.
    servo.stopVelocityCmd();

    std::cout << "The end" << std::endl;

    return EXIT_SUCCESS;
  }
  catch (const vpException &e) {
    std::cout << e.getMessage() << std::endl;
    return EXIT_FAILURE;
  }
}

#else
int main()
{
  std::cout << "ViSP doesn't support Pololu 3rd party library" << std::endl;
}
#endif
