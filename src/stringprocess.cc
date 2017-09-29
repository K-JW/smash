#include <array>

#include "include/stringprocess.h"
#include "include/angles.h"
#include "include/kinematics.h"
#include "include/random.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace Smash {

StringProcess::StringProcess() :
  pmin_gluon_lightcone_(0.001),
  pow_fgluon_beta_(0.5),
  pow_fquark_alpha_(1.),
  pow_fquark_beta_(2.5),
  sigma_qperp_(0.5),
  kappa_tension_string_(1.0) {

 // setup and initialize pythia
  pythia_ = make_unique<Pythia8::Pythia>(PYTHIA_XML_DIR, false);
  /* select only inelastic events: */
  pythia_->readString("SoftQCD:inelastic = on");
  /* suppress unnecessary output */
  pythia_->readString("Print:quiet = on");
  /* No resonance decays, since the resonances will be handled by SMASH */
  pythia_->readString("HadronLevel:Decay = off");
  /* transverse momentum spread in string fragmentation */
  pythia_->readString("StringPT:sigma = 0.25");
  /* manually set the parton distribution function */
  pythia_->readString("PDF:pSet = 13");
  pythia_->readString("PDF:pSetB = 13");
  pythia_->readString("PDF:piSet = 1");
  pythia_->readString("PDF:piSetB = 1");
  pythia_->readString("Beams:idA = 2212");
  pythia_->readString("Beams:idB = 2212");
  pythia_->readString("Beams:eCM = 10.");
  /* set particle masses and widths in PYTHIA to be same with those in SMASH */
  for (auto &ptype : ParticleType::list_all()){
    int pdgid = ptype.pdgcode().get_decimal();
    double mass_pole = ptype.mass();
    double width_pole = ptype.width_at_pole();
    /* check if the particle specie is in PYTHIA */
    if (pythia_->particleData.isParticle(pdgid)){
      /* set mass and width in PYTHIA */
      pythia_->particleData.m0(pdgid, mass_pole);
      pythia_->particleData.mWidth(pdgid, width_pole);
    }
  }
  pythia_->init();
  pythia_sigmatot_.init(&pythia_->info, pythia_->settings,
                        &pythia_->particleData);

  for (int imu = 0; imu < 4; imu++) {
    evecBasisAB_[imu] = ThreeVector(0., 0., 0.);
  }

  PPosA = 0.;
  PNegA = 0.;
  PPosB = 0.;
  PNegB = 0.;
  massA = massB = 0.;
  sqrtsAB_ = 0.;
  time_collision_ = 0.;
  gamma_factor_com_ = 1.;

  NpartFinal = 0;
  NpartString[0] = 0;
  NpartString[1] = 0;
  final_state.clear();
}

