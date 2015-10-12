/*
 *
 *    Copyright (c) 2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "unittest.h"
#include "../include/particletype.h"

using namespace Smash;

TEST(assign) {
  PdgCode smashon("9003234");
  // there is a double for mass and a float for width. This is
  // intentional.
  ParticleType A("σ", 3.243, 0.234f, smashon);
  COMPARE(A.name(), "σ");
  COMPARE(A.mass(), 3.243f);
  COMPARE(A.width_at_pole(), 0.234f);
  COMPARE(A.pdgcode(), smashon);
  COMPARE(A.isospin(), 0);
  COMPARE(A.charge(), smashon.charge());
  COMPARE(A.spin(), smashon.spin());
}

TEST_CATCH(load_from_incorrect_string, ParticleType::LoadFailure) {
  ParticleType::create_type_list("Hallo Welt! (wave)");
}

TEST_CATCH(load_one_particle_with_incorrect_newline, ParticleType::LoadFailure) {
  const std::string parts("pi0 0.1350\n-1.0 111");
  ParticleType::create_type_list(parts);
}

TEST_CATCH(load_duplicate_particle, ParticleType::LoadFailure) {
  ParticleType::create_type_list("π⁺ 0.138 0.0 211\nπ⁺ 0.138 0.0 211\n");
}

TEST(create_type_list) {
  ParticleType::create_type_list(
      "π0 0.1350 -1.0 111\n"
      "# Hello\n"
      "  # Will you ignore me? #### sldfkjsdf\n"
      "\t\t  \t # yes?\n"
      "π+ 0.1396 -1.0 211 # This is pi+. Swell.\n"
      "\t\n\t  ρ0 0.7755 \t 0.149 113\n"
      "ρ+ 0.7755 0.149 213\n"
      "η 0.5479 1.0e-6 221\n"
      "ω 0.7827 0.0085 223\n"
      "K⁺ 0.494 0.0    321\n"
      "K⁰ 0.494 0.0    311\n"
      "N+ 0.9383 -1.0 2212\n"
      "N0 0.9396 -1.0 2112\n"
      "Δ++ 1.232 0.117 2224\n"
      "Δ+ 1.232 0.117 2214\n"
      "Δ0 1.232 0.117 2114\n"
      "Δ- 1.232 0.117 1114\n"
      "Λ  1.116 0.0   3122\n"
      "Σ⁺ 1.189 0.0   3222\n"
      "e⁻ 0.000511 0.0 11\n"
      "μ⁻ 0.105 0.0 13\n"
      "γ  0.0  0.0  22");

  COMPARE(ParticleType::list_all().size(), 33u);  // 19 given explicitly + 14 antiparticles

  // pi0
  ParticleTypePtr type = &ParticleType::find(0x111);
  COMPARE(type->mass(), 0.135f);
  COMPARE(type->width_at_pole(), -1.f);
  COMPARE(type->pdgcode(), PdgCode(0x111));
  COMPARE(type->charge(), 0);
  COMPARE(type->spin(), 0u);
  COMPARE(type->isospin(), 2);
  COMPARE(type->isospin3(), 0);
  COMPARE(type->isospin3_rel(), 0.f);

  // pi+
  type = &ParticleType::find(0x211);
  COMPARE(type->mass(), 0.1396f);
  COMPARE(type->width_at_pole(), -1.f);
  COMPARE(type->pdgcode(), PdgCode(0x211));
  COMPARE(type->charge(), 1);
  COMPARE(type->spin(), 0u);
  COMPARE(type->isospin(), 2);
  COMPARE(type->isospin3(), 2);
  COMPARE(type->isospin3_rel(), 1.f);

  // rho0
  type = &ParticleType::find(0x113);
  COMPARE(type->mass(), 0.7755f);
  COMPARE(type->width_at_pole(), .149f);
  COMPARE(type->pdgcode(), PdgCode(0x113));
  COMPARE(type->charge(), 0);
  COMPARE(type->spin(), 2u);
  COMPARE(type->isospin(), 2);
  COMPARE(type->isospin3(), 0);
  COMPARE(type->isospin3_rel(), 0.f);

  // anti-Delta-
  type = &ParticleType::find(-0x1114);
  COMPARE(type->mass(), 1.232f);
  COMPARE(type->width_at_pole(), .117f);
  COMPARE(type->pdgcode().dump(), 0x80001114);
  COMPARE(type->charge(), 1);
  COMPARE(type->spin(), 3u);
  COMPARE(type->isospin(), 3);
  COMPARE(type->isospin3(), 3);
  COMPARE(type->isospin3_rel(), 1.f);

  // proton
  type = &ParticleType::find(0x2212);
  COMPARE(type->mass(), .9383f);
  COMPARE(type->width_at_pole(), -1.f);
  COMPARE(type->pdgcode().dump(), 0x2212u);
  COMPARE(type->charge(), 1);
  COMPARE(type->spin(), 1u);
  COMPARE(type->isospin(), 1);
  COMPARE(type->isospin3(), 1);
  COMPARE(type->isospin3_rel(), 1.f);

  // neutron
  type = &ParticleType::find(0x2112);
  COMPARE(type->mass(), .9396f);
  COMPARE(type->width_at_pole(), -1.f);
  COMPARE(type->pdgcode().dump(), 0x2112u);
  COMPARE(type->charge(), 0);
  COMPARE(type->spin(), 1u);
  COMPARE(type->isospin(), 1);
  COMPARE(type->isospin3(), -1);
  COMPARE(type->isospin3_rel(), -1.f);

  // eta
  type = &ParticleType::find(0x221);
  COMPARE(type->mass(), .5479f);
  COMPARE(type->width_at_pole(), 1.0e-6f);
  COMPARE(type->pdgcode().dump(), 0x221u);
  COMPARE(type->charge(), 0);
  COMPARE(type->spin(), 0u);
  COMPARE(type->isospin(), 0);
  COMPARE(type->isospin3(), 0);
  COMPARE(type->isospin3_rel(), 0.f);

  // electron
  type = &ParticleType::find(0x11);
  COMPARE(type->mass(), .000511f);
  COMPARE(type->width_at_pole(), 0.f);
  COMPARE(type->pdgcode().dump(), 0x11u);
  COMPARE(type->charge(), -1);
  COMPARE(type->spin(), 1u);
  COMPARE(type->isospin(), 0);
  COMPARE(type->isospin3(), 0);
  COMPARE(type->isospin3_rel(), 0.f);

  // anti-mu
  type = &ParticleType::find(-0x13);
  COMPARE(type->mass(), .105f);
  COMPARE(type->width_at_pole(), 0.f);
  COMPARE(type->pdgcode().dump(), 0x80000013u);
  COMPARE(type->charge(), +1);
  COMPARE(type->spin(), 1u);
  COMPARE(type->isospin(), 0);
  COMPARE(type->isospin3(), 0);
  COMPARE(type->isospin3_rel(), 0.f);

  // photon
  type = &ParticleType::find(0x22);
  COMPARE(type->mass(), 0.f);
  COMPARE(type->width_at_pole(), 0.f);
  COMPARE(type->pdgcode().dump(), 0x22u);
  COMPARE(type->charge(), 0);
  COMPARE(type->spin(), 2u);
  COMPARE(type->isospin(), 0);
  COMPARE(type->isospin3(), 0);
  COMPARE(type->isospin3_rel(), 0.f);

  // K0
  type = &ParticleType::find(0x311);
  COMPARE(type->mass(), .494f);
  COMPARE(type->width_at_pole(), 0.f);
  COMPARE(type->pdgcode().dump(), 0x311u);
  COMPARE(type->charge(), 0);
  COMPARE(type->spin(), 0u);
  COMPARE(type->isospin(), 1);
  COMPARE(type->isospin3(), -1);
  COMPARE(type->isospin3_rel(), -1.f);

  // K-
  type = &ParticleType::find(-0x321);
  COMPARE(type->mass(), .494f);
  COMPARE(type->width_at_pole(), 0.f);
  COMPARE(type->pdgcode().dump(), 0x80000321u);
  COMPARE(type->charge(), -1);
  COMPARE(type->spin(), 0u);
  COMPARE(type->isospin(), 1);
  COMPARE(type->isospin3(), -1);
  COMPARE(type->isospin3_rel(), -1.f);

  // Sigma+
  type = &ParticleType::find(0x3222);
  COMPARE(type->mass(), 1.189f);
  COMPARE(type->width_at_pole(), 0.f);
  COMPARE(type->pdgcode().dump(), 0x3222u);
  COMPARE(type->charge(), 1);
  COMPARE(type->spin(), 1u);
  COMPARE(type->isospin(), 2);
  COMPARE(type->isospin3(), 2);
  COMPARE(type->isospin3_rel(), 1.f);

  // Lambda
  type = &ParticleType::find(0x3122);
  COMPARE(type->mass(), 1.116f);
  COMPARE(type->width_at_pole(), 0.f);
  COMPARE(type->pdgcode().dump(), 0x3122u);
  COMPARE(type->charge(), 0);
  COMPARE(type->spin(), 1u);
  COMPARE(type->isospin(), 0);
  COMPARE(type->isospin3(), 0);
  COMPARE(type->isospin3_rel(), 0.f);
}

TEST(list_all_iteration) {
  std::size_t count = 0;
  for (const auto &type : ParticleType::list_all()) {
    const PdgCode pdg = type.pdgcode();
    const ParticleType &type2 = ParticleType::find(pdg);
    COMPARE(&type, &type2);
    ++count;
  }
  COMPARE(count, ParticleType::list_all().size());
}

TEST(exists) {
  VERIFY(ParticleType::exists(0x211));   // pi+
  VERIFY(ParticleType::exists(0x111));   // pi0
  VERIFY(ParticleType::exists(-0x211));  // pi-

  VERIFY(!ParticleType::exists(0x667));   // ttbar
}
