/*
 *    Copyright (c) 2013-2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 */
#ifndef SRC_INCLUDE_PROCESSBRANCH_H_
#define SRC_INCLUDE_PROCESSBRANCH_H_

#include "forwarddeclarations.h"
#include "particletype.h"

#include <vector>

namespace Smash {


/**
 * \ingroup data
 *
 * ProcessBranch represents one possible final state
 * of an interaction process.
 *
 * Each final state has two components;
 * 1. The list of particle types present in this state.
 * 2. The weight of this state, i.e. how probable this outcome is
 * compared to other possible outcomes. Depending on context,
 * this can be either a cross section or a branching ratio.
 * 3. The process id that identifies a certain class of processes.
 * If the outgoing particles are not known yet, e.g. for strings
 * there will be only the weight and the process id.
 *
 * For example, create a list of decay modes for \f$\Delta^+\f$ resonance:
 * \code
 * std::vector<ProcessBranch> deltaplus_decay_modes;
 * ProcessBranch branch;
 * branch.set_weight(2);
 * deltaplus_decay_modes.push_back(branch);
 * // set_weight erases the previous weight
 * branch.set_weight(1);
 * deltaplus_decay_modes.push_back(branch);
 * \endcode
 */
class ProcessBranch {
 public:
 /** Process Types are used to identify the type of the process, 
   * currently we have 5 of these: 
   * (1) elastic (ELASTIC)
   * (2) resonance formation (2->1) (TWO_TO_ONE) 
   * (3) 2->2 (inelastic) (TWO_TO_TWO)
   * (4) string excitation (STRING)
   * (5) resonance decays (DECAY) */  
  enum ProcessType {
	  NONE=0,
	  ELASTIC=1, 
	  TWO_TO_ONE=2, 
	  TWO_TO_TWO=3, 
	  STRING=4, 
	  DECAY=5
   };  
  /// Create a ProcessBranch without final states
  ProcessBranch() : branch_weight_(0.0), process_type_(NONE) {}
  /// Constructor without outgoing particles
  ProcessBranch(float w, ProcessType p_type) : branch_weight_(w), process_type_(p_type) {}

  /// Constructor with 1 particle
  ProcessBranch(const ParticleType &type, float w, ProcessType p_type) 
      : branch_weight_(w), process_type_(p_type) {
    particle_types_.reserve(1);
    particle_types_.push_back(&type);
  }

  /// Constructor with 2 particles
  ProcessBranch(const ParticleType &type_a, const ParticleType &type_b, float w, ProcessType p_type)
      : branch_weight_(w), process_type_(p_type) {
    particle_types_.reserve(2);
    particle_types_.push_back(&type_a);
    particle_types_.push_back(&type_b);
  }

  /// Create a ProcessBranch from a list of ParticleType objects
  ProcessBranch(std::vector<ParticleTypePtr> new_types, float w, ProcessType p_type)
      : particle_types_(std::move(new_types)), branch_weight_(w), process_type_(p_type) {}

  /// Copying is disabled. Use std::move or create a new object.
  ProcessBranch(const ProcessBranch &) = delete;

  /// The move constructor efficiently moves the PDG list member.
  ProcessBranch(ProcessBranch &&rhs)
      : particle_types_(std::move(rhs.particle_types_)),
        branch_weight_(rhs.branch_weight_),
        process_type_(rhs.process_type_) {}

  /**
   * Set the weight of the branch.
   * In other words, how probable this branch is
   * compared to other branches
   */
  inline void set_weight(float process_weight);
  /** Set the Process ID */
  inline void set_type(enum ProcessType);
  /// Return the process type
  inline ProcessType get_type(void) const;
  /// Clear all information from the branch
  inline void clear(void);
  /// Return the particle types associated with this branch.
  const std::vector<ParticleTypePtr> &particle_types() const {
    return particle_types_;
  }

  /**
   * Return a list of ParticleData initialized with the stored ParticleType
   * objects.
   */
  ParticleList particle_list() const;

  /// Return the branch weight
  inline float weight(void) const;
  
  
  /**
   * Determine the threshold for this branch, i.e. the minimum energy that is
   * required to produce all final-state particles.
   */
  float threshold() const;

 private:
  /**
   * List of particles appearing in this process outcome.
   *
   * \todo Is there a maximum to the number of particles? If not, a std::vector
   * is fine, otherwise (I assume 4 may be a useful limit?) switch to
   * std::array<int, 4>. std::vector<int> stores one size_t and one pointer,
   * which is as big as 4 ints. And then there's still the data of the vector
   * which is somewhere on the heap. Also the alignment of ints is only half
   * that of size_t/void*. (I was obviously talking about 64bit here...)
   */
  std::vector<ParticleTypePtr> particle_types_;
  /// Weight of the branch, typically a cross section or a branching ratio
  float branch_weight_; 
  /// Process type internal variable
  ProcessType process_type_;

};

/**
 * Set the weight of the branch.
 * In other words, how probable this branch is
 * compared to other branches
 */
inline void ProcessBranch::set_weight(float process_weight) {
  branch_weight_ = process_weight;
}

/// Clear all information from the branch
inline void ProcessBranch::clear(void) {
  particle_types_.clear();
  branch_weight_ = -1.0;
}

/// Return the branch weight
inline float ProcessBranch::weight(void) const {
  return branch_weight_;
}

/** Set the process type */
inline void ProcessBranch::set_type(ProcessType p_type) {
  process_type_ = p_type;	
}

/** Return the process type */
inline ProcessBranch::ProcessType ProcessBranch::get_type() const {
  return process_type_;	
}

/** \relates ProcessBranch
 * Calculates the total weight by summing all weights of the ProcessBranch
 * objects in the list \p l.
 */
inline float total_weight(const ProcessBranchList &l) {
  float sum = 0.f;
  for (const auto &p : l) {
    sum += p.weight();
  }
  return sum;
}

/**
 * \ingroup data
 *
 * DecayBranch is a derivative of ProcessBranch,
 * which is used to represent decay channels.
 * It contains additional information like the angular momentum.
 */
class DecayBranch : public ProcessBranch {
 public:
  template <typename... Ts>
  DecayBranch(int l, Ts &&... args)
      : ProcessBranch{std::forward<Ts>(args)...}, angular_momentum_(l) {}

  /// Get the angular momentum of this branch.
  inline int angular_momentum() const;
  /** Set the angular momentum of this branch.
   * \param[in] L new value of angular momentum
   */
  inline void set_angular_momentum(const int L);
 private:
  /// Angular momentum of final-state particles in this branch.
  int angular_momentum_;
};

inline int DecayBranch::angular_momentum() const {
  return angular_momentum_;
};

inline void DecayBranch::set_angular_momentum(const int L)
{
  angular_momentum_ = L;
};

}  // namespace Smash

#endif  // SRC_INCLUDE_PROCESSBRANCH_H_
