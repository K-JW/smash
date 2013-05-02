/*
 *
 *    Copyright (c) 2013
 *      maximilian attems <attems@fias.uni-frankfurt.de>
 *      Jussi Auvinen <auvinen@fias.uni-frankfurt.de>
 *
 *    GNU General Public License (GPLv3)
 *
 */
#include "include/particles.h"

#include <cstdio>
#include <list>

#include "include/FourVector.h"
#include "include/ParticleData.h"
#include "include/constants.h"
#include "include/box.h"
#include "include/outputroutines.h"

/* boost_COM - boost to center of momentum */
static void boost_COM(ParticleData *particle1, ParticleData *particle2,
  FourVector *velocity) {
  FourVector momentum1(particle1->momentum()), momentum2(particle2->momentum());
  FourVector position1(particle1->x()), position2(particle2->x());
  double cms_energy = momentum1.x0() + momentum2.x0();

  // CMS 4-velocity
  velocity->set_x0(1.0);
  velocity->set_x1((momentum1.x1() + momentum2.x1()) / cms_energy);
  velocity->set_x2((momentum1.x2() + momentum2.x2()) / cms_energy);
  velocity->set_x3((momentum1.x3() + momentum2.x3()) / cms_energy);

  // Boost the momenta into CMS frame
  momentum1 = momentum1.LorentzBoost(momentum1, *velocity);
  momentum2 = momentum2.LorentzBoost(momentum2, *velocity);

  // Boost the positions into CMS frame
  position1 = position1.LorentzBoost(position1, *velocity);
  position2 = position2.LorentzBoost(position2, *velocity);

  particle1->set_momentum(momentum1);
  particle1->set_position(position1);
  particle2->set_momentum(momentum2);
  particle2->set_position(position2);
}

/* boost_from_COM - boost back from center of momentum */
static void boost_from_COM(ParticleData *particle1, ParticleData *particle2,
  FourVector *velocity_orig) {
  FourVector momentum1(particle1->momentum()), momentum2(particle2->momentum());
  FourVector position1(particle1->x()), position2(particle2->x());
  FourVector velocity = *velocity_orig;

  /* To boost back set 1 + velocity */
  velocity *= -1;
  velocity.set_x0(1);

  /* Boost the momenta back to lab frame */
  momentum1 = momentum1.LorentzBoost(momentum1, velocity);
  momentum2 = momentum2.LorentzBoost(momentum2, velocity);

  /* Boost the positions back to lab frame */
  position1 = position1.LorentzBoost(position1, velocity);
  position2 = position2.LorentzBoost(position2, velocity);

  particle1->set_momentum(momentum1);
  particle1->set_position(position1);
  particle2->set_momentum(momentum2);
  particle2->set_position(position2);
}

/* particle_distance - measure distance between two particles */
static double particle_distance(ParticleData *particle_orig1,
  ParticleData *particle_orig2) {
  ParticleData particle1 = *particle_orig1, particle2 = *particle_orig2;
  FourVector velocity_com;
  double distance_squared;

  /* boost particles in center of momenta frame */
  boost_COM(&particle1, &particle2, &velocity_com);
  FourVector position_diff = particle1.x() - particle2.x();
  printd("Particle %d<->%d position diff: %g %g %g %g [fm]\n",
    particle1.id(), particle2.id(), position_diff.x0(), position_diff.x1(),
    position_diff.x2(), position_diff.x3());

  FourVector momentum_diff = particle1.momentum() - particle2.momentum();
  /* zero momentum leads to infite distance */
  if (momentum_diff.x1() == 0 || momentum_diff.x2() == 0
      || momentum_diff.x3() == 0)
    return  - position_diff.DotThree(position_diff);
  /* UrQMD distance criteria:
   * arXiv:nucl-th/9803035 (3.27): in center of momemtum frame
   * d^2_{coll} = (x1 - x2)^2 - ((x1 - x2) . (v1 - v2))^2 / (v1 - v2)^2
   */
  distance_squared = - position_diff.DotThree(position_diff)
    + position_diff.DotThree(momentum_diff)
      * position_diff.DotThree(momentum_diff)
      / momentum_diff.DotThree(momentum_diff);
  return distance_squared;
}

