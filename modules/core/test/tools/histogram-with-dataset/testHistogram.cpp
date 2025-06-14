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
 * Test histogram computation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <visp3/core/vpHistogram.h>
#include <visp3/core/vpImage.h>
#include <visp3/core/vpUniRand.h>
#include <visp3/core/vpIoTools.h>
#include <visp3/io/vpImageIo.h>
#include <visp3/io/vpParseArgv.h>

/*!
  \example testHistogram.cpp

  \brief Test histogram computation.

*/

// List of allowed command line options
#define GETOPTARGS "cdi:t:h"

#ifdef ENABLE_VISP_NAMESPACE
using namespace VISP_NAMESPACE_NAME;
#endif

/*
  Print the program options.

  \param name : Program name.
  \param badparam : Bad parameter name.
  \param ipath : Input image path.

 */
void usage(const char *name, const char *badparam, std::string ipath)
{
  fprintf(stdout, "\n\
Test histogram.\n\
\n\
SYNOPSIS\n\
  %s [-i <input image path>] [-t <nb threads>]\n\
     [-h]\n                 \
",
name);

  fprintf(stdout, "\n\
OPTIONS:                                               Default\n\
  -i <input image path>                                %s\n\
     Set image input path.\n\
     From this path read \"Klimt/Klimt.ppm\"\n\
     image.\n\
     Setting the VISP_INPUT_IMAGE_PATH environment\n\
     variable produces the same behaviour than using\n\
     this option.\n\
\n\
  -t <nb threads>\n\
     Set the number of threads to use for the computation.\n\
  -h\n\
     Print the help.\n\n",
          ipath.c_str());

  if (badparam)
    fprintf(stdout, "\nERROR: Bad parameter [%s]\n", badparam);
}

/*!

  Set the program options.

  \param argc : Command line number of parameters.
  \param argv : Array of command line parameters.
  \param ipath : Input image path.
  \param nbThreads : Number of threads to use.
  \return false if the program has to be stopped, true otherwise.

*/
bool getOptions(int argc, const char **argv, std::string &ipath, unsigned int &nbThreads)
{
  const char *optarg_;
  int c;
  while ((c = vpParseArgv::parse(argc, argv, GETOPTARGS, &optarg_)) > 1) {

    switch (c) {
    case 'i':
      ipath = optarg_;
      break;
    case 't':
      nbThreads = static_cast<unsigned int>(atoi(optarg_));
      break;
    case 'h':
      usage(argv[0], nullptr, ipath);
      return false;

    case 'c':
    case 'd':
      break;

    default:
      usage(argv[0], optarg_, ipath);
      return false;
    }
  }

  if ((c == 1) || (c == -1)) {
    // standalone param or error
    usage(argv[0], nullptr, ipath);
    std::cerr << "ERROR: " << std::endl;
    std::cerr << "  Bad argument " << optarg_ << std::endl << std::endl;
    return false;
  }

  return true;
}

/*!
  Compute the histogram of an image.

  \param I : Input color image.
  \param nbBins : Number of histogram bins.
  \param nbThreads : Number of computation threads.
*/
unsigned int histogramSum(const vpImage<unsigned char> &I, unsigned int nbBins, unsigned int nbThreads)
{
  unsigned int sum = 0;

  vpHistogram histogram;
  histogram.calculate(I, nbBins, nbThreads);

  for (unsigned int cpt = 0; cpt < histogram.getSize(); cpt++) {
    sum += histogram[cpt];
  }

  return sum;
}

/*!
  Compare two histograms.

  \param I : Input color image.
  \param nbBins : Number of histogram bins.
*/
bool compareHistogram(const vpImage<unsigned char> &I, unsigned int nbBins)
{
  vpHistogram histogram_single_threaded;
  histogram_single_threaded.calculate(I, nbBins, 1);

  vpHistogram histogram_multi_threaded;
  histogram_multi_threaded.calculate(I, nbBins, 4);

  unsigned int sum = 0;
  for (unsigned int cpt = 0; cpt < nbBins; cpt++) {
    if (histogram_single_threaded[cpt] != histogram_multi_threaded[cpt]) {
      std::cerr << "histogram_single_threaded[" << cpt << "]=" << histogram_single_threaded[cpt]
        << " ; histogram_multi_threaded[" << cpt << "]=" << histogram_multi_threaded[cpt] << std::endl;

      return false;
    }

    sum += histogram_single_threaded[cpt];
  }

  if (sum != I.getSize()) {
    std::cerr << "Sum of histogram is different with the image size!" << std::endl;
    return false;
  }

  return true;
}

