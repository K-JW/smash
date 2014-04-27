/*
 *    Copyright (c) 2012-2013
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 */
#ifndef SRC_INCLUDE_FOURVECTOR_H_
#define SRC_INCLUDE_FOURVECTOR_H_

#include <array>
#include <cmath>

namespace Smash {

/**
 * The FourVector class holds relevant values in Minkowski spacetime
 * with (+, −, −, −) metric signature.
 *
 * The overloaded operators are build according to Andrew Koenig
 * recommendations where  the compound assignment operators is used as a
 * base for their non-compound counterparts. This means that the
 * operator + is implemented in terms of +=. The operator+ returns
 * a copy of it's result. + and friends are non-members, while
 * the compound assignment counterparts, changing the left
 * argument, are a member of the FourVector class.
 */
class FourVector {
 public:
  /// default constructor nulls the fourvector components
  FourVector() : x_{0., 0., 0., 0.} {
  }
  /// copy constructor
  FourVector(double y0, double y1, double y2, double y3) : x_{y0, y1, y2, y3} {
  }
  /* t, x_\perp, z */
  double inline x0(void) const;
  void inline set_x0(double t);
  double inline x1(void) const;
  void inline set_x1(double x);
  double inline x2(void) const;
  void inline set_x2(double y);
  double inline x3(void) const;
  void inline set_x3(double z);
  /* set all four values */
  void inline set_FourVector(const double t, const double x, const double y,
                             const double z);
  /* inlined operations */
  double inline Dot(const FourVector &a) const;
  double inline Dot() const;
  double inline DotThree(const FourVector &a) const;
  double inline DotThree() const;
  double inline DiffThree(const FourVector &a) const;
  /** Returns the FourVector boosted with velocity.
   *
   * The current FourVector is not changed.
   *
   * \param velocity (\f$w^{"\mu"}\f$) is not a physical FourVector, but
   * a collection of 1 for the time-like component and \f$\vec v\f$ for
   * the space-like components. In other words, it is \f$\gamma^{-1}
   * u^\mu\f$ with \f$u^\mu\f$ being the physical Four-Velocity.
   *
   * Algorithmic
   * -----------
   *
   * (Note: \f$\vec a\f$ is a Three-Vector, \f$a^\mu\f$ is a Four-Vector
   * and \f$a^{"\mu"}\f$ is a Pseudo-Four-Vector object.)
   *
   * The gamma factor \f$\gamma = 1/\sqrt{1-(v_1^2+v_2^2+v_3^2)}\f$
   * needed in the boost can be calculated from velocity as the
   * "invariant square" of a physical FourVector:
   *
   * \f[
   * w^{"\mu"} = (1, \vec{v}) = (1, v_1, v_2, v_3)\\
   * w^{"\mu"} w_{"\mu"} = 1 - v_1^2 - v_2^2 - v_3^2\\
   * = \gamma^{-2}
   * \f]
   *
   * The time-like component of a Lorentz-boosted FourVector \f$x^\mu =
   * (x_0, x_1, x_2, x_3) = (x_0, \vec{r})\f$ with velocity \f$\vec v\f$
   * is
   *
   * \f{eqnarray*}{
   * x^\prime_0&=&\gamma \cdot (x_0 - \vec{r}\cdot\vec{v})\\
   *     &=&\gamma \cdot (x_0 \cdot 1 - x_1 \cdot v_1 - x_2 \cdot v_2 - x_3 \cdot v_3)\\
   *     &=&\gamma \cdot (x^\mu \cdot w_{"\mu"}),
   * \f}
   *
   * and the space-like components i = 1, 2, 3 are:
   * \f{eqnarray*}{
   * x^\prime_i&=&x_i + v_i \cdot (\frac{\gamma - 1}{\vec{v}^2} \cdot \vec{r}\cdot\vec{v} - \gamma \cdot x_0)\\
   *     &=&x_i + v_i \cdot (\frac{\gamma^2}{\gamma + 1} \cdot \vec{r}\cdot\vec{v} - \gamma \cdot x_0)\\
   *     &=&x_i - v_i \cdot \gamma \cdot (\frac{\gamma}{\gamma + 1} x^\mu\cdot w_{"\mu"} + \frac{x_0}{\gamma + 1})\\
   *     &=&x_i - v_i \cdot \frac{\gamma}{\gamma + 1} \cdot (\gamma x^\mu\cdot w_{"\mu"} + x_0) \\
   *     &=&x_i - v_i \cdot \frac{\gamma}{\gamma + 1} \cdot (x^\prime_0 + x_0)
   * \f}
   */
  FourVector LorentzBoost(const FourVector &velocity) const;

