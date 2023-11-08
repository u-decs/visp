/*
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
 */

#include <visp3/core/vpImageConvert.h>
#include <visp3/core/vpImageFilter.h>
#include <visp3/core/vpImageMorphology.h>

#include <visp3/imgproc/vpCircleHoughTransform.h>

#if (VISP_CXX_STANDARD >= VISP_CXX_STANDARD_11)
vpCircleHoughTransform::vpCircleHoughTransform()
  : m_algoParams()
{
  initGaussianFilters();
  initGradientFilters();
}

vpCircleHoughTransform::vpCircleHoughTransform(const vpCircleHoughTransformParameters &algoParams)
  : m_algoParams(algoParams)
{
  initGaussianFilters();
  initGradientFilters();
}

void
vpCircleHoughTransform::init(const vpCircleHoughTransformParameters &algoParams)
{
  m_algoParams = algoParams;
  initGaussianFilters();
  initGradientFilters();
}

vpCircleHoughTransform::~vpCircleHoughTransform()
{ }

#ifdef VISP_HAVE_NLOHMANN_JSON
vpCircleHoughTransform::vpCircleHoughTransform(const std::string &jsonPath)
{
  initFromJSON(jsonPath);
}

void
vpCircleHoughTransform::initFromJSON(const std::string &jsonPath)
{
  std::ifstream file(jsonPath);
  if (!file.good()) {
    std::stringstream ss;
    ss << "Problem opening file " << jsonPath << ". Make sure it exists and is readable" << std::endl;
    throw vpException(vpException::ioError, ss.str());
  }
  json j;
  try {
    j = json::parse(file);
  }
  catch (json::parse_error &e) {
    std::stringstream msg;
    msg << "Could not parse JSON file : \n";

    msg << e.what() << std::endl;
    msg << "Byte position of error: " << e.byte;
    throw vpException(vpException::ioError, msg.str());
  }
  m_algoParams = j; // Call from_json(const json& j, vpCircleHoughTransformParameters&) to read json
  file.close();
  initGaussianFilters();
  initGradientFilters();
}

void
vpCircleHoughTransform::saveConfigurationInJSON(const std::string &jsonPath) const
{
  m_algoParams.saveConfigurationInJSON(jsonPath);
}
#endif

void
vpCircleHoughTransform::initGaussianFilters()
{
  m_fg.resize(1, (m_algoParams.m_gaussianKernelSize + 1)/2);
  vpImageFilter::getGaussianKernel(m_fg.data, m_algoParams.m_gaussianKernelSize, m_algoParams.m_gaussianStdev, true);
  m_cannyVisp.setGaussianFilterParameters(m_algoParams.m_gaussianKernelSize, m_algoParams.m_gaussianStdev);
}

void
vpCircleHoughTransform::initGradientFilters()
{
  if ((m_algoParams.m_gradientFilterKernelSize % 2) != 1) {
    throw vpException(vpException::badValue, "Gradient filters Kernel size should be odd.");
  }
  m_gradientFilterX.resize(m_algoParams.m_gradientFilterKernelSize, m_algoParams.m_gradientFilterKernelSize);
  m_gradientFilterY.resize(m_algoParams.m_gradientFilterKernelSize, m_algoParams.m_gradientFilterKernelSize);
  m_cannyVisp.setGradientFilterAperture(m_algoParams.m_gradientFilterKernelSize);

  auto scaleFilter = [](vpArray2D<float> &filter, const float &scale) {
    for (unsigned int r = 0; r < filter.getRows(); r++) {
      for (unsigned int c = 0; c < filter.getCols(); c++) {
        filter[r][c] = filter[r][c] * scale;
      }
    }};

  float scaleX = 1.f;
  float scaleY = 1.f;

  if (m_algoParams.m_filteringAndGradientType == vpImageFilter::CANNY_GBLUR_SOBEL_FILTERING) {
    // Compute the Sobel filters
    scaleX = vpImageFilter::getSobelKernelX(m_gradientFilterX.data, (m_algoParams.m_gradientFilterKernelSize  - 1)/2);
    scaleY = vpImageFilter::getSobelKernelY(m_gradientFilterY.data, (m_algoParams.m_gradientFilterKernelSize  - 1)/2);
  }
  else if (m_algoParams.m_filteringAndGradientType == vpImageFilter::CANNY_GBLUR_SCHARR_FILTERING) {
    // Compute the Scharr filters
    scaleX = vpImageFilter::getScharrKernelX(m_gradientFilterX.data, (m_algoParams.m_gradientFilterKernelSize  - 1)/2);
    scaleY = vpImageFilter::getScharrKernelY(m_gradientFilterY.data, (m_algoParams.m_gradientFilterKernelSize  - 1)/2);
  }
  else {
    std::string errMsg = "[vpCircleHoughTransform::initGradientFilters] Error: gradient filtering method \"";
    errMsg += vpImageFilter::vpCannyFilteringAndGradientTypeToString(m_algoParams.m_filteringAndGradientType);
    errMsg += "\" has not been implemented yet\n";
    throw vpException(vpException::notImplementedError, errMsg);
  }
  scaleFilter(m_gradientFilterX, scaleX);
  scaleFilter(m_gradientFilterY, scaleY);
}

