/*
 *
 *    Copyright (c) 2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "unittest.h"
#include "setup.h"

#include "../include/decaymodes.h"
#include "../include/isoparticletype.h"
#include "../include/particletype.h"

using namespace Smash;

TEST(init_particle_types) {
  Test::create_actual_particletypes();
}

TEST_CATCH(load_decaymodes_missing_pdg, IsoParticleType::ParticleNotFoundFailure) {
  const std::string decays_input(
      "unknown_particle \n"
      );
  DecayModes::load_decaymodes(decays_input);
}

TEST_CATCH(load_decaymodes_no_decays, DecayModes::MissingDecays) {
  const std::string decays_input(
      "ρ  # rho\n"
      );
  DecayModes::load_decaymodes(decays_input);
}

TEST_CATCH(load_decaymodes_incorrect_start, IsoParticleType::ParticleNotFoundFailure) {
  const std::string decays_input(
      "ρ⁺  # rho+\n"
      );
  DecayModes::load_decaymodes(decays_input);
}


TEST(load_decay_modes) {
  const std::string decays_input(
      " ρ\t# rho\n"
      "\n"
      " 1.0\t1\tπ π\t# pi pi \n"
      "\n"
      "\n"
      "ω      # omega\n"
      "1. 0 π ρ   # pi rho\n"
      );
  DecayModes::load_decaymodes(decays_input);

  {
    const auto &rho0 = ParticleType::find(0x113).decay_modes();
    VERIFY(!rho0.is_empty());
    const auto &modelist = rho0.decay_mode_list();
    COMPARE(modelist.size(), 2u);
    COMPARE(modelist[0]->weight(), 0.5);
    COMPARE(modelist[0]->particle_number(), 2u);
    COMPARE(modelist[0]->particle_types()[0]->pdgcode(),  0x211);
    COMPARE(modelist[0]->particle_types()[1]->pdgcode(), -0x211);
    COMPARE(modelist[1]->weight(), 0.5);
    COMPARE(modelist[1]->particle_number(), 2u);
    COMPARE(modelist[1]->particle_types()[0]->pdgcode(), -0x211);
    COMPARE(modelist[1]->particle_types()[1]->pdgcode(),  0x211);
  }
  {
    const auto &rhoplus = ParticleType::find(0x213).decay_modes();
    VERIFY(!rhoplus.is_empty());
    const auto &modelist = rhoplus.decay_mode_list();
    COMPARE(modelist.size(), 2u);
    COMPARE(modelist[0]->weight(), 0.5);
    COMPARE(modelist[0]->particle_number(), 2u);
    COMPARE(modelist[0]->particle_types()[0]->pdgcode(), 0x111);
    COMPARE(modelist[0]->particle_types()[1]->pdgcode(), 0x211);
    COMPARE(modelist[1]->weight(), 0.5);
    COMPARE(modelist[1]->particle_number(), 2u);
    COMPARE(modelist[1]->particle_types()[0]->pdgcode(), 0x211);
    COMPARE(modelist[1]->particle_types()[1]->pdgcode(), 0x111);
  }
  {
    const auto &rhominus = ParticleType::find(-0x213).decay_modes();
    VERIFY(!rhominus.is_empty());
    const auto &modelist = rhominus.decay_mode_list();
    COMPARE(modelist.size(), 2u);
    COMPARE(modelist[0]->weight(), 0.5);
    COMPARE(modelist[0]->particle_number(), 2u);
    COMPARE(modelist[0]->particle_types()[0]->pdgcode(),  0x111);
    COMPARE(modelist[0]->particle_types()[1]->pdgcode(), -0x211);
    COMPARE(modelist[1]->weight(), 0.5);
    COMPARE(modelist[1]->particle_number(), 2u);
    COMPARE(modelist[1]->particle_types()[0]->pdgcode(), -0x211);
    COMPARE(modelist[1]->particle_types()[1]->pdgcode(),  0x111);
  }
  {
    const auto &omega = ParticleType::find(0x223).decay_modes();
    VERIFY(!omega.is_empty());
    const auto &modelist = omega.decay_mode_list();
    COMPARE(modelist.size(), 3u);
    FUZZY_COMPARE(float(modelist[0]->weight()), 1.f/3.f);
    FUZZY_COMPARE(float(modelist[1]->weight()), 1.f/3.f);
    FUZZY_COMPARE(float(modelist[2]->weight()), 1.f/3.f);
    COMPARE(modelist[0]->particle_number(), 2u);
    COMPARE(modelist[0]->particle_types()[0]->pdgcode(), 0x111);
    COMPARE(modelist[0]->particle_types()[1]->pdgcode(), 0x113);
    COMPARE(modelist[1]->particle_number(), 2u);
    COMPARE(modelist[1]->particle_types()[0]->pdgcode(),  0x211);
    COMPARE(modelist[1]->particle_types()[1]->pdgcode(), -0x213);
    COMPARE(modelist[2]->particle_number(), 2u);
    COMPARE(modelist[2]->particle_types()[0]->pdgcode(), -0x211);
    COMPARE(modelist[2]->particle_types()[1]->pdgcode(),  0x213);
  }
}


TEST_CATCH(add_no_particles, DecayModes::InvalidDecay) {
  DecayModes m;
  m.add_mode(1.f, 0, {});
}

TEST_CATCH(add_one_particle, DecayModes::InvalidDecay) {
  DecayModes m;
  m.add_mode(1.f, 0, {&ParticleType::find(0x211)});
}

TEST(add_two_particles) {
  DecayModes m;
  VERIFY(m.is_empty());
  m.add_mode(1.f, 0,
             {&ParticleType::find(0x211), &ParticleType::find(-0x211)});
  VERIFY(!m.is_empty());
}
