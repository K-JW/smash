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

float ScatterActionNucleonKaon::elastic_parametrization() {
  const PdgCode &pdg_a = incoming_particles_[0].type().pdgcode();
  const PdgCode &pdg_b = incoming_particles_[1].type().pdgcode();

  const PdgCode &nucleon = pdg_a.is_nucleon() ? pdg_a : pdg_b;
  const PdgCode &kaon = pdg_a.is_nucleon() ? pdg_b : pdg_a;
  assert(kaon != nucleon);

  const double s = mandelstam_s();

  float sig_el = 0.f;
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
      throw std::runtime_error("elastic cross section for antinucleon-kaon "
                               "not implemented");
  }

  if (sig_el > 0) {
    return sig_el;
  } else {
    std::stringstream ss;
    const auto name_a = incoming_particles_[0].type().name();
    const auto name_b = incoming_particles_[1].type().name();
    ss << "problem in CrossSections::elastic: a=" << name_a
       << " b=" << name_b << " j_a=" << pdg_a.spin() << " j_b="
       << pdg_b.spin() << " sigma=" << sig_el << " s=" << s;
    throw std::runtime_error(ss.str());
  }
}

void ScatterActionNucleonKaon::format_debug_output(std::ostream &out) const {
  out << "Nucleon-Kaon  ";
  ScatterAction::format_debug_output(out);
}

CollisionBranchList ScatterActionNucleonKaon::two_to_two_cross_sections() {
  const ParticleType &type_particle_a = incoming_particles_[0].type();
  const ParticleType &type_particle_b = incoming_particles_[1].type();

  CollisionBranchList process_list = two_to_two_inel(type_particle_a,
                                                     type_particle_b);

  return process_list;
}

CollisionBranchList ScatterActionNucleonKaon::two_to_two_inel(
                            const ParticleType &type_particle_a,
                            const ParticleType &type_particle_b) {
  CollisionBranchList process_list;

  const ParticleType &type_nucleon =
      type_particle_a.pdgcode().is_nucleon() ? type_particle_a : type_particle_b;
  const ParticleType &type_kaon =
      type_particle_a.pdgcode().is_nucleon() ? type_particle_b : type_particle_a;

  const auto pdg_nucleon = type_nucleon.pdgcode().code();
  const auto pdg_kaon = type_kaon.pdgcode().code();

  const double sqrts = sqrt_s();

  // calculate cross section
  auto add_channel
    = [&](float xsection, const ParticleType &type_a, const ParticleType &type_b) {
      if (xsection > really_small) {
        process_list.push_back(make_unique<CollisionBranch>(
          type_a, type_b, xsection, ProcessType::TwoToTwo));
      }
  };
  if (pdg_kaon == -0x321) {
    switch (pdg_nucleon) {
      case 0x2212: {  // p
        const ParticleType &type_pi0 = ParticleType::find(0x111);
        add_channel(kminusp_piminussigmaplus(sqrts),
                    ParticleType::find(-0x211), ParticleType::find(0x3222));
        add_channel(kminusp_piplussigmaminus(sqrts),
                    ParticleType::find(0x211), ParticleType::find(-0x3222));
        add_channel(kminusp_pi0sigma0(sqrts),
                    type_pi0, ParticleType::find(0x3212));
        add_channel(kminusp_pi0lambda(sqrts),
                    type_pi0, ParticleType::find(0x3122));
        break;
      }
      case 0x2112:  // n
        const ParticleType &type_piminus = ParticleType::find(-0x211);
        add_channel(kminusn_piminussigma0(sqrts),
                    type_piminus, ParticleType::find(0x3212));
        add_channel(kminusn_pi0sigmaminus(sqrts),
                    ParticleType::find(0x111), ParticleType::find(0x3222));
        add_channel(kminusn_piminuslambda(sqrts),
                    type_piminus, ParticleType::find(0x3122));
        break;
    }
  }

  return process_list;
}

}  // namespace Smash
