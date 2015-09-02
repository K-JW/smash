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
* \key Elastic_Cross_Section (float, optional, default = 0.0 [mb]) \n
* If a positive (non-zero) value is given, it will override the parametrized
* elastic cross sections (which are energy-dependent) with a constant value.
* This constant elastic cross section is used for all collisions.
*
* \key Isotropic (bool, optional, default = false) \n
* Do all collisions isotropically.
*/

ScatterActionsFinder::ScatterActionsFinder(
    Configuration config, const ExperimentParameters &parameters)
    : elastic_parameter_(config.take({"Collision_Term",
                                      "Elastic_Cross_Section"}, 0.f)),
      testparticles_(parameters.testparticles),
      isotropic_(config.take({"Collision_Term", "Isotropic"}, false)) {
  if (elastic_parameter_ < 0.) {
    throw std::invalid_argument("Elastic_Cross_Section is negative "
                                "in config file!");
  }
}

ScatterActionsFinder::ScatterActionsFinder(
    float elastic_parameter, int testparticles)
    : elastic_parameter_(elastic_parameter),
      testparticles_(testparticles) {}

double ScatterActionsFinder::collision_time(const ParticleData &p1,
                                            const ParticleData &p2) {
  const auto &log = logger<LogArea::FindScatter>();
  /** UrQMD collision time
   * \iref{Hirano:2012yy} (5.15): in computational frame
   * position of particle a: x_a
   * position of particle b: x_b
   * momentum of particle a: p_a
   * momentum of particle b: p_b
   * t_{coll} = - (x_a - x_b) . (p_a - p_b) / (p_a - p_b)^2
   */
  ThreeVector pos_diff = p1.position().threevec() - p2.position().threevec();
  ThreeVector velo_diff = p1.velocity() - p2.velocity();
  double vsqr = velo_diff.sqr();
  log.trace(source_location, "\n"
            "Scatter ", p1, "\n"
            "    <-> ", p2, "\n"
            "=> position difference: ", pos_diff, " [fm]",
            ", velocity difference: ", velo_diff, " [GeV]");
  /* Zero momentum leads to infite distance, particles are not approaching. */
  if (vsqr < really_small) {
    return -1.0;
  } else {
    return -pos_diff * velo_diff / vsqr;
  }
}

static inline bool is_kaon(const PdgCode &pdg) {
    const auto code = std::abs(pdg.code());
    return (code == 0x321) || (code == 0x311);
}

ScatterActionPtr ScatterActionsFinder::construct_scatter_action(
                                            const ParticleData &data_a,
                                            const ParticleData &data_b,
                                            float time_until_collision) const {
  const auto &pdg_a = data_a.pdgcode();
  const auto &pdg_b = data_b.pdgcode();
  ScatterActionPtr act;
  if (data_a.is_baryon() && data_b.is_baryon()) {
    if (pdg_a.iso_multiplet() == 0x1112 && pdg_b.iso_multiplet() == 0x1112) {
      act = make_unique<ScatterActionNucleonNucleon>(data_a, data_b,
                                              time_until_collision, isotropic_);
    } else {
      act = make_unique<ScatterActionBaryonBaryon>(data_a, data_b,
                                              time_until_collision, isotropic_);
    }
  } else if (data_a.is_baryon() || data_b.is_baryon()) {
    if ((pdg_a.iso_multiplet() == 0x1112 && is_kaon(pdg_b)) ||
        (pdg_b.iso_multiplet() == 0x1112 && is_kaon(pdg_a))) {
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
  if (data_a.id_process() >= 0 && data_a.id_process() == data_b.id_process()) {
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
  act->add_all_processes(elastic_parameter_);

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
