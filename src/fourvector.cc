/*
 *    Copyright (c) 2012-2013
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 */
#include <cmath>

#include "include/fourvector.h"

namespace Smash {

/* LorentzBoost - This is equivalent to -velocity Boost from ROOT */
FourVector FourVector::LorentzBoost(const FourVector &velocity) const {
  /* 4-velocity u = (1, v) = (1, v_1, v_2, v_3)
   * u.u = 1 - v_1^2 - v_2^2 - v_3^2
   */
  double velocity_squared = velocity.Dot(velocity);
  /* Lorentz gamma = 1/sqrt(1 - v^2)
   *               = 1/sqrt(1 - v_1^2 - v_2^2 - v_3^2)
   *               = 1/sqrt(u.u)
   */
  double gamma = velocity_squared > 0 ? 1.0 / std::sqrt(velocity_squared) : 0;

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
  return FourVector(gamma * this->Dot(velocity),
    this->x1() - gamma * velocity.x1()
     * (gamma / (gamma + 1) * this->Dot(velocity) + this->x0() / (gamma + 1)),
    this->x2() - gamma * velocity.x2()
     * (gamma / (gamma + 1) * this->Dot(velocity) + this->x0() / (gamma + 1)),
    this->x3() - gamma * velocity.x3()
     * (gamma / (gamma + 1) * this->Dot(velocity) + this->x0() / (gamma + 1)));
}

}  // namespace Smash
