/*
 *
 *    Copyright (c) 2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "include/action.h"

#include "include/angles.h"
#include "include/decaymodes.h"
#include "include/outputroutines.h"
#include "include/pdgcode.h"
#include "include/resonances.h"

namespace Smash {


DecayAction::DecayAction(const ParticleData &in_part, float time_of_execution)
    : Action({in_part}, time_of_execution) {}

DecayAction::DecayAction(const ParticleData &p) : Action({p}, 0.) {
  add_processes(p.type().get_partial_widths(p.effective_mass()));
}


double DecayAction::sqrt_s() const {
  return incoming_particles_[0].momentum().abs();
}


void DecayAction::one_to_two() {
  /* Sample the masses and momenta. */
  sample_cms_momenta();
}


void DecayAction::one_to_three() {
  const auto &log = logger<LogArea::DecayModes>();
  ParticleData &outgoing0 = outgoing_particles_[0];
  ParticleData &outgoing1 = outgoing_particles_[1];
  ParticleData &outgoing2 = outgoing_particles_[2];
  const ParticleType &outgoing0_type = outgoing0.type();
  const ParticleType &outgoing1_type = outgoing1.type();
  const ParticleType &outgoing2_type = outgoing2.type();

  printd("Note: Doing 1->3 decay!\n");

  const double mass_a = outgoing0_type.mass();
  const double mass_b = outgoing1_type.mass();
  const double mass_c = outgoing2_type.mass();
  const double mass_resonance = incoming_particles_[0].effective_mass();

  /* mandelstam-s limits for pairs ab and bc */
  const double s_ab_max = (mass_resonance - mass_c) * (mass_resonance - mass_c);
  const double s_ab_min = (mass_a + mass_b) * (mass_a + mass_b);
  const double s_bc_max = (mass_resonance - mass_a) * (mass_resonance - mass_a);
  const double s_bc_min = (mass_b + mass_c) * (mass_b + mass_c);

  printd("s_ab limits: %g %g \n", s_ab_min, s_ab_max);
  printd("s_bc limits: %g %g \n", s_bc_min, s_bc_max);

  /* randomly pick values for s_ab and s_bc
   * until the pair is within the Dalitz plot */
  double dalitz_bc_max = 0.0, dalitz_bc_min = 1.0;
  double s_ab = 0.0, s_bc = 0.5;
  while (s_bc > dalitz_bc_max || s_bc < dalitz_bc_min) {
    s_ab = Random::uniform(s_ab_min, s_ab_max);
    s_bc = Random::uniform(s_bc_min, s_bc_max);
    const double e_b_rest =
      (s_ab - mass_a * mass_a + mass_b * mass_b) / (2 * std::sqrt(s_ab));
    const double e_c_rest =
      (mass_resonance * mass_resonance - s_ab - mass_c * mass_c) /
      (2 * std::sqrt(s_ab));
    dalitz_bc_max = (e_b_rest + e_c_rest) * (e_b_rest + e_c_rest) -
                    (std::sqrt(e_b_rest * e_b_rest - mass_b * mass_b) -
                     std::sqrt(e_c_rest * e_c_rest - mass_c * mass_c)) *
                     (std::sqrt(e_b_rest * e_b_rest - mass_b * mass_b) -
                      std::sqrt(e_c_rest * e_c_rest - mass_c * mass_c));
    dalitz_bc_min = (e_b_rest + e_c_rest) * (e_b_rest + e_c_rest) -
                    (std::sqrt(e_b_rest * e_b_rest - mass_b * mass_b) +
                     std::sqrt(e_c_rest * e_c_rest - mass_c * mass_c)) *
                     (std::sqrt(e_b_rest * e_b_rest - mass_b * mass_b) +
                      std::sqrt(e_c_rest * e_c_rest - mass_c * mass_c));
  }

  printd("s_ab: %g s_bc: %g min: %g max: %g\n", s_ab, s_bc, dalitz_bc_min,
         dalitz_bc_max);

  /* Compute energy and momentum magnitude */
  const double energy_a =
      (mass_resonance * mass_resonance + mass_a * mass_a - s_bc) /
      (2 * mass_resonance);
  const double energy_c =
      (mass_resonance * mass_resonance + mass_c * mass_c - s_ab) /
      (2 * mass_resonance);
  const double energy_b =
      (s_ab + s_bc - mass_a * mass_a - mass_c * mass_c) / (2 * mass_resonance);
  const double momentum_a = std::sqrt(energy_a * energy_a - mass_a * mass_a);
  const double momentum_c = std::sqrt(energy_c * energy_c - mass_c * mass_c);
  const double momentum_b = std::sqrt(energy_b * energy_b - mass_b * mass_b);

  const double total_energy = sqrt_s();
  if (fabs(energy_a + energy_b + energy_c - total_energy) > really_small) {
    log.warn("1->3: Ea + Eb + Ec: ", energy_a + energy_b + energy_c,
             " Total E: ", total_energy);
  }
  log.debug("Calculating the angles...");

  /* momentum_a direction is random */
  Angles phitheta;
  phitheta.distribute_isotropically();
  /* This is the angle of the plane of the three decay particles */
  outgoing0.set_4momentum(mass_a, phitheta.threevec() * momentum_a);

  /* Angle between a and b */
  double theta_ab = acos(
      (energy_a * energy_b - 0.5 * (s_ab - mass_a * mass_a - mass_b * mass_b)) /
      (momentum_a * momentum_b));
  log.debug("theta_ab: ", theta_ab, " Ea: ", energy_a, " Eb: ", energy_b,
            " sab: ", s_ab, " pa: ", momentum_a, " pb: ", momentum_b);
  bool phi_has_changed = phitheta.add_to_theta(theta_ab);
  outgoing1.set_4momentum(mass_b, phitheta.threevec() * momentum_b);

  /* Angle between b and c */
  double theta_bc = acos(
      (energy_b * energy_c - 0.5 * (s_bc - mass_b * mass_b - mass_c * mass_c)) /
      (momentum_b * momentum_c));
  log.debug("theta_bc: ", theta_bc, " Eb: ", energy_b, " Ec: ", energy_c,
            " sbc: ", s_bc, " pb: ", momentum_b, " pc: ", momentum_c);
  // pass information on whether phi has changed during the last adding
  // on to add_to_theta:
  phitheta.add_to_theta(theta_bc, phi_has_changed);
  outgoing2.set_4momentum(mass_c, phitheta.threevec() * momentum_c);

  /* Momentum check */
  FourVector ptot = outgoing0.momentum() + outgoing1.momentum() +
                    outgoing2.momentum();

  if (fabs(ptot.x0() - total_energy) > really_small) {
    log.warn("1->3 energy not conserved! Before: ", total_energy, " After: ",
             ptot.x0());
  }
  if (fabs(ptot.x1()) > really_small || fabs(ptot.x2()) > really_small ||
      fabs(ptot.x3()) > really_small) {
    log.warn("1->3 momentum check failed. Total momentum: ", ptot.threevec());
  }

  log.debug(  "outgoing0: ", outgoing0.momentum(),
            "\noutgoing1: ", outgoing1.momentum(),
            "\noutgoing2: ", outgoing2.momentum());
}