// compute the formation time and fill the arrays with final-state particles
int StringProcess::append_final_state(const FourVector &uString,
                                      const ThreeVector &evecLong) {
  struct fragment_type {
    FourVector momentum;
    double pparallel;  // longitudinal component of momentum
    double y;          // momentum rapidity
    double xtotfac;    // cross-section scaling factor
    PdgCode pdg;
    bool is_leading;   // is leading hadron or not
  };

  std::vector<fragment_type> fragments;
  double p_pos_tot = 0.0, p_neg_tot = 0.0;
  int bstring = 0;

  for (int ipyth = 0; ipyth < pythia_->event.size(); ipyth++) {
    if (!pythia_->event[ipyth].isFinal()) {
      continue;
    }
    int pythia_id = pythia_->event[ipyth].id();
    /* K_short and K_long need are converted to K0 since SMASH only knows K0 */
    if (pythia_id == 310 || pythia_id == 130) {
      pythia_id = (Random::uniform_int(0, 1) == 0) ? 311 : -311;
    }
    PdgCode pdg = PdgCode::from_decimal(pythia_id);
    if (!fragments[ipyth].pdg.is_hadron()) {
      throw std::invalid_argument("StringProcess::append_final_state warning :"
             " particle is not meson or baryon.");
    }
    FourVector mom(pythia_->event[ipyth].e(),
                   pythia_->event[ipyth].px(),
                   pythia_->event[ipyth].py(),
                   pythia_->event[ipyth].pz());
    double pparallel = mom.threevec() * evecLong;
    double y = 0.5 * std::log((mom.x0() + pparallel) / (mom.x0() - pparallel));
    fragments.push_back({mom, pparallel, y, 0.0, pdg, false});
    // total lightcone momentum
    p_pos_tot += (mom.x0() + pparallel) / std::sqrt(2.);
    p_neg_tot += (mom.x0() - pparallel) / std::sqrt(2.);
    bstring += pythia_->particleData.baryonNumberType(pythia_id);
  }
  const int nfrag = fragments.size();
  assert(nfrag > 0);
  // Sort the particles in descending order of momentum rapidity
  std::sort(fragments.begin(), fragments.end(),
            [&](fragment_type i, fragment_type j) { return i.y > j.y; });

  std::vector<double> xvertex_pos, xvertex_neg;
  xvertex_pos.resize(nfrag + 1);
  xvertex_neg.resize(nfrag + 1);
  // x^{+} coordinates of the forward end
  xvertex_pos[0] = p_pos_tot / kappa_tension_string_;
  for (int i = 0; i < nfrag; i++) {
    // recursively compute x^{+} coordinates of q-qbar formation vertex
    xvertex_pos[i + 1] = xvertex_pos[i] -
        (fragments[i].momentum.x0() + fragments[i].pparallel) /
        (kappa_tension_string_ * std::sqrt(2.));
  }
  // x^{-} coordinates of the backward end
  xvertex_neg[nfrag] = p_neg_tot / kappa_tension_string_;
  for (int i = nfrag - 1; i >= 0; i--) {
    // recursively compute x^{-} coordinates of q-qbar formation vertex
    xvertex_neg[i] = xvertex_neg[i + 1] -
        (fragments[i].momentum.x0() - fragments[i].pparallel) /
        (kappa_tension_string_ * std::sqrt(2.));
  }

  /* compute the cross section suppression factor for leading hadrons
   * based on the number of valence quarks. */
  if (bstring == 0) {  // mesonic string
    for (int i : {0, nfrag - 1}) {
      fragments[i].xtotfac = fragments[i].pdg.is_baryon() ? 1. / 3. : 0.5;
      fragments[i].is_leading = true;
    }
  } else if (bstring == 3 || bstring == -3) {  // baryonic string
    // The first baryon in forward direction
    int i = 0;
    while (i < nfrag && fragments[i].pdg.is_meson()) { i++; }
    fragments[i].xtotfac = 2. /  3.;
    fragments[i].is_leading = true;
    // The most backward meson
    i = nfrag - 1;
    while (i >= 0 && fragments[i].pdg.is_baryon()) { i--; }
    fragments[i].xtotfac = 0.5;
    fragments[i].is_leading = true;
  } else {
    throw std::invalid_argument("StringProcess::append_final_state"
        " encountered bstring != 0, 3, -3");
  }

  // Velocity three-vector to perform Lorentz boost.
  const ThreeVector vstring = uString.velocity();

  /* compute the formation times of hadrons
   * from the lightcone coordinates of q-qbar formation vertices. */
  for (int i = 0; i < nfrag; i++) {
    // set the formation time and position in the rest frame of string
    double t_prod = (xvertex_pos[i] + xvertex_neg[i + 1]) / std::sqrt(2.);
    FourVector fragment_position = FourVector(t_prod, evecLong *
            (xvertex_pos[i] - xvertex_neg[i + 1]) / std::sqrt(2.));
    // boost into the lab frame
    fragments[i].momentum = fragments[i].momentum.LorentzBoost(  -vstring );
    fragment_position = fragment_position.LorentzBoost(-vstring );
    t_prod = fragment_position.x0();

    ParticleData new_particle(ParticleType::find(fragments[i].pdg));
    new_particle.set_4momentum(fragments[i].momentum);

    constexpr double suppression_factor = 0.7;
    new_particle.set_cross_section_scaling_factor(fragments[i].is_leading ?
        suppression_factor * fragments[i].xtotfac : 0.);
    new_particle.set_formation_time(time_collision_ +
                                    gamma_factor_com_ * t_prod);
    final_state.push_back(new_particle);
  }

  return nfrag;
}

void StringProcess::init(const ParticleList &incoming,
                         double tcoll, double gamma){
  PDGcodes_[0] = incoming[0].pdgcode();
  PDGcodes_[1] = incoming[1].pdgcode();
  massA = incoming[0].effective_mass();
  massB = incoming[1].effective_mass();

  plab_[0] = incoming[0].momentum();
  plab_[1] = incoming[1].momentum();

  sqrtsAB_ = ( plab_[0] + plab_[1] ).abs();
  /* Transverse momentum transferred to strings,
     parametrization to fit the experimental data */
  sigma_qperp_ = (sqrtsAB_ < 4.) ? 0.5 :
    0.5 + 0.2 * std::log(sqrtsAB_ / 4.) / std::log(5.);

  ucomAB_ = (plab_[0] + plab_[1])/sqrtsAB_;
  vcomAB_ = ucomAB_.velocity();

  pcom_[0] = plab_[0].LorentzBoost(vcomAB_);
  pcom_[1] = plab_[1].LorentzBoost(vcomAB_);

  make_orthonormal_basis();
  compute_incoming_lightcone_momenta();

  time_collision_ = tcoll;
  gamma_factor_com_ = gamma;

}

/**
 * single diffractive
 * channel = 1 : A + B -> A + X
 * channel = 2 : A + B -> X + B
 */
