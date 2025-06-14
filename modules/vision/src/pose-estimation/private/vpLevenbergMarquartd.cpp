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
 * Levenberg Marquartd.
 */

#include <algorithm> // (std::min)
#include <cmath>     // std::fabs
#include <iostream>
#include <limits> // numeric_limits

#include <visp3/core/vpMath.h>
#include "vpLevenbergMarquartd.h"

#define SIGN(x) ((x) < 0 ? -1 : 1)
#define SWAP(a, b, c)                                                                                                  \
  {                                                                                                                    \
    (c) = (a);                                                                                                         \
    (a) = (b);                                                                                                         \
    (b) = (c);                                                                                                         \
  }
#define MIJ(m, i, j, s) ((m) + (static_cast<long>(i) * static_cast<long>(s)) + static_cast<long>(j))
#define TRUE 1
#define FALSE 0

BEGIN_VISP_NAMESPACE

bool lmderMostInnerLoop(ComputeFunctionAndJacobian ptr_fcn, int m, int n,
                        double *x, double *fvec, double *fjac, int ldfjac, double ftol, double xtol, unsigned int maxfev,
                        double *diag, int nprint, int *info, unsigned int *nfev, int *ipvt,
                        double *qtf, double *wa1, double *wa2, double *wa3, double *wa4, const double &gnorm, int &iter, double &delta, double &par, double &pnorm,
                        int &iflag, double &fnorm, double &xnorm);

double enorm(const double *x, int n)
{
  const double rdwarf = 3.834e-20;
  const double rgiant = 1.304e19;

  int i;
  double agiant, floatn;
  double norm_eucl = 0.0;
  double s1 = 0.0, s2 = 0.0, s3 = 0.0;
  double x1max = 0.0, x3max = 0.0;

  floatn = static_cast<double>(n);
  agiant = rgiant / floatn;

  for (i = 0; i < n; ++i) {
    double xabs = std::fabs(x[i]);
    if ((xabs > rdwarf) && (xabs < agiant)) {
      /*
       *  somme pour elements intemediaires.
       */
      s2 += xabs * xabs;
    }

    else if (xabs <= rdwarf) {
      /*
       *  somme pour elements petits.
       */
      if (xabs <= x3max) {
        // --in comment: if (xabs != 0.0)
        if (xabs > std::numeric_limits<double>::epsilon()) {
          s3 += (xabs / x3max) * (xabs / x3max);
        }
      }
      else {
        s3 = 1.0 + (s3 * (x3max / xabs) * (x3max / xabs));
        x3max = xabs;
      }
    }

    else {
      /*
       *  somme pour elements grand.
       */
      if (xabs <= x1max) {
        s1 += (xabs / x1max) * (xabs / x1max);
      }
      else {
        s1 = 1.0 + (s1 * (x1max / xabs) * (x1max / xabs));
        x1max = xabs;
      }
    }
  }

  /*
   *  calcul de la norme.
   */
  // --in comment: if (s1 == 0.0)
  if (std::fabs(s1) <= std::numeric_limits<double>::epsilon()) {
    // --in comment: if (s2 == 0.0)
    if (std::fabs(s2) <= std::numeric_limits<double>::epsilon()) {
      norm_eucl = x3max * sqrt(s3);
    }
    else if (s2 >= x3max) {
      norm_eucl = sqrt(s2 * (1.0 + ((x3max / s2) * (x3max * s3))));
    }
    else { /*if (s2 < x3max)*/
      norm_eucl = sqrt(x3max * ((s2 / x3max) + (x3max * s3)));
    }
  }
  else {
    norm_eucl = x1max * sqrt(s1 + ((s2 / x1max) / x1max));
  }

  return norm_eucl;
}

