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

#include "scatteraction.h"

namespace smash {

/**
 * \ingroup action
 * ScatterActionPhoton is a special action which takes two incoming particles
 * and performs an perturbative electromagnetic scattering.
 * The final state particles are not further propagated, only written
 * to output.
 */

class ScatterActionPhoton : public ScatterAction {
 public:
  /**
   * Construct a ScatterActionPhoton object.
   *
   * \param[in] in ParticleList of incoming particles
   * \param[in] time Time relative to underlying hadronic action.
   * \param[in] n_fp Number of photons to produce for each hadronic scattering
   */

  ScatterActionPhoton(const ParticleList &in, double time, int nofp)
      : ScatterAction(in[0], in[1], time),
        number_of_fractional_photons_(nofp),
        hadron_out_t_(outgoing_hadron_type(in)) {
    reac_ = photon_reaction_type(in);
    hadron_out_mass_ = sample_out_hadron_mass(hadron_out_t_);
  }

  /**
   * Create photons and write to output
   */
  void perform_photons(const OutputsList &outputs);

  /**
   * Generate the final-state for the photon scatter process. Generates only one
   * photon / hadron pair
   */
  void generate_final_state() override;

  /**
   * Return the weight of the produced photon.
   */
  double raw_weight_value() const override { return weight_; }

  /**
   * Return the total cross section of the underlying hadronic scattering
   */
  double cross_section() const override { return total_cross_section_; }

  /**
   * Sample the mass of the outgoing hadron. Returns the pole mass if
   * particle is stable.
   *
   */
  double sample_out_hadron_mass(const ParticleTypePtr out_type);

  /**
   * Adds one hadronic channel with a given cross-section. The intended use is
   * to add the hadronic cross-section from already performed hadronic action
   * without recomputing it.
   */
  void add_dummy_hadronic_channels(double reaction_cross_section);

  /**
   * Add the photonic channel. Also compute the total cross section.
   */

  void add_single_channel() {
    add_processes<CollisionBranch>(photon_cross_sections(),
                                   collision_channels_photons_,
                                   cross_section_photons_);
  }

  /**
   * Enum for encoding the reaction channel. It is uniquely determined by the
   * incoming particles.
   */

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

  /*
   * Returns reaction channel from incoming particles. If
   * incoming particles are not part of any implemented photonic process, return
   * no_reaction
   */

  static ReactionType photon_reaction_type(const ParticleList &in);

  static bool is_photon_reaction(const ParticleList &in) {
    return photon_reaction_type(in) != ReactionType::no_reaction;
  }

  /*
   * Return ParticleTypePtr of hadron in out channel, given the incoming
   * particles. This function is overloaded since we need the hadron type in
   * different places. In some cases the ReactionType member reac_ is already
   * set, in others it is not. Having an overloaded function avoids unnecessary
   * calls to photon_reaction_type.
   */
  static ParticleTypePtr outgoing_hadron_type(const ParticleList &in);
  static ParticleTypePtr outgoing_hadron_type(const ReactionType reaction);

  /*
   *  Check if CM-energy is sufficient to produce hadron in final state.
   */
  static bool is_kinematically_possible(const double s, const ParticleList &in);

 private:
  /*
   * Holds the photon branch. As of now, this will always
   * hold only one branch.
   */
  CollisionBranchList collision_channels_photons_;

  /// Photonic process as determined from incoming particles.
  ReactionType reac_;

  /* Number of photons created for each hadronic scattering, needed for correct
   * weighting. Note that in generate_final_state() only one photon + hadron is
   * created.
   */
  const int number_of_fractional_photons_;

  ParticleTypePtr hadron_out_t_;

  /// Mass of outgoing hadron
  double hadron_out_mass_;

  /*
   * Compile-time switch for setting the handling of processes which can happen
   * via different mediating particles. Relevant only for the processes
   * pi0 + rho => pi + y and pi + rho => pi0 + gamma, which both can happen
   * via exchange of (rho, a1, pi) or omega.
   * If MediatorType::SUM is set, the cross section for both channels is added.
   * If MediatorType::PION/ OMEGA is set, only the respective channels are
   * computed.
   */

  enum class MediatorType { SUM, PION, OMEGA };
  static constexpr MediatorType default_mediator_ = MediatorType::SUM;

  /// Weight of the produced photon.
  double weight_ = 0.0;

  /// Total cross section of photonic process.
  double cross_section_photons_ = 0.0;

  /**
   * Calculate the differential cross section of photon process.
   * Formfactors are not included
   *
   * \param t Mandelstam-t
   * \param m_rho Mass of the incoming or outgoing rho-particle
   * \param mediator Switch for determing which mediating particle to use
   *
   * \returns differential cross section. [mb/\f$GeV^2\f$]
   */
  double diff_cross_section(const double t, const double m_rho,
                            MediatorType mediator = default_mediator_) const;

  /**
   * Find the mass of the participating rho-particle. In case of
   * an rho in the incoming channel it is the mass of the incoming rho, in case
   * of an rho in the outgoing channel it is the mass sampled in the
   * constructor. When an rho acts in addition as a mediator, its mass is the
   * same as the incoming / outgoing rho.
   */
  double rho_mass() const;

  /**
   * Computes the total cross section of the photon process.
   *
   * \returns List of photon reaction branches. Currently size will be one.
   */
  CollisionBranchList photon_cross_sections(
      MediatorType mediator = default_mediator_);

  /**
   * For processes which can happen via (pi, a1, rho) and omega exchange,
   * return the differential cross section for the (pi, a1, rho) channel in
   * the first argument, for the omega channel in the second. If only
   * one channel exists, both values are the same.
   *
   * \param t Mandelstam-t
   * \param t2 upper bound for t-range
   * \param t1 lower bound for t-range
   * \param m_rho Mass of the incoming or outgoing rho-particle
   *
   * \returns Diff. cross section for (pi,a1,rho) in the first argument,
   * for omega in the second.
   */
  std::pair<double, double> diff_cross_section_single(const double t,
                                                      const double m_rho);

  /**
   * For processes which can happen via (pi, a1, rho) and omega exchange,
   * return the form factor for the (pi, a1, rho) channel in
   * the first argument, for the omega channel in the second. If only
   * one channel exists, both values are the same.
   *
   * \param E_photon Energy of photon [GeV]
   *
   * \returns Form factor for (pi,a1,rho) in the first argument,
   * for omega in the second.
   */
  std::pair<double, double> form_factor_single(const double E_photon);

  /**
   * Compute the form factor for a process with a pion as the lightest exchange
   * particle See wiki for details how form factors are handled.
   */
  double form_factor_pion(const double) const;

  /**
   * Compute the form factor for a process with a omega as the lightest exchange
   * particle See wiki for details how form factors are handled.
   */
  double form_factor_omega(const double) const;

  /**
   * Compute the differential cross section with form factors included.
   * Takes care of correct handling of processes with multiple channels by
   * reading the default_mediator_ member variable.
   *
   * \param t Mandelstam-t.
   * \param t2 upper bound for t-range.
   * \param t1 lower bound for t-range.
   * \param m_rho Mass of the incoming or outgoing rho-particle.
   * \param energy of outgoing photon.
   *
   * \returns diff. cross section [mb / GeV \f$^2\f$]
   */
  double diff_cross_section_w_ff(const double t, const double m_rho,
                                 const double E_photon);

  static constexpr double m_omega_ = 0.783;
};

}  // namespace smash

#endif  // SRC_INCLUDE_SCATTERACTIONPHOTON_H_