bool StringProcess::next_SDiff(int channel) {
  bool ret;

  int ntry;
  bool foundPabsX, foundMassX;

  int pdgidH;
  double mstrMin = 0.;
  double mstrMax = 0.;
  double pabscomHX, massH, massX, rmass;
  double QTrn, QTrx, QTry;

  int nfrag;
  int idqX1, idqX2;

  FourVector pstrHcom;
  FourVector pstrHlab;
  FourVector pstrXcom;
  FourVector pstrXlab;
  ThreeVector threeMomentum;

  FourVector ustrHcom;
  FourVector ustrHlab;
  FourVector ustrXcom;
  FourVector ustrXlab;

  double pabs;
  FourVector pnull;
  FourVector prs;
  ThreeVector evec;

  NpartFinal = 0;
  NpartString[0] = 0;
  NpartString[1] = 0;
  final_state.clear();

  ntry = 0;
  foundPabsX = false;
  foundMassX = false;

  if( channel == 1 ) { // AB > AX
    mstrMin = massB;
    mstrMax = sqrtsAB_ - massA;
    pdgidH = PDGcodes_[0].get_decimal();
    massH = massA;
  } else if( channel == 2 ) { // AB > XB
    mstrMin = massA;
    mstrMax = sqrtsAB_ - massB;
    pdgidH = PDGcodes_[1].get_decimal();
    massH = massB;
  } else {
    throw std::runtime_error("invalid argument for StringProcess::next_SDiff");
  }

  while (((foundPabsX == false) || (foundMassX == false)) && (ntry < 100)) {
    ntry = ntry + 1;
    // decompose hadron into quarks
    if( channel == 1 ) { // AB > AX
      make_string_ends(PDGcodes_[1], idqX1, idqX2);
    } else if( channel == 2 ) { // AB > XB
      make_string_ends(PDGcodes_[0], idqX1, idqX2);
    }
    // sample the transverse momentum transfer
    QTrx = Random::normal(0., sigma_qperp_/std::sqrt(2.) );
    QTry = Random::normal(0., sigma_qperp_/std::sqrt(2.) );
    QTrn = std::sqrt(QTrx*QTrx + QTry*QTry);
    // sample the string mass and evaluate the three-momenta of hadron and string.
    rmass = std::log(mstrMax / mstrMin) * Random::uniform(0., 1.);
    massX = mstrMin * exp(rmass);
    pabscomHX = pCM( sqrtsAB_, massH, massX );
    // magnitude of the three momentum must be larger than the transvers momentum.
    foundPabsX = pabscomHX > QTrn;
    // string mass must be larger than threshold set by PYTHIA.
    foundMassX = massX > (pythia_->particleData.m0(idqX1) +
                          pythia_->particleData.m0(idqX2));
  }

  ret = false;
  if ((foundPabsX == true) && (foundMassX == true)) {
    double sign_direction = 0.;
    if( channel == 1 ) { // AB > AX
      sign_direction = 1.;
    } else if( channel == 2 ) { // AB > XB
      sign_direction = -1.;
    }
    threeMomentum = sign_direction * (
                        evecBasisAB_[3] * std::sqrt(pabscomHX*pabscomHX - QTrn*QTrn) +
                        evecBasisAB_[1] * QTrx +
                        evecBasisAB_[2] * QTry );
    pstrHcom = FourVector( std::sqrt(pabscomHX*pabscomHX + massH*massH), threeMomentum );
    threeMomentum = -sign_direction * (
                        evecBasisAB_[3] * std::sqrt(pabscomHX*pabscomHX - QTrn*QTrn) +
                        evecBasisAB_[1] * QTrx +
                        evecBasisAB_[2] * QTry );
    pstrXcom = FourVector( std::sqrt(pabscomHX*pabscomHX + massX*massX), threeMomentum );

    pstrHlab = pstrHcom.LorentzBoost( -vcomAB_ );
    pstrXlab = pstrXcom.LorentzBoost( -vcomAB_ );

    ustrHcom = pstrHcom / massH;
    ustrXcom = pstrXcom / massX;
    ustrHlab = pstrHlab / massH;
    ustrXlab = pstrXlab / massX;
    /* determin direction in which the string is stretched.
     * this is set to be same with the three-momentum of string
     * in the center of mass frame. */
    threeMomentum = pstrXcom.threevec();
    pnull = FourVector( threeMomentum.abs(), threeMomentum );
    prs = pnull.LorentzBoost( ustrXcom.velocity() );
    pabs = prs.threevec().abs();
    evec = prs.threevec() / pabs;
    // perform fragmentation and add particles to final_state.
    nfrag = fragment_string(idqX1, idqX2, massX, evec, false);
    if (nfrag > 0) {
      NpartString[0] = append_final_state(ustrXlab, evec);
    } else {
      nfrag = 0;
      NpartString[0] = 0;
      ret = false;
    }

    NpartString[1] = 1;
    const std::string s = std::to_string(pdgidH);
    PdgCode hadron_code(s);
    ParticleData new_particle(ParticleType::find(hadron_code));
    new_particle.set_4momentum(pstrHlab);
    new_particle.set_cross_section_scaling_factor(1.);
    new_particle.set_formation_time(0.);
    final_state.push_back(new_particle);

    if ((NpartString[0] > 0) && (NpartString[1] > 0) && (nfrag == NpartString[0])) {
      NpartFinal = NpartString[0] + NpartString[1];
      ret = true;
    }
  }

  return ret;
}