int main(int argc, const char **argv)
{
  try {
    std::string env_ipath;
    std::string opt_ipath;
    std::string ipath;
    std::string filename;
    unsigned int nbThreads = 4;

    // Get the visp-images-data package path or VISP_INPUT_IMAGE_PATH
    // environment variable value
    env_ipath = vpIoTools::getViSPImagesDataPath();

    // Set the default input path
    if (!env_ipath.empty())
      ipath = env_ipath;

    // Read the command line options
    if (getOptions(argc, argv, opt_ipath, nbThreads) == false) {
      return EXIT_FAILURE;
    }

    // Get the option values
    if (!opt_ipath.empty())
      ipath = opt_ipath;

    // Compare ipath and env_ipath. If they differ, we take into account
    // the input path coming from the command line option
    if (!opt_ipath.empty() && !env_ipath.empty()) {
      if (ipath != env_ipath) {
        std::cout << std::endl << "WARNING: " << std::endl;
        std::cout << "  Since -i <visp image path=" << ipath << "> "
          << "  is different from VISP_IMAGE_PATH=" << env_ipath << std::endl
          << "  we skip the environment variable." << std::endl;
      }
    }

    // Test if an input path is set
    if (opt_ipath.empty() && env_ipath.empty()) {
      usage(argv[0], nullptr, ipath);
      std::cerr << std::endl << "ERROR:" << std::endl;
      std::cerr << "  Use -i <visp image path> option or set VISP_INPUT_IMAGE_PATH " << std::endl
        << "  environment variable to specify the location of the " << std::endl
        << "  image path where test images are located." << std::endl
        << std::endl;
      return EXIT_FAILURE;
    }

    //
    // Here starts really the test
    //

    // Create a grey level image
    vpImage<unsigned char> I;

    // Load a grey image from the disk
    filename = vpIoTools::createFilePath(ipath, "Klimt/Klimt.ppm");
    std::cout << "Read image: " << filename << std::endl;
    vpImageIo::read(I, filename);

    std::cout << "I=" << I.getWidth() << "x" << I.getHeight() << std::endl;

    int nbIterations = 100;
    unsigned int nbBins = 256;
    unsigned int sum_single_thread = 0;
    unsigned int sum_multi_thread = 0;

    double t_single_thread = vpTime::measureTimeMs();
    for (int iteration = 0; iteration < nbIterations; iteration++) {
      sum_single_thread = histogramSum(I, nbBins, 1);
    }
    t_single_thread = vpTime::measureTimeMs() - t_single_thread;

    double t_multi_thread = vpTime::measureTimeMs();
    for (int iteration = 0; iteration < nbIterations; iteration++) {
      sum_multi_thread = histogramSum(I, nbBins, nbThreads);
    }
    t_multi_thread = vpTime::measureTimeMs() - t_multi_thread;

    std::cout << "sum_single_thread=" << sum_single_thread << " ; t_single_thread=" << t_single_thread
      << " ms ; mean=" << t_single_thread / static_cast<double>(nbIterations) << " ms" << std::endl;
    std::cout << "sum_multi_thread (nbThreads=" << nbThreads << ")=" << sum_multi_thread << " ; t_multi_thread=" << t_multi_thread
      << " ms ; mean=" << t_multi_thread / static_cast<double>(nbIterations) << " ms" << std::endl;
    std::cout << "Speed-up=" << t_single_thread / static_cast<double>(t_multi_thread) << "X" << std::endl;

    if (sum_single_thread != I.getSize() || sum_multi_thread != I.getSize()) {
      std::cerr << "Problem with histogram!" << std::endl;
      return EXIT_FAILURE;
    }

    nbBins = 101;
    if (!compareHistogram(I, nbBins)) {
      std::cerr << "Histogram are different!" << std::endl;
      return EXIT_FAILURE;
    }

    // Test histogram computation on empty image
    std::cout << "Test histogram computation on empty image" << std::endl << std::flush;
    vpHistogram histogram;
    vpImage<unsigned char> I_test(0, 0);
    histogram.calculate(I_test, 256, 4);
    if (histogram.getSize() == 256) {
      for (unsigned int cpt = 0; cpt < 256; cpt++) {
        if (histogram[cpt] != 0) {
          std::cerr << "Problem with histogram computation: histogram[" << cpt << "]=" << histogram[cpt]
            << " but should be zero!" << std::endl;
        }
      }
    }
    else {
      std::cerr << "Bad histogram size!" << std::endl;
      return EXIT_FAILURE;
    }

    // Test histogram computation on image size < nbThreads
    std::cout << "Test histogram computation on image size < nbThreads" << std::endl << std::flush;
    I_test.init(3, 1);
    I_test = 100;
    histogram.calculate(I_test, 256, 4);
    if (histogram.getSize() == 256) {
      for (unsigned int cpt = 0; cpt < 256; cpt++) {
        if (cpt == 100) {
          if (histogram[cpt] != I_test.getSize()) {
            std::cerr << "Problem with histogram computation: histogram[" << cpt << "]=" << histogram[cpt]
              << " but should be: " << I_test.getSize() << std::endl;
            return EXIT_FAILURE;
          }
        }
        else {
          if (histogram[cpt] != 0) {
            std::cerr << "Problem with histogram computation: histogram[" << cpt << "]=" << histogram[cpt]
              << " but should be zero!" << std::endl;
          }
        }
      }
    }
    else {
      std::cerr << "Bad histogram size!" << std::endl;
      return EXIT_FAILURE;
    }

    // Test histogram computation on small image size
    std::cout << "Test histogram computation on small image size" << std::endl << std::flush;
    I_test.init(7, 1);
    I_test = 50;
    histogram.calculate(I_test, 256, 4);
    if (histogram.getSize() == 256) {
      for (unsigned int cpt = 0; cpt < 256; cpt++) {
        if (cpt == 50) {
          if (histogram[cpt] != I_test.getSize()) {
            std::cerr << "Problem with histogram computation: histogram[" << cpt << "]=" << histogram[cpt]
              << " but should be: " << I_test.getSize() << std::endl;
            return EXIT_FAILURE;
          }
        }
        else {
          if (histogram[cpt] != 0) {
            std::cerr << "Problem with histogram computation: histogram[" << cpt << "]=" << histogram[cpt]
              << " but should be zero!" << std::endl;
          }
        }
      }
    }
    else {
      std::cerr << "Bad histogram size!" << std::endl;
      return EXIT_FAILURE;
    }

    // Test of smooth method
    unsigned int nbRows = 4, nbCols = 15;
    I.init(nbRows, nbCols);
    for (unsigned int r = 0; r < nbRows; ++r) {
      unsigned int c = 0;
      for (unsigned int i = 1; i <= 5; ++i) {
        for (unsigned int count = 0; count < i; ++count) {
          I[r][c] = i + r;
          ++c;
        }
      }
    }

    std::cout << "I:" << std::endl;
    std::cout << I << std::endl;

    nbBins = 256;
    std::vector<unsigned int> expectedNumber(nbBins, 0);
    expectedNumber[1] = 1;
    expectedNumber[2] = 3;
    expectedNumber[3] = 6;
    expectedNumber[4] = 10;
    expectedNumber[5] = 14;
    expectedNumber[6] = 12;
    expectedNumber[7] = 9;
    expectedNumber[8] = 5;

    vpHistogram histo;
    histo.calculate(I, nbBins);
    for (unsigned int bin = 0; bin < nbBins; ++bin) {
      if (histo[bin] != expectedNumber[bin]) {
        std::cerr << "Problem with histogram computation: histogram[" << bin << "]=" << histo[bin]
          << " but should be: " << expectedNumber[bin] << std::endl;
        return EXIT_FAILURE;
      }
    }

    std::vector<unsigned int> expectedNumberSmooth(nbBins, 0);
    expectedNumberSmooth[1] = 1;
    expectedNumberSmooth[2] = 3;
    expectedNumberSmooth[3] = 6;
    expectedNumberSmooth[4] = 10;
    expectedNumberSmooth[5] = 12;
    expectedNumberSmooth[6] = 11;
    expectedNumberSmooth[7] = 8;
    expectedNumberSmooth[8] = 4;
    expectedNumberSmooth[9] = 1;

    histo.smooth();

    for (unsigned int bin = 0; bin < nbBins; ++bin) {
      if (histo[bin] != expectedNumberSmooth[bin]) {
        std::cerr << "Problem with smooth computation: histogram[" << bin << "]=" << histo[bin]
          << " but should be: " << expectedNumberSmooth[bin] << std::endl;
        return EXIT_FAILURE;
      }
    }

    // Test of mask
    vpImage<bool> mask(nbRows, nbCols, false);
    for (unsigned int r = 0; r < nbRows; ++r) {
      for (unsigned int c = 0; c < nbCols; ++c) {
        if ((r == 0) || (r == (nbRows - 1))) {
          mask[r][c] = true;
        }
        if ((c == 0) || (c == (nbCols - 1))) {
          mask[r][c] = true;
        }
      }
    }

    std::cout << "I to which is applied the mask:" << std::endl;
    for (unsigned int r = 0; r < nbRows; ++r) {
      for (unsigned int c = 0; c < nbCols; ++c) {
        if (mask[r][c]) {
          std::cout << static_cast<int>(I[r][c]);
        }
        else {
          std::cout << "X";
        }
        std::cout << " ";
      }
      std::cout << std::endl;
    }

    vpHistogram histoWithMaskMono; // Histogram using mask monothreaded
    histoWithMaskMono.setMask(&mask);
    histoWithMaskMono.calculate(I, nbBins, 1);

    vpHistogram histoWithMaskMulti; // Histogram using mask multithreaded
    histoWithMaskMulti.setMask(&mask);
    histoWithMaskMulti.calculate(I, nbBins, 2);

    std::vector<unsigned int> expectedNumberMask(nbBins, 0);
    expectedNumberMask[1] = 1;
    expectedNumberMask[2] = 3;
    expectedNumberMask[3] = 4;
    expectedNumberMask[4] = 5;
    expectedNumberMask[5] = 7;
    expectedNumberMask[6] = 4;
    expectedNumberMask[7] = 5;
    expectedNumberMask[8] = 5;

    for (unsigned int bin = 0; bin < nbBins; ++bin) {
      if (histoWithMaskMono[bin] != expectedNumberMask[bin]) {
        std::cerr << "Problem when using mask: histogram[" << bin << "]=" << histoWithMaskMono[bin]
          << " but should be: " << expectedNumberMask[bin] << std::endl;
        return EXIT_FAILURE;
      }

      if (histoWithMaskMulti[bin] != expectedNumberMask[bin]) {
        std::cerr << "Problem when using mask: histogram[" << bin << "]=" << histoWithMaskMulti[bin]
          << " but should be: " << expectedNumberMask[bin] << std::endl;
        return EXIT_FAILURE;
      }
    }

#if (VISP_CXX_STANDARD >= VISP_CXX_STANDARD_11)
// Test of histogram on vpImage<double>
    vpImage<double> Id(nbRows, nbCols);
    nbBins = 8;
    double min = 0., max = 1.;
    double desiredStep = (max - min) / static_cast<double>(nbBins);
    vpUniRand uniRand(4221);

    for (unsigned int r = 0; r < nbRows; ++r) {
      unsigned int c = 0;
      for (unsigned int i = 1; i <= 5; ++i) {
        for (unsigned int count = 0; count < i; ++count) {
          double value = uniRand.uniform(0., desiredStep) + desiredStep * static_cast<double>(i + r - 1);
          Id[r][c] = value;
          ++c;
        }
      }
    }

    std::cout << "Id = " << std::endl;
    for (unsigned int r = 0; r < nbRows; ++r) {
      for (unsigned int c = 0; c < nbCols; ++c) {
        std::cout << std::setprecision(3) << Id[r][c];
        std::cout << " ";
      }
      std::cout << std::endl;
    }

    // test monothread
    std::vector<unsigned char> expectedNumberDouble(nbBins, 0), expectedNumberDoubleMask(nbBins, 0);
    for (unsigned int i = 0; i < nbBins; ++i) {
      expectedNumberDouble[i] = expectedNumber[i + 1];
      expectedNumberDoubleMask[i] = expectedNumberMask[i + 1];
    }
    double step = 0.;
    vpHistogram histoDoubleMono;
    histoDoubleMono.calculate(Id, min, max, step, nbBins, 1);

    if (!vpMath::equal(step, desiredStep)) {
      std::cerr << "Problem with histogram computation of floating point images" << std::endl;
      std::cout << "Computed step: " <<  step <<  " but should be: " << desiredStep << std::endl;
      return EXIT_FAILURE;
    }
    for (unsigned int bin = 0; bin < nbBins; ++bin) {
      if (histoDoubleMono[bin] != expectedNumberDouble[bin]) {
        std::cerr << "Problem with monothread histogram computation of floating point images: histogram[" << bin << "]=" << histoDoubleMono[bin]
          << " but should be: " << expectedNumberDouble[bin] << std::endl;
        return EXIT_FAILURE;
      }
    }

    // test multithread
    vpHistogram histoDoubleMulti;
    histoDoubleMulti.calculate(Id, min, max, step, nbBins, 2);

    if (!vpMath::equal(step, desiredStep)) {
      std::cerr << "Problem with histogram computation of floating point images" << std::endl;
      std::cout << "Computed step: " <<  step <<  " but should be: " << desiredStep << std::endl;
      return EXIT_FAILURE;
    }
    for (unsigned int bin = 0; bin < nbBins; ++bin) {
      if (histoDoubleMulti[bin] != expectedNumberDouble[bin]) {
        std::cerr << "Problem with multithread histogram computation of floating point images: histogram[" << bin << "]=" << histoDoubleMulti[bin]
          << " but should be: " << expectedNumberDouble[bin] << std::endl;
        return EXIT_FAILURE;
      }
    }

    std::cout << "Id with mask= " << std::endl;
    for (unsigned int r = 0; r < nbRows; ++r) {
      for (unsigned int c = 0; c < nbCols; ++c) {
        if (mask[r][c]) {
          std::cout << std::setprecision(3) << Id[r][c];
        }
        else {
          std::cout << "XXXXX";
        }
        std::cout << " ";
      }
      std::cout << std::endl;
    }

    // Test mask
    vpHistogram histoDoubleMonoMask;
    histoDoubleMonoMask.setMask(&mask);
    histoDoubleMonoMask.calculate(Id, min, max, step, nbBins, 2);

    vpHistogram histoDoubleMultiMask;
    histoDoubleMultiMask.setMask(&mask);
    histoDoubleMultiMask.calculate(Id, min, max, step, nbBins, 2);

    for (unsigned int bin = 0; bin < nbBins; ++bin) {
      if (histoDoubleMonoMask[bin] != expectedNumberDoubleMask[bin]) {
        std::cerr << "Problem with monothread histogram computation of floating point images using mask: histogram[" << bin << "]=" << histoDoubleMonoMask[bin]
          << " but should be: " << expectedNumberDoubleMask[bin] << std::endl;
        return EXIT_FAILURE;
      }

      if (histoDoubleMultiMask[bin] != expectedNumberDoubleMask[bin]) {
        std::cerr << "Problem with multithread histogram computation of floating point images using mask: histogram[" << bin << "]=" << histoDoubleMultiMask[bin]
          << " but should be: " << expectedNumberDoubleMask[bin] << std::endl;
        return EXIT_FAILURE;
      }
    }
#endif

    std::cout << "testHistogram is OK!" << std::endl;
    return EXIT_SUCCESS;
  }
  catch (const vpException &e) {
    std::cerr << "Catch an exception: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}
