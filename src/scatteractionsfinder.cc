/*
 *
 *    Copyright (c) 2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "include/scatteractionsfinder.h"

#include "include/resonances.h"
#include "include/macros.h"
#include "include/outputroutines.h"

namespace Smash {

std::vector<ActionPtr>
ScatterActionsFinder::find_possible_actions (Particles *particles,
					const ExperimentParameters &parameters,
					CrossSections *cross_sections) const
{
  std::vector<ActionPtr> actions;
  FourVector distance;
  double neighborhood_radius_squared = parameters.cross_section * fm2_mb * M_1_PI * 4;

  for (const auto &p1 : particles->data()) {
    for (const auto &p2 : particles->data()) {
      int id_a, id_b;
      ActionPtr act;
      std::vector<int> in_part;
      id_a = p1.id();
      id_b = p2.id();
      /* exclude check on same particle and double counting */
      if (id_a >= id_b) continue;
      distance = p1.position() - p2.position();
      /* skip particles that are double interaction radius length away
       * (3-product gives negative values
       * with the chosen sign convention for the metric)
       */
      if (-distance.DotThree() > neighborhood_radius_squared)
        continue;

      /* just collided with this particle */
      if (particles->data(id_a).id_process() >= 0
	  && particles->data(id_a).id_process()
	  == particles->data(id_b).id_process()) {
	printd("Skipping collided particle %d <-> %d at time %g due process %d\n",
	    id_a, id_b, particles->data(id_a).position().x0(),
	    particles->data(id_a).id_process());
	continue;
      }

      /* check according timestep: positive and smaller */
      const double time_collision = collision_time(particles->data(id_a),
	particles->data(id_b));
      if (time_collision < 0.0 || time_collision >= parameters.eps)
	continue;

      /* check for minimal collision time both particles */
      if ((particles->data(id_a).collision_time() > 0.0
	    && time_collision > particles->data(id_a).collision_time())
	  || (particles->data(id_b).collision_time() > 0
	    && time_collision > particles->data(id_b).collision_time())) {
	printd("%g Not minimal particle %d <-> %d\n",
	    particles->data(id_a).position().x0(), id_a, id_b);
	continue;
      }

      in_part.push_back(id_a);
      in_part.push_back(id_b);
      act = ActionPtr(new Action(in_part, time_collision));

      /* Compute kinematic quantities needed for cross section calculations  */
      cross_sections->compute_kinematics(particles, id_a, id_b);

      /* Resonance production cross section */
      std::vector<ProcessBranch> resonance_xsections
	= resonance_cross_section(particles->data(id_a), particles->data(id_b),
	  particles->type(id_a), particles->type(id_b), particles);
      act->add_processes(resonance_xsections);

      /* Add elastic process.  */
      ProcessBranch el;
      el.add_particle(particles->data(id_a).pdgcode());
      el.add_particle(particles->data(id_b).pdgcode());
      el.set_weight(cross_sections->elastic(particles, id_a, id_b));
      el.set_type(0);
      act->add_process(el);

      {
	/* distance criteria according to cross_section */
	const double distance_squared = particle_distance(
		  particles->data_pointer(id_a), particles->data_pointer(id_b));
	if (distance_squared >= act->weight() * fm2_mb * M_1_PI)
	  continue;
	printd("distance squared particle %d <-> %d: %g \n", id_a, id_b,
	      distance_squared);
      }

      /* TODO: handle minimal collision time of both particles */
      if (unlikely(particles->data(id_a).collision_time() > 0.0)) {
	int id_not = particles->data(id_a).id_partner();
	printd("Not colliding particle %d <-> %d\n", id_a, id_not);
	/* unset collision partner to zero time and unexisting id */
// 	if (particles->count(id_not) > 0)
// 	  particles->data_pointer(id_not)->set_collision(-1, 0.0, -1);
// 	/* remove any of those partners from the list */
// 	if (id_a < id_not) {
// 	  printd("Removing particle %d from collision list\n", id_a);
// 	  collision_list->remove(id_a);
// 	} else {
// 	  printd("Removing particle %d from collision list\n", id_not);
// 	  collision_list->remove(id_not);
// 	}
// 	/* collect statistics of multiple possible collision partner */
// 	(*rejection_conflict)++;
      }
      if (unlikely(particles->data(id_b).collision_time() > 0.0)) {
	int id_not = particles->data(id_b).id_partner();
	printd("Not colliding particle %d <-> %d\n", id_b, id_not);
	/* unset collision partner to zero time and unexisting id */
// 	if (particles->count(id_not) > 0)
// 	  particles->data_pointer(id_not)->set_collision(-1, 0.0, -1);
// 	/* remove any of those partners from the list */
// 	if (id_b < id_not) {
// 	  printd("Removing particle %d from collision list\n", id_b);
// 	  collision_list->remove(id_b);
// 	} else {
// 	  printd("Removing particle %d from collision list\n", id_not);
// 	  collision_list->remove(id_not);
// 	}
// 	/* collect statistics of multiple possible collision partner */
// 	(*rejection_conflict)++;
      }

      /* If resonance formation probability is high enough, do that,
      * otherwise do elastic collision
      */
      act->decide();

      /* setup collision partners */
      printd("collision type %d particle %d <-> %d time: %g\n", act->process_type(),
	id_a, id_b, time_collision);
      particles->data_pointer(id_a)->set_collision_time(time_collision);
      particles->data_pointer(id_a)->set_collision_time(time_collision);
      printd("collision type %d particle %d <-> %d time: %g\n",
	    particles->data(id_a).process_type(),
	    particles->data(id_a).id_partner(),
	    particles->data(id_b).id_partner(),
	    particles->data(id_a).collision_time());

      /* add to collision list */
      actions.push_back (std::move(act));
    }
  }

  return actions;
}



}  // namespace Smash
