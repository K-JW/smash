/*
 *
 *    Copyright (c) 2014-2017
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "include/scatteraction.h"

#include <cmath>

#include "Pythia8/Pythia.h"

#include "include/action_globals.h"
#include "include/angles.h"
#include "include/constants.h"
#include "include/crosssections.h"
#include "include/cxx14compat.h"
#include "include/fpenvironment.h"
#include "include/kinematics.h"
#include "include/logging.h"
#include "include/parametrizations.h"
#include "include/pdgcode.h"
#include "include/pow.h"
#include "include/processstring.h"
#include "include/random.h"

namespace smash {

ScatterAction::ScatterAction(const ParticleData &in_part_a,
                             const ParticleData &in_part_b, double time,
                             bool isotropic, double string_formation_time)
    : Action({in_part_a, in_part_b}, time),
      total_cross_section_(0.),
      isotropic_(isotropic),
      string_formation_time_(string_formation_time) {}

void ScatterAction::add_collision(CollisionBranchPtr p) {
  add_process<CollisionBranch>(p, collision_channels_, total_cross_section_);
}

void ScatterAction::add_collisions(CollisionBranchList pv) {
  add_processes<CollisionBranch>(std::move(pv), collision_channels_,
                                 total_cross_section_);
}

void ScatterAction::generate_final_state() {
  const auto &log = logger<LogArea::ScatterAction>();

  log.debug("Incoming particles: ", incoming_particles_);

  if (pot_pointer != nullptr) {
    filter_channel(collision_channels_, total_cross_section_);
  }
  /* Decide for a particular final state. */
  const CollisionBranch *proc = choose_channel<CollisionBranch>(
      collision_channels_, total_cross_section_);
  process_type_ = proc->get_type();
  outgoing_particles_ = proc->particle_list();
  partial_cross_section_ = proc->weight();

  log.debug("Chosen channel: ", process_type_, outgoing_particles_);

  /* The production point of the new particles.  */
  FourVector middle_point = get_interaction_point();

  switch (process_type_) {
    case ProcessType::Elastic:
      /* 2->2 elastic scattering */
      elastic_scattering();
      break;
    case ProcessType::TwoToOne:
      /* resonance formation */
      /* processes computed in the center of momenta */
      resonance_formation();
      break;
    case ProcessType::TwoToTwo:
      /* 2->2 inelastic scattering */
      /* Sample the particle momenta in CM system. */
      inelastic_scattering();
      break;
    case ProcessType::StringSoft:
      /* soft string excitation */
      string_excitation(true);
      break;
    case ProcessType::StringHard:
      /* hard string excitation */
      string_excitation(false);
      break;
    default:
      throw InvalidScatterAction(
          "ScatterAction::generate_final_state: Invalid process type " +
          std::to_string(static_cast<int>(process_type_)) + " was requested. " +
          "(PDGcode1=" + incoming_particles_[0].pdgcode().string() +
          ", PDGcode2=" + incoming_particles_[1].pdgcode().string() + ")");
  }

  for (ParticleData &new_particle : outgoing_particles_) {
    /* Set positions of the outgoing particles */
    if (proc->get_type() != ProcessType::Elastic) {
      new_particle.set_4position(middle_point);
    }
    /* Set momenta & boost to computational frame. */
    new_particle.boost_momentum(-beta_cm());
  }
}

void ScatterAction::add_all_scatterings(double elastic_parameter,
                                        bool two_to_one,
                                        ReactionsBitSet included_2to2,
                                        double low_snn_cut, bool strings_switch,
                                        NNbarTreatment nnbar_treatment) {
  cross_sections xs(incoming_particles_, sqrt_s());
  CollisionBranchList processes = xs.generate_collision_list(
      elastic_parameter, two_to_one, included_2to2, low_snn_cut, strings_switch,
      nnbar_treatment, string_process_);

  /* Add various subprocesses.*/
  add_collisions(std::move(processes));
}

double ScatterAction::raw_weight_value() const { return total_cross_section_; }

double ScatterAction::partial_weight() const { return partial_cross_section_; }

ThreeVector ScatterAction::beta_cm() const {
  return total_momentum().velocity();
}

double ScatterAction::gamma_cm() const {
  return (1. / std::sqrt(1.0 - beta_cm().sqr()));
}

double ScatterAction::mandelstam_s() const { return total_momentum().sqr(); }

double ScatterAction::cm_momentum() const {
  const double m1 = incoming_particles_[0].effective_mass();
  const double m2 = incoming_particles_[1].effective_mass();
  return pCM(sqrt_s(), m1, m2);
}

double ScatterAction::cm_momentum_squared() const {
  const double m1 = incoming_particles_[0].effective_mass();
  const double m2 = incoming_particles_[1].effective_mass();
  return pCM_sqr(sqrt_s(), m1, m2);
}

