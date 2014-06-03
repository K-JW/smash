/*
 *
 *    Copyright (c) 2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#ifndef SRC_INCLUDE_ANGLES_H_
#define SRC_INCLUDE_ANGLES_H_

#include <stdexcept>
#include "random.h"
#include "threevector.h"
#include "constants.h"

namespace Smash {

/** Angles provides a common interface for generating directions: i.e.,
 * two angles that should be interpreted as azimuthal and polar angles.
 *
 * Usage:
 * ------
 * \code
 * #include "Angles.h"
 *
 * Angles direction;
 *
 * direction.distribute_isotropously();
 * float azimuthal_angle = direction.phi();
 * float cosine_of_polar_angle = direction.costheta();
 * float x_projection_of_vector = direction.x();
 * direction.set_phi(0);
 * float new_azimuthal_angle = direction.phi();
 * // new_azimuthal_angle == 0.
 * \endcode
 *
 * Internals
 * ---------
 *
 * The object internally stores the azimuthal angle \f$\varphi\f$ and
 * the cosine of the polar angle \f$\cos\vartheta\f$. Nobody should rely
 * on this never changing, though; the interface user should be totally
 * oblivious to this.
 *
 * Possible future improvements
 * ----------------------------
 *
 * More distributions need to be implemented once there is a physics
 * case to use them.
 **/

class Angles {
 public:
  /// Standard initializer, points in x-direction.
  Angles();
  /** populate the object with a new direction
   *
   * the direction is taken randomly from a homogeneous distribution,
   * i.e., each point on a unit sphere is equally likely.
   **/
  void distribute_isotropically();
  /** update azimuthal angle
   *
   * sets the azimuthal angle and leaves the polar angle untouched.
   *
   * @param phi any real number to set the azimuthal angle \f$\varphi\f$
   * to.
   **/
  void set_phi(const float phi);
  /** update polar angle from its cosine.
   *
   * sets the polar angle and leaves the azimuthal angle untouched.
   * This is the preferred way of setting the polar information.
   *
   * @param cos cosine of the polar angle \f$\cos\vartheta\f$. Must be
   * in range [-1 .. 1], else an Exception is thrown.
   *
   **/
  void set_costheta(const float cos);
  /** update polar angle from itself
   *
   * sets the polar angle and leaves the azimuthal angle untouched.
   * In the interface (public functions) we don't specify if theta or
   * costheta is actually stored inside the object, so we don't name the
   * functions to indicate the internals.
   *
   * In the current implementation, costheta is stored inside the
   * object. Don't convert the angle to and from cosine, use the
   * set-function for the thing you have at hand. Accepts any real
   * number.
   *
   * @param theta any real number to set the polar angle \f$\vartheta\f$
   * to.
   */
  void set_theta(const float theta);
  /** advance polar angle
   *
   * polar angle is advanced. A positive addition means that we go
   * towards the southpole.
   *
   * \see add_to_theta(const float& delta, const bool& reverse)
   *
   * @param delta angle increment
   * @return true if pole has been crossed.
   **/
  bool add_to_theta(const float delta);
  /** advance polar angle
   *
   * polar angle is advanced. When crossing a pole, azimuthal angle is
   * changed by 180 degrees.
   *
   * @param delta angle increment. Must not be outside [\f$-\pi\f$ ..
   * \f$\pi\f$].
   * @param reverse if true, we start in the "far" hemisphere, meaning a
   * positive delta will shift the object towards the north pole.
   *
   * @return returns true if we end up in the far hemisphere, false if
   * we end up in the original hemisphere.
   **/
  bool add_to_theta(const float delta, const bool reverse);
  /// get azimuthal angle
  float phi() const;
  /// get cosine of polar angle
  float costheta() const;
  /// get sine of polar angle
  float sintheta() const;
  /** get x projection of the direction
   *
   * \f$x = \sin\vartheta \cos\varphi\f$
   **/
  float x() const;
  /** get y projection of the direction
   *
   * \f$y = \sin\vartheta \sin\varphi\f$
   **/
  float y() const;
  /** get z projection of the direction
   *
   * \f$z = \cos\vartheta\f$
   **/
  float z() const;
  /// get the three-vector
  ThreeVector inline threevec() const;
  /// returns the polar angle
  float theta() const;

