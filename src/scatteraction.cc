/*
 *
 *    Copyright (c) 2014-2017
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "include/scatteraction.h"

#include "Pythia8/Pythia.h"
#include "include/stringprocess.h"

#include "include/constants.h"
#include "include/cxx14compat.h"
#include "include/fpenvironment.h"
#include "include/kinematics.h"
#include "include/logging.h"
#include "include/parametrizations.h"
#include "include/pdgcode.h"
#include "include/random.h"

//using namespace Pythia8;

Pythia8::Pythia *pythia;
Pythia8::SigmaTotal pythia_sigmaTot;

namespace Smash {

StringProcess *string_process;

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

  /* Decide for a particular final state. */
  const CollisionBranch *proc = choose_channel<CollisionBranch>(
      collision_channels_, total_cross_section_);
  process_type_ = proc->get_type();
  outgoing_particles_ = proc->particle_list();

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
    //case ProcessType::String:
    //  /* string excitation */
    //  string_excitation();
    //  break;
    case ProcessType::StringSDiffAX:
      /* string excitation single diffractive to AX */
      string_excitation_inter(1);
      break;
    case ProcessType::StringSDiffXB:
      /* string excitation single diffractive to XB */
      string_excitation_inter(2);
      break;
    case ProcessType::StringDDiffXX:
      /* string excitation single diffractive to XX */
      string_excitation_inter(3);
      break;
    case ProcessType::StringNDiff:
      /* string excitation non-diffractive */
      string_excitation_inter(4);
      break;
    default:
      throw InvalidScatterAction(
          "ScatterAction::generate_final_state: Invalid process type " +
          std::to_string(static_cast<int>(process_type_)) + " was requested. " +
          "(PDGcode1=" + incoming_particles_[0].pdgcode().string() +
          ", PDGcode2=" + incoming_particles_[1].pdgcode().string() + ")");
  }

  /* Set positions & boost to computational frame. */
  for (ParticleData &new_particle : outgoing_particles_) {
    if (proc->get_type() != ProcessType::Elastic) {
      new_particle.set_4position(middle_point);
    }
    new_particle.boost_momentum(-beta_cm());
  }
}

