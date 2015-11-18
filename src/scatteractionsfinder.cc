/*
 *
 *    Copyright (c) 2014-2015
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "include/scatteractionsfinder.h"

#include <algorithm>

#include "include/configuration.h"
#include "include/constants.h"
#include "include/cxx14compat.h"
#include "include/experimentparameters.h"
#include "include/logging.h"
#include "include/macros.h"
#include "include/particles.h"
#include "include/scatteraction.h"
#include "include/scatteractionbaryonbaryon.h"
#include "include/scatteractionbaryonmeson.h"
#include "include/scatteractionmesonmeson.h"
#include "include/scatteractionnucleonkaon.h"
#include "include/scatteractionnucleonnucleon.h"

namespace Smash {
/*!\Userguide
* \page input_collision_term_ Collision_Term
* \key Elastic_Cross_Section (float, optional, default = -1.0 [mb]) \n
* If a non-negative value is given, it will override the parametrized
* elastic cross sections (which are energy-dependent) with a constant value.
* This constant elastic cross section is used for all collisions.
*
* \key Isotropic (bool, optional, default = false) \n
* Do all collisions isotropically.
* \key Strings (bool, optional, default = true): \n
* true - string excitation is enabled\n
* false - string excitation is disabled
*/

ScatterActionsFinder::ScatterActionsFinder(
    Configuration config, const ExperimentParameters &parameters,
    bool two_to_one, bool two_to_two)
    : elastic_parameter_(config.take({"Collision_Term",
                                      "Elastic_Cross_Section"}, -1.0f)),
      testparticles_(parameters.testparticles),
      isotropic_(config.take({"Collision_Term", "Isotropic"}, false)),
      two_to_one_(two_to_one),
      two_to_two_(two_to_two) {
  if (is_constant_elastic_isotropic()) {
    const auto &log = logger<LogArea::FindScatter>();
    log.info("Constant elastic isotropic cross-section mode:",
             " using ", elastic_parameter_, " mb as maximal cross-section.");
  }
}

    /* Read in switch to turn off strings. */
      strings_switch_(config.take({"Collision_Term", "Strings"}, false)) {}
	   

ScatterActionsFinder::ScatterActionsFinder(
    float elastic_parameter, int testparticles)
    : elastic_parameter_(elastic_parameter),
      testparticles_(testparticles),
      isotropic_(false),
      two_to_one_(true),
      two_to_two_(true) {}

ScatterActionPtr ScatterActionsFinder::construct_scatter_action(
                                            const ParticleData &data_a,
                                            const ParticleData &data_b,
                                            float time_until_collision) const {
  const auto &pdg_a = data_a.pdgcode();
  const auto &pdg_b = data_b.pdgcode();
  ScatterActionPtr act;
  if (data_a.is_baryon() && data_b.is_baryon()) {
    if (pdg_a.is_nucleon() && pdg_b.is_nucleon()) {
      act = make_unique<ScatterActionNucleonNucleon>(data_a, data_b,
                                              time_until_collision, isotropic_);
    } else {
      act = make_unique<ScatterActionBaryonBaryon>(data_a, data_b,
                                              time_until_collision, isotropic_);
    }
  } else if (data_a.is_baryon() || data_b.is_baryon()) {
    if ((pdg_a.is_nucleon() && pdg_b.is_kaon()) ||
        (pdg_b.is_nucleon() && pdg_a.is_kaon())) {
      act = make_unique<ScatterActionNucleonKaon>(data_a, data_b,
                                              time_until_collision, isotropic_);
    } else {
      act = make_unique<ScatterActionBaryonMeson>(data_a, data_b,
                                              time_until_collision, isotropic_);
    }
  } else {
    act = make_unique<ScatterActionMesonMeson>(data_a, data_b,
                                              time_until_collision, isotropic_);
  }
  return std::move(act);
}

ActionPtr ScatterActionsFinder::check_collision(
    const ParticleData &data_a, const ParticleData &data_b, float dt) const {
#ifndef NDEBUG
  const auto &log = logger<LogArea::FindScatter>();
#endif

  /* just collided with this particle */
  if (data_a.id_process() > 0 && data_a.id_process() == data_b.id_process()) {
#ifndef NDEBUG
    log.debug("Skipping collided particles at time ", data_a.position().x0(),
              " due to process ", data_a.id_process(),
              "\n    ", data_a,
              "\n<-> ", data_b);
#endif
    return nullptr;
  }

  /* Determine time of collision. */
  const float time_until_collision = collision_time(data_a, data_b);

  /* Check that collision happens in this timestep. */
  if (time_until_collision < 0.f || time_until_collision >= dt) {
    return nullptr;
  }

  /* Create ScatterAction object. */
  ScatterActionPtr act = construct_scatter_action(data_a, data_b,
                                                  time_until_collision);
  const double distance_squared = act->transverse_distance_sqr();

  /* Don't calculate cross section if the particles are very far apart. */
  if (distance_squared >= max_transverse_distance_sqr(testparticles_)) {
    return nullptr;
  }

  /* Add various subprocesses.  */
  act->add_all_processes(elastic_parameter_, two_to_one_, two_to_two_);
  /* (4) string excitation */
  if (strings_switch_) {
    act->add_process(act->string_excitation_cross_section());
  }

  /* distance criterion according to cross_section */
  if (distance_squared >= act->cross_section() * fm2_mb * M_1_PI
                          * data_a.cross_section_scaling_factor()
                          * data_b.cross_section_scaling_factor()
                          / static_cast<float>(testparticles_)) {
    return nullptr;
  
  }

#ifndef NDEBUG
  log.debug("particle distance squared: ", distance_squared,
            "\n    ", data_a,
            "\n<-> ", data_b);
#endif

  return std::move(act);
}

ActionList ScatterActionsFinder::find_actions_in_cell(
    const ParticleList &search_list, float dt) const {
  std::vector<ActionPtr> actions;
  for (const ParticleData &p1 : search_list) {
    for (const ParticleData &p2 : search_list) {
      if (p1.id() < p2.id()) {
        // Check if a collision is possible.
        ActionPtr act = check_collision(p1, p2, dt);
        if (act) {
          actions.push_back(std::move(act));
        }
      }
    }
  }
  return actions;
}

ActionList ScatterActionsFinder::find_actions_with_neighbors(
    const ParticleList &search_list, const ParticleList &neighbors_list,
    float dt) const {
  std::vector<ActionPtr> actions;
  for (const ParticleData &p1 : search_list) {
    for (const ParticleData &p2 : neighbors_list) {
      assert(p1.id() != p2.id());
      // Check if a collision is possible.
      ActionPtr act = check_collision(p1, p2, dt);
      if (act) {
        actions.push_back(std::move(act));
      }
    }
  }
  return actions;
}

ActionList ScatterActionsFinder::find_actions_with_surrounding_particles(
    const ParticleList &search_list, const Particles &surrounding_list,
    float dt) const {
  std::vector<ActionPtr> actions;
  for (const ParticleData &p2 : surrounding_list) {
    // don't look for collisions if the particle from the surrounding list is
    // also in the search list
    auto result = std::find_if(
        search_list.begin(), search_list.end(),
        [&p2](const ParticleData &p) { return p.id() == p2.id(); });
    if (result != search_list.end()) {
      continue;
    }
    for (const ParticleData &p1 : search_list) {
      // Check if a collision is possible.
      ActionPtr act = check_collision(p1, p2, dt);
      if (act) {
        actions.push_back(std::move(act));
      }
    }
  }
  return actions;
}

}  // namespace Smash
