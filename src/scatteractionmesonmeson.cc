/*
 *
 *    Copyright (c) 2015-2017
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "include/scatteractionmesonmeson.h"

namespace smash {

void ScatterActionMesonMeson::format_debug_output(std::ostream &out) const {
  out << " Meson-Meson  ";
  ScatterAction::format_debug_output(out);
}

}  // namespace smash
