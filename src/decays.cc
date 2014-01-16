/*
 *
 *    Copyright (c) 2013
 *      maximilian attems <attems@fias.uni-frankfurt.de>
 *      Jussi Auvinen <auvinen@fias.uni-frankfurt.de>
 *
 *    GNU General Public License (GPLv3)
 *
 */

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <list>
#include <map>

#include "include/decays.h"

#include "include/Modus.h"
#include "include/ParticleData.h"
#include "include/ParticleType.h"
#include "include/Particles.h"
#include "include/constants.h"
#include "include/FourVector.h"
#include "include/outputroutines.h"
#include "include/resonances.h"

/* check_decays - does a resonance decay on this timestep? */
void check_decays(Particles *particles, std::list<int> *decay_list,
  const Modus &parameters) {
  FourVector velocity_lrf;
  velocity_lrf.set_x0(1.0);

  for (std::map<int, ParticleData>::iterator i = particles->begin();
       i != particles->end(); ++i) {
    /* particle doesn't decay */
    if (particles->type(i->first).width() < 0.0)
      continue;
    /* local rest frame velocity */
    velocity_lrf.set_x1(i->second.momentum().x1() / i->second.momentum().x0());
    velocity_lrf.set_x2(i->second.momentum().x2() / i->second.momentum().x0());
    velocity_lrf.set_x3(i->second.momentum().x3() / i->second.momentum().x0());

    /* The clock goes slower in the rest frame of the resonance */
    double inverse_gamma = sqrt(velocity_lrf.Dot(velocity_lrf));
    double resonance_frame_timestep = parameters.eps * inverse_gamma;

    /* Exponential decay. Average lifetime t_avr = 1 / width
     * t / t_avr = width * t (remember GeV-fm conversion)
     * P(decay at Delta_t) = width * Delta_t
     * P(alive after n steps) = (1 - width * Delta_t)^n
     * = (1 - width * Delta_t)^(t / Delta_t)
     * -> exp(-width * t) when Delta_t -> 0
     */
    if (drand48() < resonance_frame_timestep
                    * particles->type(i->first).width() / hbarc) {
      /* Time is up! Set the particle to decay at this timestep */
      i->second.set_collision(2, 0.0, -1);
      decay_list->push_back(i->first);
    }
  }
}


