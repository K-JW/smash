/*
 *
 *    Copyright (c) 2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "tests/unittest.h"
#include "include/angles.h"

using namespace Smash;

Angles dir;

TEST(set_angles) {
  dir.set_phi(.5);
  FUZZY_COMPARE(dir.phi(), .5);
  dir.set_phi(4*M_PI);
  FUZZY_COMPARE(dir.phi(), 0.0);
  dir.set_costheta(.3);
  FUZZY_COMPARE(dir.costheta(), 0.3);
  dir.set_theta(.3);
  // for cos(acos()), we need greater leniency
  UnitTest::setFuzzyness<double>(3);
  FUZZY_COMPARE(dir.theta(), 0.3);
}

TEST(accessors_and_relations) {
  constexpr int kNumberOfTries = 1000000;
  // second: check accessors and the relations between them:
  for (int c = 0; c < kNumberOfTries; c++) {
    dir.distribute_isotropically();
    // sintheta**2 + costheta**2 = 1
    UnitTest::setFuzzyness<double>(1);
    FUZZY_COMPARE(dir.sintheta()*dir.sintheta()
                     + dir.costheta()*dir.costheta(), 1.0);
    // x**2 + y**2 + z**2 = 1
    double xyz_one = dir.x()*dir.x()
                   + dir.y()*dir.y()
                   + dir.z()*dir.z();
    UnitTest::setFuzzyness<double>(3);
    FUZZY_COMPARE(xyz_one, 1.0);

    // compare cos(theta) and costheta:
    UnitTest::setFuzzyness<double>(1024);
    FUZZY_COMPARE(cos(dir.theta()), dir.costheta()) << " (trial #" << c
                                          << " of " << kNumberOfTries << ")";
    FUZZY_COMPARE(dir.theta(), acos(dir.costheta())) << " (trial #" << c
                                          << " of " << kNumberOfTries << ")";
  }
}

TEST(unusual_set_phi) {
  constexpr int kMinM = -6;
  constexpr int kMaxM = 12;
  for (int m = kMinM; m < kMaxM; m++) {
    // set phi outside [0..2pi]
    dir.set_phi(2.0 * M_PI * m + .5);
    FUZZY_COMPARE(dir.phi(), .5) << " (m = " << m << ")";
  }
}

TEST(unusual_set_theta_even) {
  UnitTest::setFuzzyness<double>(48);
  constexpr int kMinM = -6;
  constexpr int kMaxM = 12;
  for (int m = kMinM; m < kMaxM; m++) {
    // set theta in [2*n*pi .. (2*n+1)*pi]
    dir.set_theta(2.0 * M_PI * m + .7);
    FUZZY_COMPARE(dir.theta(), .7) << " (m = " << m << ")";
  }
}
TEST(unusual_set_theta_odd) {
  UnitTest::setFuzzyness<double>(48);
  constexpr int kMinM = -6;
  constexpr int kMaxM = 12;
  for (int m = kMinM; m < kMaxM; m++) {
    // set theta in [(2*n-1)*pi .. 2*n*pi]
    dir.set_theta(2.0 * M_PI * m - .7);
    FUZZY_COMPARE(dir.theta(), .7) << " (m = " << m << ")";
  }
}

TEST(catch_invalid_cosine) {
  for (double newcos = -8.0; newcos < 8.0; newcos += .2) {
    bool invalid_input = (newcos < -1 || newcos > 1);
    // Did I catch an exception?
    bool caught = false;
    // this should not work:
    try {
      dir.set_costheta(newcos);
    } catch(...) {
      caught = true;
    }
    // check that I caught an exception if I gave an invalid number:
    if (invalid_input) {
      VERIFY(caught) << " (tried to set cos(theta) to " << newcos << ")";
    } else {
    // check that I did not catch an exception for valid input:
      VERIFY(!caught) << " (tried to set cos(theta) to " << newcos << ")";
    }
  }
}

TEST(setting_costheta_does_not_change_phi) {
  dir.set_phi(3.0);
  double old_phi = dir.phi();
  dir.set_costheta(.2);
  FUZZY_COMPARE(old_phi, dir.phi());
}

TEST(setting_theta_does_not_change_phi) {
  dir.set_phi(3.0);
  double old_phi = dir.phi();
  dir.set_theta(.2);
  FUZZY_COMPARE(old_phi, dir.phi());
}

TEST(setting_phi_does_not_change_costheta) {
  double old_costheta = dir.costheta();
  dir.set_phi(.4);
  FUZZY_COMPARE(old_costheta, dir.costheta());
}

TEST(setting_phi_does_not_change_z) {
  double old_z = dir.z();
  dir.set_phi(.4);
  FUZZY_COMPARE(old_z, dir.z());
}

TEST(add_theta) {
  UnitTest::setFuzzyness<double>(64);
  for (double current_phi = 0.0; current_phi < 2 * M_PI;
                                 current_phi += M_PI / 180.0) {
    dir.set_phi(current_phi);
    // we'll start at what I call the north pole:
    dir.set_theta(0.0);
    bool sign = dir.add_to_theta(M_PI / 2.0);
    // phi shouldn't have changed:
    FUZZY_COMPARE(current_phi, dir.phi()) << " (phi = " << current_phi << ")";
    // the sign shouldn't have changed.
    VERIFY(!sign) << " (phi = " << current_phi << ")";
    // theta, though, should have changed.
    FUZZY_COMPARE(dir.theta(), M_PI / 2.0) << " (phi = " << current_phi << ")";
    // 90+120 degrees: phi changed, theta is at 150 degrees
    sign = dir.add_to_theta(2.0 * M_PI / 3.0);
    if (current_phi < M_PI) {
      FUZZY_COMPARE(current_phi, dir.phi() - M_PI) << " (phi = " << current_phi
                                                                << ")";
    } else {
      FUZZY_COMPARE(current_phi, dir.phi() + M_PI) << " (phi = " << current_phi
                                                                << ")";
    }
    VERIFY(sign) << " (phi = " << current_phi << ")";
    FUZZY_COMPARE(dir.theta(), 5.0 * M_PI / 6.0) << " (phi = " << current_phi
                                                              << ")";
    // go back over the (south) pole: +60 degrees
    sign = dir.add_to_theta(M_PI / 3.0);
    // phi is back to original;
    FUZZY_COMPARE(current_phi, dir.phi()) << " (phi = " << current_phi << ")";
    VERIFY(sign) << " (phi = " << current_phi << ")";
    // theta is the same as before (150+60 = 210 equiv 150)
    FUZZY_COMPARE(dir.theta(), 5.0 * M_PI / 6.0) << " (phi = " << current_phi
                                                              << ")";
    // go over two poles: +120 + 120 (going over more than 180 degrees
    // in one step is forbidden; we'll check that later)
    // First, over the south pole
    sign = dir.add_to_theta(2.0 * M_PI / 3.0);
    // sign is true because we are in between south- and north pole
    VERIFY(sign) << " (phi = " << current_phi << ")";
    // second, we want to continue in the same direction along the great
    // circle, so we want to go over the north pole now. This needs to
    // be told to add_to_theta by feeding it sign.
    bool sign2 = dir.add_to_theta(2.0 * M_PI / 3.0, sign);
    // sign2 is FALSE (despite the crossing), because now we are in the
    // semi circle between north- and south pole (mind the direction!)
    VERIFY(!sign2) << " (phi = " << current_phi << ")";
    // phi stays the same (two changes)
    FUZZY_COMPARE(current_phi, dir.phi()) << " (phi = " << current_phi << ")";
    // theta is now 30 degrees:
    FUZZY_COMPARE(dir.theta(), M_PI / 6.0) << " (phi = " << current_phi << ")";
  }
}

TEST_CATCH(set_invalid_theta, Angles::InvalidTheta) {
  dir.set_costheta(2.);
}

TEST_CATCH(set_theta_too_far, Angles::InvalidTheta) {
  dir.add_to_theta(M_PI * 1.1);
}
