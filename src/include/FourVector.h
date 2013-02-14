/*
 *    Copyright (c) 2012-2013
 *      maximilian attems <attems@fias.uni-frankfurt.de>
 *      Jussi Auvinen <auvinen@fias.uni-frankfurt.de>
 *    GNU General Public License (GPLv3)
 */
#ifndef SRC_INCLUDE_FOURVECTOR_H_
#define SRC_INCLUDE_FOURVECTOR_H_

class FourVector {
  public:
    /* default constructor */
    FourVector(): x0_(0), x1_(0), x2_(0), x3_(0) {}
    /* t, z, x_\perp */
    double inline x0();
    double inline x1();
    double inline x2();
    double inline x3();
    void inline set_FourVector(const double t, const double z, const double x,
      const double y);
    double Dot(FourVector);

    /* overloaded operators */
    FourVector inline operator+=(const FourVector &a);
    FourVector inline operator*=(const double &a);
// XX:    FourVector operator+(const FourVector &a) const;

  private:
    double x0_, x1_, x2_, x3_;
};

double inline FourVector::x0(void) {
  return x0_;
}

double inline FourVector::x1(void) {
  return x1_;
}

double inline FourVector::x2(void) {
  return x2_;
}

double inline FourVector::x3(void) {
  return x3_;
}

void inline FourVector::set_FourVector(const double t, const double z,
                                       const double x, const double y) {
  x0_ = t;
  x1_ = z;
  x2_ = x;
  x3_ = y;
}

FourVector inline FourVector::operator+=(const FourVector &a) {
  FourVector x = *this;
  x.x0_ += a.x0_;
  x.x1_ += a.x1_;
  x.x2_ += a.x2_;
  x.x3_ += a.x3_;
  return x;
}

FourVector inline FourVector::operator*=(const double &a) {
  FourVector x = *this;
  x.x0_ *= a;
  x.x1_ *= a;
  x.x2_ *= a;
  x.x3_ *= a;
  return x;
}

#endif  // SRC_INCLUDE_FOURVECTOR_H_
