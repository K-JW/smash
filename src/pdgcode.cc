/*
 * Copyright (c) 2013-2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 */

#include<istream>
#include "include/pdgcode.h"

namespace Smash {

std::istream& operator>>(std::istream& is, PdgCode& code) {
  std::string codestring("");
  is >> codestring;
  if (!is) {
    code = PdgCode::invalid();
    return is;
  }
  try {
    // set the fields from the string:
    code.set_from_string(codestring);
  } catch (PdgCode::InvalidPdgCode) {
    is.setstate(std::ios::failbit);
    code = PdgCode::invalid();
  }
  return is;
}

bool PdgCode::is_Nstar() const {
  std::int32_t multi = multiplet();
  if (multi == 0x10102 ||
      multi == 0x10122 ||
      multi == 0x10202 ||
      multi == 0x10212 ||
      multi == 0x10104 ||
      multi == 0x10114 ||
      multi == 0x10204 ||
      multi == 0x10214 ||
      multi == 0x10106 ||
      multi == 0x10206) {
    return true;
  } else {
    return false;
  }
}

bool PdgCode::is_Deltastar() const {
  std::int32_t multi = multiplet();
  if (multi == 0x10112 ||
      multi == 0x10222 ||
      multi == 0x10124 ||
      multi == 0x10224 ||
      multi == 0x10216 ||
      multi == 0x10208) {
    return true;
  } else {
    return false;
  }
}

bool PdgCode::is_pion() const {
  const auto c = code();
  return (c == 0x111)    // pi0
      || (c == 0x211)    // pi+
      || (c == -0x211);  // pi-
}

bool PdgCode::is_rho() const {
  const auto c = code();
  return (c == 0x113)    // rho0
      || (c == 0x213)    // rho+
      || (c == -0x213);  // rho-
}

int PdgCode::isospin_total() const {
  // non-hadrons and η mesons (and ω and stuff):
  if (!is_hadron() || quarks() == 0x22) {
    return 0;
  }
  int number_of_u_or_d_quarks = 0;
  if (digits_.n_q3_ == 2 || digits_.n_q3_ == 1) { ++number_of_u_or_d_quarks; }
  if (digits_.n_q2_ == 2 || digits_.n_q2_ == 1) { ++number_of_u_or_d_quarks; }
  if (digits_.n_q1_ == 2 || digits_.n_q1_ == 1) { ++number_of_u_or_d_quarks; }
  // Δ and N distinction. I don't know any smart algorithm for this; I am
  // confident that special casing is the only way to do that.
  if (number_of_u_or_d_quarks == 3) {
    std::int32_t multi = std::abs(multiplet());
    // first the most common ones: p/n and Δ
    if (multi == 0x10002) { return 1; }
    if (multi == 0x10004) { return 3; }
    // now the resonances:
    if (multi == 0x10102
     || multi == 0x10122
     || multi == 0x10202
     || multi == 0x10212
     || multi == 0x10104
     || multi == 0x10114
     || multi == 0x10204
     || multi == 0x10214
     || multi == 0x10106
     || multi == 0x10206
       ) { return 1; }
    if (multi == 0x10112
     || multi == 0x10222
     || multi == 0x10124
     || multi == 0x10224
     || multi == 0x10216
     || multi == 0x10208
       ) { return 3; }
    throw InvalidPdgCode(
          "Unknown Nucleon or Delta resonance, cannot determine isospin: "
          + string());
  }
  // special case: Λ. We know already that not three quarks are up or down,
  // so we need to find where the third is:
  // 312, 412, 512 are Λ, as is 213. And, well, while this is not yet part of
  // the standard, I'll throw in 214 and 215 as well.
  if ((quarks() & 0x0ff) == 0x012 || (quarks() & 0xff0) == 0x210) {
    return 0;
  }
  return number_of_u_or_d_quarks;
}

int PdgCode::net_quark_number(const int quark) const {
  // input sanitization: Only quark numbers 1 through 8 are allowed.
  if (quark < 1 || quark > 8) {
    throw std::invalid_argument(std::string("PdgCode::net_quark_number(): ")
                 + std::string("Quark number must be in [1..8], received ")
                 + std::to_string(quark));
  }
  // non-hadrons and those that have none of this quark type: 0.
  if (!is_hadron() ||
        (digits_.n_q1_ != quark && digits_.n_q2_ != quark
                                && digits_.n_q3_ != quark)) {
    return 0;
  }
  // baryons: count quarks.
  if (baryon_number() != 0) {
    // for anti-baryons, the sign changes:
    return antiparticle_sign()*((digits_.n_q1_ == quark)
                              + (digits_.n_q2_ == quark)
                              + (digits_.n_q3_ == quark));
  }
  // mesons.
  // quarkonium state? Not open net_quark_number.
  if (digits_.n_q3_ == quark && digits_.n_q2_ == quark) {
    return 0;
  }
  // this has covered all the easy stuff
  // get the "other" quark. (We know this must exist, since they are
  // not both the right one and one of them is the right one).
  int otherquark = (digits_.n_q2_ == quark) ? digits_.n_q3_ : digits_.n_q2_;
  // "our" quark is the heavier one: 1 for u,c,t; -1 for d,s,b (and of
  // course the antiparticle sign)
  if (quark > otherquark) {
    return ((quark % 2 == 0) ? 1 : -1) * antiparticle_sign();
  }
  // ours is the lighter: If the heavier particle is u,c,t, the lighter
  // one (ours) is an antiquark.
  return ((otherquark % 2 == 0) ? -1 : 1) * antiparticle_sign();
}

std::ostream& operator<<(std::ostream& s, const PdgCode& code) {
  return s << code.string();
}

}  // namespace Smash