int lmpar(int n, double *r, int ldr, int *ipvt, double *diag, double *qtb, double *delta, double *par, double *x,
          double *sdiag, double *wa1, double *wa2)
{
  const double tol1 = 0.1; /* tolerance a 0.1  */

  int l;
  unsigned int iter; /* compteur d'iteration */
  int nsing;         /* nombre de singularite de la matrice */
  double dxnorm, fp;
  double temp;
  double dwarf = DBL_MIN; /* plus petite amplitude positive  */

  /*
   *  calcul et stockage dans "x" de la direction de Gauss-Newton. Si
   *  le jacobien a une rangee de moins, on a une solution au moindre
   *  carres.
   */
  nsing = n;

  for (int i = 0; i < n; ++i) {
    wa1[i] = qtb[i];
    double *pt = MIJ(r, i, i, ldr);
    if ((std::fabs(*pt) <= std::numeric_limits<double>::epsilon()) && (nsing == n)) {
      nsing = i - 1;
    }

    if (nsing < n) {
      wa1[i] = 0.0;
    }
  }

  if (nsing >= 0) {
    for (int k = 0; k < nsing; ++k) {
      int i = nsing - 1 - k;
      wa1[i] /= *MIJ(r, i, i, ldr);
      temp = wa1[i];
      int jm1 = i - 1;

      if (jm1 >= 0) {
        for (unsigned int j = 0; j <= static_cast<unsigned int>(jm1); ++j) {
          wa1[j] -= *MIJ(r, i, j, ldr) * temp;
        }
      }
    }
  }

  for (int j = 0; j < n; ++j) {
    l = ipvt[j];
    x[l] = wa1[j];
  }

  /*
   *  initialisation du compteur d'iteration.
   *  evaluation de la fonction a l'origine, et test
   *  d'acceptation de la direction de Gauss-Newton.
   */
  iter = 0;

  for (int i = 0; i < n; ++i) {
    wa2[i] = diag[i] * x[i];
  }

  dxnorm = enorm(wa2, n);

  fp = dxnorm - *delta;

  if (fp >(tol1 * (*delta))) {
    /*
     *  Si le jacobien n'a pas de rangee deficiente,l'etape de
     *  Newton fournit une limite inferieure, parl pour le
     *  zero de la fonction. Sinon cette limite vaut 0.0.
     */
    double parl = 0.0;

    if (nsing >= n) {
      for (int i = 0; i < n; ++i) {
        l = ipvt[i];
        wa1[i] = diag[l] * (wa2[l] / dxnorm);
      }

      for (int i = 0; i < n; ++i) {
        long im1;
        double sum = 0.0;
        im1 = (i - 1L);

        if (im1 >= 0) {
          for (unsigned int j = 0; j <= static_cast<unsigned int>(im1); ++j) {
            sum += (*MIJ(r, i, j, ldr) * wa1[j]);
          }
        }
        wa1[i] = (wa1[i] - sum) / *MIJ(r, i, i, ldr);
      }

      temp = enorm(wa1, n);
      parl = ((fp / *delta) / temp) / temp;
    }

    /*
     *  calcul d'une limite superieure, paru, pour le zero de la
     *  fonction.
     */
    for (int i = 0; i < n; ++i) {
      double sum = 0.0;

      for (int j = 0; j <= i; ++j) {
        sum += *MIJ(r, i, j, ldr) * qtb[j];
      }

      l = ipvt[i];
      wa1[i] = sum / diag[l];
    }

    double gnorm = enorm(wa1, n);
    double paru = gnorm / *delta;

    /* --in comment: if (paru == 0.0) */
    if (std::fabs(paru) <= std::numeric_limits<double>::epsilon()) {
      paru = dwarf / vpMath::minimum(*delta, tol1);
    }

    /*
     *  Si "par" en entree tombe hors de l'intervalle (parl,paru),
     *  on le prend proche du point final.
     */

    *par = vpMath::maximum(*par, parl);
    *par = vpMath::maximum(*par, paru);

    if (std::fabs(*par) <= std::numeric_limits<double>::epsilon()) {
      *par = gnorm / dxnorm;
    }

    /*
     *  debut d'une iteration.
     */
    for (;;) // iter >= 0)
    {
      ++iter;

      /*
       *  evaluation de la fonction a la valeur courant
       *  de "par".
       */
      if (std::fabs(*par) <= std::numeric_limits<double>::epsilon()) {
        const double tol001 = 0.001; /* tolerance a 0.001  */
        *par = vpMath::maximum(dwarf, (tol001 * paru));
      }

      temp = sqrt(*par);

      for (int i = 0; i < n; ++i) {
        wa1[i] = temp * diag[i];
      }

      qrsolv(n, r, ldr, ipvt, wa1, qtb, x, sdiag, wa2);

      for (int i = 0; i < n; ++i) {
        wa2[i] = diag[i] * x[i];
      }

      dxnorm = enorm(wa2, n);
      temp = fp;
      fp = dxnorm - *delta;

      /*
       *  si la fonction est assez petite, acceptation de
       *  la valeur courant de "par". de plus, test des cas
       *  ou parl est nul et ou le nombre d'iteration a
       *  atteint 10.
       */
      bool cond_part_1 = (std::fabs(fp) <= (tol1 * (*delta)));
      bool cond_part_2 = (std::fabs(parl) <= std::numeric_limits<double>::epsilon()) && ((fp <= temp) && (temp < 0.0));
      bool cond_part_3 = (iter == 10);
      if (cond_part_1 || cond_part_2 || cond_part_3) {
        // terminaison.

        return 0;
      }

      /*
       *        calcul de la correction de Newton.
       */

      for (int i = 0; i < n; ++i) {
        l = ipvt[i];
        wa1[i] = diag[l] * (wa2[l] / dxnorm);
      }

      for (unsigned int i = 0; i < static_cast<unsigned int>(n); ++i) {
        wa1[i] = wa1[i] / sdiag[i];
        temp = wa1[i];
        unsigned int jp1 = i + 1;
        if (static_cast<unsigned int>(n) >= jp1) {
          for (unsigned int j = jp1; j < static_cast<unsigned int>(n); ++j) {
            wa1[j] -= (*MIJ(r, i, j, ldr) * temp);
          }
        }
      }

      temp = enorm(wa1, n);
      double parc = ((fp / *delta) / temp) / temp;

      /*
       *  selon le signe de la fonction, mise a jour
       *  de parl ou paru.
       */
      if (fp > 0.0) {
        parl = vpMath::maximum(parl, *par);
      }

      if (fp < 0.0) {
        paru = vpMath::minimum(paru, *par);
      }

      /*
       *  calcul d'une estimee ameliree de "par".
       */
      *par = vpMath::maximum(parl, (*par + parc));
    } /* fin boucle sur iter  */
  }   /* fin fp > tol1 * delta  */

  /*
   *  terminaison.
   */
  if (iter == 0) {
    *par = 0.0;
  }

  return 0;
}

