/*
 *
 *    Copyright (c) 2014-2015
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "include/processbranch.h"

#include <limits>

#include "include/particledata.h"

namespace Smash {

ParticleList ProcessBranch::particle_list() const {
  ParticleList l;
  l.reserve(particle_number());
  for (const auto &type : particle_types()) {
    l.push_back(ParticleData{*type});
  }
  return std::move(l);
}

float ProcessBranch::threshold() const {
  // Sum up the (minimum) masses of all final-state particles
  double thr = 0.;  // this requires double-precision to ensure that the sum is
                    // never smaller than the real sum would be without rounding
  for (const auto &type : particle_types()) {
    thr += type->minimum_mass();
  }
  const float rounded = thr;  // this may round up or down. Up is good. If down
                              // we must add one ULP.
  return rounded < thr
             ? std::nextafter(rounded, std::numeric_limits<float>::max())
             : rounded;
}

}  // namespace Smash