/* 1->2 and 1->3 decay processes */
size_t decay_particles(Particles *particles, std::list<int> *decay_list,
  size_t id_process) {
  FourVector velocity_CM;

  for (std::list<int>::iterator id = decay_list->begin();
    id != decay_list->end(); ++id) {
    /* relevant particle id's for the collision */
    int id_a = *id;
    int interaction_type = particles->data(id_a).process_type();

    if (interaction_type != 2)
      printf("Decays warning: ID %i (%s) has process type %i.\n",
           id_a, particles->type(id_a).name().c_str(), interaction_type);

    /* Save a copy of the initial state */
    ParticleData initial_data = particles->data(id_a);
    ParticleType initial_type = particles->type(id_a);

    printd("Process: Resonance decay. ");
    printd_momenta("Resonance momenta before decay", particles->data(id_a));

    /* boost to rest frame */
    velocity_CM.set_x0(1.0);
    velocity_CM.set_x1(particles->data(id_a).momentum().x1()
                       / particles->data(id_a).momentum().x0());
    velocity_CM.set_x2(particles->data(id_a).momentum().x2()
                       / particles->data(id_a).momentum().x0());
    velocity_CM.set_x3(particles->data(id_a).momentum().x3()
                       / particles->data(id_a).momentum().x0());
    particles->data_pointer(id_a)->set_momentum(
        particles->data(id_a).momentum().LorentzBoost(velocity_CM));
    particles->data_pointer(id_a)->set_position(
        particles->data(id_a).position().LorentzBoost(velocity_CM));

    printd_momenta("Boosted resonance momenta before decay",
                   particles->data(id_a));

    /* Save the highest id before decay */
    size_t old_max_id = particles->id_max();
    /* Do the decay; this returns the smallest new id */
    size_t id_new_a = resonance_decay(particles, id_a);
    /* There's going to be at least 2 new particles */
    size_t id_new_b = id_new_a + 1;

    printd_momenta("particle 1 momenta in lrf", particles->data(id_new_a));
    printd_momenta("particle 2 momenta in lrf", particles->data(id_new_b));

    boost_back_CM(particles->data_pointer(id_new_a),
                  particles->data_pointer(id_new_b), &velocity_CM);

    /* How many new particles we have exactly */
    size_t new_particles = particles->id_max() - old_max_id;
    /* Write the process type (1->N) and initial state oscar output */
    write_oscar(initial_data, initial_type, 1, new_particles);
    /* Write the first 2 final state particles, which are always there */
    write_oscar(particles->data(id_new_a), particles->type(id_new_a));
    write_oscar(particles->data(id_new_b), particles->type(id_new_b));

    /* Check for the possible third final state particle */
    int id_new_c = -1;
    if (new_particles == 3) {
      id_new_c = id_new_b + 1;
      FourVector velocity = velocity_CM;
      velocity *= -1;
      velocity.set_x0(1.0);
      FourVector momentum_c(particles->data(id_new_c).momentum());
      FourVector position_c(particles->data(id_new_c).position());
      /* Boost the momenta back to lab frame */
      momentum_c = momentum_c.LorentzBoost(velocity);
      /* Boost the position back to lab frame */
      position_c = position_c.LorentzBoost(velocity);
      /* Write the oscar output for this particle */
      particles->data_pointer(id_new_c)->set_momentum(momentum_c);
      particles->data_pointer(id_new_c)->set_position(position_c);
      write_oscar(particles->data(id_new_c), particles->type(id_new_c));
    }

    printd_momenta("particle 1 momenta in comp", particles->data(id_new_a));
    printd_momenta("particle 2 momenta in comp", particles->data(id_new_b));

    FourVector final_momentum(particles->data(id_new_a).momentum()
      + particles->data(id_new_b).momentum());

    /* unset collision time for both particles + keep id + unset partner */
    particles->data_pointer(id_new_a)->set_collision_past(id_process);
    particles->data_pointer(id_new_b)->set_collision_past(id_process);
    if (new_particles == 3) {
      final_momentum += particles->data(id_new_c).momentum();
      particles->data_pointer(id_new_c)->set_collision_past(id_process);
    }
    printd("Particle map has now %zu elements. \n", particles->size());

    id_process++;

    /* Check momentum conservation */
    FourVector momentum_difference;
    momentum_difference += initial_data.momentum();
    momentum_difference -= final_momentum;
    if (fabs(momentum_difference.x0()) > really_small) {
      printf("Process %zu type %i particle %s decay to %s and %s ",
        id_process, interaction_type, particles->type(id_a).name().c_str(),
             particles->type(id_new_a).name().c_str(),
             particles->type(id_new_b).name().c_str());
      if (new_particles == 3) {
        printf("and %s ", particles->type(id_new_c).name().c_str());
      }
      printf("time %g\n", initial_data.position().x0());
      printf("Warning: Interaction type %i E conservation violation %g\n",
             interaction_type, momentum_difference.x0());
    }
    if (fabs(momentum_difference.x1()) > really_small)
      printf("Warning: Interaction type %i px conservation violation %g\n",
             interaction_type, momentum_difference.x1());
    if (fabs(momentum_difference.x2()) > really_small)
      printf("Warning: Interaction type %i py conservation violation %g\n",
             interaction_type, momentum_difference.x2());
    if (fabs(momentum_difference.x3()) > really_small)
      printf("Warning: Interaction type %i pz conservation violation %g\n",
             interaction_type, momentum_difference.x3());

    /* Remove decayed particle */
    particles->remove(id_a);
    printd("ID %i has decayed and removed from the list.\n", id_a);
  } /* end for std::list<int>::iterator id = decay_list->begin() */
  /* empty the decay table */
  decay_list->clear();
  printd("Decay list done.\n");

  /* return how many processes we have handled so far*/
  return id_process;
}