double ScatterAction::transverse_distance_sqr() const {
  // local copy of particles (since we need to boost them)
  ParticleData p_a = incoming_particles_[0];
  ParticleData p_b = incoming_particles_[1];
  /* Boost particles to center-of-momentum frame. */
  const ThreeVector velocity = beta_cm();
  p_a.boost(velocity);
  p_b.boost(velocity);
  const ThreeVector pos_diff =
      p_a.position().threevec() - p_b.position().threevec();
  const ThreeVector mom_diff =
      p_a.momentum().threevec() - p_b.momentum().threevec();

  const auto &log = logger<LogArea::ScatterAction>();
  log.debug("Particle ", incoming_particles_,
            " position difference [fm]: ", pos_diff,
            ", momentum difference [GeV]: ", mom_diff);

  const double dp2 = mom_diff.sqr();
  const double dr2 = pos_diff.sqr();
  /* Zero momentum leads to infite distance. */
  if (dp2 < really_small) {
    return dr2;
  }
  const double dpdr = pos_diff * mom_diff;

  /** UrQMD squared distance criterion:
   * \iref{Bass:1998ca} (3.27): in center of momentum frame
   * position of particle a: x_a
   * position of particle b: x_b
   * momentum of particle a: p_a
   * momentum of particle b: p_b
   * d^2_{coll} = (x_a - x_b)^2 - ((x_a - x_b) . (p_a - p_b))^2 / (p_a - p_b)^2
   */
  return dr2 - dpdr * dpdr / dp2;
}

/**
 * Computes the B coefficients from the Cugnon parametrization of the angular
 * distribution in elastic pp scattering, see equation (8) in
 * \iref{Cugnon:1996kh}.
 * Note: The original Cugnon parametrization is only applicable for
 * plab < 6 GeV and keeps rising above that. We add an upper limit of b <= 9,
 * in order to be compatible with high-energy data (up to plab ~ 25 GeV).
 * \param[in] plab Lab momentum in GeV.
 */
static double Cugnon_bpp(double plab) {
  if (plab < 2.) {
    double p8 = pow_int(plab, 8);
    return 5.5 * p8 / (7.7 + p8);
  } else {
    return std::min(9.0, 5.334 + 0.67 * (plab - 2.));
  }
}

/**
 * Computes the B coefficients from the Cugnon parametrization of the angular
 * distribution in elastic np scattering, see equation (10) in
 * \iref{Cugnon:1996kh}.
 * \param[in] plab Lab momentum in GeV.
 */
static double Cugnon_bnp(double plab) {
  if (plab < 0.225) {
    return 0.;
  } else if (plab < 0.6) {
    return 16.53 * (plab - 0.225);
  } else if (plab < 1.6) {
    return -1.63 * plab + 7.16;
  } else {
    return Cugnon_bpp(plab);
  }
}