std::vector<vpImageCircle>
vpCircleHoughTransform::detect(const vpImage<vpRGBa> &I)
{
  vpImage<unsigned char> I_gray;
  vpImageConvert::convert(I, I_gray);
  return detect(I_gray);
}

#ifdef HAVE_OPENCV_CORE
std::vector<vpImageCircle>
vpCircleHoughTransform::detect(const cv::Mat &cv_I)
{
  vpImage<unsigned char> I_gray;
  vpImageConvert::convert(cv_I, I_gray);
  return detect(I_gray);
}
#endif

std::vector<vpImageCircle>
vpCircleHoughTransform::detect(const vpImage<unsigned char> &I, const int &nbCircles)
{
  std::vector<vpImageCircle> detections = detect(I);
  size_t nbDetections = detections.size();

  // Prepare vector of tuple to sort by decreasing probabilities
  std::vector<std::tuple<vpImageCircle, unsigned int, float> > detectionsWithVotes;
  for (size_t i = 0; i < nbDetections; i++) {
    std::tuple<vpImageCircle, unsigned int, float> detectionWithVote(detections[i], m_finalCircleVotes[i], m_finalCirclesProbabilities[i]);
    detectionsWithVotes.push_back(detectionWithVote);
  }

  // Sorting by decreasing probabilities
  bool (*hasBetterProba)(std::tuple<vpImageCircle, unsigned int, float>, std::tuple<vpImageCircle, unsigned int, float>)
    = [](std::tuple<vpImageCircle, unsigned int, float> a, std::tuple<vpImageCircle, unsigned int, float> b) {
    return (std::get<2>(a) > std::get<2>(b));
    };
  std::sort(detectionsWithVotes.begin(), detectionsWithVotes.end(), hasBetterProba);

  // Clearing the storages containing the detection results
  // to have it sorted by decreasing probabilities
  size_t limitMin;
  if (nbCircles < 0) {
    limitMin = nbDetections;
  }
  else {
    limitMin = std::min(nbDetections, (size_t)nbCircles);
  }

  std::vector<vpImageCircle> bestCircles;
  for (size_t i = 0; i < nbDetections; i++) {
    m_finalCircles[i] = std::get<0>(detectionsWithVotes[i]);
    m_finalCircleVotes[i] = std::get<1>(detectionsWithVotes[i]);
    m_finalCirclesProbabilities[i] = std::get<2>(detectionsWithVotes[i]);
    if (i < limitMin) {
      bestCircles.push_back(std::get<0>(detectionsWithVotes[i]));
    }
  }

  return bestCircles;
}

std::vector<vpImageCircle>
vpCircleHoughTransform::detect(const vpImage<unsigned char> &I)
{
  // Cleaning results of potential previous detection
  m_centerCandidatesList.clear();
  m_centerVotes.clear();
  m_edgePointsList.clear();
  m_circleCandidates.clear();
  m_circleCandidatesVotes.clear();
  m_circleCandidatesProbabilities.clear();
  m_finalCircles.clear();
  m_finalCircleVotes.clear();

  // Ensuring that the difference between the max and min radii is big enough to take into account
  // the pixelization of the image
  const float minRadiusDiff = 3.f;
  if (m_algoParams.m_maxRadius - m_algoParams.m_minRadius < minRadiusDiff) {
    if (m_algoParams.m_minRadius > minRadiusDiff / 2.f) {
      m_algoParams.m_maxRadius += minRadiusDiff / 2.f;
      m_algoParams.m_minRadius -= minRadiusDiff / 2.f;
    }
    else {
      m_algoParams.m_maxRadius += minRadiusDiff - m_algoParams.m_minRadius;
      m_algoParams.m_minRadius = 0.f;
    }
  }

  // Ensuring that the difference between the max and min center position is big enough to take into account
  // the pixelization of the image
  const float minCenterPositionDiff = 3.f;
  if (m_algoParams.m_centerXlimits.second - m_algoParams.m_centerXlimits.first < minCenterPositionDiff) {
    m_algoParams.m_centerXlimits.second += minCenterPositionDiff / 2.f;
    m_algoParams.m_centerXlimits.first -= minCenterPositionDiff / 2.f;
  }
  if (m_algoParams.m_centerYlimits.second - m_algoParams.m_centerYlimits.first < minCenterPositionDiff) {
    m_algoParams.m_centerYlimits.second += minCenterPositionDiff / 2.f;
    m_algoParams.m_centerYlimits.first -= minCenterPositionDiff / 2.f;
  }

  // First thing, we need to apply a Gaussian filter on the image to remove some spurious noise
  // Then, we need to compute the image gradients in order to be able to perform edge detection
  computeGradientsAfterGaussianSmoothing(I);

  // Using the gradients, it is now possible to perform edge detection
  // We rely on the Canny edge detector
  // It will also give us the connected edged points
  edgeDetection(I);

  // From the edge map and gradient information, it is possible to compute
  // the center point candidates
  computeCenterCandidates();

  // From the edge map and center point candidates, we can compute candidate
  // circles. These candidate circles are circles whose center belong to
  // the center point candidates and whose radius is a "radius bin" that got
  // enough votes by computing the distance between each point of the edge map
  // and the center point candidate
  computeCircleCandidates();

  // Finally, we perform a merging operation that permits to merge circles
  // respecting similarity criteria (distance between centers and similar radius)
  mergeCircleCandidates();

  return m_finalCircles;
}