/* Resonance decay process */
int resonance_decay(Particles *particles, int particle_id) {
  const int pdgcode = particles->type(particle_id).pdgcode();
  /* Get the decay modes of this resonance */
  const std::vector<ProcessBranch> decaymodes
    = particles->decay_modes(pdgcode).decay_mode_list();
  /* Get the first decay mode and its branching ratio */
  std::vector<ProcessBranch>::const_iterator mode = decaymodes.begin();
  double cumulated_probability = mode->weight();
  /* Ratios of decay channels should add to 1; pick a random number
   * between 0 and 1 to select the decay mode to be used
   */
  double random_mode = drand48();
  /* Keep adding to the probability until it exceeds the random value */
  while (random_mode > cumulated_probability &&  mode != decaymodes.end()) {
    cumulated_probability += mode->weight();
    ++mode;
  }
  /* We found our decay branch, get the decay product pdgs and do the decay */
  size_t decay_particles = mode->particle_list().size();
  int type_a = 0, type_b = 0, new_id_a = -1;
  if (decay_particles > 3) {
    printf("Warning: Not a 1->2 or 1->3 process!\n");
    printf("Number of decay particles: %zu \n", decay_particles);
    printf("Decay particles: ");
    for (size_t i = 0; i < decay_particles; i++) {
      printf("%i ", mode->particle_list().at(i));
    }
    printf("\n");
  } else if (decay_particles == 2) {
    type_a = mode->particle_list().at(0);
    type_b = mode->particle_list().at(1);
    if (abs(type_a) < 100 || abs(type_b) < 100) {
      printf("Warning: decay products A: %i B: %i\n", type_a, type_b);
    }
    new_id_a = one_to_two(particles, particle_id, type_a, type_b);
  } else if (decay_particles == 3) {
    type_a = mode->particle_list().at(0);
    type_b = mode->particle_list().at(1);
    int type_c = mode->particle_list().at(2);
    if (abs(type_a) < 100 || abs(type_b) < 100 || abs(type_c) < 100) {
      printf("Warning: decay products A: %i B: %i C: %i\n",
             type_a, type_b, type_c);
    }
    printd("Note: Doing 1->3 decay!\n");
    new_id_a = one_to_three(particles, particle_id, type_a, type_b, type_c);
  }
  return new_id_a;
}


/* 1->2 process kinematics */
int one_to_two(Particles *particles, int resonance_id, int type_a, int type_b) {
  /* Add two new particles */
  ParticleData new_particle_a, new_particle_b;
  new_particle_a.set_pdgcode(type_a);
  new_particle_b.set_pdgcode(type_b);

  double mass_a = particles->particle_type(type_a).mass(),
    mass_b = particles->particle_type(type_b).mass();
  const double total_energy = particles->data(resonance_id).momentum().x0();

  /* If one of the particles is resonance, sample its mass */
  /* XXX: Other particle assumed stable! */
  if (particles->particle_type(type_a).width() > 0) {
    mass_a = sample_resonance_mass(particles, type_a, type_b, total_energy);
  } else if (particles->particle_type(type_b).width() > 0) {
    mass_b = sample_resonance_mass(particles, type_b, type_a, total_energy);
  }

  /* Sample the momenta */
  sample_cms_momenta(&new_particle_a, &new_particle_b, total_energy,
                     mass_a, mass_b);

  /* Both decay products begin from the same point */
  FourVector decay_point = particles->data(resonance_id).position();
  new_particle_a.set_position(decay_point);
  new_particle_b.set_position(decay_point);

  /* Assign IDs to new particles */
  int new_id_a = particles->id_max() + 1;
  int new_id_b = new_id_a + 1;
  new_particle_a.set_id(new_id_a);
  new_particle_b.set_id(new_id_b);

  particles->add_data(new_particle_a);
  particles->add_data(new_particle_b);

  printd("Created %s and %s with IDs %d and %d \n",
         particles->type(new_id_a).name().c_str(),
         particles->type(new_id_b).name().c_str(), new_id_a, new_id_b);

  return new_id_a;
}

