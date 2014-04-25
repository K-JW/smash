/*
 *
 *    Copyright (c) 2013-2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */
#ifndef SRC_INCLUDE_MODUSDEFAULT_H_
#define SRC_INCLUDE_MODUSDEFAULT_H_

#include <stdexcept>
#include <stdint.h>
#include <list>

#include "include/particles.h"

namespace Smash {

/* forward declarations */
class Particles;
class CrossSections;
struct ExperimentParameters;

/**
 * Baseclass for Modus classes that provides default function implementations.
 *
 * This is only a base class for actual Modus classes. Meaning there will never
 * be objects, references, or pointers to ModusDefault. Therefore, it does not
 * have - and will never need any virtual functions.
 *
 * The rules for adding functions to this class are as follows:
 * - This class is empty per default.
 * - You can add a function if you have a function that is different in at least
 *   two subclasses and equal in at least two subclasses.
 * - Code that is common to all goes into ExperimentImplementation.
 */
class ModusDefault {
 public:
  // never needs a virtual destructor

  // Missing functions for concrete Modus implementations:
  // void initial_conditions(Particles *particles);

  /**
   * Only needed for BoxModus. The default for all the other modi does nothing.
   */
  int sanity_check(Particles * /*p*/) { return 0; }

  /**
   * XXX: document what it does in general
   */
  void propagate(Particles *particles, const ExperimentParameters &parameters);

  /** BadInput is an error to throw if the configuration options are invalid.
   *
   **/
  struct BadInput : public std::invalid_argument {
    using std::invalid_argument::invalid_argument;
  };
};

}  // namespace Smash

#endif  // SRC_INCLUDE_MODUSDEFAULT_H_
