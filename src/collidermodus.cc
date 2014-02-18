/*
 *    Copyright (c) 2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 */
#include <cmath>
#include <cstdlib>
#include <list>

#include "include/collidermodus.h"
#include "include/experimentparameters.h"
#include "include/outputroutines.h"
#include "include/parameters.h"

void ColliderModus::assign_params(std::list<Parameters> *configuration) {
  bool match = false;
  std::list<Parameters>::iterator i = configuration->begin();
  while (i != configuration->end()) {
    char *key = i->key();
    char *value = i->value();
    printd("%s %s\n", key, value);
    /* integer values */
    if (strcmp(key, "PROJECTILE") == 0) {
      projectile_ = atoi(value);
      match = true;
    }
    if (strcmp(key, "TARGET") == 0) {
      target_ = atoi(value);
      match = true;
    }
    /* float values */
    if (strcmp(key, "SQRTS") == 0) {
      sqrts_ = (fabs(atof(value)));
      match = true;
    }
    /* remove processed entry */
    if (match) {
      i = configuration->erase(i);
      match = false;
    } else {
      ++i;
    }
  }
}

/* print_startup - console output on startup of box specific parameters */
void ColliderModus::print_startup() {
  printf("Projectile PDG ID: %d \n", projectile_);
  printf("Target PDG ID: %d \n", target_);
  printf("Center-of-mass energy %10.3f GeV\n", sqrts_);
}

/* initial_conditions - sets particle data for @particles */
void ColliderModus::initial_conditions(Particles *particles,
                                       const ExperimentParameters &) {
  /* Create "projectile" particle */
  particles->create(1, projectile_);
  /* Pointer to "projectile" data */
  ParticleData *data_projectile = particles->data_pointer(particles->id_max());
  float mass_projectile
    = particles->particle_type(data_projectile->pdgcode()).mass();
  printf("projectile pdgcode %d mass %f\n", data_projectile->pdgcode(),
         mass_projectile);
  /* Create "target" particle */
  particles->create(1, target_);
  /* Pointer to "target" data */
  ParticleData *data_target = particles->data_pointer(particles->id_max());
  float mass_target
    = particles->particle_type(data_target->pdgcode()).mass();
  printf("target pdgcode %d mass %f\n", data_target->pdgcode(),
         mass_target);
  /* Projectile energy in CMS */
  double cms_energy_projectile = (sqrts_ * sqrts_
                                  + mass_projectile * mass_projectile
                                  - mass_target * mass_target)
                                 / (2 * sqrts_);
  /* CMS momentum, same in magnitude for both */
  double cms_momentum = sqrt(cms_energy_projectile * cms_energy_projectile
                             - mass_projectile * mass_projectile);
  /* Sample impact parameter */
  double impact_parameter = drand48() * 5.0;
  /* Set positions and momenta */
  data_projectile->set_position(1.0, impact_parameter, 0.0, -1.0);
  data_projectile->set_momentum(mass_projectile, 0.0, 0.0, cms_momentum);
  data_target->set_position(1.0, 0.0, 0.0, 1.0);
  data_target->set_momentum(mass_target, 0.0, 0.0, -cms_momentum);
}