/** double-diffractive : A + B -> X + X */
bool StringProcess::next_DDiff() {
  bool ret;

  int ntry;
  bool foundMass1, foundMass2;

  double xfracA, xfracB;
  double QPos, QNeg;
  double QTrn, QTrx, QTry;

  int nfrag1, nfrag2;
  int idq11, idq12;
  int idq21, idq22;
  double mstr1, mstr2;

  FourVector pstr1com;
  FourVector pstr1lab;
  FourVector pstr2com;
  FourVector pstr2lab;
  ThreeVector threeMomentum;

  FourVector ustr1com;
  FourVector ustr1lab;
  FourVector ustr2com;
  FourVector ustr2lab;

  double pabs;
  FourVector pnull;
  FourVector prs;
  ThreeVector evec;

  NpartFinal = 0;
  NpartString[0] = 0;
  NpartString[1] = 0;
  final_state.clear();

  ntry = 0;
  foundMass1 = false;
  foundMass2 = false;
  while (((foundMass1 == false) || (foundMass2 == false)) && (ntry < 100)) {
    ntry = ntry + 1;

    make_string_ends(PDGcodes_[0], idq11, idq12);
    make_string_ends(PDGcodes_[1], idq21, idq22);
    // sample the lightcone momentum fraction carried by gluons
    const double xmin_gluon_fraction = pmin_gluon_lightcone_ / sqrtsAB_;
    xfracA = Random::beta_a0(xmin_gluon_fraction, pow_fgluon_beta_ + 1.);
    xfracB = Random::beta_a0(xmin_gluon_fraction, pow_fgluon_beta_ + 1.);
    // sample the transverse momentum transfer
    QTrx = Random::normal(0., sigma_qperp_/std::sqrt(2.) );
    QTry = Random::normal(0., sigma_qperp_/std::sqrt(2.) );
    QTrn = std::sqrt(QTrx*QTrx + QTry*QTry);
    // evaluate the lightcone momentum transfer
    QPos = -QTrn*QTrn / (2. * xfracB * PNegB);
    QNeg = QTrn*QTrn / (2. * xfracA * PPosA);
    // compute four-momentum of string 1
    threeMomentum = evecBasisAB_[3] * (PPosA + QPos - PNegA - QNeg) / std::sqrt(2.) +
                        evecBasisAB_[1] * QTrx + evecBasisAB_[2] * QTry;
    pstr1com = FourVector( (PPosA + QPos + PNegA + QNeg) / std::sqrt(2.), threeMomentum );
    mstr1 = pstr1com.sqr();
    // compute four-momentum of string 2
    threeMomentum = evecBasisAB_[3] * (PPosB - QPos - PNegB + QNeg) / std::sqrt(2.) -
                        evecBasisAB_[1] * QTrx - evecBasisAB_[2] * QTry;
    pstr2com = FourVector( (PPosB - QPos + PNegB - QNeg) / std::sqrt(2.), threeMomentum );
    mstr2 = pstr2com.sqr();
    // string mass must be larger than threshold set by PYTHIA.
    mstr1 = (mstr1 > 0.) ? std::sqrt(mstr1) : 0.;
    foundMass1 = mstr1 > (pythia_->particleData.m0(idq11) +
                          pythia_->particleData.m0(idq12));
    mstr2 = (mstr2 > 0.) ? std::sqrt(mstr2) : 0.;
    foundMass2 = mstr2 > (pythia_->particleData.m0(idq21) +
                          pythia_->particleData.m0(idq22));
  }

  ret = false;
  bool both_masses_above_pythia_threshold = foundMass1 && foundMass2;
  if ( both_masses_above_pythia_threshold ) {
    pstr1lab = pstr1com.LorentzBoost( -vcomAB_ );
    pstr2lab = pstr2com.LorentzBoost( -vcomAB_ );

    ustr1com = pstr1com / mstr1;
    ustr2com = pstr2com / mstr2;
    ustr1lab = pstr1lab / mstr1;
    ustr2lab = pstr2lab / mstr2;
    /* determin direction in which string 1 is stretched.
     * this is set to be same with the three-momentum of string
     * in the center of mass frame. */
    threeMomentum = pstr1com.threevec();
    pnull = FourVector( threeMomentum.abs(), threeMomentum );
    prs = pnull.LorentzBoost( ustr1com.velocity() );
    pabs = prs.threevec().abs();
    evec = prs.threevec() / pabs;
    // perform fragmentation and add particles to final_state.
    nfrag1 = fragment_string(idq11, idq12, mstr1, evec, false);
    if (nfrag1 > 0) {
      NpartString[0] = append_final_state(ustr1lab, evec);
    } else {
      nfrag1 = 0;
      NpartString[0] = 0;
      ret = false;
    }
    /* determin direction in which string 2 is stretched.
     * this is set to be same with the three-momentum of string
     * in the center of mass frame. */
    threeMomentum = pstr2com.threevec();
    pnull = FourVector( threeMomentum.abs(), threeMomentum );
    prs = pnull.LorentzBoost( ustr2com.velocity() );
    pabs = prs.threevec().abs();
    evec = prs.threevec() / pabs;
    // perform fragmentation and add particles to final_state.
    nfrag2 = fragment_string(idq21, idq22, mstr2, evec, false);
    if (nfrag2 > 0) {
      NpartString[1] = append_final_state(ustr2lab, evec);
    } else {
      nfrag2 = 0;
      NpartString[1] = 0;
      ret = false;
    }

    if ((NpartString[0] > 0) && (NpartString[1] > 0) && (nfrag1 == NpartString[0]) &&
        (nfrag2 == NpartString[1])) {
      NpartFinal = NpartString[0] + NpartString[1];
      ret = true;
    }
  }

  return ret;
}

