/*
 *    Copyright (c) 2014
 *      SMASH Team
 * 
 *    GNU General Public License (GPLv3 or later)
 */
#ifndef SRC_INCLUDE_COLLIDER_H_
#define SRC_INCLUDE_COLLIDER_H_

#include <stdint.h>
#include <cmath>
#include <list>
#include <string>

#include "../include/CrossSections.h"
#include "../include/ModusDefault.h"
#include "../include/Particles.h"
#include "../include/Parameters.h"


class ColliderModus : public ModusDefault {
 public:
  /* default constructor with probable values */
  ColliderModus(): projectile_(2212), target_(2212), sqrts_(1.0f) {}
    /* special class funtions */
    void assign_params(std::list<Parameters> *configuration);
    void print_startup();
    void initial_conditions(Particles *particles, const ExperimentParameters &parameters);

 private:
    /* Projectile particle PDG ID*/
    int projectile_;
    /* Target particle PDG ID*/
    int target_;
    /* Center-of-mass energy of the collision */
    float sqrts_;
};



#endif  // SRC_INCLUDE_COLLIDER_H_
