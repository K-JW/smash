/*
 *
 *    Copyright (c) 2012-2013
 *      Hannah Petersen <petersen@fias.uni-frankfurt.de>
 *
 *    GNU General Public License (GPLv3)
 *
 */

#include <map>
#include <list>

#include "include/Box.h"
#include "include/Experiment.h"
#include "include/CrossSections.h"
#include "include/Modus.h"
#include "include/Parameters.h"
#include "include/collisions.h"
#include "include/decays.h"
#include "include/input-particles.h"
#include "include/input-decaymodes.h"
#include "include/macros.h"
#include "include/outputroutines.h"
#include "include/param-reader.h"

/* #include "include/Sphere.h" */

/* Experiment carries everything that is needed for the evolution */
std::unique_ptr<Experiment> Experiment::create(char *modus_chooser) {
  typedef std::unique_ptr<Experiment> ExperimentPointer;
  if (strcmp(modus_chooser, "Box") == 0) {
    return ExperimentPointer(new ExperimentImplementation<BoxModus>);
// } else if (modus == 2) {
//    return ExperimentPointer(new ExperimentImplementation<SphereModus>);
  } else {
    throw std::string("Invalid Modus requested from Experiment::create.");
  }
}

/* This method reads the parameters in */
template <typename Modus>
void ExperimentImplementation<Modus>::configure(std::list<Parameters>
                                                configuration) {
    bc_.assign_params(&configuration);
    warn_wrong_params(&configuration);
    bc_.print_startup();
    /* reducing cross section according to number of test particle */
    if (bc_.testparticles > 1) {
      printf("IC test particle: %i\n", bc_.testparticles);
      bc_.cross_section = bc_.cross_section / bc_.testparticles;
      printf("Elastic cross section: %g mb\n", bc_.cross_section);
    }
}

/* This method reads the particle type and cross section information
 * and does the initialization of the system (fill the particles map)
 */
template <typename Modus>
void ExperimentImplementation<Modus>::initialize(char *path) {
    srand48(bc_.seed);
    input_particles(particles_, path);
    input_decaymodes(particles_, path);
    cross_sections_->add_elastic_parameter(bc_.cross_section);
    bc_.initial_conditions(particles_);
    bc_.energy_initial = bc_.energy_total(particles_);
    write_measurements_header(*particles_);
    print_header();
    write_particles(*particles_);
}

/* This is the loop over timesteps, carrying out collisions and decays
 * and propagating particles
 */
template <typename Modus>
void ExperimentImplementation<Modus>::run_time_evolution() {
    bc_.sanity_check(particles_);
    std::list<int> collision_list, decay_list;
    size_t interactions_total = 0, previous_interactions_total = 0,
    interactions_this_interval = 0;
    size_t rejection_conflict = 0;
    int resonances = 0, decays = 0;
    print_measurements(*particles_, interactions_total,
                       interactions_this_interval, bc_.energy_initial,
                       bc_.time_start);
    for (int step = 0; step < bc_.steps; step++) {
        /* Check resonances for decays */
        check_decays(particles_, &decay_list, bc_.eps);
        /* Do the decays */
        if (!decay_list.empty()) {
            decays += decay_list.size();
            interactions_total = decay_particles(particles_,
                                       &decay_list, interactions_total);
        }
        /* fill collision table by cells */
        bc_.check_collision_geometry(particles_, cross_sections_,
                                 &collision_list, &rejection_conflict);
        /* particle interactions */
        if (!collision_list.empty()) {
            printd_list(collision_list);
            interactions_total = collide_particles(particles_, &collision_list,
                                             interactions_total, &resonances);
        }
        bc_.propagate(particles_);
        /* physics output during the run */
        if (step > 0 && (step + 1) % bc_.output_interval == 0) {
            interactions_this_interval = interactions_total
            - previous_interactions_total;
            previous_interactions_total = interactions_total;
            print_measurements(*particles_, interactions_total,
                              interactions_this_interval, bc_.energy_initial,
                              bc_.time_start);
            printd("Resonances: %i Decays: %i\n", resonances, decays);
            printd("Ignored collisions %zu\n", rejection_conflict);
            /* save evolution data */
            write_measurements(*particles_, interactions_total,
                               interactions_this_interval, resonances, decays,
                               rejection_conflict);
            write_vtk(*particles_);
        }
    }
        /* Guard against evolution */
        if (likely(bc_.steps > 0)) {
            /* if there are no particles no interactions happened */
            if (likely(!particles_->empty())) {
             print_tail(bc_.time_start, interactions_total * 2
                        / particles_->time() / particles_->size());
            } else {
             print_tail(bc_.time_start, 0);
             printf("Total ignored collisions: %zu\n", rejection_conflict);
            }
        }
}

/* Tear down everything */
template <typename Modus>
void ExperimentImplementation<Modus>::end() {
    delete particles_;
    delete cross_sections_;
}