  /* overloaded operators */
  bool inline operator==(const FourVector &a) const;
  bool inline operator!=(const FourVector &a) const;
  bool inline operator<(const FourVector &a) const;
  bool inline operator>(const FourVector &a) const;
  bool inline operator<=(const FourVector &a) const;
  bool inline operator>=(const FourVector &a) const;
  bool inline operator==(const double &a) const;
  bool inline operator!=(const double &a) const;
  bool inline operator<(const double &a) const;
  bool inline operator>(const double &a) const;
  bool inline operator<=(const double &a) const;
  bool inline operator>=(const double &a) const;
  FourVector inline operator+=(const FourVector &a);
  FourVector inline operator-=(const FourVector &a);
  FourVector inline operator*=(const double &a);
  FourVector inline operator/=(const double &a);

  using iterator = std::array<double, 4>::iterator;
  using const_iterator = std::array<double, 4>::const_iterator;

  /**
   * Returns an iterator starting at the 0th component.
   *
   * The iterator implements the RandomIterator concept. Thus, you can simply
   * write `begin() + 1` to get an iterator that points to the 1st component.
   */
  iterator begin() { return x_.begin(); }

  /**
   * Returns an iterator pointing after the 4th component.
   */
  iterator end() { return x_.end(); }

  /// const overload of the above
  const_iterator begin() const { return x_.begin(); }
  /// const overload of the above
  const_iterator end() const { return x_.end(); }

  /// \see begin
  const_iterator cbegin() const { return x_.cbegin(); }
  /// \see end
  const_iterator cend() const { return x_.cend(); }