void ScatterAction::add_all_processes(double elastic_parameter, bool two_to_one,
                                      bool two_to_two, double low_snn_cut,
                                      bool strings_switch,
                                      NNbarTreatment nnbar_treatment) {
  /* The string fragmentation is implemented in the same way in GiBUU (Physics
   * Reports 512(2012), 1-124, pg. 33). If the center of mass energy is low, two
   * particles scatter through the resonance channels. If high, the out going
   * particles are generated by string fragmentation. If in between, the out
   * going
   * particles are generated either through the resonance channels or string
   * fragmentation by chance. In detail, the low energy regoin is from the
   * threshold to (mix_scatter_type_energy - mix_scatter_type_window_width),
   * while
   * the high energy region is from (mix_scatter_type_energy +
   * mix_scatter_type_window_width) to infinity. In between, the probability for
   * string fragmentation increases linearly from 0 to 1 as the c.m. energy.*/
  // Determine the energy region of the mixed scattering type for two types of
  // scattering.
  const ParticleType &t1 = incoming_particles_[0].type();
  const ParticleType &t2 = incoming_particles_[1].type();
  const bool both_are_nucleons = t1.is_nucleon() && t2.is_nucleon();
  bool include_pythia = false;
  double mix_scatter_type_energy;
  double mix_scatter_type_window_width;
  if (both_are_nucleons) {
    // The energy region of the mixed scattering type for nucleon-nucleon
    // collision is 4.0 - 5.0 GeV.
    mix_scatter_type_energy = 4.5;
    mix_scatter_type_window_width = 0.5;
    // nucleon-nucleon collisions are included in pythia.
    include_pythia = true;
  } else if ((t1.pdgcode().is_pion() && t2.is_nucleon()) ||
             (t1.is_nucleon() && t2.pdgcode().is_pion())) {
    // The energy region of the mixed scattering type for pion-nucleon collision
    // is 2.3 - 3.1 GeV.
    mix_scatter_type_energy = 2.7;
    mix_scatter_type_window_width = 0.4;
    // pion-nucleon collisions are included in pythia.
    include_pythia = true;
  }
  // string fragmentation is enabled when strings_switch is on and the process
  // is included in pythia.
  const bool enable_pythia = strings_switch && include_pythia;
  // Whether the scattering is through string fragmentaion
  bool is_pythia = false;
  if (enable_pythia) {
    if (sqrt_s() > mix_scatter_type_energy + mix_scatter_type_window_width) {
      // scatterings at high energies are through string fragmentation
      is_pythia = true;
    } else if (sqrt_s() >
               mix_scatter_type_energy - mix_scatter_type_window_width) {
      const double probability_pythia =
          (sqrt_s() - mix_scatter_type_energy + mix_scatter_type_window_width) /
          mix_scatter_type_window_width / 2.0;
      if (probability_pythia > Random::uniform(0., 1.)) {
        // scatterings at the middle energies are through string
        // fragmentation by chance.
        is_pythia = true;
      }
    }
  }
  /** Elastic collisions between two nucleons with sqrt_s() below
   * low_snn_cut can not happen*/
  const bool reject_by_nucleon_elastic_cutoff =
      both_are_nucleons && t1.antiparticle_sign() == t2.antiparticle_sign() &&
      sqrt_s() < low_snn_cut;
  if (two_to_two && !reject_by_nucleon_elastic_cutoff) {
    add_collision(elastic_cross_section(elastic_parameter));
  }
  if (is_pythia) {
    //add_collision(string_excitation_cross_section());
    add_collisions(string_excitation_cross_sections());
  } else {
    if (two_to_one) {
      /* resonance formation (2->1) */
      add_collisions(resonance_cross_sections());
    }
    if (two_to_two) {
      /* 2->2 (inelastic) */
      add_collisions(two_to_two_cross_sections());
    }
  }
  /** NNbar annihilation thru NNbar → ρh₁(1170); combined with the decays
   *  ρ → ππ and h₁(1170) → πρ, this gives a final state of 5 pions.
   *  Only use in cases when detailed balance MUST happen, i.e. in a box! */
  if (nnbar_treatment == NNbarTreatment::Resonances) {
    if (t1.is_nucleon() && t2.pdgcode() == t1.get_antiparticle()->pdgcode()) {
      add_collision(NNbar_annihilation_cross_section());
    }
    if ((t1.pdgcode() == pdg::rho_z && t2.pdgcode() == pdg::h1) ||
        (t1.pdgcode() == pdg::h1 && t2.pdgcode() == pdg::rho_z)) {
      add_collisions(NNbar_creation_cross_section());
    }
  }
}

double ScatterAction::raw_weight_value() const { return total_cross_section_; }

ThreeVector ScatterAction::beta_cm() const {
  return (incoming_particles_[0].momentum() + incoming_particles_[1].momentum())
      .velocity();
}

double ScatterAction::gamma_cm() const {
  return (1. / std::sqrt(1 - beta_cm().sqr()));
}

double ScatterAction::mandelstam_s() const {
  return (incoming_particles_[0].momentum() + incoming_particles_[1].momentum())
      .sqr();
}

double ScatterAction::sqrt_s() const {
  return (incoming_particles_[0].momentum() + incoming_particles_[1].momentum())
      .abs();
}

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

CollisionBranchPtr ScatterAction::elastic_cross_section(double elast_par) {
  double elastic_xs;
  if (elast_par >= 0.) {
    // use constant elastic cross section from config file
    elastic_xs = elast_par;
  } else {
    // use parametrization
    elastic_xs = elastic_parametrization();
  }
  return make_unique<CollisionBranch>(incoming_particles_[0].type(),
                                      incoming_particles_[1].type(), elastic_xs,
                                      ProcessType::Elastic);
}

CollisionBranchPtr ScatterAction::NNbar_annihilation_cross_section() {
  const auto &log = logger<LogArea::ScatterAction>();
  /* Calculate NNbar cross section:
   * Parametrized total minus all other present channels.*/
  double nnbar_xsec = std::max(0., total_cross_section() - cross_section());
  log.debug("NNbar cross section is: ", nnbar_xsec);
  // Make collision channel NNbar -> ρh₁(1170); eventually decays into 5π
  return make_unique<CollisionBranch>(ParticleType::find(pdg::h1),
                                      ParticleType::find(pdg::rho_z),
                                      nnbar_xsec, ProcessType::TwoToTwo);
}

