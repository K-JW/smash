/*
 *
 *    Copyright (c) 2015
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 *    This code has been adapted from ROOT's TGraphSmooth.cxx.
 *
 *    Copyright (c) 2006 Rene Brun and Fons Rademakers.
 *    Copyright (c) 2001 Christian Stratowa
 *    Copyright (c) 1999-2001 Robert Gentleman, Ross Ihaka and the R
 *                            Development Core Team
 */
#include <algorithm>
#include <cassert>
#include <cmath>

#include <iostream> // fixme

#include "include/lowess.h"

namespace Smash {

////////////////////////////////////////////////////////////////////////////////
/// Fit value at x[i]
///  Based on R function lowest: Translated to C++ by C. Stratowa
///  (R source file: lowess.c by R Development Core Team (C) 1999-2001)
static void lowest(double *x, double *y, int n, double &xs, double &ys, int nleft,
                   int nright, double *w, bool userw, double *rw, bool &ok) {
  int nrt, j;
  double a, b, c, d, h, h1, h9, r, range;

  x--;
  y--;
  w--;
  rw--;

  range = x[n] - x[1];
  h = std::max(xs - x[nleft], x[nright] - xs);
  h9 = 0.999 * h;
  h1 = 0.001 * h;

  // sum of weights
  a = 0.;
  j = nleft;
  while (j <= n) {
    // compute weights (pick up all ties on right)
    w[j] = 0.;
    r = std::abs(x[j] - xs);
    if (r <= h9) {
      if (r <= h1) {
        w[j] = 1.;
      } else {
        d = (r / h) * (r / h) * (r / h);
        w[j] = (1. - d) * (1. - d) * (1. - d);
      }
      if (userw)
        w[j] *= rw[j];
      a += w[j];
    } else if (x[j] > xs)
      break;
    j = j + 1;
  }

  // rightmost pt (may be greater than nright because of ties)
  nrt = j - 1;
  if (a <= 0.)
    ok = false;
  else {
    ok = true;
    // weighted least squares: make sum of w[j] == 1
    for (j = nleft; j <= nrt; j++)
      w[j] /= a;
    if (h > 0.) {
      a = 0.;
      // use linear fit weighted center of x values
      for (j = nleft; j <= nrt; j++)
        a += w[j] * x[j];
      b = xs - a;
      c = 0.;
      for (j = nleft; j <= nrt; j++)
        c += w[j] * (x[j] - a) * (x[j] - a);
      if (std::sqrt(c) > 0.001 * range) {
        b /= c;
        // points are spread out enough to compute slope
        for (j = nleft; j <= nrt; j++)
          w[j] *= (b * (x[j] - a) + 1.);
      }
    }
    ys = 0.;
    for (j = nleft; j <= nrt; j++)
      ys += w[j] * y[j];
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Lowess regression smoother.
/// Based on R function clowess: Translated to C++ by C. Stratowa
/// (R source file: lowess.c by R Development Core Team (C) 1999-2001)
static void lowess(double *x, double *y, int n, double *ys, double span, int iter,
                   double delta, double *rw, double *res) {
  int i, iiter, j, last, m1, m2, nleft, nright, ns;
  double alpha, c1, c9, cmad, cut, d1, d2, denom, r;
  bool ok;

  if (n < 2) {
    ys[0] = y[0];
    return;
  }

  // nleft, nright, last, etc. must all be shifted to get rid of these:
  x--;
  y--;
  ys--;

  // at least two, at most n points
  ns = std::max(2, std::min(n, static_cast<int>(span * n + 1e-7)));

  // robustness iterations
  iiter = 1;
  while (iiter <= iter + 1) {
    nleft = 1;
    nright = ns;
    last = 0;  // index of prev estimated point
    i = 1;     // index of current point

    for (;;) {
      if (nright < n) {
        // move nleft,  nright to right if radius decreases
        d1 = x[i] - x[nleft];
        d2 = x[nright + 1] - x[i];

        // if d1 <= d2 with x[nright+1] == x[nright], lowest fixes
        if (d1 > d2) {
          // radius will not decrease by move right
          nleft++;
          nright++;
          continue;
        }
      }

      // fitted value at x[i]
      bool iterg1 = iiter > 1;
      lowest(&x[1], &y[1], n, x[i], ys[i], nleft, nright, res, iterg1, rw, ok);
      if (!ok)
        ys[i] = y[i];

      // all weights zero copy over value (all rw==0)
      if (last < i - 1) {
        denom = x[i] - x[last];

        // skipped points -- interpolate non-zero - proof?
        for (j = last + 1; j < i; j++) {
          alpha = (x[j] - x[last]) / denom;
          ys[j] = alpha * ys[i] + (1. - alpha) * ys[last];
        }
      }

      // last point actually estimated
      last = i;

      // x coord of close points
      cut = x[last] + delta;
      for (i = last + 1; i <= n; i++) {
        if (x[i] > cut)
          break;
        if (x[i] == x[last]) {
          ys[i] = ys[last];
          last = i;
        }
      }
      i = std::max(last + 1, i - 1);
      if (last >= n)
        break;
    }

    // residuals
    for (i = 0; i < n; i++)
      res[i] = y[i + 1] - ys[i + 1];

    // compute robustness weights except last time
    if (iiter > iter)
      break;
    for (i = 0; i < n; i++)
      rw[i] = std::abs(res[i]);

    // compute cmad := 6 * median(rw[], n)
    m1 = n / 2;
    // partial sort, for m1 & m2
    std::partial_sort(rw, rw + n, rw + m1);  // Psort(rw, n, m1);
    if (n % 2 == 0) {
      m2 = n - m1 - 1;
      std::partial_sort(rw, rw + n, rw + m2);  // Psort(rw, n, m2);
      cmad = 3. * (rw[m1] + rw[m2]);
    } else { /* n odd */
      cmad = 6. * rw[m1];
    }

    c9 = 0.999 * cmad;
    c1 = 0.001 * cmad;
    for (i = 0; i < n; i++) {
      r = std::abs(res[i]);
      if (r <= c1)
        rw[i] = 1.;
      else if (r <= c9)
        rw[i] = (1. - (r / cmad) * (r / cmad)) * (1. - (r / cmad) * (r / cmad));
      else
        rw[i] = 0.;
    }
    iiter++;
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Smooth data with Lowess smoother
///
/// This function performs the computations for the LOWESS smoother
/// (see the reference below). Lowess returns the output points
/// x and y which give the coordinates of the smooth.
///
/// \param[in] grin Input graph
/// \param[in] span the smoother span. This gives the proportion of points in
/// the plot
///     which influence the smooth at each value. Larger values give more
///     smoothness.
/// \param[in] iter the number of robustifying iterations which should be
/// performed.
///     Using smaller values of iter will make lowess run faster.
/// \param[in] delta values of x which lie within delta of each other replaced
/// by a
///     single value in the output from lowess.
///     For delta = 0, delta will be calculated.
///
/// References:
///
/// - Cleveland, W. S. (1979) Robust locally weighted regression and smoothing
///        scatterplots. J. Amer. Statist. Assoc. 74, 829-836.
/// - Cleveland, W. S. (1981) LOWESS: A program for smoothing scatterplots
///        by robust locally weighted regression.
///        The American Statistician, 35, 54.
std::vector<double> smooth(std::vector<double>& x, std::vector<double>& y, double span, int iter, double delta) {
    assert(x.size() == y.size());
    std::vector<double> result;
    result.resize(x.size());
    std::vector<double> rw;
    rw.resize(x.size());
    std::vector<double> res;
    res.resize(x.size());
    // TODO: initialize to zero?
    lowess(&x.front(), &y.front(), x.size(), &result.front(), span, iter, delta, &rw.front(), &res.front());
    return std::move(result);
}

}  // namespace Smash
