/*
 *
 *    Copyright (c) 2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "unittest.h"
#include "../include/boxmodus.h"
#include "../include/configuration.h"
#include "../include/collidermodus.h"
#include "../include/experiment.h"
#include "../include/modusdefault.h"
#include "../include/potentials.h"
#include "../include/spheremodus.h"

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
  particle_stop.set_4momentum(FourVector(4.0, 0.0, 0.0, 0.0));
  particle_fast.set_4momentum(FourVector(sqrt(0.02), 0.1, -.1, 0.0));
  particle_slow.set_4momentum(FourVector(sqrt(1.13), 0.1, 0.2, -.3));
  particle_x_hi.set_4momentum(FourVector(0.1, 0.1, 0.0, 0.0));
  particle_y_lo.set_4momentum(FourVector(0.1, 0.0, -.1, 0.0));
  particle_xlzh.set_4momentum(FourVector(0.5, -.3, 0.0, 0.4));

  // set positions:
  particle_stop.set_4position(FourVector(0.0, 0.6, 0.7, 0.8));
  particle_fast.set_4position(FourVector(0.5, 0.7, 0.8, 0.9));
  particle_slow.set_4position(FourVector(0.7, 0.1, 0.2, 0.3));
  particle_x_hi.set_4position(FourVector(1.2, 4.5, 0.0, 0.0));
  particle_y_lo.set_4position(FourVector(1.8, 0.0, 0.2, 0.0));
  particle_xlzh.set_4position(FourVector(2.2, 0.2, 0.0, 4.8));

  // add particles (and make sure the particles get the correct ID):
  COMPARE(P.add_data(particle_stop), 0);
  COMPARE(P.add_data(particle_fast), 1);
  COMPARE(P.add_data(particle_slow), 2);
  COMPARE(P.add_data(particle_x_hi), 3);
  COMPARE(P.add_data(particle_y_lo), 4);
  COMPARE(P.add_data(particle_xlzh), 5);

  return;
}

TEST(propagate_default) {
  ModusDefault m;
  Particles Pdef;
  create_particle_list(Pdef);
  OutputsList out;
  // clock, output interval, cross-section, testparticles
  ExperimentParameters param{{0.f, 1.f}, 1.f, 1, 1.0};
  Potentials* pot = NULL;
  m.propagate(&Pdef, param, out, pot);
  // after propagation: Momenta should be unchanged.
  COMPARE(Pdef.data(0).momentum(), FourVector(4.0, 0.0, 0.0, 0.0));
  COMPARE(Pdef.data(1).momentum(), FourVector(sqrt(0.02), 0.1, -.1, 0.0));
  COMPARE(Pdef.data(2).momentum(), FourVector(sqrt(1.13), 0.1, 0.2, -.3));
  COMPARE(Pdef.data(3).momentum(), FourVector(0.1, 0.1, 0.0, 0.0));
  COMPARE(Pdef.data(4).momentum(), FourVector(0.1, 0.0, -.1, 0.0));
  COMPARE(Pdef.data(5).momentum(), FourVector(0.5, -.3, 0.0, 0.4));
  // positions should be updated:
  COMPARE(Pdef.data(0).position(), FourVector(0.0, 0.6, 0.7, 0.8));
  COMPARE(Pdef.data(1).position(), FourVector(0.0, 0.7 + std::sqrt(0.5),
                                                   0.8 - std::sqrt(0.5), 0.9));
  COMPARE(Pdef.data(2).position(), FourVector(0., 0.1 + 0.1 / std::sqrt(1.13),
                                                  0.2 + 0.2 / std::sqrt(1.13),
                                                  0.3 - 0.3 / std::sqrt(1.13)));
  COMPARE(Pdef.data(3).position(), FourVector(0.0, 4.5 + 1.0, 0.0, 0.0));
  COMPARE(Pdef.data(4).position(), FourVector(0.0, 0.0, 0.2 - 1.0, 0.0));
  COMPARE(Pdef.data(5).position(), FourVector(0.0, 0.2 - 0.6, 0.0, 4.8 + 0.8));
}

TEST(propagate_box) {
  ModusDefault m;
  Configuration conf(TEST_CONFIG_PATH);
  conf["Modi"]["Box"]["Initial_Condition"] = 1;
  conf["Modi"]["Box"]["Length"] = 5.0;
  conf["Modi"]["Box"]["Temperature"] = 0.13;
  conf["Modi"]["Box"]["Start_Time"] = 0.2;
  ExperimentParameters param{{0.f, 1.f}, 1.f, 1, 1.0};
  BoxModus b(conf["Modi"], param);
  Particles Pdef, Pbox;
  create_particle_list(Pdef);
  create_particle_list(Pbox);
  OutputsList out;
  // clock, output interval, cross-section, testparticles
  Potentials* pot = NULL;
  m.propagate(&Pdef, param, out, pot);
  b.propagate(&Pbox, param, out, pot);
  // Box and Default modus should do the same to momentum:
  COMPARE(Pdef.data(0).momentum(), Pbox.data(0).momentum());
  COMPARE(Pdef.data(1).momentum(), Pbox.data(1).momentum());
  COMPARE(Pdef.data(2).momentum(), Pbox.data(2).momentum());
  COMPARE(Pdef.data(3).momentum(), Pbox.data(3).momentum());
  COMPARE(Pdef.data(4).momentum(), Pbox.data(4).momentum());
  COMPARE(Pdef.data(5).momentum(), Pbox.data(5).momentum());
  // stop, fast and slow are propagated equally:
  COMPARE(Pdef.data(0).position(), Pbox.data(0).position());
  COMPARE(Pdef.data(1).position(), Pbox.data(1).position());
  COMPARE(Pdef.data(2).position(), Pbox.data(2).position());
  // those particles are expected to be wraped around in Box Modus
  COMPARE(Pdef.data(3).position(), Pbox.data(3).position()
                                 + FourVector(0.0, 5.0, 0.0, 0.0));
  COMPARE(Pdef.data(4).position(), Pbox.data(4).position()
                                 + FourVector(0.0, 0.0, -5., 0.0));
  COMPARE(Pdef.data(5).position(), Pbox.data(5).position()
                                 + FourVector(0.0, -5., 0.0, 5.0));
}

TEST(propagate_collider) {
  ModusDefault m;
  Configuration conf(TEST_CONFIG_PATH);
  conf["Modi"]["Collider"]["Sqrtsnn"] = 1.0;
  conf.take({"Modi", "Collider", "Projectile"});
  conf.take({"Modi", "Collider", "Target"});
  conf["Modi"]["Collider"]["Projectile"]["Particles"]["661"] = 1;
  conf["Modi"]["Collider"]["Target"]["Particles"]["661"] = 1;
  conf["Modi"]["Collider"]["Sqrts_Reps"][0] = "661";
  conf["Modi"]["Collider"]["Sqrts_Reps"][1] = "661";
  ExperimentParameters param{{0.f, 1.f}, 1.f, 1, 1.0};
  ColliderModus c(conf["Modi"], param);
  Particles Pdef, Pcol;
  create_particle_list(Pdef);
  create_particle_list(Pcol);
  OutputsList out;
  Potentials* pot = NULL;
  m.propagate(&Pdef, param, out, pot);
  c.propagate(&Pcol, param, out, pot);
  // Collider and Default modus should do the same everywhere:
  COMPARE(Pdef.data(0).momentum(), Pcol.data(0).momentum());
  COMPARE(Pdef.data(1).momentum(), Pcol.data(1).momentum());
  COMPARE(Pdef.data(2).momentum(), Pcol.data(2).momentum());
  COMPARE(Pdef.data(3).momentum(), Pcol.data(3).momentum());
  COMPARE(Pdef.data(4).momentum(), Pcol.data(4).momentum());
  COMPARE(Pdef.data(5).momentum(), Pcol.data(5).momentum());
  COMPARE(Pdef.data(0).position(), Pcol.data(0).position());
  COMPARE(Pdef.data(1).position(), Pcol.data(1).position());
  COMPARE(Pdef.data(2).position(), Pcol.data(2).position());
  COMPARE(Pdef.data(3).position(), Pcol.data(3).position());
  COMPARE(Pdef.data(4).position(), Pcol.data(4).position());
  COMPARE(Pdef.data(5).position(), Pcol.data(5).position());
}

TEST(propagate_sphere) {
   ModusDefault m;
   Configuration conf(TEST_CONFIG_PATH);
   conf["Modi"]["Sphere"]["Radius"] = 10;
   conf["Modi"]["Sphere"]["Sphere_Temperature"] = 0.2;
   conf["Modi"]["Sphere"]["Start_Time"] = 0.0;
   conf.take({"Modi", "Sphere", "Init_Multiplicities"});
   conf["Modi"]["Sphere"]["Init_Multiplicities"]["661"] = 500;
   ExperimentParameters param{{0.f, 1.f}, 1.f, 1, 1.0};
   SphereModus s(conf["Modi"], param);
   Particles Pdef, Psph;
   create_particle_list(Pdef);
   create_particle_list(Psph);
   OutputsList out;
   Potentials* pot = NULL;
   m.propagate(&Pdef, param, out, pot);
   s.propagate(&Psph, param, out, pot);
   // Sphere and Default modus should do the same everywhere:
   COMPARE(Pdef.data(0).momentum(), Psph.data(0).momentum());
   COMPARE(Pdef.data(1).momentum(), Psph.data(1).momentum());
   COMPARE(Pdef.data(2).momentum(), Psph.data(2).momentum());
   COMPARE(Pdef.data(3).momentum(), Psph.data(3).momentum());
   COMPARE(Pdef.data(4).momentum(), Psph.data(4).momentum());
   COMPARE(Pdef.data(5).momentum(), Psph.data(5).momentum());
   COMPARE(Pdef.data(0).position(), Psph.data(0).position());
   COMPARE(Pdef.data(1).position(), Psph.data(1).position());
   COMPARE(Pdef.data(2).position(), Psph.data(2).position());
   COMPARE(Pdef.data(3).position(), Psph.data(3).position());
   COMPARE(Pdef.data(4).position(), Psph.data(4).position());
   COMPARE(Pdef.data(5).position(), Psph.data(5).position());
}