double pythag(double a, double b)
{
  double pyth, p, r, t;

  p = vpMath::maximum(std::fabs(a), std::fabs(b));

  /* --in comment: if (p == 0.0) */
  if (std::fabs(p) <= std::numeric_limits<double>::epsilon()) {
    pyth = p;
    return pyth;
  }

  r = (std::min<double>(std::fabs(a), std::fabs(b)) / p) * (std::min<double>(std::fabs(a), std::fabs(b)) / p);
  t = 4.0 + r;

  /* --in comment: while (t != 4.0) */
  while (std::fabs(t - 4.0) < (std::fabs(vpMath::maximum(t, 4.0)) * std::numeric_limits<double>::epsilon())) {
    double s = r / t;
    double u = 1.0 + (2.0 * s);
    p *= u;
    r *= (s / u) * (s / u);
    t = 4.0 + r;
  }

  pyth = p;
  return pyth;
}

int qrfac(int m, int n, double *a, int lda, int *pivot, int *ipvt, int /* lipvt */, double *rdiag, double *acnorm,
          double *wa)
{
  const double tolerance = 0.05;

  int i, j, ip1, k, kmax, minmn;
  double epsmch;
  double sum, temp, tmp;

  /*
   *  epsmch est la precision machine.
   */
  epsmch = std::numeric_limits<double>::epsilon();

  /*
   *  calcul des normes initiales des lignes et initialisation
   *  de plusieurs tableaux.
   */
  for (i = 0; i < m; ++i) {
    acnorm[i] = enorm(MIJ(a, i, 0, lda), n);
    rdiag[i] = acnorm[i];
    wa[i] = rdiag[i];

    if (pivot) {
      ipvt[i] = i;
    }
  }
  /*
   *     reduction de "a" en "r" avec les transformations de Householder.
   */
  minmn = vpMath::minimum(m, n);
  for (i = 0; i < minmn; ++i) {
    if (pivot) {
      /*
       *  met la ligne de plus grande norme en position
       *  de pivot.
       */
      kmax = i;
      for (k = i; k < m; ++k) {
        if (rdiag[k] > rdiag[kmax]) {
          kmax = k;
        }
      }

      if (kmax != i) {
        for (j = 0; j < n; ++j) {
          SWAP(*MIJ(a, i, j, lda), *MIJ(a, kmax, j, lda), tmp);
        }

        rdiag[kmax] = rdiag[i];
        wa[kmax] = wa[i];

        SWAP(ipvt[i], ipvt[kmax], k);
      }
    }

    /*
     *  calcul de al transformationde Householder afin de reduire
     *  la jeme ligne de "a" a un multiple du jeme vecteur unite.
     */
    double ajnorm = enorm(MIJ(a, i, i, lda), n - i);

    /* --in comment: if (ajnorm != 0.0) */
    if (std::fabs(ajnorm) > std::numeric_limits<double>::epsilon()) {
      if (*MIJ(a, i, i, lda) < 0.0) {
        ajnorm = -ajnorm;
      }

      for (j = i; j < n; ++j) {
        *MIJ(a, i, j, lda) /= ajnorm;
      }
      *MIJ(a, i, i, lda) += 1.0;

      /*
       *  application de la transformation aux lignes
       *  restantes et mise a jour des normes.
       */
      ip1 = i + 1;

      if (m >= ip1) {
        for (k = ip1; k < m; ++k) {
          sum = 0.0;
          for (j = i; j < n; ++j) {
            sum += *MIJ(a, i, j, lda) * *MIJ(a, k, j, lda);
          }

          temp = sum / *MIJ(a, i, i, lda);

          for (j = i; j < n; ++j) {
            *MIJ(a, k, j, lda) -= temp * *MIJ(a, i, j, lda);
          }

          if (pivot && (std::fabs(rdiag[k]) > std::numeric_limits<double>::epsilon())) {
            temp = *MIJ(a, k, i, lda) / rdiag[k];
            rdiag[k] *= sqrt(vpMath::maximum(0.0, (1.0 - (temp * temp))));

            if ((tolerance * (rdiag[k] / wa[k]) * (rdiag[k] / wa[k])) <= epsmch) {
              rdiag[k] = enorm(MIJ(a, k, ip1, lda), (n - 1 - static_cast<int>(i)));
              wa[k] = rdiag[k];
            }
          }
        } /* fin boucle for k  */
      }

    } /* fin if (ajnorm) */

    rdiag[i] = -ajnorm;
  } /* fin for (i = 0; i < minmn; i++) */
  return 0;
}

