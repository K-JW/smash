/*
 *
 *    Copyright (c) 2013-2014
 *      SMASH Team
 * 
 *    GNU General Public License (GPLv3 or later)
 *
 */
#ifndef SRC_INCLUDE_MODUS_H_
#define SRC_INCLUDE_MODUS_H_

#include <stdint.h>
#include <time.h>
#include <cmath>
#include <list>

#include "../include/Parameters.h"
#include "../include/Particles.h"
#include "../include/time.h"

/* forward declarations */
class Particles;
class CrossSections;

/*
 * This is only a base class for actual Modus classes. The class will never be
 * used polymorphically, therefore it never needs virtual functions.
 *
 * Consider that there never are and never will be objects of type ModusDefault.
 *
 * This class is empty per default. You can add a function if you have a
 * function that is different in at least two subclasses and equal in at least
 * two subclasses.  Code that common to all goes into ExperimentImplementation.
 */
class ModusDefault {
 public:
    /* default constructor with probable values */
    ModusDefault(): steps(10000), output_interval(100), testparticles(1),
        eps(0.001f), cross_section(10.0f), seed(1), energy_initial(0.0f),
    time_start(set_timer_start()) {}

    // never needs a virtual destructor

    /* special function should be called by specific subclass */
    void assign_params(std::list<Parameters> *configuration);
    void print_startup();
    void initial_conditions(Particles *) { return; }
    float energy_total(Particles *particles);
    int sanity_check(Particles *particles);
    void check_collision_geometry(Particles *particles,
      CrossSections *cross_sections, std::list<int> *collision_list,
      size_t *rejection_conflict);
    void propagate(Particles *particles);
    FourVector boundary_condition(FourVector position,
                                          bool *boundary_hit);
    inline timespec set_timer_start();

 public:
    /* number of steps */
    int steps;
    /* number of steps before giving measurables */
    int output_interval;
    /* number of test particle */
    int testparticles;
    /* temporal time step */
    float eps;
    /* cross section of the elastic scattering */
    float cross_section;
    /* initial seed for random generator */
    int64_t seed;
    /* initial total energy of the system */
    float energy_initial;
    /* starting time of the simulation */
    timespec time_start;
};


/* set the timer to the actual time in nanoseconds precision */
timespec inline ModusDefault::set_timer_start(void) {
    timespec time;
    clock_gettime(&time);
    return time;
}

#endif  // SRC_INCLUDE_MODUS_H_
