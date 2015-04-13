/*
 *
 *    Copyright (c) 2015
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "unittest.h"
#include "setup.h"

#include "../include/action.h"
#include "../include/scatteractionsfinder.h"

using namespace Smash;

TEST(init_particle_types) {
  Test::create_smashon_particletypes();
}

// test collision_time:
// parallel momenta => impossible collision
TEST(impossible_collision) {
  const auto a = Test::smashon({1., 1., 1., 1.}, {0.1, 0.3, -0.1, 0.2});
  const auto b = Test::smashon({2., 2., 2., 2.}, {0.1, 0.3, -0.1, 0.2});

  VERIFY(ScatterActionsFinder::collision_time(a, b) < 0.0);
}

// test particle_distance:
// particles with null momenta
TEST(particle_distance) {
  const auto a = Test::smashon({1., 1., 1., 1.}, {0.1, 0., 0., 0.});
  const auto b = Test::smashon({2., 2., 2., 2.}, {0.1, 0., 0., 0.});

  ScatterAction act(a, b, 0.);
  const auto distance_squared = act.particle_distance();
  VERIFY(distance_squared >= 0.);
  VERIFY(distance_squared <= 100.);
}

// particles with finite momenta
TEST(finite_momenta) {
  const auto a = Test::smashon({1., 1., 1., 1.}, {0.1, 10.0, 9.0, 8.0});
  const auto b = Test::smashon({2., 2., 2., 2.}, {0.1, -10.0, -90.0, -80.0});
  ScatterAction act(a, b, 0.);
  VERIFY(act.particle_distance() >= 0.);
}
