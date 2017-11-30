/*
 *
 *    Copyright (c) 2014-2017
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#ifndef SRC_INCLUDE_ACTION_H_
#define SRC_INCLUDE_ACTION_H_

#include <stdexcept>
#include <utility>
#include <vector>

#include "lattice.h"
#include "particles.h"
#include "pauliblocking.h"
#include "potentials.h"
#include "processbranch.h"
#include "random.h"

namespace smash {

/**
 * \ingroup action
 * Action is the base class for a generic process that takes a number of
 * incoming particles and transforms them into any number of outgoing particles.
 * Currently such an action can be either a decay or a two-body collision
 * (see derived classes).
 */
class Action {
 public:
  /**
   * Construct an action object.
   *
   * \param[in] in_part list of incoming particles
   * \param[in] time Time at which the action is supposed to take place
   *                 (relative to the current time of the incoming particles)
   */
  Action(const ParticleList &in_part, double time);

  Action(const ParticleData &in_part, const ParticleData &out_part, double time,
         ProcessType type)
      : incoming_particles_({in_part}),
        outgoing_particles_({out_part}),
        time_of_execution_(time + in_part.position().x0()),
        process_type_(type) {}

  Action(const ParticleList &in_part, const ParticleList &out_part,
         double absolute_execution_time, ProcessType type)
      : incoming_particles_(std::move(in_part)),
        outgoing_particles_(std::move(out_part)),
        time_of_execution_(absolute_execution_time),
        process_type_(type) {}

  /** Copying is disabled. Use pointers or create a new Action. */
  Action(const Action &) = delete;

  /** Virtual Destructor.
   * The declaration of the destructor is necessary to make it virtual.
   */
  virtual ~Action();

  /** For sorting by time of execution. */
  bool operator<(const Action &rhs) const {
    return time_of_execution_ < rhs.time_of_execution_;
  }

  /** Return the raw weight value, which is a cross section in case of a
   * ScatterAction, a decay width in case of a DecayAction and a shining
   * weight in case of a DecayActionDilepton.
   *
   * Prefer to use a more specific function.
   */
  virtual double raw_weight_value() const = 0;

  /** Return the specific weight for the chosen outgoing channel.
   *  For scatterings it will be partial cross-section, for
   *  decays - partial width, for dileptons - shining weight*branching.
   */
  virtual double partial_weight() const = 0;

  /** Return the process type. */
  virtual ProcessType get_type() const { return process_type_; }

  /** Add a new subprocess.  */
  template <typename Branch>
  void add_process(ProcessBranchPtr<Branch> &p,
                   ProcessBranchList<Branch> &subprocesses,
                   double &total_weight) {
    /* Evaluate the total kinentic energy of the final state particles
     * of this new subprocess. */
    const ThreeVector r = get_interaction_point().threevec();
    const auto out_particle_types = p->particle_types();
    const double kin_energy_cms = kinetic_energy_cms(r, out_particle_types);
    /* Reject the process if the total kinetic energy is smaller than the 
     * threshold. */
    if (p->weight() > 0 && kin_energy_cms > p->threshold()) {
      total_weight += p->weight();
      subprocesses.emplace_back(std::move(p));
    }
  }
  /** Add several new subprocesses at once.  */
  template <typename Branch>
  void add_processes(ProcessBranchList<Branch> pv,
                     ProcessBranchList<Branch> &subprocesses,
                     double &total_weight) {
    const ThreeVector r = get_interaction_point().threevec();
    subprocesses.reserve(subprocesses.size() + pv.size());
    for (auto &proc : pv) {
      /* Evaluate the total kinentic energy of the final state particles
       * of this new subprocess. */
      const auto out_particle_types = proc->particle_types();
      const double kin_energy_cms = kinetic_energy_cms(r, out_particle_types);
      /* Reject the process if the total kinetic energy is smaller than the 
       * threshold. */
      if (proc->weight() > 0 && kin_energy_cms > proc->threshold()) {
        total_weight += proc->weight();
        subprocesses.emplace_back(std::move(proc));
      }
    }
  }

  /**
   * Generate the final state for this action.
   *
   * This function selects a subprocess by Monte-Carlo decision and sets up
   * the final-state particles in phase space.
   */
  virtual void generate_final_state() = 0;

  /**
   * Actually perform the action, e.g. carry out a decay or scattering by
   * updating the particle list.
   *
   * This function removes the initial-state particles from the particle list
   * and then inserts the final-state particles. It does not do any sanity
   * checks, but assumes that is_valid has been called to determine if the
   * action is still valid.
   *
   * Note that you are required to increase id_process before the next call,
   * such that you get unique numbers.
   */
  virtual void perform(Particles *particles, uint32_t id_process);

  /**
   * Check whether the action still applies.
   *
   * It can happen that a different action removed the incoming_particles from
   * the set of existing particles in the experiment, or that the particle has
   * scattered elastically in the meantime. In this case the Action doesn't
   * apply anymore and should be discarded.
   */
  bool is_valid(const Particles &) const;

  /**
   *  Check if the action is Pauli-blocked. If there are baryons in the final
   *  state then blocking probability is \f$ 1 - \Pi (1-f_i) \f$, where
   *  product is taken by all fermions in the final state and \f$ f_i \f$
   *  denotes phase-space density at the position of i-th final-state
   *  fermion.
   */
  bool is_pauli_blocked(const Particles &, const PauliBlocker &) const;

  /**
   * Return the list of particles that go into the interaction.
   */
  const ParticleList &incoming_particles() const;

  /**
   * Update the incoming particles that are stored in this action to the state
   * they have in the global particle list.
   */
  void update_incoming(const Particles &particles);

  /**
   * Return the list of particles that resulted from the interaction.
   */
  const ParticleList &outgoing_particles() const { return outgoing_particles_; }