CollisionBranchList ScatterAction::NNbar_creation_cross_section() {
  const auto &log = logger<LogArea::ScatterAction>();
  CollisionBranchList channel_list;
  /* Calculate NNbar reverse cross section:
   * from reverse reaction (see NNbar_annihilation_cross_section).*/
  const double s = mandelstam_s();
  const double sqrts = sqrt_s();
  const double pcm = cm_momentum();

  const auto &type_N = ParticleType::find(pdg::p);
  const auto &type_Nbar = ParticleType::find(-pdg::p);

  // Check available energy
  if (sqrts - 2 * type_N.mass() < 0) {
    return channel_list;
  }

  double xsection = detailed_balance_factor_RR(
                        sqrts, pcm, incoming_particles_[0].type(),
                        incoming_particles_[1].type(), type_N, type_Nbar) *
                    std::max(0., ppbar_total(s) - ppbar_elastic(s));
  log.debug("NNbar reverse cross section is: ", xsection);
  channel_list.push_back(make_unique<CollisionBranch>(
      type_N, type_Nbar, xsection, ProcessType::TwoToTwo));
  channel_list.push_back(make_unique<CollisionBranch>(
      ParticleType::find(pdg::n), ParticleType::find(-pdg::n), xsection,
      ProcessType::TwoToTwo));
  return channel_list;
}

CollisionBranchPtr ScatterAction::string_excitation_cross_section() {
  const auto &log = logger<LogArea::ScatterAction>();
  /* Calculate string-excitation cross section:
   * Parametrized total minus all other present channels. */
  double sig_string =
      std::max(0., high_energy_cross_section() - elastic_parametrization());
  log.debug("String cross section is: ", sig_string);
  return make_unique<CollisionBranch>(sig_string, ProcessType::String);
}

CollisionBranchList ScatterAction::string_excitation_cross_sections() {
  const auto &log = logger<LogArea::ScatterAction>();
  CollisionBranchList channel_list;
  /* Calculate string-excitation cross section:
   * Parametrized total minus all other present channels. */
  double sig_string_all =
      std::max(0., high_energy_cross_section() - elastic_parametrization());
  int pdgidA = incoming_particles_[0].type().pdgcode().get_decimal();
  int pdgidB = incoming_particles_[1].type().pdgcode().get_decimal();
  if ( pdgidA > 0 ) pdgidA = pdgidA%10000;
  else pdgidA = - ( std::abs(pdgidA)%10000 );
  if ( pdgidB > 0 ) pdgidB = pdgidB%10000;
  else pdgidB = - ( std::abs(pdgidB)%10000 );
  int idAbsA = std::abs(pdgidA);
  int idAbsB = std::abs(pdgidB);
  int idModA = (idAbsA > 1000) ? idAbsA : 10 * (idAbsA/10) + 3;
  int idModB = (idAbsB > 1000) ? idAbsB : 10 * (idAbsB/10) + 3;
  double sqrts = sqrt_s();
  double sqrts_threshold = pythia->particleData.m0(idModA)
      + pythia->particleData.m0(idModB) + 2.*(1. + 1.0e-6);
  /* Compute the parametrized cross sections of relevant subprocesses. */
  double sig_sd_AX, sig_sd_XB, sig_dd_XX, sig_nd;
  if( sqrts > sqrts_threshold ) pythia_sigmaTot.calc(pdgidA, pdgidB, sqrts);
  else pythia_sigmaTot.calc(pdgidA, pdgidB, sqrts_threshold);
  double sig_sd_all = pythia_sigmaTot.sigmaAX() + pythia_sigmaTot.sigmaXB();
  double sig_diff = sig_sd_all + pythia_sigmaTot.sigmaXX();
  sig_nd = std::max( 0., sig_string_all - sig_diff );
  sig_diff = sig_string_all - sig_nd;
  sig_dd_XX = std::max( 0., sig_diff - sig_sd_all );
  sig_sd_AX = ( sig_diff - sig_dd_XX )*pythia_sigmaTot.sigmaAX()/sig_sd_all;
  sig_sd_XB = ( sig_diff - sig_dd_XX )*pythia_sigmaTot.sigmaXB()/sig_sd_all;
  log.debug("String cross section (single-diffractive AB->AX) is: ", sig_sd_AX);
  log.debug("String cross section (single-diffractive AB->XB) is: ", sig_sd_XB);
  log.debug("String cross section (double-diffractive AB->XX) is: ", sig_dd_XX);
  log.debug("String cross section (non-diffractive) is: ", sig_nd);
  /* fill the list of process channels */
  channel_list.push_back(make_unique<CollisionBranch>( sig_sd_AX,
      ProcessType::StringSDiffAX) );
  channel_list.push_back(make_unique<CollisionBranch>( sig_sd_XB,
      ProcessType::StringSDiffXB) );
  channel_list.push_back(make_unique<CollisionBranch>( sig_dd_XX,
      ProcessType::StringDDiffXX) );
  channel_list.push_back(make_unique<CollisionBranch>( sig_nd,
      ProcessType::StringNDiff) );
  return channel_list;
}