void ScatterAction::sample_angles(std::pair<double, double> masses) {
  if ((process_type_ == ProcessType::StringSoft) ||
      (process_type_ == ProcessType::StringHard)) {
    // We potentially have more than two particles, so the following angular
    // distributions don't work. Instead we just keep the angular
    // distributions generated by string fragmentation.
    return;
  }
  assert(outgoing_particles_.size() == 2);
  const auto &log = logger<LogArea::ScatterAction>();

  // only NN scattering is anisotropic currently
  const bool nn_scattering = incoming_particles_[0].type().is_nucleon() &&
                             incoming_particles_[1].type().is_nucleon();

  ParticleData *p_a = &outgoing_particles_[0];
  ParticleData *p_b = &outgoing_particles_[1];

  const double mass_a = masses.first;
  const double mass_b = masses.second;

  const double cms_energy = kinetic_energy_cms();

  const std::array<double, 2> t_range = get_t_range<double>(
      cms_energy, nucleon_mass, nucleon_mass, mass_a, mass_b);
  Angles phitheta;
  if (nn_scattering && p_a->pdgcode().is_nucleon() &&
      p_b->pdgcode().is_nucleon() &&
      p_a->pdgcode().antiparticle_sign() ==
          p_b->pdgcode().antiparticle_sign() &&
      !isotropic_) {
    /** NN → NN: Choose angular distribution according to Cugnon
     * parametrization,
     * see \iref{Cugnon:1996kh}. */
    double bb, a, plab = plab_from_s(mandelstam_s());
    if (p_a->type().charge() + p_b->type().charge() == 1) {
      // pn
      bb = std::max(Cugnon_bnp(plab), really_small);
      a = (plab < 0.8) ? 1. : 0.64 / (plab * plab);
    } else {
      // pp or nn
      bb = std::max(Cugnon_bpp(plab), really_small);
      a = 1.;
    }
    double t = Random::expo(bb, t_range[0], t_range[1]);
    if (Random::canonical() > 1. / (1. + a)) {
      t = t_range[0] + t_range[1] - t;
    }
    // determine scattering angles in center-of-mass frame
    phitheta = Angles(2. * M_PI * Random::canonical(),
                      1. - 2. * (t - t_range[0]) / (t_range[1] - t_range[0]));
  } else if (nn_scattering && p_a->pdgcode().is_Delta() &&
             p_b->pdgcode().is_nucleon() &&
             p_a->pdgcode().antiparticle_sign() ==
                 p_b->pdgcode().antiparticle_sign() &&
             !isotropic_) {
    /** NN → NΔ: Sample scattering angles in center-of-mass frame from an
     * anisotropic angular distribution, using the same distribution as for
     * elastic pp scattering, as suggested in \iref{Cugnon:1996kh}. */
    const double plab = plab_from_s(mandelstam_s());
    const double bb = std::max(Cugnon_bpp(plab), really_small);
    double t = Random::expo(bb, t_range[0], t_range[1]);
    if (Random::canonical() > 0.5) {
      t = t_range[0] + t_range[1] - t;  // symmetrize
    }
    phitheta = Angles(2. * M_PI * Random::canonical(),
                      1. - 2. * (t - t_range[0]) / (t_range[1] - t_range[0]));
  } else if (nn_scattering && p_b->pdgcode().is_nucleon() && !isotropic_ &&
             (p_a->type().is_Nstar() || p_a->type().is_Deltastar())) {
    /** NN → NR: Fit to HADES data, see \iref{Agakishiev:2014wqa}. */
    const std::array<double, 4> p{1.46434, 5.80311, -6.89358, 1.94302};
    const double a = p[0] + mass_a * (p[1] + mass_a * (p[2] + mass_a * p[3]));
    /*  If the resonance is so heavy that the index "a" exceeds 30,
     *  the power function turns out to be too sharp. Take t directly to be
     *  t_0 in such a case. */
    double t = t_range[0];
    if (a < 30) {
      t = Random::power(-a, t_range[0], t_range[1]);
    }
    if (Random::canonical() > 0.5) {
      t = t_range[0] + t_range[1] - t;  // symmetrize
    }
    phitheta = Angles(2. * M_PI * Random::canonical(),
                      1. - 2. * (t - t_range[0]) / (t_range[1] - t_range[0]));
  } else {
    /* isotropic angular distribution */
    phitheta.distribute_isotropically();
  }

  ThreeVector pscatt = phitheta.threevec();
  // 3-momentum of first incoming particle in center-of-mass frame
  ThreeVector pcm =
      incoming_particles_[0].momentum().LorentzBoost(beta_cm()).threevec();
  pscatt.rotate_z_axis_to(pcm);

  // final-state CM momentum
  const double p_f = pCM(cms_energy, mass_a, mass_b);
  if (!(p_f > 0.0)) {
    log.warn("Particle: ", p_a->pdgcode(), " radial momentum: ", p_f);
    log.warn("Etot: ", cms_energy, " m_a: ", mass_a, " m_b: ", mass_b);
  }
  p_a->set_4momentum(mass_a, pscatt * p_f);
  p_b->set_4momentum(mass_b, -pscatt * p_f);

  log.debug("p_a: ", *p_a, "\np_b: ", *p_b);
}

void ScatterAction::elastic_scattering() {
  // copy initial particles into final state
  outgoing_particles_[0] = incoming_particles_[0];
  outgoing_particles_[1] = incoming_particles_[1];
  // resample momenta
  sample_angles({outgoing_particles_[0].effective_mass(),
                 outgoing_particles_[1].effective_mass()});
}

void ScatterAction::inelastic_scattering() {
  // create new particles
  sample_2body_phasespace();
  /* Set the formation time of the 2 particles to the larger formation time of
   * the
   * incoming particles, if it is larger than the execution time; execution time
   * is otherwise taken to be the formation time */
  const double t0 = incoming_particles_[0].formation_time();
  const double t1 = incoming_particles_[1].formation_time();

  const size_t index_tmax = (t0 > t1) ? 0 : 1;
  const double sc =
      incoming_particles_[index_tmax].cross_section_scaling_factor();
  if (t0 > time_of_execution_ || t1 > time_of_execution_) {
    outgoing_particles_[0].set_formation_time(std::max(t0, t1));
    outgoing_particles_[1].set_formation_time(std::max(t0, t1));
    outgoing_particles_[0].set_cross_section_scaling_factor(sc);
    outgoing_particles_[1].set_cross_section_scaling_factor(sc);
  } else {
    outgoing_particles_[0].set_formation_time(time_of_execution_);
    outgoing_particles_[1].set_formation_time(time_of_execution_);
  }
}