  /**
   * Return the time at which the action is supposed to be performed
   * (absolute time in the lab frame in fm/c).
   */
  double time_of_execution() const { return time_of_execution_; }

  /** Check various conservation laws.
   *
   * `id_process` is only used for debugging output. */
  void check_conservation(const uint32_t id_process) const;

  /// determine the total energy in the center-of-mass frame
  double sqrt_s() const { return total_momentum().abs(); }

  /**
   * Calculate the total kinetic energy of the outgoing particles in
   * the center of mass frame. This function is used when the species
   * of the outgoing particles are already determined.
   */
  double kinetic_energy_cms() const;

  /**
   * Calculate the total kinetic energy of the outgoing particles in
   * the center of mass frame. This function is used when the species
   * of the outgoing particles are not yet determined. They can be any
   * values in the possible branching processes.
   */
  double kinetic_energy_cms(ThreeVector r,
         ParticleTypePtrList p_out_types) const;

  /** Get the interaction point */
  FourVector get_interaction_point();

  /** Input the information on the potential */
  void input_potential(RectangularLattice<double> *UB_lat,
        RectangularLattice<double> *UI3_lat, Potentials *pot) {
        UB_lat_ = UB_lat;
        UI3_lat_ = UI3_lat;
        pot_ = pot;
  }

  /**
   * \ingroup exception
   * Thrown for example when ScatterAction is called to perform with a wrong
   * number of final-state particles or when the energy is too low to produce
   * the resonance.
   */
  class InvalidResonanceFormation : public std::invalid_argument {
    using std::invalid_argument::invalid_argument;
  };

 protected:
  /** List with data of incoming particles.  */
  ParticleList incoming_particles_;
  /**
   * Initially this stores only the PDG codes of final-state particles.
   *
   * After perform was called it contains the complete particle data of the
   * outgoing particles.
   */
  ParticleList outgoing_particles_;
  /**
   * Time at which the action is supposed to be performed
   * (absolute time in the lab frame in fm/c). */
  const double time_of_execution_;
  /** type of process */
  ProcessType process_type_;

  /// Sum of 4-momenta of incoming particles
  FourVector total_momentum() const {
    FourVector mom(0.0, 0.0, 0.0, 0.0);
    for (const auto &p : incoming_particles_) {
      mom += p.momentum();
    }
    return mom;
  }

  /**
   * Decide for a particular final-state channel via Monte-Carlo
   * and return it as a ProcessBranch
   */
  template <typename Branch>
  const Branch *choose_channel(const ProcessBranchList<Branch> &subprocesses,
                               double total_weight) {
    const auto &log = logger<LogArea::Action>();
    double random_weight = Random::uniform(0., total_weight);
    double weight_sum = 0.;
    /* Loop through all subprocesses and select one by Monte Carlo, based on
     * their weights.  */
    for (const auto &proc : subprocesses) {
      /* All processes apart from strings should have
       * a well-defined final state. */
      if (proc->particle_number() < 1 &&
          proc->get_type() != ProcessType::StringSoft &&
          proc->get_type() != ProcessType::StringHard) {
        continue;
      }
      weight_sum += proc->weight();
      if (random_weight <= weight_sum) {
        /* Return the full process information. */
        return proc.get();
      }
    }
    /* Should never get here. */
    log.fatal(source_location,
              "Problem in choose_channel: ", subprocesses.size(), " ",
              weight_sum, " ", total_weight, " ",
              //          random_weight, "\n", *this);
              random_weight, "\n");
    throw std::runtime_error("problem in choose_channel");
  }

  /**
   * Sample final-state masses in general X->2 processes
   * (thus also fixing the absolute c.o.m. momentum).
   *
   * \throws InvalidResonanceFormation
   */
  virtual std::pair<double, double> sample_masses() const;

  /**
   * Sample final-state angles in general X->2 processes
   * (here: using an isotropical angular distribution).
   */
  virtual void sample_angles(std::pair<double, double> masses);

  /**
   * Sample the full 2-body phase-space (masses, momenta, angles)
   * in the center-of-mass frame.
   */
  void sample_2body_phasespace();

  /**
   * \ingroup logging
   * Writes information about this action to the \p out stream.
   */
  virtual void format_debug_output(std::ostream &out) const = 0;

  /**
   * \ingroup logging
   * Dispatches formatting to the virtual Action::format_debug_output function.
   */
  friend std::ostream &operator<<(std::ostream &out, const Action &action) {
    action.format_debug_output(out);
    return out;
  }
 private:
  /** Pointer to the skyrme potential on the lattice */
  RectangularLattice<double> *UB_lat_ = nullptr;
  /** Pointer to the symmmetry potential on the lattice */
  RectangularLattice<double> *UI3_lat_ = nullptr;
  /** Pointer to a Potential class */
  Potentials *pot_ = nullptr;
};

inline std::vector<ActionPtr> &operator+=(std::vector<ActionPtr> &lhs,
                                          std::vector<ActionPtr> &&rhs) {
  if (lhs.size() == 0) {
    lhs = std::move(rhs);
  } else {
    lhs.insert(lhs.end(), std::make_move_iterator(rhs.begin()),
               std::make_move_iterator(rhs.end()));
  }
  return lhs;
}

/**
 * \ingroup logging
 * Convenience: dereferences the ActionPtr to Action.
 */
inline std::ostream &operator<<(std::ostream &out, const ActionPtr &action) {
  return out << *action;
}

/**
 * \ingroup logging
 * Writes multiple actions to the \p out stream.
 */
std::ostream &operator<<(std::ostream &out, const ActionList &actions);

}  // namespace smash

#endif  // SRC_INCLUDE_ACTION_H_
