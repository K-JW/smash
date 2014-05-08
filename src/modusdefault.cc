/*
 *
 *    Copyright (c) 2013-2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */
#include <cinttypes>
#include <list>

#include "include/modusdefault.h"
#include "include/constants.h"
#include "include/experiment.h"
#include "include/outputroutines.h"

namespace Smash {

/*general propagation routine */
void ModusDefault::propagate(Particles *particles,
                             const ExperimentParameters &parameters) {
  FourVector distance, position;
  for (ParticleData &data : particles->data()) {
    /* propagation for this time step */
    distance.set_FourVector(parameters.labclock.timestep_size(),
                      data.velocity_x() * parameters.labclock.timestep_size(),
                      data.velocity_y() * parameters.labclock.timestep_size(),
                      data.velocity_z() * parameters.labclock.timestep_size());
    printd("Particle %d motion: %g %g %g %g\n", data.id(), distance.x0(),
           distance.x1(), distance.x2(), distance.x3());
    position = data.position();
    position += distance;
    data.set_position(position);
  }
}

}  // namespace Smash