/* time_collision - measure collision time of two particles */
static double collision_time(ParticleData *particle1,
  ParticleData *particle2) {
  /* UrQMD distance criteria
   * arXiv:1203.4418 (5.15): t_{coll} = - (x1 - x2) . (v1 - v2) / (v1 - v2)^2
   */
  FourVector position_diff = particle1->x() - particle2->x();
  FourVector velocity_diff = particle1->momentum() / particle1->momentum().x0()
    - particle2->momentum() / particle2->momentum().x0();
  return - position_diff.DotThree(velocity_diff)
           / velocity_diff.DotThree(velocity_diff);
}

/* momenta_exchange - soft scattering */
static void momenta_exchange(ParticleData *particle1, ParticleData *particle2) {
  FourVector momentum_copy = particle1->momentum();
  particle1->set_momentum(particle2->momentum());
  particle2->set_momentum(momentum_copy);
}

/* check_collision_criteria - check if a collision happens between particles */
void check_collision_criteria(ParticleData *particle,
  std::list<int> *collision_list, box box, int id, int id_other) {
  double distance_squared, time_collision;

  /* distance criteria according to cross_section */
  distance_squared = particle_distance(&particle[id], &particle[id_other]);
  if (distance_squared >= box.cross_section() * fm2_mb * M_1_PI)
    return;

  /* check according timestep: positive and smaller */
  time_collision = collision_time(&particle[id], &particle[id_other]);
  if (time_collision < 0 || time_collision >= box.eps())
    return;

  /* check for minimal collision time */
  if (particle[id].collision_time() > 0
        && time_collision > particle[id].collision_time()) {
    printd("%g Not minimal particle %d <-> %d\n", particle[id].x().x0(), id,
        id_other);
    return;
  }

  /* just collided with this particle */
  if (particle[id].collision_time() == 0
      && id_other == particle[id].collision_id()) {
    printd("%g Skipping particle %d <-> %d\n", particle[id].x().x0(), id,
        id_other);
    return;
  }

  /* handle minimal collision time */
  if (unlikely(particle[id].collision_time() > 0)) {
    int not_id = particle[id].collision_id();
    printd("Not colliding particle %d <-> %d\n", id, not_id);
    /* unset collision partner to zero time and unexisting id */
    particle[not_id].set_collision(0, -1);
    /* remove any of those partners from the list */
    collision_list->remove(id);
    collision_list->remove(not_id);
    /* XXX: keep track of multiple possible collision partners */
  }

  /* setup collision partners */
  printd("distance particle %d <-> %d: %g \n", id, id_other, distance_squared);
  printd("t_coll particle %d <-> %d: %g \n", id, id_other, time_collision);
  particle[id].set_collision(time_collision, id_other);
  particle[id_other].set_collision(time_collision, id);
  /* add to collision list */
  collision_list->push_back(id);
}

/* colliding_particle - particle interaction */
void collide_particles(ParticleData *particle,
  std::list<int> *collision_list) {
  FourVector velocity_com;

  /* collide: 2 <-> 2 soft momenta exchange */
  for (std::list<int>::iterator id = collision_list->begin();
    id != collision_list->end(); ++id) {
    int id_other = particle[*id].collision_id();
    printd("particle colliding %d<->%d %g\n", *id, id_other,
      particle[*id].x().x0());
    write_oscar(particle[*id], particle[id_other], 1);

    /* exchange in center of momenta */
    boost_COM(&particle[*id], &particle[id_other], &velocity_com);
    momenta_exchange(&particle[*id], &particle[id_other]);
    boost_from_COM(&particle[*id], &particle[id_other],
      &velocity_com);
    write_oscar(particle[*id], particle[id_other], -1);

    /* unset collision time for both particles + keep id */
    particle[*id].set_collision_time(0);
    particle[id_other].set_collision_time(0);
  }
  /* empty the collision table */
  collision_list->clear();
}
