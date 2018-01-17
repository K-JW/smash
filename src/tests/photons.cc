/*
 *    Copyright (c) 2016-2017
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 */

#include "unittest.h"  // This include has to be first

#include "setup.h"

#include "../include/scatteractionphoton.h"

using namespace smash;

TEST(init_particle_types) {
  // enable debugging output
  create_all_loggers(Configuration(""));
  Test::create_actual_particletypes();
}

TEST(init_decay_modes) { Test::create_actual_decaymodes(); }

TEST(pi_rho0_pi_gamma) {
  // set up a π+ and a ρ0 in center-of-momentum-frame
  const ParticleType &type_pi = ParticleType::find(0x211);
  ParticleData pi{type_pi};
  pi.set_4momentum(type_pi.mass(),  // pole mass
                   ThreeVector(0., 0., 2.));
  const ParticleType &type_rho0 = ParticleType::find(0x113);
  ParticleData rho0{type_rho0};
  rho0.set_4momentum(type_rho0.mass(),  // pole mass
                     ThreeVector(0., 0., -2.));
  const int number_of_photons = 10000;
  ParticleList in{pi, rho0};
  const auto act =
      make_unique<ScatterActionPhoton>(in, 0.05, number_of_photons);
  act->add_single_channel();
  double tot_weight = 0.0;
  for (int i = 0; i < number_of_photons; i++) {
    act->generate_final_state();
    tot_weight += act->raw_weight_value();
  }
  COMPARE_RELATIVE_ERROR(tot_weight, 1.0, 0.08);
}

TEST(is_photon_reaction_function) {
  const ParticleData pip{ParticleType::find(0x211)};
  const ParticleData pim{ParticleType::find(-0x211)};
  const ParticleData piz{ParticleType::find(0x111)};
  const ParticleData rhop{ParticleType::find(0x213)};
  const ParticleData rhom{ParticleType::find(-0x213)};
  const ParticleData rhoz{ParticleType::find(0x113)};
  const ParticleData eta{ParticleType::find(0x221)};
  const ParticleData p{ParticleType::find(0x2112)};

  const ParticleList l1{pip, pim}, l2{rhop, pim}, l3{p, pim}, l4{pip, eta};

  VERIFY(ScatterActionPhoton::is_photon_reaction(l1) !=
         ScatterActionPhoton::ReactionType::no_reaction);
  VERIFY(ScatterActionPhoton::is_photon_reaction(l2) !=
         ScatterActionPhoton::ReactionType::no_reaction);
  VERIFY(ScatterActionPhoton::is_photon_reaction(l3) ==
         ScatterActionPhoton::ReactionType::no_reaction);
  VERIFY(ScatterActionPhoton::is_photon_reaction(l4) !=
         ScatterActionPhoton::ReactionType::no_reaction);
}
