/*
 *    Copyright (c) 2012-2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 */
#include <cmath>

#include "include/fourvector.h"

namespace Smash {

/* LorentzBoost - This is equivalent to -velocity Boost from ROOT */
FourVector FourVector::LorentzBoost(const ThreeVector &velocity) const {
  const double velocity_squared = velocity.sqr();

  /* Lorentz gamma = 1/sqrt(1 - v^2) */
  const double gamma = velocity_squared < 1. ?
                       1. / std::sqrt(1. - velocity_squared) : 0;

  /* Lorentz boost for a four-vector x = (x_0, x_1, x_2, x_3) = (x_0, r)
   * and velocity u = (1, v_1, v_2, v_3) = (1, v) (r and v 3-vectors):
   * x'_0 = gamma * (x_0 - r.v)
   *      = gamma * (x_0 - x_1 * v_1 - x_2 * v_2 - x_3 * v_3
   *      = gamma * (x.u)
   * For i = 1, 2, 3:
   * x'_i = x_i + v_i * [(gamma - 1) / v^2 * r.v - gamma * x0]
   *      = x_i + v_i * [(gamma^2 / (gamma + 1) * r.v - gamma * x0]
   *      = x_i - gamma * v_i * [gamma / (gamma + 1) x.u + x_0 / (gamma + 1)]
   */
  // this is used four times in the Vector:
  const double xprime_0 = gamma * (this->x0() - this->threevec()*velocity);
  // this is the part of the space-like components that is always the same:
  const double constantpart = gamma / (gamma + 1) * (xprime_0 + this->x0());
  return FourVector (xprime_0, this->threevec() - velocity * constantpart);
}

}  // namespace Smash