void
vpCircleHoughTransform::computeGradientsAfterGaussianSmoothing(const vpImage<unsigned char> &I)
{
  if (m_algoParams.m_filteringAndGradientType == vpImageFilter::CANNY_GBLUR_SOBEL_FILTERING
     || m_algoParams.m_filteringAndGradientType == vpImageFilter::CANNY_GBLUR_SCHARR_FILTERING) {
    // Computing the Gaussian blurr
    vpImage<float> Iblur, GIx;
    vpImageFilter::filterX(I, GIx, m_fg.data, m_algoParams.m_gaussianKernelSize);
    vpImageFilter::filterY(GIx, Iblur, m_fg.data, m_algoParams.m_gaussianKernelSize);

    // Computing the gradients
    vpImageFilter::filter(Iblur, m_dIx, m_gradientFilterX, true);
    vpImageFilter::filter(Iblur, m_dIy, m_gradientFilterY, true);
  }
  else {
    std::string errMsg("[computeGradientsAfterGaussianSmoothing] The filtering + gradient operators \"");
    errMsg += vpImageFilter::vpCannyFilteringAndGradientTypeToString(m_algoParams.m_filteringAndGradientType);
    errMsg += "\" is not implemented (yet).";
    throw(vpException(vpException::notImplementedError, errMsg));
  }
}

void
vpCircleHoughTransform::edgeDetection(const vpImage<unsigned char> &I)
{
  if (m_algoParams.m_cannyBackendType == vpImageFilter::CANNY_VISP_BACKEND) {
    // This is done to increase the time performances, because it avoids to
    // recompute the gradient in the vpImageFilter::canny method
    m_cannyVisp.setFilteringAndGradientType(m_algoParams.m_filteringAndGradientType);
    m_cannyVisp.setCannyThresholds(m_algoParams.m_lowerCannyThresh, m_algoParams.m_upperCannyThresh);
    m_cannyVisp.setCannyThresholdsRatio(m_algoParams.m_lowerCannyThreshRatio, m_algoParams.m_upperCannyThreshRatio);
    m_cannyVisp.setGradients(m_dIx, m_dIy);
    m_edgeMap = m_cannyVisp.detect(I);
  }
  else {
    // We will have to recompute the gradient in the desired backend format anyway so we let
    // the vpImageFilter::canny method take care of it
    vpImageFilter::canny(I, m_edgeMap, m_algoParams.m_gaussianKernelSize, m_algoParams.m_lowerCannyThresh,
    m_algoParams.m_upperCannyThresh, m_algoParams.m_gradientFilterKernelSize, m_algoParams.m_gaussianStdev,
    m_algoParams.m_lowerCannyThreshRatio, m_algoParams.m_upperCannyThreshRatio, true,
    m_algoParams.m_cannyBackendType, m_algoParams.m_filteringAndGradientType);
  }

  for (int i = 0; i < m_algoParams.m_edgeMapFilteringNbIter; i++) {
    filterEdgeMap();
  }
}

void
vpCircleHoughTransform::filterEdgeMap()
{
  vpImage<unsigned char> J = m_edgeMap;

  for (unsigned int i = 1; i < J.getHeight() - 1; i++) {
    for (unsigned int j = 1; j < J.getWidth() - 1; j++) {
      if (J[i][j] == 255) {
        // Consider 8 neighbors
        int topLeftPixel = (int)J[i - 1][j - 1];
        int topPixel = (int)J[i - 1][j];
        int topRightPixel = (int)J[i - 1][j + 1];
        int botLeftPixel = (int)J[i + 1][j - 1];
        int bottomPixel = (int)J[i + 1][j];
        int botRightPixel = (int)J[i + 1][j + 1];
        int leftPixel = (int)J[i][j - 1];
        int rightPixel = (int)J[i][j + 1];
        if ((topLeftPixel + topPixel + topRightPixel
             + botLeftPixel + bottomPixel + botRightPixel
             + leftPixel + rightPixel
             ) >= 2 * 255) {
          // At least 2 of the 8-neighbor points are also an edge point
          // so we keep the edge point
          m_edgeMap[i][j] = 255;
        }
        else {
          // The edge point is isolated => we erase it
          m_edgeMap[i][j] = 0;
        }
      }
    }
  }
}

