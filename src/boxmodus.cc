/*
 *    Copyright (c) 2013-2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 */
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <list>
#include <map>
#include <utility>
#include <vector>

#include "include/algorithms.h"
#include "include/angles.h"
#include "include/boxmodus.h"
#include "include/configuration.h"
#include "include/constants.h"
#include "include/crosssections.h"
#include "include/distributions.h"
#include "include/experimentparameters.h"
#include "include/macros.h"
#include "include/outputroutines.h"
#include "include/outputinterface.h"
#include "include/particles.h"
#include "include/random.h"
#include "include/threevector.h"

namespace Smash {

BoxModus::BoxModus(Configuration modus_config, const ExperimentParameters &)
    : initial_condition_(modus_config.take({"Box", "INITIAL_CONDITION"})),
      length_           (modus_config.take({"Box", "LENGTH"})),
      temperature_      (modus_config.take({"Box", "TEMPERATURE"})),
      start_time_       (modus_config.take({"Box", "START_TIME"})) {
}

/* print_startup - console output on startup of box specific parameters */
void BoxModus::print_startup() {
  printf("Size of the box: %g x %g x %g fm\n", length_, length_, length_);
  printf("Initial temperature: %g GeV\n", temperature_);
  printf("IC type %d\n", initial_condition_);
}

/* initial_conditions - sets particle data for @particles */
float BoxModus::initial_conditions(Particles *particles,
                                  const ExperimentParameters &parameters) {
  double momentum_radial, number_density_total = 0;
  Angles phitheta;
  FourVector momentum_total(0, 0, 0, 0);
  size_t number_total = 0;
  /* loop over all the particle types */
  for (const ParticleType &type : particles->types()) {
    /* Particles with width > 0 (resonances) do not exist in the beginning */
    if (type.width() > 0.0) {
      continue;
    }
    printd("%s mass: %g [GeV]\n", type.name().c_str(), type.mass());
    /* Particle densities according to Maxwell-Boltzmann statistics */
    double number_density =
        number_density_maxwellboltzmann(type.mass(), this->temperature_);
    // calculate the expected of particles in the box
    double real_number = this->length_ * this->length_ * this->length_ *
                         number_density * parameters.testparticles;
    size_t int_number = static_cast<size_t>(real_number);
    // decide if we have an extra particle: the probability for that is
    // equal to the fractional part of the number.
    if (real_number - int_number > Random::canonical()) {
      int_number++;
    }
    printf("IC number density %.6g [fm^-3]\n", number_density);
    printf("IC %zu number of %s\n", int_number, type.name().c_str());
    number_density_total += number_density;
    /* create bunch of particles */
    printf("IC creating %zu particles\n", int_number);
    particles->create(int_number, type.pdgcode());
    number_total += int_number;
  }
  printf("IC total number density %.6g [fm^-3]\n", number_density_total);
  printf("IC contains %zu particles\n", number_total);
  auto uniform_length = Random::make_uniform_distribution(0.0,
                                         static_cast<double>(this->length_));
  /* Set paricles IC: */
  for (ParticleData &data : particles->data()) {
    /* back to back pair creation with random momenta direction */
    if (unlikely(data.id() == particles->id_max() && !(data.id() % 2))) {
      /* poor last guy just sits around */
      data.set_momentum(particles->particle_type(data.pdgcode()).mass(), 0, 0, 0);
    } else if (!(data.id() % 2)) {
      if (this->initial_condition_ != 2) {
        /* thermal momentum according Maxwell-Boltzmann distribution */
        momentum_radial = sample_momenta(this->temperature_,
                                         particles->particle_type(data.pdgcode()).mass());
      } else {
        /* IC == 2 initial thermal momentum is the average 3T */
        momentum_radial = 3.0 * this->temperature_;
      }
      phitheta.distribute_isotropically();
      printd("Particle %d radial momenta %g phi %g cos_theta %g\n", data.id(),
             momentum_radial, phitheta.phi(), phitheta.costheta());
      data.set_momentum(
          particles->particle_type(data.pdgcode()).mass(), phitheta.threevec() * momentum_radial);
    } else {
      data.set_momentum(particles->particle_type(data.pdgcode()).mass(),
                       -particles->data(data.id() - 1).momentum().threevec());
    }
    momentum_total += data.momentum();
    /* random position in a quadratic box */
    ThreeVector pos{uniform_length(), uniform_length(), uniform_length()};
    data.set_position(FourVector(start_time_, pos));
    /* IC: debug checks */
    printd_momenta(data);
    printd_position(data);
  }
  /* Display on startup if pseudo grid is used */
  size_t number = number_total;
  int const grid_number = round(
      this->length_ / sqrt(parameters.cross_section * fm2_mb * M_1_PI) * 0.5);
  /* pseudo grid not used for 3^3 or extremely small particle numbers */
  if (grid_number >= 4 && number > 10)
    printf("Simulation with pseudo grid: %d^3\n", grid_number);
  else
    printf("W: Not using pseudo grid: %d^3\n", grid_number);
  /* allows to check energy conservation */
  printf("IC total energy: %g [GeV]\n", momentum_total.x0());
  number_density_initial_ = number_density_total;
  return 0.f;
}

/* evolve - the core of the box, stepping forward in time */
int BoxModus::sanity_check(Particles *particles) {
  /* fixup positions on startup, particles need to be *inside* the box */
  for (ParticleData &data : particles->data()) {
    FourVector p = data.position();
    enforce_periodic_boundaries(p.begin() + 1, p.end(), length_);
    data.set_position(p);
  }
  return start_time_;
}


/* propagate all particles */
void BoxModus::propagate(Particles *particles,
                         const ExperimentParameters &parameters,
                         const OutputsList &output_list) {
  FourVector distance, position;
  for (ParticleData &data : particles->data()) {
    /* propagation for this time step */
    distance = FourVector(parameters.timestep_size(),
                          data.velocity() * parameters.timestep_size());
    printd("Particle %d motion: %g %g %g %g\n", data.id(), distance.x0(),
           distance.x1(), distance.x2(), distance.x3());
    /* treat the box boundaries */
    position = data.position();
    position.set_x0(parameters.new_particle_time());
    position += distance;
    bool wall_hit = enforce_periodic_boundaries(position.begin() + 1,
                                                position.end(), length_);
    const ParticleList incoming_particle{1, data};
    data.set_position(position);
    if (wall_hit) {
      for (const auto &output : output_list) {
        output->write_interaction(incoming_particle, {1, data});
      }
    }
    printd_position(data);
  }
}

}  // namespace Smash
