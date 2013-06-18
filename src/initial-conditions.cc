/*
 *
 *    Copyright (c) 2012-2013
 *      maximilian attems <attems@fias.uni-frankfurt.de>
 *      Jussi Auvinen <auvinen@fias.uni-frankfurt.de>
 *
 *    GNU General Public License (GPLv3)
 *
 */
#include "include/initial-conditions.h"

#include <stdio.h>
#include <stdint.h>
#include <gsl/gsl_sf_bessel.h>

#include "include/Box.h"
#include "include/constants.h"
#include "include/distributions.h"
#include "include/macros.h"
#include "include/Parameters.h"
#include "include/ParticleData.h"
#include "include/ParticleType.h"
#include "include/outputroutines.h"

/* initial_conditions - sets particle type */
void initial_particles(std::vector<ParticleType> *type) {
  /* XXX: use nosql table for particle type values */
  (*type).resize(3);
  (*type)[0].set("pi+", 0.13957, -1.0, 211, 1, 1);
  (*type)[1].set("pi-", 0.13957, -1.0, -211, 1, -1);
  (*type)[2].set("pi0", 0.134977, -1.0, 111, 1, 0);
}

/* initial_conditions - sets particle data for @particles */
void initial_conditions(std::map<int, ParticleData> *particles,
  std::vector<ParticleType> *type, std::map<int, int> *map_type,
  Parameters *parameters, Box *box, size_t *largest_id) {
  double phi, cos_theta, sin_theta, momentum_radial, number_density_total = 0;
  FourVector momentum_total(0, 0, 0, 0);
  size_t number_total = 0, number = 0;

  /* initialize random seed */
  srand48(parameters->seed());

  if ((*type).empty()) {
    fprintf(stderr, "E: No particle types\n");
    exit(EXIT_FAILURE);
  }

  /* loop over all the particle types */
  for (size_t i = 0; i < (*type).size(); i++) {
    /* Particles with width > 0 (resonances) do not exist in the beginning */
    if ((*type)[i].width() > 0.0)
      continue;

    printd("%s mass: %g [GeV]\n", (*type)[i].name().c_str(), (*type)[i].mass());
    /*
     * The particle number depends on distribution function
     * (assumes Bose-Einstein):
     * Volume m^2 T BesselK[2, m/T] / (2\pi^2)
     */
    double number_density = (*type)[i].mass() * (*type)[i].mass()
      * box->temperature()
      * gsl_sf_bessel_Knu(2, (*type)[i].mass() / box->temperature())
      * 0.5 * M_1_PI * M_1_PI / hbarc / hbarc / hbarc;
    /* cast while reflecting probability of extra particle */
    number = box->length() * box->length() * box->length() * number_density
      * parameters->testparticles();
    if (box->length() * box->length() * box->length() * number_density - number
      > drand48())
      number++;
    printf("IC number density %.6g [fm^-3]\n", number_density);
    printf("IC %lu number of %s\n", number, (*type)[i].name().c_str());
    number_density_total += number_density;

    /* Set random IC:
     * particles at random position in the box with thermal momentum
     */
    /* allocate the particles */
    for (size_t id = number_total; id < number_total + number; id++) {
      double x_pos, y_pos, z_pos, time_start;
      /* ID uniqueness check */
      if (unlikely(particles->count(id) > 0))
        continue;

      ParticleData new_particle;
      (*particles)[id] = new_particle;
      /* Whenever a particle is created, bump the largest ID */
      (*largest_id)++;

      /* set id and particle type */
      (*particles)[id].set_id(id);
      (*map_type)[id] = i;

      /* back to back pair creation with random momenta direction */
      if (unlikely(id == number + number_total - 1 && !(id % 2) && i == 2
                   && box->initial_condition() != 2)) {
        /* poor last guy just sits around */
        (*particles)[id].set_momentum((*type)[i].mass(), 0, 0, 0);
      } else if (!(id % 2) || box->initial_condition() == 2) {
        if (box->initial_condition() != 2) {
          /* thermal momentum according Maxwell-Boltzmann distribution */
          momentum_radial = sample_momenta(box, (*type)[i]);
        } else {
          /* IC == 2 initial thermal momentum is the average 3T */
          momentum_radial = 3.0 * box->temperature();
        }
        /* phi in the range from [0, 2 * pi) */
        phi = 2.0 * M_PI * drand48();
        /* cos(theta) in the range from [-1.0, 1.0) */
        cos_theta = -1.0 + 2.0 * drand48();
        sin_theta = sqrt(1.0 - cos_theta * cos_theta);
        printd("Particle %lu radial momenta %g phi %g cos_theta %g\n", id,
          momentum_radial, phi, cos_theta);
        (*particles)[id].set_momentum((*type)[i].mass(),
          momentum_radial * cos(phi) * sin_theta,
          momentum_radial * sin(phi) * sin_theta,
          momentum_radial * cos_theta);
      } else if (box->initial_condition() != 2) {
        (*particles)[id].set_momentum((*type)[i].mass(),
          - (*particles)[id - 1].momentum().x1(),
          - (*particles)[id - 1].momentum().x2(),
          - (*particles)[id - 1].momentum().x3());
      }
      momentum_total += (*particles)[id].momentum();

      /* ramdom position in the box */
      time_start = 1.0;
      x_pos = drand48() * box->length();
      y_pos = drand48() * box->length();
      z_pos = drand48() * box->length();
      (*particles)[id].set_position(time_start, x_pos, y_pos, z_pos);

      /* no collision yet hence zero time and unexisting id */
      (*particles)[id].set_collision(-1, 0, -1);

      /* IC: debug checks */
      printd_momenta((*particles)[id]);
      printd_position((*particles)[id]);
    }
    number_total += number;
  }
  printf("IC total number density %.6g [fm^-3]\n", number_density_total);
  printf("IC contains %lu particles\n", number_total);
  printf("IC type %d\n", box->initial_condition());
  /* loop over all particles */
  number = number_total;
  /* reducing cross section according to number of test particle */
  if (parameters->testparticles() > 1) {
    printf("IC test particle: %i\n", parameters->testparticles());
    parameters->set_cross_section(parameters->cross_section()
                                  / parameters->testparticles());
    printf("Elastic cross section: %g [mb]\n", parameters->cross_section());
  }

  /* Display on startup if pseudo grid is used */
  int const grid_number = round(box->length()
              / sqrt(parameters->cross_section() * fm2_mb * M_1_PI) * 0.5);
  if (grid_number > 4 && number > 10)
    printf("Simulation with pseudo grid: %d^3\n", grid_number);

  /* allows to check energy conservation */
  printf("IC total energy: %g [GeV]\n", momentum_total.x0());
  box->set_energy_initial(momentum_total.x0());
  box->set_number_density_inital(number_density_total);
  print_header();
}
