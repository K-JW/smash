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

#include <cmath>
#include <cstdio>
#include <cstdlib>

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
 * double azimuthal_angle = direction.phi();
 * double cosine_of_polar_angle = direction.costheta();
 * double x_projection_of_vector = direction.x();
 * direction.set_phi(0);
 * double new_azimuthal_angle = direction.phi();
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
  void set_phi(const double& phi);
  /** update polar angle from its cosine.
   *
   * sets the polar angle and leaves the azimuthal angle untouched.
   * This is the preferred way of setting the polar information.
   *
   * @param cos cosine of the polar angle \f$\cos\vartheta\f$. Must be
   * in range [-1 .. 1], else an Exception is thrown.
   *
   **/
  void set_costheta(const double& cos);
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
  void set_theta(const double& theta);
  /** advance polar angle
   *
   * polar angle is advanced. A positive addition means that we go
   * towards the southpole.
   *
   * \see add_to_theta(const double& delta, const bool& reverse)
   *
   * @param delta angle increment
   * @return true if pole has been crossed.
   **/
  bool add_to_theta(const double& delta);
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
  bool add_to_theta(const double& delta, const bool& reverse);
  /// get azimuthal angle
  double phi() const;
  /// get cosine of polar angle
  double costheta() const;
  /// get sine of polar angle
  double sintheta() const;
  /** get x projection of the direction
   *
   * \f$x = \sin\vartheta \cos\varphi\f$
   **/
  double x() const;
  /** get y projection of the direction
   *
   * \f$y = \sin\vartheta \sin\varphi\f$
   **/
  double y() const;
  /** get z projection of the direction
   *
   * \f$z = \cos\vartheta\f$
   **/
  double z() const;
  /// returns the polar angle
  double theta() const;

 private:
  /// azimuthal angle \f$\varphi\f$
  double phi_;
  /// cosine of polar angle \f$\cos\varphi\f$
  double costheta_;
};

inline Angles::Angles() : phi_(0), costheta_(0) {}

void inline Angles::distribute_isotropically() {
  /* isotropic distribution: phi in [0, 2pi) and cos(theta) in [-1,1] */
  phi_ = 2.0 * M_PI * drand48();
  costheta_ = -1.0 + 2.0 * drand48();
}

void inline Angles::set_phi(const double& newphi) {
  phi_ = newphi;
  /* check if phi is in 0 .. 2pi. If not, we simply transform it
   * there by subtracting/adding 2pi as often as needed.
   * floor(phi/(2pi) is the number of (2pi)s that we need to subtract
   * (why use a loop if one statement can do it?)
   */
  if (phi_ < 0 || phi_ >= 2.0 * M_PI) {
    phi_ -= 2.0 * M_PI * floor(phi_ / (2.0 * M_PI));
  }
}
void inline Angles::set_costheta(const double& newcos) {
  costheta_ = newcos;
  /* check if costheta_ is in -1..1. If not, well. Error handling here
   * is a lot harder than in the above. Still, I will silently do the
   * same as above. Note, though, that costheta = 1 is allowed, even if
   * it cannot be generated by distribute_isotropically().
   */
  if (costheta_ < -1 || costheta_ > 1) {
    char errormsg[50];
    snprintf(errormsg, sizeof(errormsg),
             "Wrong value for costheta (must be in [-1,1]): %g",
             costheta_);
    throw(errormsg);
  }
}
void inline Angles::set_theta(const double& newtheta) {
  /* no error handling necessary, because this gives a sensible answer
   * for every real number.
   */
  set_costheta(cos(newtheta));
}

bool inline Angles::add_to_theta(const double& delta) {
  if (delta < -M_PI || delta > M_PI) {
    char errormsg[50];
    snprintf(errormsg, sizeof(errormsg),
             "Cannot advance polar angle by %g",
             delta);
    throw(errormsg);
  }
  double theta_plus_delta = delta + theta();
  /* if sum is not in [0, PI], force it to be there:
   * "upper" overflow:
   * theta + delta + the_new_angle = 2*M_PI
   */
  if (theta_plus_delta > M_PI) {
    set_theta(2.0*M_PI - theta_plus_delta);
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
bool inline Angles::add_to_theta(const double& delta, const bool& reverse) {
  double plusminus_one = reverse ? -1.0 : +1.0;
  bool this_reverse = add_to_theta(plusminus_one*delta);
  /* if we had to reverse first time and now reverse again OR if we
   * didn't reverse in either part, we do not reverse in total.
   * else: if we reverse in one, but not the other part, we reverse in
   * total.
   */
  return this_reverse ^ reverse;
}

double inline Angles::costheta() const { return costheta_; }
double inline Angles::phi() const { return phi_; }
double inline Angles::sintheta() const {
  return sqrt(1.0 - costheta_*costheta_);
}
double inline Angles::x() const { return sintheta()*cos(phi_); }
double inline Angles::y() const { return sintheta()*sin(phi_); }
double inline Angles::z() const { return costheta_; }
double inline Angles::theta() const { return acos(costheta_); }

#endif  // SRC_INCLUDE_ANGLES_H_