void
vpCircleHoughTransform::computeCenterCandidates()
{
  // For each edge point EP_i, check the image gradient at EP_i
  // Then, for each image point in the direction of the gradient,
  // increment the accumulator
  // We can perform bilinear interpolation in order not to vote for a "line" of
  // points, but for an "area" of points
  unsigned int nbRows = m_edgeMap.getRows();
  unsigned int nbCols = m_edgeMap.getCols();

  // Computing the minimum and maximum horizontal position of the center candidates
  // The miminum horizontal position of the center is at worst -maxRadius outside the image
  // The maxinum horizontal position of the center is at worst +maxRadiusoutside the image
  // The width of the accumulator is the difference between the max and the min
  int minimumXposition = std::max(m_algoParams.m_centerXlimits.first, -1 * (int)m_algoParams.m_maxRadius);
  int maximumXposition = std::min(m_algoParams.m_centerXlimits.second, (int)(m_algoParams.m_maxRadius + nbCols));
  minimumXposition = std::min(minimumXposition, maximumXposition - 1);
  float minimumXpositionFloat = static_cast<float>(minimumXposition);
  float maximumXpositionFloat = static_cast<float>(maximumXposition);
  int offsetX = minimumXposition;
  int accumulatorWidth = maximumXposition - minimumXposition + 1;
  if (accumulatorWidth <= 0) {
    throw(vpException(vpException::dimensionError, "[vpCircleHoughTransform::computeCenterCandidates] Accumulator width <= 0!"));
  }

  // Computing the minimum and maximum vertical position of the center candidates
  // The miminum vertical position of the center is at worst -maxRadius outside the image
  // The maxinum vertical position of the center is at worst +maxRadiusoutside the image
  // The height of the accumulator is the difference between the max and the min
  int minimumYposition = std::max(m_algoParams.m_centerYlimits.first, -1 * (int)m_algoParams.m_maxRadius);
  int maximumYposition = std::min(m_algoParams.m_centerYlimits.second, (int)(m_algoParams.m_maxRadius + nbRows));
  minimumYposition = std::min(minimumYposition, maximumYposition - 1);
  float minimumYpositionFloat = static_cast<float>(minimumYposition);
  float maximumYpositionFloat = static_cast<float>(maximumYposition);
  int offsetY = minimumYposition;
  int accumulatorHeight = maximumYposition - minimumYposition + 1;
  if (accumulatorHeight <= 0) {
    std::string errMsg("[vpCircleHoughTransform::computeCenterCandidates] Accumulator height <= 0!");
    throw(vpException(vpException::dimensionError, errMsg));
  }

  vpImage<float> centersAccum(accumulatorHeight, accumulatorWidth + 1, 0.); /*!< Votes for the center candidates.*/

  for (unsigned int r = 0; r < nbRows; r++) {
    for (unsigned int c = 0; c < nbCols; c++) {
      if (m_edgeMap[r][c] == 255) {
        // Voting for points in both direction of the gradient
        // Step from min_radius to max_radius in both directions of the gradient
        float mag = std::sqrt(m_dIx[r][c] * m_dIx[r][c] + m_dIy[r][c] * m_dIy[r][c]);

        float sx = 0.f, sy = 0.f;
        if (std::abs(mag) >= std::numeric_limits<float>::epsilon()) {
          sx = m_dIx[r][c] / mag;
          sy = m_dIy[r][c] / mag;
        }
        else {
          continue;
        }

        // Saving the edge point for further use
        m_edgePointsList.push_back(std::pair<unsigned int, unsigned int>(r, c));

        for (int k1 = 0; k1 < 2; k1++) {
          bool hasToStopLoop = false;
          int x_low_prev = std::numeric_limits<int>::max(), y_low_prev, y_high_prev;
          int x_high_prev = y_low_prev = y_high_prev = x_low_prev;

          float rstart = m_algoParams.m_minRadius, rstop = m_algoParams.m_maxRadius;
          float min_minus_c = minimumXpositionFloat - (float)c;
          float min_minus_r = minimumYpositionFloat - (float)r;
          float max_minus_c = maximumXpositionFloat - (float)c;
          float max_minus_r = maximumYpositionFloat - (float)r;
          if (sx > 0) {
            float rmin = (min_minus_c) / sx;
            rstart = std::max(rmin, m_algoParams.m_minRadius);
            float rmax = (max_minus_c) / sx;
            rstop = std::min(rmax, m_algoParams.m_maxRadius);
          }
          else if (sx < 0) {
            float rmin = (max_minus_c) / sx;
            rstart = std::max(rmin, m_algoParams.m_minRadius);
            float rmax = (min_minus_c) / sx;
            rstop = std::min(rmax, m_algoParams.m_maxRadius);
          }

          if (sy > 0) {
            float rmin = (min_minus_r) / sy;
            rstart = std::max(rmin, rstart);
            float rmax = (max_minus_r) / sy;
            rstop = std::min(rmax, rstop);
          }
          else if (sy < 0) {
            float rmin = (max_minus_r) / sy;
            rstart = std::max(rmin, rstart);
            float rmax = (min_minus_r) / sy;
            rstop = std::min(rmax, rstop);
          }

          float deltar_x = 1.f / std::abs(sx);
          float deltar_y = 1.f / std::abs(sy);
          float deltar = std::min(deltar_x, deltar_y);

          for (float rad = rstart; rad <= rstop && !hasToStopLoop; rad += deltar) {
            float x1 = (float)c + (float)rad * sx;
            float y1 = (float)r + (float)rad * sy;

            if (x1 < minimumXpositionFloat || y1 < minimumYpositionFloat
               || x1 > maximumXpositionFloat || y1 > maximumYpositionFloat) {
              continue; // It means that the center is outside the search region.
            }

            int x_low, x_high;
            int y_low, y_high;

            if (x1 > 0.) {
              x_low = static_cast<int>(std::floor(x1));
              x_high = static_cast<int>(std::ceil(x1));
            }
            else {
              x_low = -(static_cast<int>(std::ceil(-x1)));
              x_high = -(static_cast<int>(std::floor(-x1)));
            }

            if (y1 > 0.) {
              y_low = static_cast<int>(std::floor(y1));
              y_high = static_cast<int>(std::ceil(y1));
            }
            else {
              y_low = -(static_cast<int>(std::ceil(-1. * y1)));
              y_high = -(static_cast<int>(std::floor(-1. * y1)));
            }

            if (x_low_prev == x_low && x_high_prev == x_high && y_low_prev == y_low && y_high_prev == y_high) {
              // Avoid duplicated votes to the same center candidate
              continue;
            }
            else {
              x_low_prev = x_low;
              x_high_prev = x_high;
              y_low_prev = y_low;
              y_high_prev = y_high;
            }

            auto updateAccumulator =
              [](const float &x_orig, const float &y_orig,
                  const int &x, const int &y,
                  const int &offsetX, const int &offsetY,
                  const int &nbCols, const int &nbRows,
                  vpImage<float> &accum, bool &hasToStop) {
                    if (x - offsetX <  0      ||
                        x - offsetX >= nbCols ||
                        y - offsetY <  0      ||
                        y - offsetY >= nbRows
                      ) {
                      hasToStop = true;
                    }
                    else {
                      float dx = (x_orig - (float)x);
                      float dy = (y_orig - (float)y);
                      accum[y - offsetY][x - offsetX] += std::abs(dx) + std::abs(dy);
                    }
              };

            updateAccumulator(x1, y1, x_low, y_low,
                              offsetX, offsetY,
                              accumulatorWidth, accumulatorHeight,
                              centersAccum, hasToStopLoop
            );

            updateAccumulator(x1, y1, x_high, y_high,
                              offsetX, offsetY,
                              accumulatorWidth, accumulatorHeight,
                              centersAccum, hasToStopLoop
            );
          }

          sx = -sx;
          sy = -sy;
        }
      }
    }
  }

  // Use dilatation with large kernel in order to determine the
  // accumulator maxima
  vpImage<float> centerCandidatesMaxima = centersAccum;
  int dilatationKernelSize = std::max(m_algoParams.m_dilatationKernelSize, 3); // Ensure at least a 3x3 dilatation operation is performed
  vpImageMorphology::dilatation(centerCandidatesMaxima, dilatationKernelSize);

  // Look for the image points that correspond to the accumulator maxima
  // These points will become the center candidates
  // find the possible circle centers
  int nbColsAccum = centersAccum.getCols();
  int nbRowsAccum = centersAccum.getRows();
  int nbVotes = -1;
  std::vector<std::pair<std::pair<float, float>, float>> peak_positions_votes;
  for (int y = 0; y < nbRowsAccum; y++) {
    int left = -1;
    for (int x = 0; x < nbColsAccum; x++) {
      if (centersAccum[y][x] >= m_algoParams.m_centerMinThresh
         && centersAccum[y][x] == centerCandidatesMaxima[y][x]
         && centersAccum[y][x] >  centersAccum[y][x + 1]
         ) {
        if (left < 0)
          left = x;
        nbVotes = std::max(nbVotes, (int)centersAccum[y][x]);
      }
      else if (left >= 0) {
        int cx = (int)((left + x - 1) * 0.5f);
        float sumVotes = 0.;
        float x_avg = 0., y_avg = 0.;
        int averagingWindowHalfSize = m_algoParams.m_averagingWindowSize / 2;
        int startingRow = std::max(0, y - averagingWindowHalfSize);
        int startingCol = std::max(0, cx - averagingWindowHalfSize);
        int endRow = std::min(accumulatorHeight, y + averagingWindowHalfSize + 1);
        int endCol = std::min(accumulatorWidth, cx + averagingWindowHalfSize + 1);
        for (int r = startingRow; r < endRow; r++) {
          for (int c = startingCol; c < endCol; c++) {
            sumVotes += centersAccum[r][c];
            x_avg += centersAccum[r][c] * (c);
            y_avg += centersAccum[r][c] * (r);
          }
        }
        float avgVotes = sumVotes / (float)(m_algoParams.m_averagingWindowSize * m_algoParams.m_averagingWindowSize);
        if (avgVotes > m_algoParams.m_centerMinThresh) {
          x_avg /= (float)(sumVotes);
          y_avg /= (float)(sumVotes);
          std::pair<float, float> position(y_avg + (float)offsetY, x_avg + (float)offsetX);
          std::pair<std::pair<float, float>, float> position_vote(position, avgVotes);
          peak_positions_votes.push_back(position_vote);
        }
        if (nbVotes < 0) {
          throw(vpException(vpException::badValue, "nbVotes (" + std::to_string(nbVotes) + ") < 0, thresh = " + std::to_string(m_algoParams.m_centerMinThresh)));
        }
        left = -1;
        nbVotes = -1;
      }
    }
  }

  unsigned int nbPeaks = peak_positions_votes.size();
  if (nbPeaks > 0) {
    std::vector<bool> has_been_merged(nbPeaks, false);
    std::vector<std::pair<std::pair<float, float>, float>> merged_peaks_position_votes;
    float squared_distance_max = m_algoParams.m_centerMinDist * m_algoParams.m_centerMinDist;
    for (unsigned int idPeak = 0; idPeak < nbPeaks; idPeak++) {
      float votes = peak_positions_votes[idPeak].second;
      if (has_been_merged[idPeak]) {
        // Ignoring peak that has already been merged
        continue;
      }
      else if (votes < m_algoParams.m_centerMinThresh) {
        // Ignoring peak whose number of votes is lower than the threshold
        has_been_merged[idPeak] = true;
        continue;
      }
      std::pair<float, float> position = peak_positions_votes[idPeak].first;
      std::pair<float, float> barycenter;
      barycenter.first = position.first * peak_positions_votes[idPeak].second;
      barycenter.second = position.second * peak_positions_votes[idPeak].second;
      float total_votes = peak_positions_votes[idPeak].second;
      float nb_electors = 1.f;
      // Looking for potential similar peak in the following peaks
      for (unsigned int idCandidate = idPeak + 1; idCandidate < nbPeaks; idCandidate++) {
        float votes_candidate = peak_positions_votes[idCandidate].second;
        if (has_been_merged[idCandidate]) {
          continue;
        }
        else if (votes_candidate < m_algoParams.m_centerMinThresh) {
          // Ignoring peak whose number of votes is lower than the threshold
          has_been_merged[idCandidate] = true;
          continue;
        }
        // Computing the distance with the peak of insterest
        std::pair<float, float> position_candidate = peak_positions_votes[idCandidate].first;
        double squared_distance = (position.first - position_candidate.first) * (position.first - position_candidate.first)
          + (position.second - position_candidate.second) * (position.second - position_candidate.second);

        // If the peaks are similar, update the barycenter peak between them and corresponding votes
        if (squared_distance < squared_distance_max) {
          barycenter.first += position_candidate.first * votes_candidate;
          barycenter.second += position_candidate.second * votes_candidate;
          total_votes += votes_candidate;
          nb_electors += 1.f;
          has_been_merged[idCandidate] = true;
        }
      }

      float avg_votes = total_votes / nb_electors;
      // Only the centers having enough votes are considered
      if (avg_votes > m_algoParams.m_centerMinThresh) {
        barycenter.first /= total_votes;
        barycenter.second /= total_votes;
        std::pair<std::pair<float, float>, float> barycenter_votes(barycenter, avg_votes);
        merged_peaks_position_votes.push_back(barycenter_votes);
      }
    }

    auto sortingCenters = [](const std::pair<std::pair<float, float>, float> &position_vote_a,
                            const std::pair<std::pair<float, float>, float> &position_vote_b)
      {
        return position_vote_a.second > position_vote_b.second;
      };

    std::sort(merged_peaks_position_votes.begin(), merged_peaks_position_votes.end(), sortingCenters);

    nbPeaks = merged_peaks_position_votes.size();
    int nbPeaksToKeep = (m_algoParams.m_expectedNbCenters > 0 ? m_algoParams.m_expectedNbCenters : nbPeaks);
    nbPeaksToKeep = std::min(nbPeaksToKeep, (int)nbPeaks);
    for (int i = 0; i < nbPeaksToKeep; i++) {
      std::cout << "Peak : (" << merged_peaks_position_votes[i].first.first << " ; " << merged_peaks_position_votes[i].first.second << ")\tVotes = " << merged_peaks_position_votes[i].second << std::endl;
      m_centerCandidatesList.push_back(merged_peaks_position_votes[i].first);
      m_centerVotes.push_back(merged_peaks_position_votes[i].second);
    }
  }
}

