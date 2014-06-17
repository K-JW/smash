/*
 *
 *    Copyright (c) 2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "include/particledata.h"

#include "include/width.h"

namespace Smash {

float ParticleData::total_width() const {
  return type_->width_total(momentum_.abs());
}

}  // namespace Smash
