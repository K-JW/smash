/*
 *    Copyright (c) 2012-2013
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 */
#ifndef SRC_INCLUDE_OUTPUTROUTINES_H_
#define SRC_INCLUDE_OUTPUTROUTINES_H_

#include <list>

#include "include/particles.h"

/* forward declarations */
class ModusDefault;
class Parameters;
class ParticleData;
class ParticleType;

/* console output */
void print_startup(const ModusDefault &parameters);
void print_header(void);
void print_measurements(const Particles &particles,
                        const size_t &scatterings_total,
                        const size_t &scatterings_this_interval,
                        float energy_ini, timespec time_start);
void print_tail(const timespec time_start, const double &scattering_rate);
void warn_wrong_params(std::list<Parameters> *configuration);
/* data directory */
void mkdir_data(void);

/* Compile time debug info */
#ifdef DEBUG
# define printd printf
#else
# define printd(...) ((void)0)
#endif

/* console debug output */
void printd_position(const ParticleData &particle);
void printd_position(const char *message, const ParticleData &particle);
void printd_momenta(const ParticleData &particle);
void printd_momenta(const char *message, const ParticleData &particle);
void printd_list(const std::list<int> &collision_list);

/* output data files */
void write_particles(const Particles &particles);
void write_measurements_header(const Particles &particles);
void write_measurements(const Particles &particles,
  int interactions_total, int interactions_this_interval, int decays,
  int resonances, const size_t &rejection_conflict);
void write_oscar_header(void);
void write_oscar(const ParticleData &particle_data,
                 const ParticleType &particle_type, int initial, int final);
void write_oscar(const ParticleData &particle_data,
                 const ParticleType &particle_type);
void write_vtk(const Particles &particles);

/* timing measure */
double measure_timediff(const timespec time_start);

#endif  // SRC_INCLUDE_OUTPUTROUTINES_H_
