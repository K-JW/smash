/*
 *
 *    Copyright (c) 2013
 *      maximilian attems <attems@fias.uni-frankfurt.de>
 *      Jussi Auvinen <auvinen@fias.uni-frankfurt.de>
 *
 *    GNU General Public License (GPLv3)
 *
 */

#include <getopt.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <list>
#include <map>
#include <utility>
#include <vector>

#include "include/Box.h"
#include "include/CrossSections.h"
#include "include/FourVector.h"
#include "include/Particles.h"
#include "include/ParticleData.h"
#include "include/collisions.h"
#include "include/constants.h"
#include "include/decays.h"
#include "include/input-decaymodes.h"
#include "include/input-particles.h"
#include "include/initial-conditions.h"
#include "include/macros.h"
#include "include/param-reader.h"
#include "include/Laboratory.h"
#include "include/outputroutines.h"
#include "include/propagation.h"
#include "include/Sphere.h"

/* build dependent variables */
#include "include/Config.h"


void SphereModus::assign_params_specific(std::list<Parameters> *configuration) {
    bool match = false;
    std::list<Parameters>::iterator i = configuration->begin();
    while (i != configuration->end()) {
        char *key = i->key();
        char *value = i->value();
        printd("%s %s\n", key, value);
        /* double or float values */
        if (strcmp(key, "RADIUS") == 0) {
            radius_ = (fabs(atof(value)));
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


/* print_startup - console output on startup of sphere specific parameters */
/* void print_startup(const SphereModus &ball) {
   /* printf("Volume of the sphere: 4 * pi * %g^2 [fm]\n", ball.radius);
   /* }


/* initial_conditions - sets particle data for @particles */
void SphereModus::initial_conditions(Particles *particles) {
    size_t number_total = 0;
    double time_start = 1.0;
    FourVector momentum_total(0, 0, 0, 0);
    /* loop over all the particle types creating each particles */
    for (auto i = particles->types_cbegin(); i != particles->types_cend();
         ++i) {
      /* Particles with width > 0 (resonances) do not exist in the beginning */
        if (i->second.width() > 0.0)
            continue;
        printd("%s mass: %g [GeV]\n", i->second.name().c_str(),
                                      i->second.mass());
        /* bose einstein distribution function with temperature 0.3 GeV */
        double number_density = number_density_bose(i->second.mass(), 0.3);
        printf("IC number density %.6g [fm^-3]\n", number_density);
        /* cast while reflecting probability of extra particle */
        size_t number = 4.0 / 3.0 * M_PI * radius_ * radius_
        * radius_ * number_density * testparticles;
        if (4.0 / 3.0 * M_PI * radius_ * radius_ * radius_
            * number_density - number > drand48())
            number++;
        /* create bunch of particles */
        printf("IC creating %zu particles\n", number);
        particles->create(number, i->second.pdgcode());
        number_total += number;
    }
    printf("IC contains %zu particles\n", number_total);
    /* now set position and momentum of the particles */
    double momentum_radial;
    Angles phitheta = Angles();
    for (auto i = particles->begin(); i != particles->end(); ++i) {
        if (unlikely(i->first == particles->id_max() && !(i->first % 2))) {
            /* poor last guy just sits around */
            i->second.set_momentum(particles->type(i->first).mass(), 0, 0, 0);
        } else if (!(i->first % 2)) {
            /* thermal momentum according Maxwell-Boltzmann distribution */
            momentum_radial = sample_momenta(0.3,
                              particles->type(i->first).mass());
            phitheta = Angles().distribute_isotropically();
            printd("Particle %d radial momenta %g phi %g cos_theta %g\n",
                   i->first, momentum_radial, phitheta.phi(),
                   phitheta.costheta());
            i->second.set_momentum(particles->type(i->first).mass(),
                                   momentum_radial * phitheta.x(),
                                   momentum_radial * phitheta.y(),
                                   momentum_radial * phitheta.z());
        } else {
            i->second.set_momentum(particles->type(i->first).mass(),
                               - particles->data(i->first - 1).momentum().x1(),
                               - particles->data(i->first - 1).momentum().x2(),
                               - particles->data(i->first - 1).momentum().x3());
        }
        momentum_total += i->second.momentum();
        double x, y, z;
        /* ramdom position in a sphere
         * box length here has the meaning of the sphere radius
         */
        x = -radius_ + 2.0 * drand48() * radius_;
        y = -radius_ + 2.0 * drand48() * radius_;
        z = -radius_ + 2.0 * drand48() * radius_;
        /* sampling points inside of the sphere, rejected if outside */
        while (sqrt(x * x + y * y + z * z) > radius_) {
            x = -radius_ + 2.0 * drand48() * radius_;
            y = -radius_ + 2.0 * drand48() * radius_;
            z = -radius_ + 2.0 * drand48() * radius_;
        }
        i->second.set_position(time_start, x, y, z);
        /* IC: debug checks */
        printd_momenta(i->second);
        printd_position(i->second);
    }
    printf("IC total energy: %g [GeV]\n", momentum_total.x0());
}

/* boundary_condition - enforce specific type of boundaries */
// FourVector boundary_condition(FourVector position,
//  const SphereModus &sphere __attribute__((unused)), bool *boundary_hit) {
  /* no boundary */
//  *boundary_hit = false;
//  return position;
// }

/* check_collision_geometry - check if a collision happens between particles */
void SphereModus::check_collision_geometry(Particles *particles,
  CrossSections *cross_sections,
  std::list<int> *collision_list, Modus const &parameters,
  BoxModus const &box, size_t *rejection_conflict) {
  std::vector<std::vector<std::vector<std::vector<int> > > > grid;
  int N, x, y, z;
  /* the maximal radial propagation for light particle */
  int a = box.length + particles->time();
  /* For small boxes no point in splitting up in grids */
  /* calculate approximate grid size according to double interaction length */
  N = round(2.0 * a / sqrt(parameters.cross_section() * fm2_mb * M_1_PI) * 0.5);
  /* for small boxes not possible to split upo */
  if (unlikely(N < 4 || particles->size() < 10)) {
    FourVector distance;
    double radial_interaction = sqrt(parameters.cross_section() * fm2_mb
                                     * M_1_PI) * 2;
    for (auto i = particles->begin(); i != particles->end(); ++i) {
      for (auto j = particles->begin(); j != particles->end(); ++j) {
        /* exclude check on same particle and double counting */
        if (i->first >= j->first)
          continue;
        distance = i->second.position() - j->second.position();
        /* skip particles that are double interaction radius length away */
        if (distance > radial_interaction)
           continue;
        collision_criteria_geometry(particles, cross_sections, collision_list,
          parameters, i->first, j->first, rejection_conflict);
      }
    }
    return;
  }
  /* allocate grid */
  grid.resize(N);
  for (int i = 0; i < N; i++) {
    grid[i].resize(N);
    for (int j = 0; j < N; j++)
      grid[i][j].resize(N);
  }
  /* populate grid */
  for (auto i = particles->begin(); i != particles->end(); ++i) {
    /* XXX: function - map particle position to grid number */
    x = round((a + i->second.position().x1()) / (N - 1));
    y = round((a + i->second.position().x2()) / (N - 1));
    z = round((a + i->second.position().x3()) / (N - 1));
    printd_position(i->second);
    printd("grid cell particle %i: %i %i %i of %i\n", i->first, x, y, z, N);
    grid[x][y][z].push_back(i->first);
  }
  /* semi optimised nearest neighbour search:
   * http://en.wikipedia.org/wiki/Cell_lists
   */
  FourVector shift;
  for (auto i = particles->begin(); i != particles->end(); ++i) {
    /* XXX: function - map particle position to grid number */
    x = round((a + i->second.position().x1()) / (N - 1));
    y = round((a + i->second.position().x2()) / (N - 1));
    z = round((a + i->second.position().x3()) / (N - 1));
    if (unlikely(x >= N || y >= N || z >= N))
      printf("grid cell particle %i: %i %i %i of %i\n", i->first, x, y, z, N);
    /* check all neighbour grids */
    for (int cx = -1; cx < 2; cx++) {
      int sx = cx + x;
      if (sx < 0 || sx >= N)
        continue;
      for (int cy = -1; cy <  2; cy++) {
        int sy = cy + y;
        if (sy < 0 || sy >= N)
          continue;
        for (int cz = -1; cz < 2; cz++) {
          int sz = cz + z;
          if (sz < 0 || sz >= N)
            continue;
          /* empty grid cell */
          if (grid[sx][sy][sz].empty())
            continue;
          /* grid cell particle list */
          for (auto id_b = grid[sx][sy][sz].begin();
               id_b != grid[sx][sy][sz].end(); ++id_b) {
            /* only check against particles above current id
             * to avoid double counting
             */
            if (*id_b <= i->first)
              continue;
            printd("grid cell particle %i <-> %i\n", i->first, *id_b);
            collision_criteria_geometry(particles, cross_sections,
              collision_list, parameters, i->first, *id_b, rejection_conflict);
          } /* grid particles loop */
        } /* grid sy */
      } /* grid sx */
    } /* grid sz */
  } /* outer particle loop */
}


/* Evolve - the core of the box, stepping forward in time */
int SphereModus::Evolve(Particles *particles, CrossSections *cross_sections,
                        int *resonances, int *decays) {
  std::list<int> collision_list, decay_list;
  size_t interactions_total = 0, previous_interactions_total = 0,
    interactions_this_interval = 0;
  size_t rejection_conflict = 0;
  /* startup values */
  print_measurements(*particles, interactions_total,
                     interactions_this_interval, ball);
  for (int steps = 0; steps < parameters.steps(); steps++) {
    /* Check resonances for decays */
    check_decays(particles, &decay_list, parameters);
    /* Do the decays */
    if (!decay_list.empty()) {
      (*decays) += decay_list.size();
      interactions_total = decay_particles(particles, &decay_list,
        interactions_total);
    }
    /* fill collision table by cells */
    check_collision_geometry(particles, cross_sections, &collision_list,
      parameters, lab, &rejection_conflict);
    /* particle interactions */
    if (!collision_list.empty())
      interactions_total = collide_particles(particles, &collision_list,
        interactions_total, resonances);
    /* propagate all particles */
    propagate_particles(particles, parameters, lab);
    /* physics output during the run */
    if (steps > 0 && (steps + 1) % parameters.output_interval() == 0) {
      interactions_this_interval = interactions_total
        - previous_interactions_total;
      previous_interactions_total = interactions_total;
      print_measurements(*particles, interactions_total,
                         interactions_this_interval, lab);
      printd("Resonances: %i Decays: %i\n", *resonances, *decays);
      printd("Ignored collisions %zu\n", rejection_conflict);
      /* save evolution data */
      write_measurements(*particles, interactions_total,
        interactions_this_interval, *resonances, *decays, rejection_conflict);
      write_vtk(*particles);
    }
  }
  /* Guard against evolution */
  if (likely(parameters.steps > 0)) {
    /* if there are not particles no interactions happened */
    if (likely(!particles->empty()))
      print_tail(lab, interactions_total * 2
                 / particles->time() / particles->size());
    else
      print_tail(ball, 0);
    printf("Total ignored collisions: %zu\n", rejection_conflict);
  }
  return 0;
}

/* start up a sphere and run it */
// int Sphere::evolve(const Laboratory &lab, char *path) {
//  /* Read sphere config file parameters */
//  Sphere *ball = new Sphere(lab);
//  process_config_sphere(ball, path);
  /* Initialize box */
//  print_startup(*ball);
//  Particles *particles = new Particles;
//  input_particles(particles, path);
//  initial_conditions(particles, ball);
// input_decaymodes(particles, path);
//  CrossSections *cross_sections = new CrossSections;
//  cross_sections->add_elastic_parameter(lab.cross_section());
  /* Compute stuff */
//  int rc = Evolve(particles, cross_sections, lab);
  /* record IC startup */
//  write_measurements_header(*particles);
//  print_header();
//  write_particles(*particles);
  /* tear down */
//  delete particles;
// delete cross_sections;
// delete ball;
//  return rc;
// }