double ScatterAction::two_to_one_formation(const ParticleType &type_resonance,
                                           double srts,
                                           double cm_momentum_sqr) {
  const ParticleType &type_particle_a = incoming_particles_[0].type();
  const ParticleType &type_particle_b = incoming_particles_[1].type();
  /* Check for charge conservation. */
  if (type_resonance.charge() !=
      type_particle_a.charge() + type_particle_b.charge()) {
    return 0.;
  }

  /* Check for baryon-number conservation. */
  if (type_resonance.baryon_number() !=
      type_particle_a.baryon_number() + type_particle_b.baryon_number()) {
    return 0.;
  }

  /* Calculate partial in-width. */
  const double partial_width = type_resonance.get_partial_in_width(
      srts, incoming_particles_[0], incoming_particles_[1]);
  if (partial_width <= 0.) {
    return 0.;
  }

  /* Calculate spin factor */
  const double spinfactor =
      static_cast<double>(type_resonance.spin() + 1) /
      ((type_particle_a.spin() + 1) * (type_particle_b.spin() + 1));
  const int sym_factor =
      (type_particle_a.pdgcode() == type_particle_b.pdgcode()) ? 2 : 1;
  /** Calculate resonance production cross section
   * using the Breit-Wigner distribution as probability amplitude.
   * See Eq. (176) in \iref{Buss:2011mx}. */
  return spinfactor * sym_factor * 2. * M_PI * M_PI / cm_momentum_sqr *
         type_resonance.spectral_function(srts) * partial_width * hbarc *
         hbarc / fm2_mb;
}