void DecayAction::perform(Particles *particles, size_t &id_process) {
  const auto &log = logger<LogArea::DecayModes>();
  log.debug("Process: Resonance decay. ");

  /*
   * Execute a decay process for the selected particle.
   *
   * Randomly select one of the decay modes of the particle
   * according to their relative weights. Then decay the particle
   * by calling function one_to_two or one_to_three.
   */
  outgoing_particles_ = choose_channel();

  switch (outgoing_particles_.size()) {
  case 2:
    one_to_two();
    break;
  case 3:
    one_to_three();
    break;
  default:
    throw InvalidDecay(
        "DecayAction::perform: Only 1->2 or 1->3 processes are supported. "
        "Decay from 1->" +
        std::to_string(outgoing_particles_.size()) + " was requested.");
  }

  /* Set positions and boost back. */
  ThreeVector velocity_CM = incoming_particles_[0].velocity();
  for (auto &p : outgoing_particles_) {
    log.debug("particle momenta in lrf ", p);
    p.set_4momentum(p.momentum().LorentzBoost(-velocity_CM));
    p.set_4position(incoming_particles_[0].position());
    log.debug("particle momenta in comp ", p);
    // unset collision time for both particles + keep id + unset partner
    p.set_collision_past(id_process);
  }

  id_process++;

  check_conservation(id_process);

  /* Remove decayed particle */
  particles->remove(incoming_particles_[0].id());
  log.debug("ID ", incoming_particles_[0].id(),
            " has decayed and removed from the list.");

  for (auto &p : outgoing_particles_) {
    p.set_id(particles->add_data(p));
  }
  log.debug("Particle map now has ", particles->size(), " elements.");
}

void DecayAction::format_debug_output(std::ostream &out) const {
  out << "Decay of " << incoming_particles_ << " to " << outgoing_particles_;
}

}  // namespace Smash