/** non-diffractive */
bool StringProcess::next_NDiff() {
  bool ret;

  int ntry;
  bool foundMass1, foundMass2;

  double xfracA, xfracB;
  double QPos, QNeg;
  double dPPos, dPNeg;
  double QTrn, QTrx, QTry;

  int nfrag1, nfrag2;
  int idqA1, idqA2;
  int idqB1, idqB2;
  int idq11, idq12;
  int idq21, idq22;
  double mstr1, mstr2;

  FourVector pstr1com;
  FourVector pstr1lab;
  FourVector pstr2com;
  FourVector pstr2lab;
  ThreeVector threeMomentum;

  FourVector ustr1com;
  FourVector ustr1lab;
  FourVector ustr2com;
  FourVector ustr2lab;

  double pabs;
  FourVector pnull;
  FourVector prs;
  ThreeVector evec;

  NpartFinal = 0;
  NpartString[0] = 0;
  NpartString[1] = 0;
  final_state.clear();

  ntry = 0;
  foundMass1 = false;
  foundMass2 = false;
  while (((foundMass1 == false) || (foundMass2 == false)) && (ntry < 100)) {
    ntry = ntry + 1;

    make_string_ends(PDGcodes_[0], idqA1, idqA2);
    make_string_ends(PDGcodes_[1], idqB1, idqB2);

    const int baryonA = 3*PDGcodes_[0].baryon_number();
    const int baryonB = 3*PDGcodes_[1].baryon_number();
    if ((baryonA == 3) && (baryonB == 3)) {  // baryon-baryon
      idq11 = idqB1;
      idq12 = idqA2;
      idq21 = idqA1;
      idq22 = idqB2;
    } else if ((baryonA == 3) && (baryonB == 0)) {  // baryon-meson
      idq11 = idqB1;
      idq12 = idqA2;
      idq21 = idqA1;
      idq22 = idqB2;
    } else if ((baryonA == 3) && (baryonB == -3)) {  // baryon-antibaryon
      idq11 = idqB1;
      idq12 = idqA2;
      idq21 = idqA1;
      idq22 = idqB2;
    } else if ((baryonA == 0) && (baryonB == 3)) {  // meson-baryon
      idq11 = idqB1;
      idq12 = idqA2;
      idq21 = idqA1;
      idq22 = idqB2;
    } else if ((baryonA == 0) && (baryonB == 0)) {  // meson-meson
      idq11 = idqB1;
      idq12 = idqA2;
      idq21 = idqA1;
      idq22 = idqB2;
    } else if ((baryonA == 0) && (baryonB == -3)) {  // meson-antibaryon
      idq11 = idqA1;
      idq12 = idqB2;
      idq21 = idqB1;
      idq22 = idqA2;
    } else if ((baryonA == -3) && (baryonB == 3)) {  // antibaryon-baryon
      idq11 = idqA1;
      idq12 = idqB2;
      idq21 = idqB1;
      idq22 = idqA2;
    } else if ((baryonA == -3) && (baryonB == 0)) {  // antibaryon-meson
      idq11 = idqA1;
      idq12 = idqB2;
      idq21 = idqB1;
      idq22 = idqA2;
    } else if ((baryonA == -3) && (baryonB == -3)) {  // antibaryon-antibaryon
      idq11 = idqA1;
      idq12 = idqB2;
      idq21 = idqB1;
      idq22 = idqA2;
    } else {
      fprintf(stderr,
              "  StringProcess::next_NDiff : incorrect baryon number of incoming "
              "hadrons.\n");
      fprintf(stderr, "  StringProcess::next_NDiff : baryonA = %d, baryonB = %d\n",
              baryonA, baryonB);
      exit(1);
    }
    // sample the lightcone momentum fraction carried by quarks
    xfracA = Random::beta(pow_fquark_alpha_, pow_fquark_beta_);
    xfracB = Random::beta(pow_fquark_alpha_, pow_fquark_beta_);
    // sample the transverse momentum transfer
    QTrx = Random::normal(0., sigma_qperp_/std::sqrt(2.) );
    QTry = Random::normal(0., sigma_qperp_/std::sqrt(2.) );
    QTrn = std::sqrt(QTrx*QTrx + QTry*QTry);
    // evaluate the lightcone momentum transfer
    QPos = -QTrn*QTrn / (2. * xfracB * PNegB);
    QNeg = QTrn*QTrn / (2. * xfracA * PPosA);
    dPPos = -xfracA * PPosA - QPos;
    dPNeg = xfracB * PNegB - QNeg;
    // compute four-momentum of string 1
    threeMomentum = evecBasisAB_[3] * (PPosA + dPPos - PNegA - dPNeg) / std::sqrt(2.) +
                        evecBasisAB_[1] * QTrx + evecBasisAB_[2] * QTry;
    pstr1com = FourVector( (PPosA + dPPos + PNegA + dPNeg) / std::sqrt(2.), threeMomentum );
    mstr1 = pstr1com.sqr();
    // compute four-momentum of string 2
    threeMomentum = evecBasisAB_[3] * (PPosB - dPPos - PNegB + dPNeg) / std::sqrt(2.) -
                        evecBasisAB_[1] * QTrx - evecBasisAB_[2] * QTry;
    pstr2com = FourVector( (PPosB - dPPos + PNegB - dPNeg) / std::sqrt(2.), threeMomentum );
    mstr2 = pstr2com.sqr();
    // string mass must be larger than threshold set by PYTHIA.
    mstr1 = (mstr1 > 0.) ? std::sqrt(mstr1) : 0.;
    foundMass1 = mstr1 > (pythia_->particleData.m0(idq11) +
                          pythia_->particleData.m0(idq12));
    mstr2 = (mstr2 > 0.) ? std::sqrt(mstr2) : 0.;
    foundMass2 = mstr2 > (pythia_->particleData.m0(idq21) +
                          pythia_->particleData.m0(idq22));
  }

  ret = false;
  bool both_masses_above_pythia_threshold = foundMass1 && foundMass2;
  if ( both_masses_above_pythia_threshold ) {
    pstr1lab = pstr1com.LorentzBoost( -vcomAB_ );
    pstr2lab = pstr2com.LorentzBoost( -vcomAB_ );

    ustr1com = pstr1com / mstr1;
    ustr2com = pstr2com / mstr2;
    ustr1lab = pstr1lab / mstr1;
    ustr2lab = pstr2lab / mstr2;
    /* determin direction in which string 1 is stretched.
     * this is set to be same with the three-momentum of string
     * in the center of mass frame. */
    threeMomentum = pstr1com.threevec();
    pnull = FourVector( threeMomentum.abs(), threeMomentum );
    prs = pnull.LorentzBoost( ustr1com.velocity() );
    pabs = prs.threevec().abs();
    evec = prs.threevec() / pabs;
    // perform fragmentation and add particles to final_state.
    nfrag1 = fragment_string(idq11, idq12, mstr1, evec, false);
    if (nfrag1 > 0) {
      NpartString[0] = append_final_state(ustr1lab, evec);
    } else {
      nfrag1 = 0;
      NpartString[0] = 0;
      ret = false;
    }
    /* determin direction in which string 2 is stretched.
     * this is set to be same with the three-momentum of string
     * in the center of mass frame. */
    threeMomentum = pstr2com.threevec();
    pnull = FourVector( threeMomentum.abs(), threeMomentum );
    prs = pnull.LorentzBoost( ustr2com.velocity() );
    pabs = prs.threevec().abs();
    evec = prs.threevec() / pabs;
    // perform fragmentation and add particles to final_state.
    nfrag2 = fragment_string(idq21, idq22, mstr2, evec, false);
    if (nfrag2 > 0) {
      NpartString[1] = append_final_state(ustr2lab, evec);
    } else {
      nfrag2 = 0;
      NpartString[1] = 0;
      ret = false;
    }

    if ((NpartString[0] > 0) && (NpartString[1] > 0) && (nfrag1 == NpartString[0]) &&
        (nfrag2 == NpartString[1])) {
      NpartFinal = NpartString[0] + NpartString[1];
      ret = true;
    }
  }

  return ret;
}