void ScatterAction::resonance_formation() {
  const auto &log = logger<LogArea::ScatterAction>();

  if (outgoing_particles_.size() != 1) {
    std::string s =
        "resonance_formation: "
        "Incorrect number of particles in final state: ";
    s += std::to_string(outgoing_particles_.size()) + " (";
    s += incoming_particles_[0].pdgcode().string() + " + ";
    s += incoming_particles_[1].pdgcode().string() + ")";
    throw InvalidResonanceFormation(s);
  }

  const double cms_kin_energy = kinetic_energy_cms();
  /* 1 particle in final state: Center-of-momentum frame of initial particles
   * is the rest frame of the resonance.  */
  outgoing_particles_[0].set_4momentum(FourVector(cms_kin_energy, 0., 0., 0.));

  /* Set the formation time of the resonance to the larger formation time of the
   * incoming particles, if it is larger than the execution time; execution time
   * is otherwise taken to be the formation time */
  const double t0 = incoming_particles_[0].formation_time();
  const double t1 = incoming_particles_[1].formation_time();

  const size_t index_tmax = (t0 > t1) ? 0 : 1;
  const double sc =
      incoming_particles_[index_tmax].cross_section_scaling_factor();
  if (t0 > time_of_execution_ || t1 > time_of_execution_) {
    outgoing_particles_[0].set_formation_time(std::max(t0, t1));
    outgoing_particles_[0].set_cross_section_scaling_factor(sc);
  } else {
    outgoing_particles_[0].set_formation_time(time_of_execution_);
  }
  log.debug("Momentum of the new particle: ",
            outgoing_particles_[0].momentum());
}

/* This function will generate outgoing particles in CM frame
 * from a hard process.
 * The way to excite soft strings is based on the UrQMD model */
void ScatterAction::string_excitation(bool is_soft_proc) {
  assert(incoming_particles_.size() == 2);
  const auto &log = logger<LogArea::Pythia>();
  // Disable floating point exception trap for Pythia
  {
    DisableFloatTraps guard;
    /* initialize the string_process_ object for this particular collision */
    string_process_->init(incoming_particles_, time_of_execution_, gamma_cm());
    /* implement collision */
    bool success = false;
    StringSoftType iproc = string_process_->get_subproc();
    int ntry = 0;
    const int ntry_max = 10000;
    while (!success && ntry < ntry_max) {
      ntry++;
      if (is_soft_proc) {
        switch (iproc) {
          case StringSoftType::SingleDiffAX:
            /* single diffractive to A+X */
            success = string_process_->next_SDiff(true);
            break;
          case StringSoftType::SingleDiffXB:
            /* single diffractive to X+B */
            success = string_process_->next_SDiff(false);
            break;
          case StringSoftType::DoubleDiff:
            /* double diffractive */
            success = string_process_->next_DDiff();
            break;
          case StringSoftType::NonDiff:
            /* soft non-diffractive */
            success = string_process_->next_NDiffSoft();
            break;
          case StringSoftType::None:
            success = false;
        }
      } else {
        success = string_process_->next_NDiffHard();
      }
    }
    if (ntry == ntry_max) {
      throw std::runtime_error("too many tries in string_excitation().");
    }
    outgoing_particles_ = string_process_->get_final_state();
    /* If the incoming particles already were unformed, the formation
     * times and cross section scaling factors need to be adjusted */
    const double tform_in = std::max(incoming_particles_[0].formation_time(),
                                     incoming_particles_[1].formation_time());
    if (tform_in > time_of_execution_) {
      const double fin =
          (incoming_particles_[0].formation_time() >
           incoming_particles_[1].formation_time())
              ? incoming_particles_[0].cross_section_scaling_factor()
              : incoming_particles_[1].cross_section_scaling_factor();
      for (size_t i = 0; i < outgoing_particles_.size(); i++) {
        const double tform_out = outgoing_particles_[i].formation_time();
        const double fout =
            outgoing_particles_[i].cross_section_scaling_factor();
        outgoing_particles_[i].set_cross_section_scaling_factor(fin * fout);
        /* If the unformed incoming particles' formation time is larger than
         * the current outgoing particle's formation time, then the latter
         * is overwritten by the former*/
        if (tform_in > tform_out) {
          outgoing_particles_[i].set_formation_time(tform_in);
        }
      }
    }
    /* Check momentum difference for debugging */
    FourVector out_mom;
    for (ParticleData data : outgoing_particles_) {
      out_mom += data.momentum();
    }
    log.debug("Incoming momenta string:", total_momentum());
    log.debug("Outgoing momenta string:", out_mom);
  }
}

void ScatterAction::format_debug_output(std::ostream &out) const {
  out << "Scatter of " << incoming_particles_;
  if (outgoing_particles_.empty()) {
    out << " (not performed)";
  } else {
    out << " to " << outgoing_particles_;
  }
}

}  // namespace smash
