/*
 *
 *    Copyright (c) 2015
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "include/scatteractionnucleonkaon.h"

#include "include/cxx14compat.h"
#include "include/parametrizations.h"

namespace Smash {

static inline bool is_kaon(const PdgCode &pdg) {
    const auto code = std::abs(pdg.code());
    return (code == 0x321) || (code == 0x311);
}

static inline bool is_nucleon(const PdgCode &pdg) {
    const auto code = std::abs(pdg.code());
    return (code == 0x2212) || (code == 0x2112);
}

CollisionBranchPtr ScatterActionNucleonKaon::elastic_cross_section(float) {
  const PdgCode &pdg_a = incoming_particles_[0].type().pdgcode();
  const PdgCode &pdg_b = incoming_particles_[1].type().pdgcode();

  const PdgCode &nucleon = is_nucleon(pdg_a) ? pdg_a : pdg_b;
  const PdgCode &kaon = is_kaon(pdg_a) ? pdg_a : pdg_b;
  assert(kaon != nucleon);

  const double s = mandelstam_s();

  float sig_el;
  switch (nucleon.code()) {
    case 0x2212:  // p
      switch (kaon.code()) {
        case 0x321:  // K+
          sig_el = kplusp_elastic(s);
          break;
        case -0x321:  // K-
          sig_el = kminusp_elastic(s);
          break;
        case 0x311:  // K0
          sig_el = k0p_elastic(s);
          break;
        case -0x311:  // Kbar0
          sig_el = kbar0p_elastic(s);
          break;
      }
      break;
    case 0x2112:  // n
      switch (kaon.code()) {
        case 0x321:  // K+
          sig_el = kplusn_elastic(s);
          break;
        case -0x321:  // K-
          sig_el = kminusn_elastic(s);
          break;
        case 0x311:  // K0
          sig_el = k0n_elastic(s);
          break;
        case -0x311:  // Kbar0
          sig_el = kbar0n_elastic(s);
          break;
      }
      break;
    default:
      throw std::runtime_error("not implemented");
  }

  if (sig_el > 0) {
      return make_unique<CollisionBranch>(incoming_particles_[0].type(),
              incoming_particles_[1].type(), sig_el, ProcessType::Elastic);
  } else {
    std::stringstream ss;
    ss << "problem in CrossSections::elastic: " << pdg_a.string().c_str()
      << " " << pdg_b.string().c_str() << " " << pdg_a.spin() << " "
      << pdg_b.spin() << " " << sig_el << " " << s;
    throw std::runtime_error(ss.str());
  }
}

void ScatterActionNucleonKaon::format_debug_output(std::ostream &out) const {
  out << "Nucleon-Kaon  ";
  ScatterAction::format_debug_output(out);
}


}  // namespace Smash