/** baryon-antibaryon annihilation */
bool StringProcess::next_BBbarAnn() {
  const std::array<FourVector, 2> ustrlab = {ucomAB_, ucomAB_};

  NpartFinal = 0;
  NpartString[0] = 0;
  NpartString[1] = 0;
  final_state.clear();

  PdgCode baryon = PDGcodes_[0], antibaryon = PDGcodes_[1];
  if (baryon.baryon_number() == -1) {
    std::swap(baryon, antibaryon);
  }
  if (baryon.baryon_number() != 1 || antibaryon.baryon_number() != -1) {
    throw std::invalid_argument("Expected baryon-antibaryon pair.");
  }

  // Count how many qqbar combinations are possible for each quark type
  constexpr int n_q_types = 5;  // u, d, s, c, b
  std::vector<int> qcount_bar, qcount_antibar;
  std::vector<int> n_combinations;
  bool no_combinations = true;
  for (int i = 0; i < n_q_types; i++) {
    qcount_bar.push_back(baryon.net_quark_number(i));
    qcount_antibar.push_back(-antibaryon.net_quark_number(i));
    const int n_i = qcount_bar[i] * qcount_antibar[i];
    n_combinations.push_back(n_i);
    if (n_i > 0) {
      no_combinations = false;
    }
  }

  /* if it is a BBbar pair but there is no qqbar pair to annihilate,
   * nothing happens */
  if (no_combinations) {
    for (int i = 0; i < 2; i++) {
      NpartString[i] = 1;
      ParticleData new_particle(ParticleType::find(PDGcodes_[i]));
      new_particle.set_4momentum(plab_[i]);
      new_particle.set_cross_section_scaling_factor(1.);
      new_particle.set_formation_time(0.);
      final_state.push_back(new_particle);
    }
    NpartFinal = NpartString[0] + NpartString[1];
    return true;
  }

  // Select qqbar pair to annihilate and remove it away
  auto discrete_distr = Random::discrete_dist<int>(n_combinations);
  const int q_annihilate = discrete_distr() + 1;
  qcount_bar[q_annihilate - 1]--;
  qcount_antibar[q_annihilate - 1]--;

  // Get the remaining quarks and antiquarks
  std::vector<int> remaining_quarks, remaining_antiquarks;
  for (int i = 0; i < n_q_types; i++) {
    for (int j = 0; j < qcount_bar[i]; j++) {
      remaining_quarks.push_back(i + 1);
    }
    for (int j = 0; j < qcount_antibar[i]; j++) {
      remaining_antiquarks.push_back(-(i + 1));
    }
  }
  assert(remaining_quarks.size() == 2);
  assert(remaining_antiquarks.size() == 2);

  const int max_ntry = 100;
  const std::array<double,2> mstr = {0.5*sqrtsAB_, 0.5*sqrtsAB_};
  for (int ntry = 0; ntry < max_ntry; ntry++) {
    // Randomly select two quark-antiquark pairs
    if (Random::uniform_int(0, 1) == 0) {
      std::swap(remaining_quarks[0], remaining_quarks[1]);
    }
    if (Random::uniform_int(0, 1) == 0) {
      std::swap(remaining_antiquarks[0], remaining_antiquarks[1]);
    }
    // Make sure it satisfies kinematical threshold constraint
    bool kin_threshold_satisfied = true;
    for (int i = 0; i < 2; i++) {
      const double mstr_min = pythia_->particleData.m0(remaining_quarks[i]) +
                              pythia_->particleData.m0(remaining_antiquarks[i]);
      if (mstr_min > mstr[i]) {
        kin_threshold_satisfied = false;
      }
    }
    if (!kin_threshold_satisfied) {
      continue;
    }
    // Fragment two strings
    for (int i = 0; i < 2; i++) {
      ThreeVector evec = pcom_[i].threevec() / pcom_[i].threevec().abs();
      const int nfrag = fragment_string(remaining_quarks[i],
                            remaining_antiquarks[i], mstr[i], evec, false);
      if (nfrag <= 0) {
        NpartString[i] = 0;
        return false;
      }
      NpartString[i] = append_final_state(ustrlab[i], evec);
    }
    NpartFinal = NpartString[0] + NpartString[1];
    return true;
  }

  return false;
}