void
vpCircleHoughTransform::computeCircleCandidates()
{
  size_t nbCenterCandidates = m_centerCandidatesList.size();
  int nbBins = static_cast<int>((m_algoParams.m_maxRadius - m_algoParams.m_minRadius + 1)/ m_algoParams.m_mergingRadiusDiffThresh);
  nbBins = std::max((int)1, nbBins); // Avoid having 0 bins, which causes segfault
  std::vector<float> radiusAccumList; /*!< Radius accumulator for each center candidates.*/
  std::vector<float> radiusActualValueList; /*!< Vector that contains the actual distance between the edge points and the center candidates.*/

  float rmin2 = m_algoParams.m_minRadius * m_algoParams.m_minRadius;
  float rmax2 = m_algoParams.m_maxRadius * m_algoParams.m_maxRadius;
  float circlePerfectness2 = m_algoParams.m_circlePerfectness * m_algoParams.m_circlePerfectness;

  for (size_t i = 0; i < nbCenterCandidates; i++) {
    std::pair<float, float> centerCandidate = m_centerCandidatesList[i];
    // Initialize the radius accumulator of the candidate with 0s
    radiusAccumList.clear();
    radiusAccumList.resize(nbBins, 0);
    radiusActualValueList.clear();
    radiusActualValueList.resize(nbBins, 0.);

    for (auto edgePoint : m_edgePointsList) {
      // For each center candidate CeC_i, compute the distance with each edge point EP_j d_ij = dist(CeC_i; EP_j)
      float rx = edgePoint.first  - centerCandidate.first;
      float ry = edgePoint.second - centerCandidate.second;
      float r2 = rx * rx + ry * ry;
      if ((r2 > rmin2) && (r2 < rmax2)) {
        float gx = m_dIx[edgePoint.first][edgePoint.second];
        float gy = m_dIy[edgePoint.first][edgePoint.second];
        float grad2 = gx * gx + gy * gy;

        float scalProd = rx * gx + ry * gy;
        float scalProd2 = scalProd * scalProd;
        if (scalProd2 >= circlePerfectness2 * r2 * grad2) {
          // Look for the Radius Candidate Bin RCB_k to which d_ij is "the closest" will have an additionnal vote
          float r = std::sqrt(r2);
          int r_bin = static_cast<int>(std::floor((r - m_algoParams.m_minRadius)/ m_algoParams.m_mergingRadiusDiffThresh));
          r_bin = std::min(r_bin, nbBins - 1);
          if ((r < m_algoParams.m_minRadius + m_algoParams.m_mergingRadiusDiffThresh * 0.5f)
            || (r > m_algoParams.m_minRadius + m_algoParams.m_mergingRadiusDiffThresh * (nbBins - 1 + 0.5f))) {
            // If the radius is at the very beginning of the allowed radii or at the very end, we do not span the vote
            radiusAccumList[r_bin] += 1.f;
            radiusActualValueList[r_bin] += r;
          }
          else {
            float midRadiusPrevBin = m_algoParams.m_minRadius + m_algoParams.m_mergingRadiusDiffThresh * (r_bin - 1.f + 0.5f);
            float midRadiusCurBin = m_algoParams.m_minRadius + m_algoParams.m_mergingRadiusDiffThresh * (r_bin + 0.5f);
            float midRadiusNextBin = m_algoParams.m_minRadius + m_algoParams.m_mergingRadiusDiffThresh * (r_bin + 1.f + 0.5f);

            if (r >= midRadiusCurBin && r <= midRadiusNextBin) {
              // The radius is at  the end of the current bin or beginning of the next, we span the vote with the next bin
              float voteCurBin = (midRadiusNextBin - r) / m_algoParams.m_mergingRadiusDiffThresh; // If the difference is big, it means that we are closer to the current bin
              float voteNextBin = 1.f - voteCurBin;
              radiusAccumList[r_bin] += voteCurBin;
              radiusActualValueList[r_bin] += r * voteCurBin;
              radiusAccumList[r_bin + 1] += voteNextBin;
              radiusActualValueList[r_bin + 1] += r * voteNextBin;
            }
            else {
              // The radius is at the end of the previous bin or beginning of the current, we span the vote with the previous bin
              float votePrevBin = (r - midRadiusPrevBin) / m_algoParams.m_mergingRadiusDiffThresh; // If the difference is big, it means that we are closer to the previous bin
              float voteCurBin = 1.f - votePrevBin;
              radiusAccumList[r_bin] += voteCurBin;
              radiusActualValueList[r_bin] += r * voteCurBin;
              radiusAccumList[r_bin - 1] += votePrevBin;
              radiusActualValueList[r_bin - 1] += r * votePrevBin;
            }
          }
        }
      }
    }

    auto computeEffectiveRadius = [](const float &votes, const float &weigthedSumRadius) {
      float r_effective = -1.f;
      if (votes > std::numeric_limits<float>::epsilon()) {
        r_effective = weigthedSumRadius / votes;
      }
      return r_effective;
      };

    std::vector<float> v_r_effective;
    std::vector<float> v_votes_effective;
    for (int idBin = 0; idBin < nbBins; idBin++) {
      float r_effective = computeEffectiveRadius(radiusAccumList[idBin], radiusActualValueList[idBin]);
      float effective_votes = radiusAccumList[idBin];
      bool is_r_effective_similar = (r_effective > 0.f);
      // Looking for potential similar radii in the following bins
      // If so, compute the barycenter radius between them
      for (int idCandidate = idBin + 1; idCandidate < nbBins && is_r_effective_similar; idCandidate++) {
        float r_effective_candidate = computeEffectiveRadius(radiusAccumList[idCandidate], radiusActualValueList[idCandidate]);
        if (std::abs(r_effective_candidate - r_effective) < m_algoParams.m_mergingRadiusDiffThresh) {
          r_effective = (r_effective * effective_votes + r_effective_candidate * radiusAccumList[idCandidate]) / (effective_votes + radiusAccumList[idCandidate]);
          effective_votes += radiusAccumList[idCandidate];
          radiusAccumList[idCandidate] = -.1f;
          radiusActualValueList[idCandidate] = -1.f;
          is_r_effective_similar = true;
        }
        else {
          is_r_effective_similar = false;
        }
      }

      if (effective_votes > m_algoParams.m_centerMinThresh) {
        // Only the circles having enough votes are considered
        v_r_effective.push_back(r_effective);
        v_votes_effective.push_back(effective_votes);
      }
    }

    unsigned int nbCandidates = v_r_effective.size();
    for (unsigned int idBin = 0; idBin < nbCandidates; idBin++) {
      // If the circle of center CeC_i  and radius RCB_k has enough votes, it is added to the list
      // of Circle Candidates
      float r_effective = v_r_effective[idBin];
      vpImageCircle candidateCircle(vpImagePoint(centerCandidate.first, centerCandidate.second), r_effective);
      float proba = computeCircleProbability(candidateCircle, v_votes_effective[idBin]);
      if (proba > m_algoParams.m_circleProbaThresh) {
        m_circleCandidates.push_back(candidateCircle);
        m_circleCandidatesProbabilities.push_back(proba);
        m_circleCandidatesVotes.push_back(v_votes_effective[idBin]);
      }
    }
  }
}

