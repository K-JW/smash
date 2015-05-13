/*
 *
 *    Copyright (c) 2015
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#ifndef SRC_TESTS_SETUP_H_
#define SRC_TESTS_SETUP_H_

#include "../include/cxx14compat.h"
#include "../include/experiment.h"
#include "../include/particles.h"
#include "../include/particletype.h"
#include "../include/particledata.h"
#include "../include/random.h"

#include <boost/filesystem.hpp>

namespace Smash {
namespace Test {

/**
 * \addtogroup unittest
 * @{
 */

/**
 * Creates the ParticleType list containing the actual particles that SMASH
 * uses.
 */
inline void create_actual_particletypes() {
#include <particles.txt.h>
  ParticleType::create_type_list(data);
}

/// The mass of the smashon particle.
static constexpr float smashon_mass = 0.123f;
/// The decay width of the smashon particle.
static constexpr float smashon_width = 1.2f;
static constexpr const char smashon_pdg_string[] = "661";

/**
 * Creates a ParticleType list containing only the smashon test particle.
 */
inline void create_smashon_particletypes() {
  ParticleType::create_type_list(
      "# NAME MASS[GEV] WIDTH[GEV] PDG\n"
      "smashon " +
      std::to_string(smashon_mass) + ' ' + std::to_string(smashon_width) +
      " 661\n");
}

/// A FourVector that is marked as a position vector.
struct Position : public FourVector {
  using FourVector::FourVector;
};
/// A FourVector that is marked as a momentum vector.
struct Momentum : public FourVector {
  using FourVector::FourVector;
};

/**
 * Create a particle with 0 position and momentum vectors and optionally a given
 * \p id.
 */
inline ParticleData smashon(int id = -1) {
  ParticleData p{ParticleType::find(0x661), id};
  return p;
}
/**
 * Create a particle with 0 momentum vector, the given \p position, and
 * optionally a given \p id.
 */
inline ParticleData smashon(const Position &position, int id = -1) {
  ParticleData p{ParticleType::find(0x661), id};
  p.set_4position(position);
  return p;
}
/**
 * Create a particle with 0 position vector, the given \p momentum, and
 * optionally a given \p id.
 */
inline ParticleData smashon(const Momentum &momentum, int id = -1) {
  ParticleData p{ParticleType::find(0x661), id};
  p.set_4momentum(momentum);
  return p;
}
/**
 * Create a particle with the given \p position and \p momentum vectors, and
 * optionally a given \p id.
 */
inline ParticleData smashon(const Position &position, const Momentum &momentum,
                            int id = -1) {
  ParticleData p{ParticleType::find(0x661), id};
  p.set_4position(position);
  p.set_4momentum(momentum);
  return p;
}
/**
 * Create a particle with the given \p position and \p momentum vectors, and
 * optionally a given \p id.
 *
 * Convenience overload of the above to allow arbitrary order of momentum and
 * position.
 */
inline ParticleData smashon(const Momentum &momentum, const Position &position,
                            int id = -1) {
  ParticleData p{ParticleType::find(0x661), id};
  p.set_4position(position);
  p.set_4momentum(momentum);
  return p;
}
/**
 * Create a particle with random position and momentum vectors and optionally a
 * given \p id.
 */
inline ParticleData smashon_random(int id = -1) {
  auto random_value = Random::make_uniform_distribution(-15.0, +15.0);
  ParticleData p{ParticleType::find(0x661), id};
  p.set_4position(
      {random_value(), random_value(), random_value(), random_value()});
  p.set_4momentum(smashon_mass,
                  {random_value(), random_value(), random_value()});
  return p;
}

/**
 * Return a configuration object filled with data from src/config.yaml. Note
 * that a change to that file may affect test results if you use it.
 *
 * If you want specific values in the config for testing simply overwrite the
 * relevant settings e.g. with:
 * \code
 * auto config = Test::configuration(
 *   "General:\n"
 *   "  Modus: Box\n"
 *   "  Testparticles: 100\n"
 * );
 * \endcode
 */
inline Configuration configuration(std::string overrides = {}) {
  Configuration c{TEST_CONFIG_PATH};
  if (!overrides.empty()) {
    c.merge_yaml(overrides);
  }
  return c;
}

/**
 * Create an experiment.
 *
 * If you want a specific configuration you can pass it as parameter, otherwise
 * it will use the result from \ref configuration above.
 */
inline std::unique_ptr<ExperimentBase> experiment(
    const Configuration &c = configuration()) {
  return ExperimentBase::create(c);
}

/**
 * Creates an experiment using the default config and the specified \p
 * configOverrides.
 */
inline std::unique_ptr<ExperimentBase> experiment(const char *configOverrides) {
  return ExperimentBase::create(configuration(configOverrides));
}

/**
 * Generate a list of particles from the given generator function.
 */
template <typename G>
inline ParticleList create_particle_list(std::size_t n, G &&generator) {
  ParticleList list;
  list.reserve(n);
  for (auto i = n; i; --i) {
    list.emplace_back(generator());
  }
  return list;
}

/// A type alias for a unique_ptr of Particles.
using ParticlesPtr = std::unique_ptr<Particles>;

/**
 * Creates a Particles object and fills it with \p n particles generated by the
 * \p generator function.
 */
template <typename G>
inline ParticlesPtr create_particles(int n, G &&generator) {
  ParticlesPtr p = make_unique<Particles>();
  for (auto i = n; i; --i) {
    p->insert(generator());
  }
  return p;
}

/**
 * Creates a Particles object and fills it with the particles passed as
 * initializer_list to this function.
 */
inline ParticlesPtr create_particles(
    const std::initializer_list<ParticleData> &init) {
  ParticlesPtr p = make_unique<Particles>();
  for (const auto &data : init) {
    p->insert(data);
  }
  return p;
}

/**
 * Creates a standard ExperimentParameters object which works for almost all
 * testing purposes.
 *
 * If needed you can set the testparticles parameter to a different value than
 * 1.
 */
inline ExperimentParameters default_parameters(int testparticles = 1) {
  return ExperimentParameters{{0.f, 1.f}, 1.f, testparticles, 1.0};
}

/**
 * @}
 */
}  // namespace Test
}  // namespace Smash

#endif  // SRC_TESTS_SETUP_H_