void StringProcess::make_incoming_com_momenta(){
}

void StringProcess::make_orthonormal_basis(){
  const double pabscomAB = pCM(sqrtsAB_, massA, massB);
  if (std::abs(pcom_[0].x3()) < (1. - 1.0e-8) * pabscomAB) {
    double ex, ey, et;
    double theta, phi;

    evecBasisAB_[3] = pcom_[0].threevec() / pabscomAB;

    theta = std::acos(evecBasisAB_[3].x3());

    ex = evecBasisAB_[3].x1();
    ey = evecBasisAB_[3].x2();
    et = std::sqrt(ex*ex + ey*ey);
    if (ey > 0.) {
      phi = std::acos(ex / et);
    } else {
      phi = -std::acos(ex / et);
    }

    evecBasisAB_[1].set_x1( cos(theta) * cos(phi) );
    evecBasisAB_[1].set_x2( cos(theta) * sin(phi) );
    evecBasisAB_[1].set_x3( -sin(theta) );

    evecBasisAB_[2].set_x1( -sin(phi) );
    evecBasisAB_[2].set_x2( cos(phi) );
    evecBasisAB_[2].set_x3( 0. );
  } else {
    if (pcom_[0].x3() > 0.) {
      evecBasisAB_[1] = ThreeVector(1., 0., 0.);
      evecBasisAB_[2] = ThreeVector(0., 1., 0.);
      evecBasisAB_[3] = ThreeVector(0., 0., 1.);
    } else {
      evecBasisAB_[1] = ThreeVector(0., 1., 0.);
      evecBasisAB_[2] = ThreeVector(1., 0., 0.);
      evecBasisAB_[3] = ThreeVector(0., 0., -1.);
    }
  }
}