float
vpCircleHoughTransform::computeCircleProbability(const vpImageCircle &circle, const unsigned int &nbVotes)
{
  float proba(0.f);
  float visibleArc((float)nbVotes);
  float theoreticalLenght = circle.computeArcLengthInRoI(vpRect(vpImagePoint(0, 0), m_edgeMap.getWidth(), m_edgeMap.getHeight()));
  if (theoreticalLenght < std::numeric_limits<float>::epsilon()) {
    proba = 0.f;
  }
  else {
    proba = std::min(visibleArc / theoreticalLenght, 1.f);
  }
  return proba;
}

void
vpCircleHoughTransform::mergeCircleCandidates()
{
  std::vector<vpImageCircle> circleCandidates = m_circleCandidates;
  std::vector<unsigned int> circleCandidatesVotes = m_circleCandidatesVotes;
  std::vector<float> circleCandidatesProba = m_circleCandidatesProbabilities;
  // First iteration of merge
  mergeCandidates(circleCandidates, circleCandidatesVotes, circleCandidatesProba);

  // Second iteration of merge
  mergeCandidates(circleCandidates, circleCandidatesVotes, circleCandidatesProba);

  // Saving the results
  m_finalCircles = circleCandidates;
  m_finalCircleVotes = circleCandidatesVotes;
  m_finalCirclesProbabilities = circleCandidatesProba;
}

