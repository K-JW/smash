/*
 *
 *    Copyright (c) 2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "unittest.h"
#include "../include/modusdefault.h"
#include "../include/boxmodus.h"
#include "../include/collidermodus.h"
#include "../include/experiment.h"
#include "../include/configuration.h"
#include "../include/spheremodus.h"

#include <boost/filesystem.hpp>

using namespace Smash;

TEST(init_particle_types) {
  ParticleType::create_type_list(
      "smashon 0.4 0.0 661\n");
}

TEST(initialize_box) {
  einhard::Logger<> log(einhard::ALL);
  ExperimentParameters param{{0.f, 1.f}, 1.f, 1};
  BoxModus b({"Box:\n"
              "  Initial_Condition: 1\n"
              "  Length: 7.9615\n"
              "  Temperature: 0.5\n"
              "  Start_Time: 0.2\n"
              "  Init_Multiplicities:\n"
              "    661: 724\n"},
             param);

  Particles P;
  // should return START_TIME and set P:
  COMPARE(b.initial_conditions(&P, param), 0.2f);
  COMPARE(P.size(), 724u);
  COMPARE(P.data(67).pdgcode(), 0x661);
  // we might also look at other properties of Particles, like total
  // momentum and such.
  FourVector momentum(0.0, 0.0, 0.0, 0.0);
  for (auto p : P.data()) {
    momentum += p.momentum();
    VERIFY(p.position().x1() <  7.9615);
    VERIFY(p.position().x1() >= 0.0);
    VERIFY(p.position().x2() <  7.9615);
    VERIFY(p.position().x2() >= 0.0);
    VERIFY(p.position().x3() <  7.9615);
    VERIFY(p.position().x3() >= 0.0);
  }
  COMPARE_ABSOLUTE_ERROR(momentum.x1(), 0.0, 1e-12);
  COMPARE_ABSOLUTE_ERROR(momentum.x2(), 0.0, 1e-12);
  COMPARE_ABSOLUTE_ERROR(momentum.x3(), 0.0, 1e-12);
}

TEST(initialize_collider_normal) {
  ExperimentParameters param{{0.f, 1.f}, 1.f, 1};
  ColliderModus n({"Collider:\n"
                   "  Sqrtsnn: 1.6\n"
                   "  Projectile:\n"
                   "    Particles: {661: 1}\n"
                   "  Target:\n"
                   "    Particles: {661: 8}\n"
                   "  Sqrts_Reps: [661, 661]\n"
                   "  Initial_Distance: 0\n"
                   "  Impact:\n"
                   "    Value: 0\n"},
                  param);
  Particles P;
  COMPARE(n.initial_conditions(&P, param), 0.f);
  COMPARE(P.size(), 9u);
  for (auto p : P.data()) {
    // velocity should be +- sqrt(3/4)
    COMPARE_RELATIVE_ERROR(p.velocity().sqr(), 0.75, 1e-6);
    // this is the mass squared
    COMPARE_RELATIVE_ERROR(p.momentum().sqr(), 0.16, 1e-6);
    COMPARE(p.position().x0(), 0.0);
    COMPARE(p.pdgcode(), PdgCode(0x661));
    COMPARE_RELATIVE_ERROR(p.momentum().x0(), 0.8, 1e-6);
    COMPARE_ABSOLUTE_ERROR(p.momentum().x1(), 0.0, 1e-6);
    COMPARE_ABSOLUTE_ERROR(p.momentum().x2(), 0.0, 1e-6);
    COMPARE_RELATIVE_ERROR(std::abs(p.momentum().x3()), std::sqrt(0.48), 1e-6);
  }
  // all other things can only be tested with statistics.
}

TEST_CATCH(initialize_collider_low_energy, ModusDefault::InvalidEnergy) {
  ExperimentParameters param{{0.f, 1.f}, 1.f, 1};
  ColliderModus n({"Collider:\n"
                   "  Sqrtsnn: 0.5\n"
                   "  Projectile:\n"
                   "    Particles: {661: 1}\n"
                   "  Target:\n"
                   "    Particles: {661: 8}\n"
                   "  Sqrts_Reps: [661, 661]\n"
                   "  Initial_Distance: 0\n"},
                  param);
  Particles P;
  n.initial_conditions(&P, param);
}

TEST_CATCH(initialize_nucleus_empty_projectile, ColliderModus::ColliderEmpty) {
  ExperimentParameters param{{0.f, 1.f}, 1.f, 1};
  ColliderModus n({"Collider:\n"
                   "  Sqrtsnn: 1.6\n"
                   "  Projectile:\n"
                   "    Particles: {661: 0}\n"
                   "  Target:\n"
                   "    Particles: {661: 8}\n"
                   "  Sqrts_Reps: [0, 0]\n"
                   "  Initial_Distance: 0\n"},
                  param);
  Particles P;
  n.initial_conditions(&P, param);
}

TEST_CATCH(initialize_nucleus_empty_target, ColliderModus::ColliderEmpty) {
  ExperimentParameters param{{0.f, 1.f}, 1.f, 1};
  ColliderModus n({"Collider:\n"
                   "  Sqrtsnn: 1.6\n"
                   "  Projectile:\n"
                   "    Particles: {661: 8}\n"
                   "  Target:\n"
                   "    Particles: {661: 0}\n"
                   "  Sqrts_Reps: [0, 0]\n"
                   "  Initial_Distance: 0\n"},
                  param);
  Particles P;
  n.initial_conditions(&P, param);
}

TEST(initialize_sphere) {
  ExperimentParameters param{{0.f, 1.f}, 1.f, 1};
  SphereModus s({"Sphere:\n"
                 "  Radius: 10\n"
                 "  Start_Time: 0.0\n"
                 "  Init_Multiplicities: {661: 500}\n"
                 "  Sphere_Temperature: 0.2\n"},
                param);
  Particles P;
//Is the correct number of particles in the map?
  COMPARE(s.initial_conditions(&P, param), 0.0f);
  COMPARE(P.size(), 500u);
  COMPARE(P.data(67).pdgcode(), 0x661);
// total momentum check
  FourVector momentum(0.0, 0.0, 0.0, 0.0);
// position less than radius?
  float radius = 0.0;
  for (auto p : P.data()) {
    momentum += p.momentum();
    radius = sqrt(p.position().x1()*p.position().x1()+
    p.position().x2()*p.position().x2()+p.position().x3()*p.position().x3()); 
    VERIFY(radius <  10.0);
  }
  COMPARE_ABSOLUTE_ERROR(momentum.x1(), 0.0, 1e-12);
  COMPARE_ABSOLUTE_ERROR(momentum.x2(), 0.0, 1e-12);
  COMPARE_ABSOLUTE_ERROR(momentum.x3(), 0.0, 1e-12);
}
