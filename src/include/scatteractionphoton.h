/*
 *
 *    Copyright (c) 2016-2017
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#ifndef SRC_INCLUDE_SCATTERACTIONPHOTON_H_
#define SRC_INCLUDE_SCATTERACTIONPHOTON_H_

#include <algorithm>

#include "constants.h"
#include "scatteraction.h"

namespace Smash {

class ScatterActionPhoton : public ScatterAction {
 public:
  ScatterActionPhoton(const ParticleList &in, double time, int nofp)
      : ScatterAction(in[0], in[1], time),
        number_of_fractional_photons_(nofp) {}

  void generate_final_state() override;

  double raw_weight_value() const override { return weight_; }

  double cross_section() const override {
    return total_cross_section_;
  }

  /** Overridden to effectively return the reaction channel. */
  ProcessType get_type() const override {
    return static_cast<ProcessType>(reac);
  }

  /** Adds one dummy channel with a given cross-section. The intended use is to
   * add the hadronic cross-section from already performed hadronic action
   * without recomputing it. The photon action is never performed, so
   * this channel itself will never play any role. Only its cross-section will.
   */
  void add_dummy_hadronic_channels(double reaction_cross_section);

  /** To add only one reaction for testing purposes */
  void add_single_channel() {
    add_processes<CollisionBranch>(photon_cross_sections(),
                                   collision_channels_photons_,
                                   cross_section_photons_);
  }

  enum class ReactionType {
    no_reaction,
    pi_z_pi_p_rho_p,
    pi_z_pi_m_rho_m,
    pi_p_rho_z_pi_p,
    pi_m_rho_z_pi_m,
    pi_m_rho_p_pi_z,
    pi_p_rho_m_pi_z,
    pi_z_rho_p_pi_p,
    pi_z_rho_m_pi_m,
    pi_p_pi_m_rho_z,
    pi_z_rho_z_pi_z
  };

  ReactionType reac = ReactionType::no_reaction;

  /// Tells if the given incoming particles may produce photon
  //static ReactionType is_photon_reaction(const ParticleList &in);
  static ReactionType photon_reaction_type(const ParticleList &in);

  //static bool is_photon_reaction(const double s, const ParticleList &in);

 ParticleTypePtr outgoing_hadron(const ParticleList &in) const;

  bool is_kinematically_possible(const double s, const ParticleList &in);


 private:
  CollisionBranchList photon_cross_sections();

  int const number_of_fractional_photons_;

  double weight_ = 0.0;

  /** List of possible collisions producing photons */
  CollisionBranchList collision_channels_photons_;

  double cross_section_photons_ = 0.0;

  double diff_cross_section(double t, double t2, double t1) const;

  double form_factor(double E_photon);
};

}  // namespace Smash

#endif  // SRC_INCLUDE_SCATTERACTIONPHOTON_H_