CollisionBranchList ScatterAction::resonance_cross_sections() {
  const auto &log = logger<LogArea::ScatterAction>();
  CollisionBranchList resonance_process_list;
  const ParticleType &type_particle_a = incoming_particles_[0].type();
  const ParticleType &type_particle_b = incoming_particles_[1].type();

  const double srts = sqrt_s();
  const double p_cm_sqr = cm_momentum_squared();

  /* Find all the possible resonances */
  for (const ParticleType &type_resonance : ParticleType::list_all()) {
    /* Not a resonance, go to next type of particle */
    if (type_resonance.is_stable()) {
      continue;
    }

    /* Same resonance as in the beginning, ignore */
    if ((!type_particle_a.is_stable() &&
         type_resonance.pdgcode() == type_particle_a.pdgcode()) ||
        (!type_particle_b.is_stable() &&
         type_resonance.pdgcode() == type_particle_b.pdgcode())) {
      continue;
    }

    double resonance_xsection =
        two_to_one_formation(type_resonance, srts, p_cm_sqr);

    /* If cross section is non-negligible, add resonance to the list */
    if (resonance_xsection > really_small) {
      resonance_process_list.push_back(make_unique<CollisionBranch>(
          type_resonance, resonance_xsection, ProcessType::TwoToOne));
      log.debug("Found resonance: ", type_resonance);
      log.debug("2->1 with original particles: ", type_particle_a,
                type_particle_b);
    }
  }
  return resonance_process_list;
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

  /* 1 particle in final state: Center-of-momentum frame of initial particles
   * is the rest frame of the resonance.  */
  outgoing_particles_[0].set_4momentum(FourVector(sqrt_s(), 0., 0., 0.));

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
 * from a hard process. */
void ScatterAction::string_excitation() {
  assert(incoming_particles_.size() == 2);
  const auto &log = logger<LogArea::Pythia>();
  // Disable doubleing point exception trap for Pythia
  {
    bool isinit = false;
    bool isnext = false;
    DisableFloatTraps guard;
    /* set all necessary parameters for Pythia
     * Create Pythia object */
    log.debug("Creating Pythia object.");
    //static /*thread_local (see #3075)*/ Pythia8::Pythia pythia(PYTHIA_XML_DIR,
    //                                                           false);
    /* select only inelastic events: */
    pythia->readString("SoftQCD:inelastic = on");
    /* suppress unnecessary output */
    pythia->readString("Print:quiet = on");
    /* Create output of the Pythia particle list */
    // pythia.readString("Init:showAllParticleData = on");
    /* No resonance decays, since the resonances will be handled by SMASH */
    pythia->readString("HadronLevel:Decay = off");
    /* manually set the parton distribution function */
    pythia->readString("PDF:pSet = 13");
    pythia->readString("PDF:pSetB = 13");
    pythia->readString("PDF:piSet = 1");
    pythia->readString("PDF:piSetB = 1");
    /* Set the random seed of the Pythia Random Number Generator.
     * Please note: Here we use a random number generated by the
     * SMASH, since every call of pythia.init should produce
     * different events. */
    pythia->readString("Random:setSeed = on");
    std::stringstream buffer1;
    buffer1 << "Random:seed = " << Random::canonical();
    pythia->readString(buffer1.str());
    /* set the incoming particles */
    std::stringstream buffer2;
    buffer2 << "Beams:idA = " << incoming_particles_[0].type().pdgcode();
    pythia->readString(buffer2.str());
    log.debug("First particle in string excitation: ",
              incoming_particles_[0].type().pdgcode());
    std::stringstream buffer3;
    buffer3 << "Beams:idB = " << incoming_particles_[1].type().pdgcode();
    log.debug("Second particle in string excitation: ",
              incoming_particles_[1].type().pdgcode());
    pythia->readString(buffer3.str());
    /* Calculate the center-of-mass energy of this collision */
    double sqrts =
        (incoming_particles_[0].momentum() + incoming_particles_[1].momentum())
            .abs();
    std::stringstream buffer4;
    buffer4 << "Beams:eCM = " << sqrts;
    pythia->readString(buffer4.str());
    log.debug("Pythia call with eCM = ", buffer4.str());
    /* Initialize. */
    isinit = pythia->init();
    /* Short notation for Pythia event */
    Pythia8::Event &event = pythia->event;
    while ((isinit == true) && (isnext == false)) {
      isnext = pythia->next();

      if (isnext == true) {
        /* energy-momentum conservation check */
        double Efin = 0.;
        double pxfin = 0.;
        double pyfin = 0.;
        double pzfin = 0.;
        for (int i = 0; i < event.size(); i++) {
          if (event[i].isFinal()) {
            if (event[i].isHadron()) {
              Efin = Efin + event[i].e();
              pxfin = pxfin + event[i].px();
              pyfin = pyfin + event[i].py();
              pzfin = pzfin + event[i].pz();
            }
          }
        }
        if (fabs(Efin - sqrts) > 1e-8 * sqrts) {
          isnext = false;
        }
        if (fabs(pxfin) > 1e-8 * sqrts) {
          isnext = false;
        }
        if (fabs(pyfin) > 1e-8 * sqrts) {
          isnext = false;
        }
        if (fabs(pzfin) > 1e-8 * sqrts) {
          isnext = false;
        }
        /* if conservation is violated, try again */
      }
    }
    ParticleList new_intermediate_particles;
    for (int i = 0; i < event.size(); i++) {
      if (event[i].isFinal()) {
        if (event[i].isHadron()) {
          int pythia_id = event[i].id();
          log.debug("PDG ID from Pythia:", pythia_id);
          /* K_short and K_long need to be converted to K0
           * since SMASH only knows K0 */
          if (pythia_id == 310 || pythia_id == 130) {
            const double prob = Random::uniform(0., 1.);
            if (prob <= 0.5) {
              pythia_id = 311;
            } else {
              pythia_id = -311;
            }
          }
          const std::string s = std::to_string(pythia_id);
          PdgCode pythia_code(s);
          ParticleData new_particle(ParticleType::find(pythia_code));
          FourVector momentum;
          momentum.set_x0(event[i].e());
          momentum.set_x1(event[i].px());
          momentum.set_x2(event[i].py());
          momentum.set_x3(event[i].pz());
          new_particle.set_4momentum(momentum);
          log.debug("4-momentum from Pythia: ", momentum);
          new_intermediate_particles.push_back(new_particle);
        }
      }
    }
    /*
     * sort new_intermediate_particles according to z-Momentum
     */
    std::sort(new_intermediate_particles.begin(),
              new_intermediate_particles.end(),
              [&](ParticleData i, ParticleData j) {
                return std::fabs(i.momentum().x3()) > std::fabs(j.momentum().x3());
              });
    for (ParticleData data : new_intermediate_particles) {
      log.debug("Particle momenta after sorting: ", data.momentum());
      /* The hadrons are not immediately formed, currently a formation time of
       * 1 fm is universally applied and cross section is reduced to zero and
       * to a fraction corresponding to the valence quark content. Hadrons
       * containing a valence quark are determined by highest z-momentum. */
      log.debug("The formation time is: ", string_formation_time_, "fm/c.");
      /* Additional suppression factor to mimic coherence taken as 0.7
       * from UrQMD (CTParam(59) */
      const double suppression_factor = 0.7;
      if (incoming_particles_[0].is_baryon() ||
          incoming_particles_[1].is_baryon()) {
        if (data == 0) {
          data.set_cross_section_scaling_factor(suppression_factor * 0.66);
        } else if (data == 1) {
          data.set_cross_section_scaling_factor(suppression_factor * 0.34);
        } else {
          data.set_cross_section_scaling_factor(suppression_factor * 0.0);
        }
      } else {
        if (data == 0 || data == 1) {
          data.set_cross_section_scaling_factor(suppression_factor * 0.50);
        } else {
          data.set_cross_section_scaling_factor(suppression_factor * 0.0);
        }
      }
      // Set formation time: actual time of collision + time to form the
      // particle
      data.set_formation_time(string_formation_time_ * gamma_cm() +
                              time_of_execution_);
      outgoing_particles_.push_back(data);
    }
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
    FourVector in_mom =
        incoming_particles_[0].momentum() + incoming_particles_[1].momentum();
    FourVector out_mom;
    for (ParticleData data : outgoing_particles_) {
      out_mom = out_mom + data.momentum();
    }
    log.debug("Incoming momenta string:", in_mom);
    log.debug("Outgoing momenta string:", out_mom);
  }
}

/* This function will generate outgoing particles in CM frame
 * from a hard process.
 * The way to excite strings is based on the UrQMD model */
void ScatterAction::string_excitation_inter(int subprocess) {
  assert(incoming_particles_.size() == 2);
  const auto &log = logger<LogArea::Pythia>();
  // Disable doubleing point exception trap for Pythia
  {
    bool isinit = false;
    bool isnext = false;
    DisableFloatTraps guard;
    /* compute sqrts */
    double sqrts =
        (incoming_particles_[0].momentum() + incoming_particles_[1].momentum())
            .abs();
    /* transverse momentum transferred to strings */
    double sigQperp;
    /* parametrization to fit the experimental data */
    if (sqrts < 4.) {
      sigQperp = 0.5;
    } else {
      sigQperp = 0.5 + 0.2 * std::log(sqrts / 4.) / std::log(5.);
    }
    /* initialize the string_process object */
    string_process->set_sigma_qperp(sigQperp);
    string_process->set_pmin_gluon_lightcone(0.001);
    isinit =
        string_process->init(incoming_particles_,
                             time_of_execution_, gamma_cm());
    /* implement collision */
    while ((isinit == true) && (isnext == false)) {
      if( subprocess == 1 ) {
        isnext = string_process->next_SDiff(1);
      } else if( subprocess == 2 ) {
        isnext = string_process->next_SDiff(2);
      } else if( subprocess == 3 ) {
        isnext = string_process->next_DDiff();
      } else if( subprocess == 4 ) {
        isnext = string_process->next_NDiff();
      } else {
        throw std::runtime_error("invalid subprocess of StringProcess");
      }
    }
    int npart = string_process->final_state.size();
    for(int ipart = 0; ipart < npart; ipart++){
      outgoing_particles_.push_back( string_process->final_state[ipart] );
    }
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
    FourVector in_mom =
        incoming_particles_[0].momentum() + incoming_particles_[1].momentum();
    FourVector out_mom;
    for (ParticleData data : outgoing_particles_) {
      out_mom = out_mom + data.momentum();
    }
    log.debug("Incoming momenta string:", in_mom);
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

}  // namespace Smash
