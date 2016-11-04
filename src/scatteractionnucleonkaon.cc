/*
 *
 *    Copyright (c) 2015
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "include/scatteractionnucleonkaon.h"

#include "include/clebschgordan.h"
#include "include/cxx14compat.h"
#include "include/parametrizations.h"
#include "include/pdgcode_constants.h"

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
      case pdg::p:
      switch (kaon.code()) {
        case pdg::K_p:
          sig_el = kplusp_elastic(s);
          break;
        case pdg::K_m:
          sig_el = kminusp_elastic(s);
          break;
        case pdg::K_z:
          sig_el = k0p_elastic(s);
          break;
        case pdg::Kbar_z:
          sig_el = kbar0p_elastic(s);
          break;
      }
      break;
      case pdg::n:
      switch (kaon.code()) {
        case pdg::K_p:
          sig_el = kplusn_elastic(s);
          break;
        case pdg::K_m:
          sig_el = kminusn_elastic(s);
          break;
        case pdg::K_z:
          sig_el = k0n_elastic(s);
          break;
        case pdg::Kbar_z:
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

  const double s = mandelstam_s();
  const double sqrts = sqrt_s();

  switch (pdg_kaon) {
    case pdg::K_m: {
      // All inelastic K- N channels here are strangeness exchange, plus one
      // charge exchange.
      switch (pdg_nucleon) {
        case pdg::p: {
          const ParticleType &type_pi0 = ParticleType::find(pdg::pi_z);
          add_channel(process_list,
                      [&] { return kminusp_piminussigmaplus(sqrts); },
                      sqrts, ParticleType::find(pdg::pi_m), ParticleType::find(pdg::Sigma_p));
          add_channel(process_list,
                      [&] { return kminusp_piplussigmaminus(sqrts); },
                      sqrts, ParticleType::find(pdg::pi_p), ParticleType::find(pdg::Sigma_m));
          add_channel(process_list,
                      [&] { return kminusp_pi0sigma0(sqrts); },
                      sqrts, type_pi0, ParticleType::find(pdg::Sigma_z));
          add_channel(process_list,
                      [&] { return kminusp_pi0lambda(sqrts); },
                      sqrts, type_pi0, ParticleType::find(pdg::Lambda));
          add_channel(process_list,
                      [&] { return kminusp_kbar0n(s); },
                      sqrts, ParticleType::find(pdg::Kbar_z), ParticleType::find(pdg::n));
          break;
        }
        case pdg::n: {
          const ParticleType &type_piminus = ParticleType::find(pdg::pi_m);
          add_channel(process_list,
                      [&] { return kminusn_piminussigma0(sqrts); },
                      sqrts, type_piminus, ParticleType::find(pdg::Sigma_z));
          add_channel(process_list,
                      [&] { return kminusn_pi0sigmaminus(sqrts); },
                      sqrts, ParticleType::find(pdg::pi_z), ParticleType::find(pdg::Sigma_m));
          add_channel(process_list,
                      [&] { return kminusn_piminuslambda(sqrts); },
                      sqrts, type_piminus, ParticleType::find(pdg::Lambda));
          break;
        }
      }
      break;
    }
    case pdg::K_p: {
      // All inelastic channels are K+ N -> K Delta -> K pi N, with identical
      // cross section, weighted by the isospin factor.
      switch (pdg_nucleon) {
        case pdg::p: {
          const auto sigma_kplusp = kplusp_inelastic(s);
          const auto& type_K_z = ParticleType::find(pdg::K_z);
          const auto& type_Delta_pp = ParticleType::find(pdg::Delta_pp);
          const auto& type_K_p = ParticleType::find(pdg::K_p);
          const auto& type_Delta_p = ParticleType::find(pdg::Delta_p);

          add_channel(process_list,
                      [&] { return sigma_kplusp * kplusn_ratios.get_ratio(
                                   type_nucleon, type_kaon, type_K_z, type_Delta_pp); },
                      sqrts, type_K_z, type_Delta_pp);
          add_channel(process_list,
                      [&] { return sigma_kplusp * kplusn_ratios.get_ratio(
                                   type_nucleon, type_kaon, type_K_p, type_Delta_p); },
                      sqrts, type_K_p, type_Delta_p);
          break;
        }
        case pdg::n: {
          const auto sigma_kplusn = kplusn_inelastic(s);
          const auto& type_K_z = ParticleType::find(pdg::K_z);
          const auto& type_Delta_p = ParticleType::find(pdg::Delta_p);
          const auto& type_K_p = ParticleType::find(pdg::K_p);
          const auto& type_Delta_z = ParticleType::find(pdg::Delta_z);

          add_channel(process_list,
                      [&] { return sigma_kplusn * kplusn_ratios.get_ratio(
                                   type_nucleon, type_kaon, type_K_z, type_Delta_p); },
                      sqrts, type_K_z, type_Delta_p);
          add_channel(process_list,
                      [&] { return sigma_kplusn * kplusn_ratios.get_ratio(
                                   type_nucleon, type_kaon, type_K_p, type_Delta_z); },
                      sqrts, type_K_p, type_Delta_z);
          break;
        }
      }
      break;
    }
    case pdg::K_z: {
      // K+ and K0 have the same isospin projection, they are assumed to have
      // the same cross section here.

      switch (pdg_nucleon) {
        case pdg::p: {
          const auto sigma_kplusp = kplusp_inelastic(s);
          const auto& type_K_z = ParticleType::find(pdg::K_z);
          const auto& type_Delta_p = ParticleType::find(pdg::Delta_p);
          const auto& type_K_p = ParticleType::find(pdg::K_p);
          const auto& type_Delta_z = ParticleType::find(pdg::Delta_z);

          add_channel(process_list,
                      [&] { return sigma_kplusp * kplusn_ratios.get_ratio(
                                   type_nucleon, type_kaon, type_K_z, type_Delta_p); },
                      sqrts, type_K_z, type_Delta_p);
          add_channel(process_list,
                      [&] { return sigma_kplusp * kplusn_ratios.get_ratio(
                                   type_nucleon, type_kaon, type_K_p, type_Delta_z); },
                      sqrts, type_K_p, type_Delta_z);
          break;
        }
        case pdg::n: {
          const auto sigma_kplusn = kplusn_inelastic(s);
          const auto& type_K_z = ParticleType::find(pdg::K_z);
          const auto& type_Delta_z = ParticleType::find(pdg::Delta_z);
          const auto& type_K_p = ParticleType::find(pdg::K_p);
          const auto& type_Delta_m = ParticleType::find(pdg::Delta_m);

          add_channel(process_list,
                      [&] { return sigma_kplusn * kplusn_ratios.get_ratio(
                                   type_nucleon, type_kaon, type_K_z, type_Delta_z); },
                      sqrts, type_K_z, type_Delta_z);
          add_channel(process_list,
                      [&] { return sigma_kplusn * kplusn_ratios.get_ratio(
                                   type_nucleon, type_kaon, type_K_p, type_Delta_m); },
                      sqrts, type_K_p, type_Delta_m);
          break;
        }
      }
      break;
    }
    case pdg::Kbar_z:
      if (pdg_nucleon == pdg::n) {
        add_channel(process_list,
                    [&] { return kminusp_kbar0n(s); },
                    sqrts, ParticleType::find(pdg::K_m), ParticleType::find(pdg::p));
      }
      break;
  }

  return process_list;
}

}  // namespace Smash
