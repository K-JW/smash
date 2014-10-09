/*
 *    Copyright (c) 2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 */
#ifndef SRC_INCLUDE_NUCLEUSMODUS_H_
#define SRC_INCLUDE_NUCLEUSMODUS_H_

#include "modusdefault.h"

#include "forwarddeclarations.h"
#include "nucleus.h"
#include "deformednucleus.h"
#include "pdgcode.h"

#include <cstring>
#include <memory>
#include <utility>

namespace Smash {

struct ExperimentParameters;

/*!\Userguide
 * \page input_modi_nucleus_ Nucleus
 *
 * Possible Incident Energies:
 * \key SQRTSNN: \n
 * Defines the energy of the collision as center-of-mass
 * energy in the collision of one participant each from both nuclei.
 * Since not all participants have the same mass, and hence
 * \f$\sqrt{s_{\rm NN}}\f$ is different for \f$NN\f$ = proton+proton and
 * \f$NN\f$=neutron+neutron, you can specify which \f$NN\f$-pair you
 * want this to refer to with `SQRTS_REPS`. This expects a vector of two
 * PDG Codes, e.g. `SQRTS_N: [2212, 2212]` for proton-proton.
 *
 * \key E_LAB: \n
 * Defines the energy of the collision by the initial energy of
 * the projectile nucleus.  This assumes the target nucleus is at rest.
 *
 * \key P_LAB: \n
 * Defines the energy of the collision by the initial momentum
 * of the projectile nucleus.  This assumes the target nucleus is at rest.
 *
 * \key Projectile: \n
 * Section for projectile nucleus. The projectile will
 * start at \f$z < 0\f$ and fly in positive \f$z\f$-direction, at \f$x
 * \ge 0\f$.
 *
 * \key Target: \n
 * Section for target nucleus. The target will start at \f$z
 * > 0\f$ and fly in negative \f$z\f$-direction, at \f$x \le 0\f$.
 *
 * \key CALCULATION FRAME: \n
 * The frame in which the collision is calculated.
 * Options are the center of velocity (1, default), the center of mass (2),
 * and the fixed target (3). Set the number to specify the desired frame.
 *
 * \key Projectile: and \key Target: \n
 * \li \key PARTICLES: A map in which the keys are PDG codes and the
 * values are number of particles with that PDG code that should be in
 * the current nucleus. E.g.\ `PARTICLES: {2212: 82, 2112: 126}` for a
 * lead-208 nucleus (82 protons and 126 neutrons = 208 nucleons), and
 * `PARTICLES: {2212: 1, 2112: 1, 3122: 1}` for Hyper-Triton (one
 * proton, one neutron and one Lambda).
 * \li \key DEFORMED: \n
 * Whether to construct nuclei using the nucleus class
 * or the deformed nucleus class (true=deformednucleus/false=nucleus).
 * \li \key AUTOMATIC: \n
 * Whether or not to use default values based on the
 * current nucleus atomic number (true/false).
 *
 * Additional Woods-Saxon Parameters: \n
 * There are also many other
 * parameters for specifying the shape of the Woods-Saxon distribution,
 * and other nucleus specific properties. See NUCLEUS.CC and
 * DEFORMEDNUCLEUS.CC for more on these choices.
 *
 * \key Impact: \n
 * A section for the impact parameter (= distance of the two
 * straight lines that the center of masses of the nuclei travel on).
 *
 * \li \key VALUE: fixed value for the impact parameter. No other \key
 * Impact: directive is looked at.
 * \li \key SAMPLE: if \key uniform, use uniform sampling of the impact
 * parameter (\f$dP(b) = db\f$). If else, use areal input sampling
 * (the probability of an input parameter range is proportional to the
 * area corresponding to that range, \f$dP(b) = b\cdot db\f$).
 * \li \key RANGE: A vector of minimal and maximal impact parameters
 * between which b should be chosen. (The order of these is not
 * important.)
 * \li \key MAX: Like `RANGE: [0.0, MAX]`. Note that if both \key RANGE and
 * \key MAX are specified, \key MAX takes precedence.
 *
 * Note that there are no safeguards to prevent you from specifying
 * negative impact parameters. The value chosen here is simply the
 * x-component of \f$\vec b\f$. The result will be that the projectile
 * and target will have switched position in x.
 *
 * \key INITIAL_DISTANCE: \n
 * The initial distance of the two nuclei. That
 * means \f$z_{\rm min}^{\rm target} - z_{\rm max}^{\rm projectile}\f$.
 */

/**
 * \ingroup modus
 * NucleusModus: Provides a modus for colliding nuclei.
 *
 * To use this modus, chose
 * \code
 * General:
 *      MODUS: Nucleus
 * \endcode
 * in the configuration file.
 *
 * Options for NucleusModus go in the "Modi"→"Nucleus" section of the
 * configuration.
 *
 * The following configuration options are understood: \ref input_modi_nucleus_
 */
class NucleusModus : public ModusDefault {
 public:
  /** Constructor
   *
   * Takes all there is to take from the (truncated!) configuration
   * object (only contains configuration for this modus).
   **/
  explicit NucleusModus(Configuration modus_config,
           const ExperimentParameters &parameters);

