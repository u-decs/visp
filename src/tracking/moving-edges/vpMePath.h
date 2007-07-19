/****************************************************************************
 *
 * $Id: vpMePath.h,v 1.4 2007-07-19 09:08:50 acherubi Exp $
 *
 * Copyright (C) 1998-2006 Inria. All rights reserved.
 *
 * This software was developed at:
 * IRISA/INRIA Rennes
 * Projet Lagadic
 * Campus Universitaire de Beaulieu
 * 35042 Rennes Cedex
 * http://www.irisa.fr/lagadic
 *
 * This file is part of the ViSP toolkit.
 *
 * This file may be distributed under the terms of the Q Public License
 * as defined by Trolltech AS of Norway and appearing in the file
 * LICENSE included in the packaging of this file.
 *
 * Licensees holding valid ViSP Professional Edition licenses may
 * use this file in accordance with the ViSP Commercial License
 * Agreement provided with the Software.
 *
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Contact visp@irisa.fr if any conditions of this licensing are
 * not clear to you.
 *
 * Description:
 * Moving edges.
 *
 * Authors:
 * Andrea Cherubini
 *
 *****************************************************************************/


#ifndef vpMePath_HH
#define vpMePath_HH


#include <math.h>


#include <visp/vpConfig.h>

#include <visp/vpMatrix.h>
#include <visp/vpColVector.h>

#include <visp/vpMeTracker.h>
#include <visp/vpMeSite.h>

#include <visp/vpImage.h>
#include <visp/vpColor.h>



class VISP_EXPORT vpMePath : public vpMeTracker
{

public:

  vpMePath();
  virtual ~vpMePath();
  void display(vpImage<unsigned char>&I, vpColor::vpColorType col);
  void track(vpImage<unsigned char>& Im);
  void initTracking(vpImage<unsigned char> &I);
  void initTracking(vpImage<unsigned char> &I, int n,
		    unsigned *i, unsigned *j);
  inline void setVerboseMode(bool on=false) {this->verbose = on;};

private:

  void getParameters();
  void sample(vpImage<unsigned char>&image);
  void leastSquare();
  void leastSquareParabola();
  void leastSquareParabolaGivenOrientation();
  void leastSquareLine();        
  void updateNormAng();
  void suppressPoints();
  void seekExtremities(vpImage<unsigned char> &I);
  void getParabolaPoints();
  void displayList(vpImage<unsigned char> &I);
  void computeNormAng(double &norm_ang, vpColVector &K, 
		      double i, double j, bool isLine);
  void reduceList(vpList<vpMeSite> &list, int newSize); 

public:
  
  double aFin, bFin, cFin, thetaFin; //line OR parabola parameters
  bool line; //indicates that the path is a straight line
  double i1, j1, i2, j2; //extremity coordinates
  double *i_par, *j_par; //parabola point coordinates used to find circle
  int numPointPar; //parabola points used to find circle
  int numPoints; // initial points used to find parabola
  unsigned *i_ref, *j_ref; //reference parabola point coordinates
    
private:
  
  vpColVector K, K_line, K_par; //conic parameters
  double aPar, bPar, cPar, thetaPar; //parabola parameters
  bool firstIter;
  double line_error, parab_error, parab_errorTot;
  int lineGoodPoints, parGoodPoints, parGoodPointsTot;
  bool verbose;// verbose mode

  //parameters
  int LSiter;  //least square iterations 
  double good_point_thresh; //threshold on least square line error
  int sampleIter; //every sampleIter iterations sample the curve
  double pointPercentageWithinExtremes; //percent of samples within extremities
  int seekLoops; //number of times extremities are seeked at each iteration
  int numExtr; //number of points seeked after each extremity
  int goodPointGain; //gain for considering good points when selecing curve
  int maxLineScore; //max error tolerated on line before parabola selection
  double par_det_threshold; //parabola det threshold for selecting a line
  double aParThreshold;	//aPar threshold for selecting a line
};




#endif