void
vpCircleHoughTransform::mergeCandidates(std::vector<vpImageCircle> &circleCandidates, std::vector<unsigned int> &circleCandidatesVotes,
                       std::vector<float> &circleCandidatesProba)
{
  size_t nbCandidates = circleCandidates.size();
  for (size_t i = 0; i < nbCandidates; i++) {
    vpImageCircle cic_i = circleCandidates[i];
    // // For each other circle candidate CiC_j do:
    for (size_t j = i + 1; j < nbCandidates; j++) {
      vpImageCircle cic_j = circleCandidates[j];
      // // // Compute the similarity between CiC_i and CiC_j
      double distanceBetweenCenters = vpImagePoint::distance(cic_i.getCenter(), cic_j.getCenter());
      double radiusDifference = std::abs(cic_i.getRadius() - cic_j.getRadius());
      bool areCirclesSimilar = (distanceBetweenCenters < m_algoParams.m_centerMinDist
                               && radiusDifference  < m_algoParams.m_mergingRadiusDiffThresh
                               );

      if (areCirclesSimilar) {
        // // // If the similarity exceeds a threshold, merge the circle candidates CiC_i and CiC_j and remove CiC_j of the list
        unsigned int totalVotes = circleCandidatesVotes[i] + circleCandidatesVotes[j];
        float totalProba = circleCandidatesProba[i] + circleCandidatesProba[j];
        float newProba = 0.5f * totalProba;
        float newRadius = (cic_i.getRadius() * circleCandidatesProba[i] + cic_j.getRadius() * circleCandidatesProba[j]) / totalProba;
        vpImagePoint newCenter = (cic_i.getCenter() * circleCandidatesProba[i]+ cic_j.getCenter()  * circleCandidatesProba[j]) / totalProba;
        cic_i = vpImageCircle(newCenter, newRadius);
        circleCandidates[j] = circleCandidates[nbCandidates - 1];
        circleCandidatesVotes[i] = totalVotes / 2;
        circleCandidatesVotes[j] = circleCandidatesVotes[nbCandidates - 1];
        circleCandidatesProba[i] = newProba;
        circleCandidatesProba[j] = circleCandidatesProba[nbCandidates - 1];
        circleCandidates.pop_back();
        circleCandidatesVotes.pop_back();
        circleCandidatesProba.pop_back();
        nbCandidates--;
        j--;
      }
    }
    // // Add the circle candidate CiC_i, potentially merged with other circle candidates, to the final list of detected circles
    circleCandidates[i] = cic_i;
  }
}

std::string
vpCircleHoughTransform::toString() const
{
  return m_algoParams.toString();
}

std::ostream &operator<<(std::ostream &os, const vpCircleHoughTransform &detector)
{
  os << detector.toString();
  return os;
}

#endif