 private:
  std::array<double, 4> x_;
};

double inline FourVector::x0(void) const {
  return x_[0];
}

void inline FourVector::set_x0(const double t) {
  x_[0] = t;
}

double inline FourVector::x1(void) const {
  return x_[1];
}

void inline FourVector::set_x1(const double z) {
  x_[1] = z;
}

double inline FourVector::x2(void) const {
  return x_[2];
}

void inline FourVector::set_x2(const double x) {
  x_[2] = x;
}

double inline FourVector::x3(void) const {
  return x_[3];
}

void inline FourVector::set_x3(const double y) {
  x_[3] = y;
}

void inline FourVector::set_FourVector(const double t, const double z,
                                       const double x, const double y) {
  x_ = {t, z, x, y};
}

/// check if all four vector components are equal
bool inline FourVector::operator==(const FourVector &a) const {
  return std::abs(x_[0] - a.x_[0]) < 1e-12 &&
         std::abs(x_[1] - a.x_[1]) < 1e-12 &&
         std::abs(x_[2] - a.x_[2]) < 1e-12 &&
         std::abs(x_[3] - a.x_[3]) < 1e-12;
}

/// use == operator for the inverse != check
bool inline FourVector::operator!=(const FourVector &a) const {
  return !(*this == a);
}

/// all four vector components are below comparison vector
bool inline FourVector::operator<(const FourVector &a) const {
  return (x_[0] < a.x_[0]) && (x_[1] < a.x_[1]) && (x_[2] < a.x_[2]) && (x_[3] < a.x_[3]);
}

/// use < operator for the inverse by switching arguments
bool inline FourVector::operator>(const FourVector &a) const {
  return a < *this;
}

/// use > operator for less equal
bool inline FourVector::operator<=(const FourVector &a) const {
  return !(*this > a);
}

/// use < operator for greater equal
bool inline FourVector::operator>=(const FourVector &a) const {
  return !(*this < a);
}

/// all vector components are equal to that number
bool inline FourVector::operator==(const double &a) const {
  return std::abs(x_[0] - a) < 1e-12 && std::abs(x_[1] - a) < 1e-12 &&
         std::abs(x_[2] - a) < 1e-12 && std::abs(x_[3] - a) < 1e-12;
}

/// use == operator for the inverse !=
bool inline FourVector::operator!=(const double &a) const {
  return !(*this == a);
}

/// all vector components are below that number
bool inline FourVector::operator<(const double &a) const {
  return (x_[0] < a) && (x_[1] < a) && (x_[2] < a) && (x_[3] < a);
}

/// all vector components are above that number
bool inline FourVector::operator>(const double &a) const {
  return (x_[0] > a) && (x_[1] > a) && (x_[2] > a) && (x_[3] > a);
}

/// all vector components are less equal that number
bool inline FourVector::operator<=(const double &a) const {
  return !(*this > a);
}

/// all vector components are greater equal that number
bool inline FourVector::operator>=(const double &a) const {
  return !(*this < a);
}

/// += assignement addition
FourVector inline FourVector::operator+=(const FourVector &a) {
  this->x_[0] += a.x_[0];
  this->x_[1] += a.x_[1];
  this->x_[2] += a.x_[2];
  this->x_[3] += a.x_[3];
  return *this;
}

/// addition +operator uses +=
inline FourVector operator+(FourVector a, const FourVector &b) {
  a += b;
  return a;
}

/// -= assignement subtraction
FourVector inline FourVector::operator-=(const FourVector &a) {
  this->x_[0] -= a.x_[0];
  this->x_[1] -= a.x_[1];
  this->x_[2] -= a.x_[2];
  this->x_[3] -= a.x_[3];
  return *this;
}

/// subtraction operator- uses -=
inline FourVector operator-(FourVector a, const FourVector &b) {
  a -= b;
  return a;
}

/// assignement factor multiplication
FourVector inline FourVector::operator*=(const double &a) {
  this->x_[0] *= a;
  this->x_[1] *= a;
  this->x_[2] *= a;
  this->x_[3] *= a;
  return *this;
}

/// factor multiplication uses *=
inline FourVector operator*(FourVector a, const double &b) {
  a *= b;
  return a;
}

/// assignement factor division
FourVector inline FourVector::operator/=(const double &a) {
  this->x_[0] /= a;
  this->x_[1] /= a;
  this->x_[2] /= a;
  this->x_[3] /= a;
  return *this;
}

/// factor division uses /=
inline FourVector operator/(FourVector a, const double &b) {
  a /= b;
  return a;
}

double inline FourVector::Dot(const FourVector &a) const {
  return x_[0] * a.x_[0] - x_[1] * a.x_[1] - x_[2] * a.x_[2] - x_[3] * a.x_[3];
}

double inline FourVector::Dot() const {
  return x_[0] * x_[0] - x_[1] * x_[1] - x_[2] * x_[2] - x_[3] * x_[3];
}

double inline FourVector::DotThree(const FourVector &a) const {
  return - x_[1] * a.x_[1] - x_[2] * a.x_[2] - x_[3] * a.x_[3];
}

double inline FourVector::DotThree() const {
  return - x_[1] * x_[1] - x_[2] * x_[2] - x_[3] * x_[3];
}

double inline FourVector::DiffThree(const FourVector &a) const {
  return x_[1] - a.x_[1] + x_[2] - a.x_[2] + x_[3] - a.x_[3];
}

}  // namespace Smash

#endif  // SRC_INCLUDE_FOURVECTOR_H_