/* 1->3 process kinematics */
int one_to_three(Particles *particles, int resonance_id,
                 int type_a, int type_b, int type_c) {
  /* Add three new particles */
  ParticleData new_particle_a, new_particle_b, new_particle_c;
  new_particle_a.set_pdgcode(type_a);
  new_particle_b.set_pdgcode(type_b);
  new_particle_c.set_pdgcode(type_c);

  FourVector momentum_resonance = particles->data(resonance_id).momentum();
  const double mass_a = particles->particle_type(type_a).mass(),
    mass_b = particles->particle_type(type_b).mass(),
    mass_c = particles->particle_type(type_c).mass(),
    mass_resonance = sqrt(momentum_resonance.Dot(momentum_resonance));

  /* mandelstam-s limits for pairs ab and bc */
  double s_ab_max = (mass_resonance - mass_c) * (mass_resonance - mass_c);
  double s_ab_min = (mass_a + mass_b) * (mass_a + mass_b);
  double s_bc_max = (mass_resonance - mass_a) * (mass_resonance - mass_a);
  double s_bc_min = (mass_b + mass_c) * (mass_b + mass_c);

  printd("s_ab limits: %g %g \n", s_ab_min, s_ab_max);
  printd("s_bc limits: %g %g \n", s_bc_min, s_bc_max);

  /* randomly pick values for s_ab and s_bc
   * until the pair is within the Dalitz plot */
  double dalitz_bc_max = 0.0, dalitz_bc_min = 1.0;
  double s_ab = 0.0, s_bc = 0.5;
  while (s_bc > dalitz_bc_max || s_bc < dalitz_bc_min) {
    s_ab = (s_ab_max - s_ab_min) * drand48() + s_ab_min;
    s_bc = (s_bc_max - s_bc_min) * drand48() + s_bc_min;
    double e_b_rest = (s_ab - mass_a * mass_a + mass_b * mass_b)
                           / (2 * sqrt(s_ab));
    double e_c_rest = (mass_resonance * mass_resonance - s_ab
                            - mass_c * mass_c) / (2 * sqrt(s_ab));
    dalitz_bc_max = (e_b_rest + e_c_rest) * (e_b_rest + e_c_rest)
      - (sqrt(e_b_rest * e_b_rest - mass_b * mass_b)
         - sqrt(e_c_rest * e_c_rest - mass_c * mass_c))
      * (sqrt(e_b_rest * e_b_rest - mass_b * mass_b)
         - sqrt(e_c_rest * e_c_rest - mass_c * mass_c));
    dalitz_bc_min = (e_b_rest + e_c_rest) * (e_b_rest + e_c_rest)
      - (sqrt(e_b_rest * e_b_rest - mass_b * mass_b)
         + sqrt(e_c_rest * e_c_rest - mass_c * mass_c))
      * (sqrt(e_b_rest * e_b_rest - mass_b * mass_b)
         + sqrt(e_c_rest * e_c_rest - mass_c * mass_c));
  }

  printd("s_ab: %g s_bc: %g min: %g max: %g\n",
         s_ab, s_bc, dalitz_bc_min, dalitz_bc_max);

  /* Compute energy and momentum magnitude */
  const double energy_a = (mass_resonance * mass_resonance + mass_a * mass_a
                           - s_bc) / (2 * mass_resonance);
  const double energy_c = (mass_resonance * mass_resonance + mass_c * mass_c
                           - s_ab) / (2 * mass_resonance);
  const double energy_b = (s_ab + s_bc - mass_a * mass_a - mass_c * mass_c)
                           / (2 * mass_resonance);
  const double momentum_a = sqrt(energy_a * energy_a - mass_a * mass_a);
  const double momentum_c = sqrt(energy_c * energy_c - mass_c * mass_c);
  const double momentum_b = sqrt(energy_b * energy_b - mass_b * mass_b);

  const double total_energy = momentum_resonance.x0();
  if (fabs(energy_a + energy_b + energy_c - total_energy) > really_small)
    printf("1->3 warning: Ea + Eb + Ec: %g Total E: %g",
           energy_a + energy_b + energy_c, total_energy);
  printd("Calculating the angles...\n");

  /* momentum_a direction is random */
  /* phi in the range from [0, 2 * pi) */
  /* This is the angle of the plane of the three decay particles */
  double phi = 2.0 * M_PI * drand48();
  /* cos(theta) in the range from [-1.0, 1.0) */
  double cos_theta_a = -1.0 + 2.0 * drand48();
  double sin_theta_a = sqrt(1.0 - cos_theta_a * cos_theta_a);
  new_particle_a.set_momentum(mass_a,
                              momentum_a * cos(phi) * sin_theta_a,
                              momentum_a * sin(phi) * sin_theta_a,
                              momentum_a * cos_theta_a);

  /* Angle between a and b */
  double theta_ab = acos((energy_a * energy_b - 0.5 * (s_ab - mass_a * mass_a
                          - mass_b * mass_b)) / (momentum_a * momentum_b));
  printd("theta_ab: %g Ea: %g Eb: %g sab: %g pa: %g pb: %g\n",
         theta_ab, energy_a, energy_b, s_ab, momentum_a, momentum_b);
  /* b angle is sum of a angle and ab angle */
  double theta_b = theta_ab + acos(cos_theta_a);
  new_particle_b.set_momentum(mass_b,
                              momentum_b * cos(phi) * sin(theta_b),
                              momentum_b * sin(phi) * sin(theta_b),
                              momentum_b * cos(theta_b));

  /* Angle between b and c */
  double theta_bc = acos((energy_b * energy_c - 0.5 *(s_bc - mass_b * mass_b
                         - mass_c * mass_c)) / (momentum_b * momentum_c));
  printd("theta_bc: %g Eb: %g Ec: %g sbc: %g pb: %g pc: %g\n",
         theta_bc, energy_b, energy_c, s_bc, momentum_b, momentum_c);
  /* c angle is sum of b angle and bc angle */
  double theta_c = theta_bc + theta_b;
  new_particle_c.set_momentum(mass_c,
                              momentum_c * cos(phi) * sin(theta_c),
                              momentum_c * sin(phi) * sin(theta_c),
                              momentum_c * cos(theta_c));

  /* Momentum check */
  double energy = new_particle_a.momentum().x0()
    + new_particle_b.momentum().x0() + new_particle_c.momentum().x0();
  double px = new_particle_a.momentum().x1() + new_particle_b.momentum().x1()
    + new_particle_c.momentum().x1();
  double py = new_particle_a.momentum().x2() + new_particle_b.momentum().x2()
    + new_particle_c.momentum().x2();
  double pz = new_particle_a.momentum().x3() + new_particle_b.momentum().x3()
    + new_particle_c.momentum().x3();

  if (fabs(energy - total_energy) > really_small)
    printf("1->3 energy not conserved! Before: %g After: %g",
           total_energy, energy);

  if (fabs(px) > really_small || fabs(py) > really_small
      || fabs(pz) > really_small)
    printf("1->3 momentum check failed. Total momentum: %g %g %g\n",
           px, py, pz);

  /* All decay products begin from the same point */
  FourVector decay_point = particles->data(resonance_id).position();
  new_particle_a.set_position(decay_point);
  new_particle_b.set_position(decay_point);
  new_particle_c.set_position(decay_point);

  /* Assign IDs to new particles */
  int new_id_a = particles->id_max() + 1;
  int new_id_b = new_id_a + 1;
  int new_id_c = new_id_b + 1;
  new_particle_a.set_id(new_id_a);
  new_particle_b.set_id(new_id_b);
  new_particle_c.set_id(new_id_c);

  particles->add_data(new_particle_a);
  particles->add_data(new_particle_b);
  particles->add_data(new_particle_c);

  printd("Created %s %s %s with IDs %d %d %d \n",
         particles->type(new_id_a).name().c_str(),
         particles->type(new_id_b).name().c_str(),
         particles->type(new_id_c).name().c_str(),
         new_id_a, new_id_b, new_id_c);

  printd("p0: %g %g %g \n", new_particle_a.momentum().x0(),
         new_particle_b.momentum().x0(), new_particle_c.momentum().x0());
  printd("p1: %g %g %g \n", new_particle_a.momentum().x1(),
         new_particle_b.momentum().x1(), new_particle_c.momentum().x1());
  printd("p2: %g %g %g \n", new_particle_a.momentum().x2(),
         new_particle_b.momentum().x2(), new_particle_c.momentum().x2());
  printd("p3: %g %g %g \n", new_particle_a.momentum().x3(),
         new_particle_b.momentum().x3(), new_particle_c.momentum().x3());

  return new_id_a;
}
