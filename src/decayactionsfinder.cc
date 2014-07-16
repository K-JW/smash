/*
 *
 *    Copyright (c) 2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "include/decayactionsfinder.h"
#include "include/action.h"
#include "include/constants.h"
#include "include/crosssections.h"
#include "include/experimentparameters.h"
#include "include/fourvector.h"
#include "include/particles.h"
#include "include/random.h"

namespace Smash {

DecayActionsFinder::DecayActionsFinder(const ExperimentParameters &parameters)
                     : ActionFinderFactory(parameters.timestep_duration()) {
}

ActionList DecayActionsFinder::find_possible_actions(Particles *particles) {
  ActionList actions;

  for (const auto &p : particles->data()) {
    if (p.type().is_stable()) {
      continue;      /* particle doesn't decay */
    }

    /* local rest frame velocity */
    FourVector velocity_lrf = FourVector(1., p.velocity());
    /* The clock goes slower in the rest frame of the resonance */
    double inverse_gamma = sqrt(velocity_lrf.Dot(velocity_lrf));
    double resonance_frame_timestep = dt_ * inverse_gamma;

    std::unique_ptr<DecayAction> act(new DecayAction(p));
    float width = act->weight();   // total decay width (mass-dependent)

    /* Exponential decay. Lifetime tau = 1 / width
     * t / tau = width * t (remember GeV-fm conversion)
     * P(decay at Delta_t) = width * Delta_t
     * P(alive after n steps) = (1 - width * Delta_t)^n
     * = (1 - width * Delta_t)^(t / Delta_t)
     * -> exp(-width * t) when Delta_t -> 0
     */
    if (Random::canonical() < resonance_frame_timestep * width / hbarc) {
      /* Time is up! Set the particle to decay at this timestep. */
      actions.emplace_back(std::move(act));
    }
  }
  return std::move(actions);
}

}  // namespace Smash