int qrsolv(int n, double *r, int ldr, int *ipvt, double *diag, double *qtb, double *x, double *sdiag, double *wa)
{
  int i, j, k, kp1, l; /* compteur de boucle  */
  int nsing;
  double cosi, cotg, qtbpj, sinu, tg, temp;

  /*
   *  copie de r et (q transpose) * b afin de preserver l'entree
   *  et initialisation de "s". En particulier, sauvegarde des elements
   *  diagonaux de "r" dans "x".
   */
  for (i = 0; i < n; ++i) {
    for (j = i; j < n; ++j) {
      *MIJ(r, i, j, ldr) = *MIJ(r, j, i, ldr);
    }

    x[i] = *MIJ(r, i, i, ldr);
    wa[i] = qtb[i];
  }

  /*
   *  Elimination de la matrice diagonale "d" en utlisant une rotation
   *  connue.
   */

  for (i = 0; i < n; ++i) {
    /*
     *  preparation de la colonne de d a eliminer, reperage de
     *  l'element diagonal par utilisation de p de la
     *  factorisation qr.
     */
    l = ipvt[i];

    if (std::fabs(diag[l]) > std::numeric_limits<double>::epsilon()) {
      for (k = i; k < n; ++k) {
        sdiag[k] = 0.0;
      }

      sdiag[i] = diag[l];

      /*
       *  Les transformations qui eliminent la colonne de d
       *  modifient seulement qu'un seul element de
       *  (q transpose)*b avant les n premiers elements
       *  lesquels sont inialement nuls.
       */

      qtbpj = 0.0;

      for (k = i; k < n; ++k) {
        /*
         *  determination d'une rotation qui elimine
         *  les elements appropriees dans la colonne
         *  courante de d.
         */

        if (std::fabs(sdiag[k]) > std::numeric_limits<double>::epsilon()) {
          if (std::fabs(*MIJ(r, k, k, ldr)) >= std::fabs(sdiag[k])) {
            tg = sdiag[k] / *MIJ(r, k, k, ldr);
            cosi = 0.5 / sqrt(0.25 + (0.25 * (tg * tg)));
            sinu = cosi * tg;
          }
          else {
            cotg = *MIJ(r, k, k, ldr) / sdiag[k];
            sinu = 0.5 / sqrt(0.25 + (0.25 * (cotg * cotg)));
            cosi = sinu * cotg;
          }

          /*
           *  calcul des elements de la diagonale modifiee
           *  de r et des elements modifies de
           *  ((q transpose)*b,0).
           */
          *MIJ(r, k, k, ldr) = (cosi * *MIJ(r, k, k, ldr)) + (sinu * sdiag[k]);
          temp = (cosi * wa[k]) + (sinu * qtbpj);
          qtbpj = (-sinu * wa[k]) + (cosi * qtbpj);
          wa[k] = temp;

          /*
           *  accumulation des transformations dans
           *  les lignes de s.
           */

          kp1 = k + 1;

          if (n >= kp1) {
            for (j = kp1; j < n; ++j) {
              temp = (cosi * *MIJ(r, k, j, ldr)) + (sinu * sdiag[j]);
              sdiag[j] = (-sinu * *MIJ(r, k, j, ldr)) + (cosi * sdiag[j]);
              *MIJ(r, k, j, ldr) = temp;
            }
          }
        } /* fin if diag[] !=0  */
      }   /* fin boucle for k -> n */
    }     /* fin if diag =0  */

    /*
     *  stokage de l'element diagonal de s et restauration de
     *  l'element diagonal correspondant de r.
     */
    sdiag[i] = *MIJ(r, i, i, ldr);
    *MIJ(r, i, i, ldr) = x[i];
  } /* fin boucle for j -> n  */

  /*
   *  resolution du systeme triangulaire pour z. Si le systeme est
   *  singulier, on obtient une solution au moindres carres.
   */
  nsing = n;

  for (i = 0; i < n; ++i) {
    if ((std::fabs(sdiag[i]) <= std::numeric_limits<double>::epsilon()) && (nsing == n)) {
      nsing = i - 1;
    }

    if (nsing < n) {
      wa[i] = 0.0;
    }
  }

  if (nsing >= 0) {
    for (k = 0; k < nsing; ++k) {
      i = nsing - 1 - k;
      double sum = 0.0;
      int jp1 = i + 1;

      if (nsing >= jp1) {
        for (j = jp1; j < nsing; ++j) {
          sum += *MIJ(r, i, j, ldr) * wa[j];
        }
      }
      wa[i] = (wa[i] - sum) / sdiag[i];
    }
  }
  /*
   *  permutation arriere des composants de z et des componants de x.
   */

  for (j = 0; j < n; ++j) {
    l = ipvt[j];
    x[l] = wa[j];
  }
  return 0;
}

