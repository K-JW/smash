/*
 *    Copyright (c) 2013
 *      SMASH Team
 * 
 *    GNU General Public License (GPLv3 or later)
 */
#ifndef SRC_INCLUDE_COLLISIONS_H_
#define SRC_INCLUDE_COLLISIONS_H_

#include <list>

#include "../include/CrossSections.h"
#include "../include/Particles.h"

class Modus;

/* populates collision list if collision applies */
void collision_criteria_geometry(Particles *particles,
  CrossSections *cross_sections,
  std::list<int> *collision_list, const float timestep, int id_a,
  int id_b, size_t *rejection_conflict);

/* does collisions according to collision table */
size_t collide_particles(Particles *particles, std::list<int> *collision_list,
                         size_t id_event, int *resonance_formations);

#endif  // SRC_INCLUDE_COLLISIONS_H_
