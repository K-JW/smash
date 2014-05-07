/*
 *    Copyright (c) 2012-2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 */
#ifndef SRC_INCLUDE_OUTPUTROUTINES_H_
#define SRC_INCLUDE_OUTPUTROUTINES_H_

#include <chrono>
#include <list>

#include "include/particles.h"

namespace Smash {

/* forward declarations */
class ModusDefault;
class ParticleData;
class ParticleType;

/* console output */
void print_startup(const ModusDefault &parameters);
void print_header(void);
void print_measurements(const Particles &particles,
                        const size_t &scatterings_total,
                        const size_t &scatterings_this_interval,
                        float energy_ini,
                 std::chrono::time_point<std::chrono::system_clock> time_start);
void print_tail(const
                std::chrono::time_point<std::chrono::system_clock> time_start,
                const double &scattering_rate);

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
void write_oscar_header(void);
void write_oscar_event_block(Particles *particles,
                             size_t initial, size_t final, int event_id);
/**
 * Write a line (plus prefix line) to OSCAR output.
 *
 * The first particle in a process needs to specify \p initial and \p final.
 *
 * If \p initial and \p final are 0 (which is the default) then no prefix is
 * written. This is used for the other particles in the same process.
 *
 */
void write_oscar(const ParticleData &particle_data,
                 const ParticleType &particle_type, int initial = 0, int final = 0);

}  // namespace Smash

#endif  // SRC_INCLUDE_OUTPUTROUTINES_H_