  /** Creates initial conditions from the particles.
   *
   * In particular, it initializes the nuclei.
   */
  float initial_conditions(Particles *particles,
                          const ExperimentParameters &parameters);

  /// \ingroup exception
  /// Thrown when either \a projectile_ or \a target_ nuclei are empty.
  struct NucleusEmpty : public ModusDefault::BadInput {
    using ModusDefault::BadInput::BadInput;
  };

 private:
  /** Projectile.
   *
   * The object that comes from negative z-values at positive x-values
   * with positive velocity.
   **/
  std::unique_ptr<Nucleus> projectile_;
  /** Target.
   *
   * The object that comes from positive z-values at negative x-values
   * with negative velocity. In fixed target experiments, the target is
   * at rest.
   **/
  std::unique_ptr<Nucleus> target_;
  // Center-of-mass energy of the nucleus-nucleus collision.
  float total_s_;
  /** impact parameter
   *
   * The nuclei projectile_ and target_ will be shifted along the x axis
   * so that their centers move on antiparallel lines that are this
   * distance apart from each other.
   **/
  float impact_ = 0.f;
  /** sample impact parameter
   *
   * sets the impact parameter to a value between min and max.
   *
   * @param s if true, use quadratic sampling (probability for a given
   * impact parameter \f$dP(b)\f$ is proportional to \f$b\f$: \f$dP(b) =
   * b\cdot db\f$), else every \f$b\f$ has same probability.
   * @param min minimum value for impact parameter
   * @param max maximum value for impact parameter
   *
   * Note that max less than min also works fine.
   *
   **/
  void sample_impact(bool s, float min, float max);
  /** initial z displacement of nuclei
   *
   * each nucleus is shifted so that
   * the outermost particle on the side facing the other nucleus is at
   * \f$\pm\f$ this value.
   **/
  double initial_z_displacement_ = 1.0;
  // Reference frame for the system.
  // 1 = Center of velocity
  // 2 = Center of mass
  // 3 = Fixed target
  int frame_ = 1;
  // Get the frame dependent velocity for each nucleus, using
  // the current reference frame. \see frame_
  //
  // @param s The total mandelstam S of the system
  // @param m1 The mass of the projectile.
  // @param m2 The mass of the target.
  // @return < v1, v2 >
  std::pair<double, double> get_velocities(float mandelstam_s, float m1, float m2);

  /**\ingroup logging
   * Writes the initial state for the NucleusModus to the output stream.
   */
  friend std::ostream &operator<<(std::ostream &, const NucleusModus &);
};

}  // namespace Smash

#endif  // SRC_INCLUDE_NUCLEUSMODUS_H_
