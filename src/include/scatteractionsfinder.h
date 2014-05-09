/*
 *
 *    Copyright (c) 2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#ifndef SRC_INCLUDE_SCATTERACTIONSFINDER_H_
#define SRC_INCLUDE_SCATTERACTIONSFINDER_H_

#include <vector>

#include "actionfinderfactory.h"

namespace Smash {

/** A simple scatter finder:
 * Just loops through all particles and checks each pair for a collision.  */
class ScatterActionsFinder : public ActionFinderFactory {
 public:
  /** Check for a single pair of particles (id_a, id_b) if a collision will happen
   * in the next timestep and create a corresponding Action object in that case. */
  ActionPtr check_collision(const int id_a, const int id_b,
                            Particles *particles,
                            const ExperimentParameters &parameters,
                            CrossSections *cross_sections = nullptr) const;

  /** Check the whole particle list for collisions
   * and return a list with the corrsponding Action objects. */
  std::vector<ActionPtr> find_possible_actions(
      Particles *particles, const ExperimentParameters &parameters,
      CrossSections *cross_sections = nullptr) const override;

 private:
};

#if 0
/* An advanced scatter finder:
 * Sets up a grid and sorts the particles into grid cells. */
class GridScatterFinder : public ScatterActionsFinder {
 public:
  GridScatterFinder (float length);
  void find_possible_actions (std::vector<ActionPtr> &actions,
                              Particles *particles,
                              const ExperimentParameters &parameters,
                              CrossSections *cross_sections = nullptr) const override;
 private:
  /* Cube edge length. */
  const float length_;
};
#endif

}  // namespace Smash

#endif  // SRC_INCLUDE_SCATTERACTIONSFINDER_H_
