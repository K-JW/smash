/*
 *
 *    Copyright (c) 2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "include/particletype.h"

#include "include/lineparser.h"
#include "include/outputroutines.h"
#include "include/pdgcode.h"

#include <algorithm>
#include <assert.h>
#include <map>
#include <vector>

namespace Smash {

namespace {
const std::vector<ParticleType> *all_particle_types = nullptr;
}  // unnamed namespace

const std::vector<ParticleType> &ParticleType::list_all() {
  assert(all_particle_types);
  return *all_particle_types;
}

const ParticleType &ParticleType::find(PdgCode pdgcode) {
  const auto found = std::lower_bound(
      all_particle_types->begin(), all_particle_types->end(), pdgcode,
      [](const ParticleType &l, const PdgCode &r) { return l.pdgcode() < r; });
  assert(found != all_particle_types->end());
  assert(found->pdgcode() == pdgcode);
  return *found;
}

bool ParticleType::exists(PdgCode pdgcode) {
  const auto found = std::lower_bound(
      all_particle_types->begin(), all_particle_types->end(), pdgcode,
      [](const ParticleType &l, const PdgCode &r) { return l.pdgcode() < r; });
  if (found != all_particle_types->end()) {
    return found->pdgcode() == pdgcode;
  }
  return false;
}

void ParticleType::create_type_list(const std::string &input) {  //{{{
  static std::vector<ParticleType> type_list;
  type_list.clear();  // in case LoadFailure was thrown and caught and we should
                      // try again
  for (const Line &line : line_parser(input)) {
    std::istringstream lineinput(line.text);
    std::string name;
    float mass, width;
    PdgCode pdgcode;
    lineinput >> name >> mass >> width >> pdgcode;
    if (lineinput.fail()) {
      throw Particles::LoadFailure(build_error_string(
          "While loading the Particle data:\nFailed to convert the input "
          "string to the expected data types.",
          line));
    }
    ensure_all_read(lineinput, line);

    printd("Setting particle type %s mass %g width %g pdgcode %s\n",
           name.c_str(), mass, width, pdgcode.string().c_str());
    printd("Setting particle type %s isospin %i/2 charge %i spin %i/2\n",
           name.c_str(), pdgcode.isospin_total(), pdgcode.charge(),
                                                  pdgcode.spin());

    type_list.emplace_back(name, mass, width, pdgcode);
  }
  type_list.shrink_to_fit();

  std::sort(type_list.begin(), type_list.end(),
            [](const ParticleType &l,
               const ParticleType &r) { return l.pdgcode() < r.pdgcode(); });

  assert(nullptr == all_particle_types);
  all_particle_types = &type_list;
}/*}}}*/

}  // namespace Smash
