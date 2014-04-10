/*
 *
 *    Copyright (c) 2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#ifndef SRC_INCLUDE_RANDOM_H_
#define SRC_INCLUDE_RANDOM_H_

#include<random>

namespace Smash {

/** Namespace Random provides functions for Random Number Generation.
 */

namespace Random {

/// The random number engine used is std::randlux48
using Engine = std::ranlux48;
/// For all intents and purposes, there is a common RN engine "engine"
/// defined in \file src/random.cc
extern /*thread_local*/ Engine engine;

/** Provides uniform random numbers on a fixed interval.
 *
 * objects of uniform_dist can be used to provide a large number of
 * random numbers in the same interval. Example:
 *
 * \code
 *   using namespace Random;
 *   double sum = 0.0;
 *   auto uniform_0_to_3 = uniform_dist(0., 3.);
 *   for (MANY_TIMES) {
 *     sum += uniform_0_to_3();
 *   }
 * \endcode
 *
 * The random number engine is completely hidden inside the object.
 */
template <typename T> class uniform_dist {
 public:
  /** creates the object and fixes the interval */
  uniform_dist(T min, T max)
    : distribution(min, max) {
  }
  /** returns a random number in the interval */
  T operator ()() {
    return distribution(engine);
  }
  /** the distribution object that is being used. */
 private:
  std::uniform_real_distribution<T> distribution;
};

/** sets the seed of the random number engine. */
template <typename T> void set_seed(T &&seed) {
  engine.seed(std::forward<T>(seed));
}
/** returns a uniformly distributed random number \f$\chi \in [{\rm
 * min}, {\rm max})\f$ */
template <typename T> T uniform(T min, T max) {
  return std::uniform_real_distribution<T>(min, max)(engine);
}
/** returns a uniformly distributed random number \f$\chi \in [0,1)\f$
 */
template <typename T = double> T canonical() {
  static uniform_dist<T> canonical(0.0, 1.0);
  return canonical();
}
/** returns a uniform_dist object */
template <typename T>
uniform_dist<T> make_uniform_distribution(T min, T max) {
  return uniform_dist<T>(min, max);
}
/** returns an exponentially distributed random number
 *
 * Probability for a given return value \f$\chi\f$ is \f$p(\chi) =
 * \Theta(\chi) \cdot \exp(-t)\f$
 */
template <typename T = double> T exponential() {
  return std::exponential_distribution<T>(1)(engine);
}

}  // namespace Random
}  // namespace Smash

#endif  // SRC_INCLUDE_RANDOM_H_