bool lmderMostInnerLoop(ComputeFunctionAndJacobian ptr_fcn, int m, int n,
          double *x, double *fvec, double *fjac, int ldfjac, double ftol, double xtol, unsigned int maxfev,
          double *diag, int nprint, int *info, unsigned int *nfev, int *ipvt,
          double *qtf, double *wa1, double *wa2, double *wa3, double *wa4, const double &gnorm, int &iter, double &delta, double &par, double &pnorm,
          int &iflag, double &fnorm, double &xnorm)
{
  const double tol1 = 0.1, tol5 = 0.5, tol25 = 0.25, tol75 = 0.75, tol0001 = 0.0001;
  /* epsmch est la precision machine.  */
  const double epsmch = std::numeric_limits<double>::epsilon();
  /*
   *  debut de la boucle la plus interne.
   */
  double ratio = 0.0;
  while (ratio < tol0001) {

    /*
     *  determination du parametre de Levenberg-Marquardt.
     */
    lmpar(n, fjac, ldfjac, ipvt, diag, qtf, &delta, &par, wa1, wa2, wa3, wa4);

    /*
     *  stockage de la direction p et x + p. calcul de la norme de p.
     */

    for (int j = 0; j < n; ++j) {
      wa1[j] = -wa1[j];
      wa2[j] = x[j] + wa1[j];
      wa3[j] = diag[j] * wa1[j];
    }

    pnorm = enorm(wa3, n);

    /*
     *  a la premiere iteration, ajustement de la premiere limite de
     *  l'etape.
     */

    if (iter == 1) {
      delta = vpMath::minimum(delta, pnorm);
    }

    /*
     *  evaluation de la fonction en x + p et calcul de leur norme.
     */

    iflag = 1;
    (*ptr_fcn)(m, n, wa2, wa4, fjac, ldfjac, iflag);

    ++(*nfev);

    if (iflag < 0) {
      // termination, normal ou imposee par l'utilisateur.
      if (iflag < 0) {
        *info = iflag;
      }

      iflag = 0;

      if (nprint > 0) {
        (*ptr_fcn)(m, n, x, fvec, fjac, ldfjac, iflag);
      }

      return true;
    }

    double fnorm1 = enorm(wa4, m);

    /*
     *  calcul de la reduction reelle mise a l'echelle.
     */

    double actred = -1.0;

    if ((tol1 * fnorm1) < fnorm) {
      actred = 1.0 - ((fnorm1 / fnorm) * (fnorm1 / fnorm));
    }

    /*
     *  calcul de la reduction predite mise a l'echelle et
     *  de la derivee directionnelle mise a l'echelle.
     */

    for (int i = 0; i < n; ++i) {
      wa3[i] = 0.0;
      int l = ipvt[i];
      double temp = wa1[l];
      for (int j = 0; j <= i; ++j) {
        wa3[j] += *MIJ(fjac, i, j, ldfjac) * temp;
      }
    }

    double temp1 = enorm(wa3, n) / fnorm;
    double temp2 = (sqrt(par) * pnorm) / fnorm;
    double prered = (temp1 * temp1) + ((temp2 * temp2) / tol5);
    double dirder = -((temp1 * temp1) + (temp2 * temp2));

    /*
     *  calcul du rapport entre la reduction reel et predit.
     */

    ratio = 0.0;

    // --in comment: if (prered != 0.0)
    if (std::fabs(prered) > std::numeric_limits<double>::epsilon()) {
      ratio = actred / prered;
    }

    /*
     * mise a jour de la limite de l'etape.
     */

    if (ratio > tol25) {
      // --comment: if par eq 0.0 or ratio lesseq tol75
      if ((std::fabs(par) <= std::numeric_limits<double>::epsilon()) || (ratio <= tol75)) {
        delta = pnorm / tol5;
        par *= tol5;
      }
    }
    else {
      double temp;
      if (actred >= 0.0) {
        temp = tol5;
      }
      else {
        temp = (tol5 * dirder) / (dirder + (tol5 * actred));
      }

      if (((tol1 * fnorm1) >= fnorm) || (temp < tol1)) {
        temp = tol1;
      }

      delta = temp * vpMath::minimum(delta, (pnorm / tol1));
      par /= temp;
    }

    /*
     *  test pour une iteration reussie.
     */
    if (ratio >= tol0001) {
      /*
       *  iteration reussie. mise a jour de x, de fvec, et  de
       *  leurs normes.
       */

      for (int j = 0; j < n; ++j) {
        x[j] = wa2[j];
        wa2[j] = diag[j] * x[j];
      }

      for (int i = 0; i < m; ++i) {
        fvec[i] = wa4[i];
      }

      xnorm = enorm(wa2, n);
      fnorm = fnorm1;
      ++iter;
    }

    /*
     *  tests pour convergence.
     */

    if ((std::fabs(actred) <= ftol) && (prered <= ftol) && ((tol5 * ratio) <= 1.0)) {
      *info = 1;
    }

    if (delta <= (xtol * xnorm)) {
      const int flagInfo = 2;
      *info = flagInfo;
    }

    const int info2 = 2;
    if ((std::fabs(actred) <= ftol) && (prered <= ftol) && ((tol5 * ratio) <= 1.0) && (*info == info2)) {
      const int flagInfo = 3;
      *info = flagInfo;
    }

    if (*info != 0) {
      /*
       * termination, normal ou imposee par l'utilisateur.
       */
      if (iflag < 0) {
        *info = iflag;
      }

      iflag = 0;

      if (nprint > 0) {
        (*ptr_fcn)(m, n, x, fvec, fjac, ldfjac, iflag);
      }

      return true;
    }
    /*
     *  tests pour termination et
     *  verification des tolerances.
     */

    if (*nfev >= maxfev) {
      const int flagInfo = 5;
      *info = flagInfo;
    }

    if ((std::fabs(actred) <= epsmch) && (prered <= epsmch) && ((tol5 * ratio) <= 1.0)) {
      const int flagInfo = 6;
      *info = flagInfo;
    }

    if (delta <= (epsmch * xnorm)) {
      const int flagInfo = 7;
      *info = flagInfo;
    }

    if (gnorm <= epsmch) {
      const int flagInfo = 8;
      *info = flagInfo;
    }

    if (*info != 0) {
      /*
       * termination, normal ou imposee par l'utilisateur.
       */
      if (iflag < 0) {
        *info = iflag;
      }

      iflag = 0;

      if (nprint > 0) {
        (*ptr_fcn)(m, n, x, fvec, fjac, ldfjac, iflag);
      }

      return true;
    }
  } /* fin while ratio >=tol0001  */
  return false;
}

