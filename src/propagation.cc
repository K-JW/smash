/*
 *
 *    Copyright (c) 2013
 *      maximilian attems <attems@fias.uni-frankfurt.de>
 *      Jussi Auvinen <auvinen@fias.uni-frankfurt.de>
 *
 *    GNU General Public License (GPLv3)
 *
 */
#include "include/propagation.h"

#include <cstdio>
#include <map>
#include <vector>

#include "include/Box.h"
#include "include/Parameters.h"
#include "include/ParticleData.h"
#include "include/ParticleType.h"
#include "include/outputroutines.h"

/* propagate all particles */
void propagate_particles(std::map<int, ParticleData> *particles,
  std::vector<ParticleType> *particle_type, std::map<int, int> *map_type,
  Parameters const &parameters, Box const &box) {
    FourVector distance, position;

    for (std::map<int, ParticleData>::iterator i = particles->begin();
         i != particles->end(); ++i) {
      /* The particle has formed a resonance or has decayed
       * and is not active anymore
       */
      if (i->second.process_type() > 0)
        continue;

      /* propagation for this time step */
      distance.set_FourVector(parameters.eps(),
        i->second.velocity_x() * parameters.eps(),
        i->second.velocity_y() * parameters.eps(),
        i->second.velocity_z() * parameters.eps());
      printd("Particle %d motion: %g %g %g %g\n", i->first,
         distance.x0(), distance.x1(), distance.x2(), distance.x3());

      /* treat the box boundaries */
      bool wall_hit = false;
      position = i->second.position();
      position += distance;
      position = boundary_condition(position, box, &wall_hit);
      if (wall_hit)
        write_oscar((*particles)[i->first],
                    (*particle_type)[(*map_type)[i->first]], 1, 1);
      i->second.set_position(position);
      if (wall_hit)
        write_oscar((*particles)[i->first],
                    (*particle_type)[(*map_type)[i->first]]);
      printd_position(i->second);
    }
}