  /// thrown for invalid values for theta
  struct InvalidTheta : public std::invalid_argument {
    using std::invalid_argument::invalid_argument;
  };

 private:
  /// azimuthal angle \f$\varphi\f$
  float phi_;
  /// cosine of polar angle \f$\cos\varphi\f$
  float costheta_;
};

inline Angles::Angles() : phi_(0), costheta_(0) {}

void inline Angles::distribute_isotropically() {
  /* isotropic distribution: phi in [0, 2pi) and cos(theta) in [-1,1] */
  phi_ = Random::uniform(0.0, twopi);
  costheta_ = Random::uniform(-1.0, 1.0);
}

void inline Angles::set_phi (const float newphi) {
  /* Make sure that phi is in the range [0,2pi).  */
    phi_ = newphi;
  if (newphi < 0 || newphi >= twopi) {
    phi_ -= twopi * floor(newphi / twopi);
  }
}

void inline Angles::set_costheta(const float newcos) {
  costheta_ = newcos;
  /* check if costheta_ is in -1..1. If not, well. Error handling here
   * is a lot harder than in the above. Still, I will silently do the
   * same as above. Note, though, that costheta = 1 is allowed, even if
   * it cannot be generated by distribute_isotropically().
   */
  if (costheta_ < -1 || costheta_ > 1) {
    throw InvalidTheta("Wrong value for costheta (must be in [-1,1]): " +
                       std::to_string(costheta_));
  }
}
void inline Angles::set_theta(const float newtheta) {
  /* no error handling necessary, because this gives a sensible answer
   * for every real number.
   */
  set_costheta(cos(newtheta));
}

bool inline Angles::add_to_theta(const float delta) {
  if (delta < -M_PI || delta > M_PI) {
    throw InvalidTheta("Cannot advance polar angle by " +
                       std::to_string(delta));
  }
  float theta_plus_delta = delta + theta();
  /* if sum is not in [0, PI], force it to be there:
   * "upper" overflow:
   * theta + delta + the_new_angle = 2*M_PI
   */
  if (theta_plus_delta > M_PI) {
    set_theta(twopi - theta_plus_delta);
    /* set_phi takes care that phi_ is in [0 .. 2*M_PI] */
    set_phi(phi() + M_PI);
    return true;  // meaning "we did change phi"
  /* "lower" overflow: theta + delta switches sign */
  } else if (theta_plus_delta < 0) {
    set_theta(-theta_plus_delta);
    set_phi(phi() + M_PI);
    return true;  // meaning "we did change phi"
  /* no overflow: set theta, do not touch phi: */
  } else {
    set_theta(theta_plus_delta);
  }
  return false;  // meaning "we did NOT change phi"
}
bool inline Angles::add_to_theta(const float delta, const bool reverse) {
  float plusminus_one = reverse ? -1.0 : +1.0;
  bool this_reverse = add_to_theta(plusminus_one*delta);
  /* if we had to reverse first time and now reverse again OR if we
   * didn't reverse in either part, we do not reverse in total.
   * else: if we reverse in one, but not the other part, we reverse in
   * total.
   */
  return this_reverse ^ reverse;
}

float inline Angles::costheta() const { return costheta_; }
float inline Angles::phi() const { return phi_; }
float inline Angles::sintheta() const {
  return sqrt(1.0 - costheta_*costheta_);
}
float inline Angles::x() const { return sintheta()*cos(phi_); }
float inline Angles::y() const { return sintheta()*sin(phi_); }
float inline Angles::z() const { return costheta_; }

ThreeVector inline Angles::threevec() const {
  return ThreeVector(x(),y(),z());
}

float inline Angles::theta() const { return acos(costheta_); }

}  // namespace Smash

#endif  // SRC_INCLUDE_ANGLES_H_
