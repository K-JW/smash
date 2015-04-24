/*
 *
 *    Copyright (c) 2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "include/particledata.h"

#include <iomanip>
#include <iostream>
#include <vector>

#include "include/iomanipulators.h"

namespace Smash {

std::ostream &operator<<(std::ostream &out, const ParticleData &p) {
  using namespace std;
  out.fill(' ');
  return out << p.type().name() << right
             << "{id:" << field<6> << p.id()
             << ", pos [fm]:"
             << p.position() << ", mom [GeV]:" << p.momentum() << "}";
}

std::ostream &operator<<(std::ostream &out, const ParticleList &particle_list) {
  using namespace std;
  auto column = out.tellp();
  out << '[';
  for (const auto &p : particle_list) {
    if (out.tellp() - column >= 201) {
      out << '\n';
      column = out.tellp();
      out << ' ';
    }
    out << setw(5) << setprecision(3) << p.momentum().abs3() << p.type().name();
  }
  return out << ']';
}

std::ostream &operator<<(std::ostream &out,
                         const PrintParticleListDetailed &particle_list) {
  using namespace std;
  bool first = true;
  out << '[';
  for (const auto &p : particle_list.list) {
    if (first) {
      first = false;
    } else {
      out << "\n ";
    }
    out << p;
  }
  return out << ']';
}

}  // namespace Smash