void StringProcess::compute_incoming_lightcone_momenta(){
  PPosA = ( pcom_[0].x0() + evecBasisAB_[3] * pcom_[0].threevec() ) / std::sqrt(2.);
  PNegA = ( pcom_[0].x0() - evecBasisAB_[3] * pcom_[0].threevec() ) / std::sqrt(2.);
  PPosB = ( pcom_[1].x0() + evecBasisAB_[3] * pcom_[1].threevec() ) / std::sqrt(2.);
  PNegB = ( pcom_[1].x0() - evecBasisAB_[3] * pcom_[1].threevec() ) / std::sqrt(2.);
}

int StringProcess::diquark_from_quarks(int q1, int q2) {
  assert((q1 > 0 && q2 > 0) || (q1 < 0 && q2 < 0));
  if (std::abs(q1) < std::abs(q2)) {
    std::swap(q1, q2);
  }
  int diquark = std::abs(q1 * 1000 + q2 * 100);
  /* Adding spin degeneracy = 2S+1. For identical quarks spin cannot be 0
   * because of Pauli exclusion principle, so spin 1 is assumed. Otherwise
   * S = 0 with probability 1/4 and S = 1 with probability 3/4. */
  diquark += (q1 != q2 && Random::uniform_int(0, 3) == 0) ? 1 : 3;
  return (q1 < 0) ? -diquark : diquark;
}

void StringProcess::make_string_ends(const PdgCode &pdg,
                                     int &idq1, int &idq2) {
  std::array<int, 3> quarks = pdg.quark_content();

  if (pdg.is_meson()) {
    idq1 = quarks[1];
    idq2 = quarks[2];
    /* Some mesons with PDG id 11X are actually mixed state of uubar and ddbar.
     * have a random selection whether we have uubar or ddbar in this case. */
    if (idq1 == 1 && idq2 == -1 && Random::uniform_int(0, 1) == 0) {
      idq1 =  2;
      idq2 = -2;
    }
  } else {
    assert(pdg.is_baryon());
    // Get random quark to position 0
    std::swap(quarks[Random::uniform_int(0, 2)], quarks[0]);
    idq1 = quarks[0];
    idq2 = diquark_from_quarks(quarks[1], quarks[2]);
  }
  // Fulfil the convention: idq1 should be quark or anti-diquark
  if (idq1 < 0) {
    std::swap(idq1, idq2);
  }
}

int StringProcess::fragment_string(int idq1, int idq2, double mString,
                            ThreeVector &evecLong, bool random_rotation) {
  pythia_->event.reset();
  // evaluate 3 times total baryon number of the string
  const int bstring = pythia_->particleData.baryonNumberType(idq1) +
                      pythia_->particleData.baryonNumberType(idq2);
  /* diquark (anti-quark) with PDG id idq2 is going in the direction of evecLong.
   * quark with PDG id idq1 is going in the direction opposite to evecLong. */
  double sign_direction = 1.;
  if (bstring == -3) {  // anti-baryonic string
    /* anti-diquark with PDG id idq1 is going in the direction of evecLong.
     * anti-quark with PDG id idq2 is going in the direction
     * opposite to evecLong. */
    sign_direction = -1;
  }

  const double m1 = pythia_->particleData.m0(idq1);
  const double m2 = pythia_->particleData.m0(idq2);
  if (m1 + m2 > mString) {
    throw std::runtime_error("String fragmentation: m1 + m2 > mString");
  }

  // evaluate momenta of quarks
  const double pCMquark = pCM(mString, m1, m2);
  const double E1 = std::sqrt(m1*m1 + pCMquark*pCMquark);
  const double E2 = std::sqrt(m2*m2 + pCMquark*pCMquark);

  ThreeVector direction;
  if (random_rotation) {
    Angles phitheta;
    phitheta.distribute_isotropically();
    direction = phitheta.threevec();
  } else if (Random::uniform_int(0, 1) == 0) {
    /* in the case where we flip the string ends,
     * we need to flip the longitudinal unit vector itself
     * since it is set to be direction of diquark (anti-quark) or anti-diquark. */
    evecLong = -evecLong;
    direction = sign_direction * evecLong;
  }

  // For status and (anti)color see \iref{Sjostrand:2007gs}.
  const int status1 = 1, color1 = 1, anticolor1 = 0;
  Pythia8::Vec4 pquark = set_Vec4(E1, -direction * pCMquark);
  pythia_->event.append(idq1, status1, color1, anticolor1, pquark, m1);

  const int status2 = 1, color2 = 0, anticolor2 = 1;
  pquark = set_Vec4(E2, direction * pCMquark);
  pythia_->event.append(idq2, status2, color2, anticolor2, pquark, m2);

  // implement PYTHIA fragmentation
  const bool successful_hadronization = pythia_->forceHadronLevel();
  int number_of_fragments = 0;
  if (successful_hadronization) {
    for (int ipart = 0; ipart < pythia_->event.size(); ipart++) {
      if (pythia_->event[ipart].isFinal()) {
        number_of_fragments++;
      }
    }
  }

  return number_of_fragments;
}

}  // namespace Smash