int lmder(ComputeFunctionAndJacobian ptr_fcn, int m, int n,
          double *x, double *fvec, double *fjac, int ldfjac, double ftol, double xtol, double gtol, unsigned int maxfev,
          double *diag, int mode, const double factor, int nprint, int *info, unsigned int *nfev, int *njev, int *ipvt,
          double *qtf, double *wa1, double *wa2, double *wa3, double *wa4)
{
  int oncol = TRUE;
  int iflag, iter;
  int count = 0;
  int i, j, l;
  double delta, fnorm;
  double par, pnorm;
  double sum, temp, xnorm = 0.0;

  *info = 0;
  iflag = 0;
  *nfev = 0;
  *njev = 0;

  /*  verification des parametres d'entree.  */
  if (m < n) {
    return 0;
  }
  if (ldfjac < m) {
    return 0;
  }
  if (ftol < 0.0) {
    return 0;
  }
  if (xtol < 0.0) {
    return 0;
  }
  if (gtol < 0.0) {
    return 0;
  }
  if (maxfev == 0) {
    return 0;
  }
  if (factor <= 0.0) {
    return 0;
  }
  bool cond_part_one = (n <= 0) || (m < n) || (ldfjac < m);
  bool cond_part_two = (ftol < 0.0) || (xtol < 0.0) || (gtol < 0.0);
  bool cond_part_three = (maxfev == 0) || (factor <= 0.0);
  if (cond_part_one || cond_part_two || cond_part_three) {
    /*
     * termination, normal ou imposee par l'utilisateur.
     */
    if (iflag < 0) {
      *info = iflag;
    }

    iflag = 0;

    if (nprint > 0) {
      (*ptr_fcn)(m, n, x, fvec, fjac, ldfjac, iflag);
    }

    return count;
  }

  const int mode2 = 2;
  if (mode == mode2) {
    for (j = 0; j < n; ++j) {
      if (diag[j] <= 0.0) {
        /*
         * termination, normal ou imposee par l'utilisateur.
         */
        if (iflag < 0) {
          *info = iflag;
        }

        iflag = 0;

        if (nprint > 0) {
          (*ptr_fcn)(m, n, x, fvec, fjac, ldfjac, iflag);
        }

        return count;
      }
    }
  }

  /*
   *  evaluation de la fonction au point de depart
   *  et calcul de sa norme.
   */
  iflag = 1;

  (*ptr_fcn)(m, n, x, fvec, fjac, ldfjac, iflag);

  *nfev = 1;

  if (iflag < 0) {
    /*
     * termination, normal ou imposee par l'utilisateur.
     */
    if (iflag < 0) {
      *info = iflag;
    }

    iflag = 0;

    if (nprint > 0) {
      (*ptr_fcn)(m, n, x, fvec, fjac, ldfjac, iflag);
    }

    return count;
  }

  fnorm = enorm(fvec, m);

  /*
   *  initialisation du parametre de Levenberg-Marquardt
   *  et du conteur d'iteration.
   */

  par = 0.0;
  iter = 1;
  const int iflag2 = 2;

  /*
   *  debut de la boucle la plus externe.
   */
  while (count < static_cast<int>(maxfev)) {
    ++count;
    /*
     *  calcul de la matrice jacobienne.
     */

    iflag = iflag2;

    (*ptr_fcn)(m, n, x, fvec, fjac, ldfjac, iflag);

    ++(*njev);

    if (iflag < 0) {
      /*
       * termination, normal ou imposee par l'utilisateur.
       */
      if (iflag < 0) {
        *info = iflag;
      }

      iflag = 0;

      if (nprint > 0) {
        (*ptr_fcn)(m, n, x, fvec, fjac, ldfjac, iflag);
      }

      return count;
    }

    /*
     *  si demandee, appel de fcn pour impression des iterees.
     */
    if (nprint > 0) {
      iflag = 0;
      if (((iter - 1) % nprint) == 0) {
        (*ptr_fcn)(m, n, x, fvec, fjac, ldfjac, iflag);
      }

      if (iflag < 0) {
        /*
         * termination, normal ou imposee par l'utilisateur.
         */
        if (iflag < 0) {
          *info = iflag;
        }

        iflag = 0;

        if (nprint > 0) {
          (*ptr_fcn)(m, n, x, fvec, fjac, ldfjac, iflag);
        }

        return count;
      }
    }

    /*
     * calcul de la factorisation qr du jacobien.
     */
    qrfac(n, m, fjac, ldfjac, &oncol, ipvt, n, wa1, wa2, wa3);

    /*
     *  a la premiere iteration et si mode est 1, mise a l'echelle
     *  en accord avec les normes des colonnes du jacobien initial.
     */

    if (iter == 1) {
      if (mode != mode2) {
        for (j = 0; j < n; ++j) {
          diag[j] = wa2[j];
          if (std::fabs(wa2[j]) <= std::numeric_limits<double>::epsilon()) {
            diag[j] = 1.0;
          }
        }
      }

      /*
       *  a la premiere iteration, calcul de la norme de x mis
       *  a l'echelle et initialisation de la limite delta de
       *  l'etape.
       */

      for (j = 0; j < n; ++j) {
        wa3[j] = diag[j] * x[j];
      }

      xnorm = enorm(wa3, n);
      delta = factor * xnorm;

      if (std::fabs(delta) <= std::numeric_limits<double>::epsilon()) {
        delta = factor;
      }
    }

    /*
     *  formation de (q transpose) * fvec et  stockage des n premiers
     *  composants dans qtf.
     */
    for (i = 0; i < m; ++i) {
      wa4[i] = fvec[i];
    }

    for (i = 0; i < n; ++i) {
      double *pt = MIJ(fjac, i, i, ldfjac);
      if (std::fabs(*pt) > std::numeric_limits<double>::epsilon()) {
        sum = 0.0;

        for (j = i; j < m; ++j) {
          sum += *MIJ(fjac, i, j, ldfjac) * wa4[j];
        }

        temp = -sum / *MIJ(fjac, i, i, ldfjac);

        for (j = i; j < m; ++j) {
          wa4[j] += *MIJ(fjac, i, j, ldfjac) * temp;
        }
      }

      *MIJ(fjac, i, i, ldfjac) = wa1[i];
      qtf[i] = wa4[i];
    }

    /*
     *  calcul de la norme du gradient mis a l'echelle.
     */

    double gnorm = 0.0;

    if (std::fabs(fnorm) > std::numeric_limits<double>::epsilon()) {
      for (i = 0; i < n; ++i) {
        l = ipvt[i];
        if (std::fabs(wa2[l]) > std::numeric_limits<double>::epsilon()) {
          sum = 0.0;
          for (j = 0; j <= i; ++j) {
            sum += *MIJ(fjac, i, j, ldfjac) * (qtf[j] / fnorm);
          }

          gnorm = vpMath::maximum(gnorm, std::fabs(sum / wa2[l]));
        }
      }
    }

    /*
     *  test pour la  convergence de la norme du gradient .
     */

    if (gnorm <= gtol) {
      const int info4 = 4;
      *info = info4;
    }

    if (*info != 0) {
      /*
       * termination, normal ou imposee par l'utilisateur.
       */
      if (iflag < 0) {
        *info = iflag;
      }

      iflag = 0;

      if (nprint > 0) {
        (*ptr_fcn)(m, n, x, fvec, fjac, ldfjac, iflag);
      }

      return count;
    }

    /*
     * remise a l'echelle si necessaire.
     */

    if (mode != mode2) {
      for (j = 0; j < n; ++j) {
        diag[j] = vpMath::maximum(diag[j], wa2[j]);
      }
    }

    bool hasFinished = lmderMostInnerLoop(ptr_fcn, m, n, x, fvec, fjac, ldfjac, ftol, xtol, maxfev,
          diag, nprint, info, nfev, ipvt, qtf, wa1, wa2, wa3, wa4, gnorm, iter, delta, par, pnorm,
          iflag, fnorm, xnorm);
    if (hasFinished) {
      return count;
    }
  }   /*fin while 1*/

  return 0;
}

