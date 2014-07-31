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
#include "../include/nucleusmodus.h"
#include "../include/experiment.h"
#include "../include/configuration.h"

#include <boost/filesystem.hpp>

using namespace Smash;

TEST(init_particle_types) {
  ParticleType::create_type_list(
      "# NAME MASS[GEV] WIDTH[GEV] PDG\n"
      "smashon 0.123 1.2 661\n");
}

static ParticleData create_smashon_particle(int id = -1) {
  return ParticleData{ParticleType::find(0x661), id};
}

// create a particle list with various interesting particles. We will
// assume a box of 5 fm length and a time step (for propagation) of 1
// fm.
static void create_particle_list(Particles &P) {
  // particle that doesn't move:
  ParticleData particle_stop = create_smashon_particle();
  // particle that moves with speed of light
  ParticleData particle_fast = create_smashon_particle();
  // particle that moves slowly:
  ParticleData particle_slow = create_smashon_particle();
  // particle that will cross a box boundary at high x:
  ParticleData particle_x_hi = create_smashon_particle();
  // particle that will cross a box boundary at low y:
  ParticleData particle_y_lo = create_smashon_particle();
  // particle that will cross a box boundary at low x and high z:
  ParticleData particle_xlzh = create_smashon_particle();

  // set momenta:
  particle_stop.set_momentum(FourVector(4.0, 0.0, 0.0, 0.0));
  particle_fast.set_momentum(FourVector(sqrt(0.02), 0.1, -.1, 0.0));
  particle_slow.set_momentum(FourVector(sqrt(1.13), 0.1, 0.2, -.3));
  particle_x_hi.set_momentum(FourVector(0.1, 0.1, 0.0, 0.0));
  particle_y_lo.set_momentum(FourVector(0.1, 0.0, -.1, 0.0));
  particle_xlzh.set_momentum(FourVector(0.5, -.3, 0.0, 0.4));

  // set positions:
  particle_stop.set_position(FourVector(0.0, 5.6, 0.7, 0.8));
  particle_fast.set_position(FourVector(0.5, -.7, 0.8, 8.9));
  particle_slow.set_position(FourVector(0.7, 0.1, 0.2, 0.3));
  particle_x_hi.set_position(FourVector(1.2, 4.5, 5.0, 0.0));
  particle_y_lo.set_position(FourVector(1.8, 0.0, 19., 0.0));
  particle_xlzh.set_position(FourVector(2.2, 0.2, 0.0, 4.8));

  // add particles (and make sure the particles get the correct ID):
  COMPARE(P.add_data(particle_stop), 0);
  COMPARE(P.add_data(particle_fast), 1);
  COMPARE(P.add_data(particle_slow), 2);
  COMPARE(P.add_data(particle_x_hi), 3);
  COMPARE(P.add_data(particle_y_lo), 4);
  COMPARE(P.add_data(particle_xlzh), 5);

  return;
}

TEST(sanity_default) {
  ModusDefault m;
  Particles P;
  create_particle_list(P);
  COMPARE(m.sanity_check(&P), 0);
}

TEST(sanity_box) {
  Configuration conf(TEST_CONFIG_PATH);
  conf["Modi"]["Box"]["INITIAL_CONDITION"] = 1;
  conf["Modi"]["Box"]["LENGTH"] = 5.0;
  conf["Modi"]["Box"]["TEMPERATURE"] = 0.13;
  conf["Modi"]["Box"]["START_TIME"] = 0.2;
  ExperimentParameters param{{0.f, 1.f}, 1.f, 0.0, 1};
  BoxModus b(conf["Modi"], param);
  Particles P;
  create_particle_list(P);
  COMPARE(b.sanity_check(&P), 4);
}

TEST(sanity_collider) {
  Configuration conf(TEST_CONFIG_PATH);
  conf["Modi"]["Collider"]["SQRTS"] = 1.0;
  conf["Modi"]["Collider"]["PROJECTILE"] = "661";
  conf["Modi"]["Collider"]["TARGET"] = "661";
  ExperimentParameters param{{0.f, 1.f}, 1.f, 0.0, 1};
  ColliderModus c(conf["Modi"], param);
  Particles P;
  create_particle_list(P);
  COMPARE(c.sanity_check(&P), 0);
}

TEST(sanity_nucleus) {
  Configuration conf(TEST_CONFIG_PATH);
  conf.take({"Modi", "Nucleus", "Projectile"});
  conf.take({"Modi", "Nucleus", "Target"});
  conf["Modi"]["Nucleus"]["SQRTSNN"] = 1.0;
  conf["Modi"]["Nucleus"]["SQRTS_REPS"][0] = "661";
  conf["Modi"]["Nucleus"]["SQRTS_REPS"][1] = "661";
  conf["Modi"]["Nucleus"]["Projectile"]["PARTICLES"]["661"] = 1;
  conf["Modi"]["Nucleus"]["Target"]["PARTICLES"]["661"] = 1;
  ExperimentParameters param{{0.f, 1.f}, 1.f, 0.0, 1};
  NucleusModus n(conf["Modi"], param);
  Particles P;
  create_particle_list(P);
  COMPARE(n.sanity_check(&P), 0);
}

//TEST(sanity_sphere) {
//  Configuration conf(TEST_CONFIG_PATH);
//  Particles P{""};
//   conf["Modi"]["Sphere"]["..."] = 1.0;
//   ExperimentParameters param{{0.f, 1.f}, 1.f, 0.0, 1};
//   SphereModus s(conf["Modi"], param);
//  create_particle_list(P);
//  COMPARE(s.sanity_check(&P), 0);
//}
