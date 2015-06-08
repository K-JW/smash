/*
 *    Copyright (c) 2015
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 */

#include "include/tabulation.h"

#include <gsl/gsl_integration.h>
#include <algorithm>

#include "include/integrate.h"

namespace Smash {

Tabulation::Tabulation(float x_min, float range, int num_points,
                       IntegParam ip, IntegrandFunction f)
                      : x_min_(x_min), inv_dx_(num_points/range) {
  values_.resize(num_points);
  const float dx = range/num_points;
  for (int i = 0; i < num_points; i++) {
    values_[i] = calculate_value(x_min_ + i*dx, ip, f);
  }
}

float Tabulation::calculate_value(float x, IntegParam ip, IntegrandFunction f) {
  ip.srts = x;

  Integrator integrate;
  return integrate(ip.type->minimum_mass(), ip.srts - ip.m2, [&](float y) {return f(y, &ip);});
}

float Tabulation::get_value_step(float x) const {
  if (x < x_min_) {
    return 0.;
  }
  // this rounds correctly because float -> int conversion truncates
  const unsigned int n = (x - x_min_) * inv_dx_ + 0.5f;
  if (n >= values_.size()) {
    return values_.back();
  } else {
    return values_[n];
  }
}

float Tabulation::get_value_linear(float x) const {
  if (x < x_min_) {
    return 0.;
  }
  // here n is the lower index
  const unsigned int n = (x - x_min_) * inv_dx_;
  const float r = (x - x_min_) * inv_dx_ - n;
  if (n >= values_.size() - 1) {
    return values_.back();
  } else {
    return values_[n]*(1.-r) + values_[n+1]*r;
  }
}

}  // namespace Smash