int lmder1(ComputeFunctionAndJacobian ptr_fcn, int m, int n,
           double *x, double *fvec, double *fjac, int ldfjac, double tol, int *info, int *ipvt, int lwa, double *wa)
{
  const double factor = 100.0;
  unsigned int maxfev, nfev;
  int mode, njev, nprint;
  double ftol, gtol, xtol;

  *info = 0;

  /* verification des parametres en entree qui causent des erreurs */
  const int lwaThresh = ((5 * n) + m);
  if (/*(n <= 0) ||*/ (m < n) || (ldfjac < m) || (tol < 0.0) || (lwa < lwaThresh)) {
    printf("%d %d %d  %d \n", (m < n), (ldfjac < m), (tol < 0.0), (lwa < lwaThresh));
    return (-1);
  }

  /* appel a lmder  */
  const int factor100 = 100;
  maxfev = static_cast<unsigned int>(factor100 * (n + 1));
  ftol = tol;
  xtol = tol;
  gtol = 0.0;
  mode = 1;
  nprint = 0;

  const int factor2 = 2, factor3 = 3, factor4 = 4, factor5 = 5;
  lmder(ptr_fcn, m, n, x, fvec, fjac, ldfjac, ftol, xtol, gtol, maxfev, wa, mode, factor, nprint, info, &nfev, &njev,
        ipvt, &wa[n], &wa[factor2 * n], &wa[factor3 * n], &wa[factor4 * n], &wa[factor5 * n]);

  const int info8 = 8, info4 = 4;
  if (*info == info8) {
    *info = info4;
  }

  return 0;
}

END_VISP_NAMESPACE

#undef TRUE
#undef FALSE
