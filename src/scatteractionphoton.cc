/*
 *
 *    Copyright (c) 2016
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include <fstream>
#include <iostream>

#include "include/scatteractionphoton.h"

#include "include/angles.h"
#include "include/cxx14compat.h"
#include "include/integrate.h"
#include "include/kinematics.h"
#include "include/particletype.h"
#include "include/pdgcode.h"
#include "include/pow.h"
#include "include/random.h"
#include "include/tabulation.h"

using std::sqrt;
using std::pow;
using std::atan;

namespace Smash {

std::unique_ptr<Tabulation> tabulation_pi_pi_rho0 = nullptr;
std::unique_ptr<Tabulation> tabulation_pi0_pi_rho = nullptr;

void ScatterActionPhoton::generate_final_state() {
  /* Decide for a particular final state. */
  const CollisionBranch *proc = choose_channel<CollisionBranch>(
      collision_channels_photons_, cross_section_photons_);
  process_type_ = proc->get_type();
  outgoing_particles_ = proc->particle_list();

  /* The production point of the new particles.  */
  FourVector middle_point = get_interaction_point();

  /* 2->2 inelastic scattering */
  /* Sample the particle momenta in CM system. */
  const std::pair<double, double> masses = sample_masses();
  const double m1 = incoming_particles_[0].effective_mass();
  const double m2 = incoming_particles_[1].effective_mass();
  const double m3 = masses.first;
  const double s = mandelstam_s();
  const double sqrts = sqrt_s();
  std::array<double, 2> mandelstam_t = get_t_range(sqrts, m1, m2, m3, 0.0);
  const double t1 = mandelstam_t[1];
  const double t2 = mandelstam_t[0];

  const double pcm = cm_momentum();
  double diff_xsection_max = 0;
  double t = t1;
  double dummy = 0;
  while (t < t2) {
    dummy = diff_cross_section(t, m3);
    if (dummy > diff_xsection_max) {
      diff_xsection_max = dummy;
    }
    t = t + 0.01;
  }
  t = Random::uniform(t1, t2);
  dummy = 0;
  while (diff_cross_section(t, m3) < Random::uniform(0.0, diff_xsection_max)) {
    t = Random::uniform(t1, t2);
    dummy++;
    if (dummy > 100) break;
  }

  double costheta =
      (t - pow_int(m1, 2) +
       0.5 * (s + pow_int(m1, 2) - pow_int(m2, 2)) * (s - pow_int(m3, 2)) / s) /
      (pcm * (s - pow_int(m3, 2)) / sqrts);
  if (costheta > 1)
    costheta = 1;
  if (costheta < -1)
    costheta = -1;
  Angles phitheta(Random::uniform(0.0, twopi), costheta);
  outgoing_particles_[0].set_4momentum(masses.first, phitheta.threevec() * pcm);
  outgoing_particles_[1].set_4momentum(masses.second,
                                       -phitheta.threevec() * pcm);

  /* Weighing of the fractional photons */
  if (number_of_fractional_photons_ > 1) {
    weight_ = diff_cross_section(t, m3) * (t2 - t1)
          / (number_of_fractional_photons_ * cross_section());
  } else {
    weight_ = proc->weight() / cross_section();
  }
  /* Set positions & boost to computational frame. */
  for (ParticleData &new_particle : outgoing_particles_) {
    new_particle.set_4position(middle_point);
    new_particle.boost_momentum(-beta_cm());
  }
}

void ScatterActionPhoton::add_dummy_hadronic_channels(
                            float reaction_cross_section) {
  CollisionBranchPtr dummy_process = make_unique<CollisionBranch>(
    incoming_particles_[0].type(),
    incoming_particles_[1].type(),
    reaction_cross_section,
    ProcessType::TwoToTwo);
  add_collision(std::move(dummy_process));
}

ScatterActionPhoton::ReactionType
  ScatterActionPhoton::is_photon_reaction(const ParticleList &in) {
    if (in.size() != 2) {
      return ReactionType::no_reaction;
    }

  PdgCode a = in[0].pdgcode();
  PdgCode b = in[1].pdgcode();

  // swap so that pion should be first and there are less cases to be listed

  if (!a.is_pion()) {
    std::swap(a, b);
  }

  switch (pack(a.code(), b.code())) {
    case(pack(pdg::pi_p, pdg::pi_z)):
    case(pack(pdg::pi_z, pdg::pi_p)):
    case(pack(pdg::pi_m, pdg::pi_z)):
    case(pack(pdg::pi_z, pdg::pi_m)):
      return ReactionType::pi0_pi;
    case(pack(pdg::pi_p, pdg::rho_z)):
    case(pack(pdg::pi_m, pdg::rho_z)):
      return ReactionType::pi_rho0;
    case(pack(pdg::pi_m, pdg::rho_p)):
    case(pack(pdg::pi_p, pdg::rho_m)):
      return ReactionType::pi_rho;
    case(pack(pdg::pi_z, pdg::rho_p)):
    case(pack(pdg::pi_z, pdg::rho_m)):
      return ReactionType::pi0_rho;
    case(pack(pdg::pi_p, pdg::eta)):
    case(pack(pdg::pi_m, pdg::eta)):
      return ReactionType::pi_eta;
    case(pack(pdg::pi_p, pdg::pi_m)):
    case(pack(pdg::pi_m, pdg::pi_p)):
      return ReactionType::pi_pi;
    case(pack(pdg::pi_z, pdg::rho_z)):
      return ReactionType::pi0_rho0;
    default:
      return ReactionType::no_reaction;
  }
}

CollisionBranchList ScatterActionPhoton::photon_cross_sections() {
  CollisionBranchList process_list;
  ParticleTypePtr rho0_particle = &ParticleType::find(pdg::rho_z);
  ParticleTypePtr rho_plus_particle = &ParticleType::find(pdg::rho_p);
  ParticleTypePtr rho_minus_particle = &ParticleType::find(pdg::rho_m);
  ParticleTypePtr eta_particle = &ParticleType::find(pdg::eta);
  ParticleTypePtr pi0_particle = &ParticleType::find(pdg::pi_z);
  ParticleTypePtr pi_plus_particle = &ParticleType::find(pdg::pi_p);
  ParticleTypePtr pi_minus_particle = &ParticleType::find(pdg::pi_m);
  ParticleTypePtr photon_particle = &ParticleType::find(pdg::photon);
  ParticleTypePtr omega_particle = &ParticleType::find(pdg::omega);
  const float m_rho = rho0_particle->mass();
  const float m_rho_2 = pow_int(m_rho, 2);
  const float m_pi = pi0_particle->mass();
  const float m_pi_2 = pow_int(m_pi, 2);
  const float m_eta = eta_particle->mass();
  const float m_eta_2 = pow_int(m_eta, 2);
  const float gamma_rho_tot = rho0_particle->width_at_pole();
//  const float g_rho_2 = 24 * twopi * gamma_rho_tot * pow_int(m_rho, 2) /
//                        pow(pow_int(m_rho, 2) - 4 * pow_int(m_pi, 2), 3.0/2.0);
  const float g_rho_2 = 2.9*4.0*M_PI;
  const float to_mb = 0.3894;
  const float Const = 0.059;
  const float g_POR = 25.8;
  const float ma1 = 1.26;
  const float ghat = 6.4483;
  const float eta1 = 2.3920;
  const float eta2 = 1.9430;
  const float delta = -0.6426;
  const float C4 = -0.14095;
  const float Gammaa1 = 0.4;
  const float Pi = M_PI;
  float m_omega = omega_particle->mass();

  ParticleData part_a = incoming_particles_[0];
  ParticleData part_b = incoming_particles_[1];

  bool pion_found = true;

  if (!part_a.type().pdgcode().is_pion()) {
    if (part_b.type().pdgcode().is_pion()) {
      part_a = incoming_particles_[1];
      part_b = incoming_particles_[0];
    } else {
      pion_found = false;
    }
  }

  if (pion_found) {
    // do a check according to incoming_particles_ and calculate the
    // cross sections (xsection) for all possible reactions

    float mrho = rho0_particle->mass();
    float m_rho_case = rho0_particle->mass();
    const double s = mandelstam_s();
    double sqrts = sqrt_s();
    const double &m1 = part_a.effective_mass();
    const double &m2 = part_b.effective_mass();
    double m3 = 0.0;  // will be fixed according to reaction outcome
    const double p_cm_2 = cm_momentum_squared();
    ParticleTypePtr part_out = photon_particle;
    ParticleTypePtr photon_out = photon_particle;

    reac = is_photon_reaction(Action::incoming_particles());

    if (sqrts <= m1 + m2) {
      reac = ReactionType::no_reaction;
    }

    if (reac != ReactionType::no_reaction) {

      std::array<double, 2> mandelstam_t = get_t_range(sqrts, m1, m2, m3, 0.0);
      double t1 = mandelstam_t[1];
      double t2 = mandelstam_t[0];

      double u1 = pow_int(m1, 2) + pow_int(m2, 2) + pow_int(m3, 2) - s - t1;
      double u2 = pow_int(m1, 2) + pow_int(m2, 2) + pow_int(m3, 2) - s - t2;

      double e, I0, I1;
      float xsection = 0.0;

      Integrator1dMonte integrate;

      switch (reac) {
        case ReactionType::pi_pi:  // there are three possible reaction channels
          // the first possible reaction has part_out = photon_particle with
          // m3 = 0, which is the default declared above

          // now the second possible reaction (produces eta)
          part_out = eta_particle;
          m3 = part_out->mass();

          if (sqrts > m3) {
            mandelstam_t = get_t_range(sqrts, m1, m2, m3, 0.0);
            t1 = mandelstam_t[1];
            t2 = mandelstam_t[0];
            u1 = pow_int(m1, 2) + pow_int(m2, 2) + pow_int(m3, 2) - s - t1;
            u2 = pow_int(m1, 2) + pow_int(m2, 2) + pow_int(m3, 2) - s - t2;
            xsection = M_PI * alpha * 4.7 * pow(m_rho, 4) /
                       (pow_int(s - m_rho_2, 2) + pow_int(gamma_rho_tot, 2) *
                       m_rho_2)/(16 * m_eta_2 * pow_int(m_rho, 4) * s * p_cm_2);
            xsection = xsection *
                       ((2 * m_pi_2 + m_eta_2 - s) * s / 2 *
                            (pow_int(t2, 2) - pow_int(t1, 2)) -
                        s / 3 * (pow_int(t2, 3) - pow_int(t1, 3)) -
                        (t2 - t1) * m_pi_2 *
                            (pow_int(m_eta, 4) + s * (m_pi_2 - m_eta_2))) *
                       to_mb;
            process_list.push_back(make_unique<CollisionBranch>(
                *part_out, *photon_out, xsection, ProcessType::TwoToTwo));
          }

          // and the third possible reaction (produces rho0)

          part_out = rho0_particle;
          m3 = part_out->mass();

          if (sqrts > m3) {
          if (gamma_rho_tot > really_small) {
            if (tabulation_pi_pi_rho0 == nullptr) {
              tabulation_pi_pi_rho0 = make_unique<Tabulation>(
                2.0f * m_pi, 15.0f - 2.0f * m_pi, num_tab_pts_,
                  [&](float sqrts1) {
                    return integrate(2.0f * m_pi, sqrts1, [&](float M) {
                      return pi_pi_rho0(M, pow_int(sqrts1, 2)) *
                             part_out->spectral_function(M);
                    });
                  });
            }
            xsection = tabulation_pi_pi_rho0->get_value_linear(sqrts);
          } else {
            xsection = pi_pi_rho0(m3, pow_int(sqrts, 2));
          }
          process_list.push_back(make_unique<CollisionBranch>(
              *part_out, *photon_out, xsection, ProcessType::TwoToTwo));
          }

          //Photon channel
          part_out = photon_particle;
          m3 = 0.0;

          xsection = twopi * pow_int(alpha, 2) / (s * p_cm_2);
          t1 += -m_pi_2;  // is t+
          t2 += -m_pi_2;
          u1 = -s - t1;
          u2 = -s - t2;
          e = t2 - t1 +
              2 * m_pi_2 * ((1 - 2 * m_pi_2 / s) * std::log(t2 / t1) +
                            m_pi_2 * (t2 - t1) / (t1 * t2));
          e += 2 * m_pi_2 * ((1 - 2 * m_pi_2 / s) * std::log(u1 / u2) +
                             m_pi_2 * (u1 - u2) / (u1 * u2));
          xsection = xsection * e * to_mb;
          process_list.push_back(make_unique<CollisionBranch>(
              *part_out, *photon_out, xsection, ProcessType::TwoToTwo));
          break;

         case ReactionType::pi0_pi:
          if (part_a.type().pdgcode() == pdg::pi_p ||
              part_b.type().pdgcode() == pdg::pi_p) {
            part_out = rho_plus_particle;
          } else {
            part_out = rho_minus_particle;
          }
           m3 = part_out->mass();
          if (sqrts > m3) {
           if (gamma_rho_tot > really_small) {
             if (tabulation_pi0_pi_rho == nullptr) {
               tabulation_pi0_pi_rho = make_unique<Tabulation>(
                 2.0f * m_pi, 15.0f - 2.0f * m_pi, num_tab_pts_,
                 [&](float sqrts1) {
                   return integrate(2.0f * m_pi, sqrts1, [&](float M) {
                      return pi_pi0_rho(M, pow_int(sqrts1, 2)) *
                             part_out->spectral_function(M);
                    });
                  });
            }
            xsection = tabulation_pi0_pi_rho->get_value_linear(sqrts);
          } else {
             xsection = pi_pi0_rho(m3, pow_int(sqrts, 2));
          }
          process_list.push_back(make_unique<CollisionBranch>(
              *part_out, *photon_out, xsection, ProcessType::TwoToTwo));
          break;
        }

        case ReactionType::pi_rho0:
          if (part_a.type().pdgcode() == pdg::pi_p) {
            part_out = pi_plus_particle;
          } else {
            part_out = pi_minus_particle;
          }
          m3 = part_out->mass();

          if (incoming_particles_[0].pdgcode().is_rho()){
            mrho = incoming_particles_[0].effective_mass();
          } else {
            mrho = incoming_particles_[1].effective_mass();
          }

        //  xsection = alpha * g_rho_2 / (12 * s * p_cm_2);
        //  t1 += -m_pi_2;
        //  t2 += -m_pi_2;
        /*  xsection = xsection * (2 * (t2 - t1) -
                                 s * (pow_int(m2, 2) - 4 * m_pi_2) /
                                     pow_int(s - m_pi_2, 2) * (t2 - t1) -
                                 (pow_int(m2, 2) - 4 * m_pi_2) *
                                     ((s - pow_int(m2, 2)
                                     +m_pi_2)/(s - m_pi_2) *
                                          std::log(t2 / t1) +
                                      m_pi_2 * (t2 - t1) / (t1 * t2))) *
                     to_mb; */

                     xsection = to_mb*(pow(Const,2)*pow(ghat,4)*((pow(eta1 - eta2,2)*(-2*eta1*eta2*
                      (pow(ma1,8) + pow(m_pi,8) - pow(m_pi,4)*pow(mrho,4) - 2*pow(ma1,2)*pow(m_pi,2)*(pow(m_pi,2) - pow(mrho,2))*(pow(mrho,2) + s) +
                        pow(ma1,6)*(-4*pow(m_pi,2) + 2*s) + pow(ma1,4)*
                         (4*pow(m_pi,4) - pow(mrho,4) + 2*pow(m_pi,2)*(pow(mrho,2) - 2*s) - 2*pow(mrho,2)*s + 2*pow(s,2))) +
                     pow(eta2,2)*(pow(ma1,8) + pow(m_pi,4)*pow(pow(m_pi,2) - pow(mrho,2),2) + 2*pow(ma1,6)*(-2*pow(m_pi,2) + pow(mrho,2) + s) +
                        2*pow(ma1,2)*pow(m_pi,2)*(-pow(mrho,4) + pow(m_pi,2)*(2*pow(mrho,2) - s) + pow(mrho,2)*s) +
                        pow(ma1,4)*(4*pow(m_pi,4) + pow(mrho,4) - 2*pow(mrho,2)*s + 2*pow(s,2) - 4*pow(m_pi,2)*(pow(mrho,2) + s))) +
                     pow(eta1,2)*(pow(ma1,8) + pow(m_pi,8) - 2*pow(m_pi,6)*pow(mrho,2) - 2*pow(ma1,6)*(2*pow(m_pi,2) + pow(mrho,2) - s) -
                        2*pow(m_pi,2)*pow(mrho,4)*s + pow(m_pi,4)*(3*pow(mrho,4) + 2*pow(mrho,2)*s) +
                        pow(ma1,4)*(4*pow(m_pi,4) + pow(mrho,4) + pow(m_pi,2)*(8*pow(mrho,2) - 4*s) - 4*pow(mrho,2)*s + 2*pow(s,2)) -
                        2*pow(ma1,2)*(pow(mrho,2)*s*(-pow(mrho,2) + s) + pow(m_pi,4)*(3*pow(mrho,2) + s) + pow(m_pi,2)*(2*pow(mrho,4) - 3*pow(mrho,2)*s)))))
                  /((pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))*(pow(ma1,2) - t2)) +
                (8*pow(-2 + delta,2)*pow(m_pi,2)*(4*pow(m_pi,2) - pow(mrho,2)))/
                 ((pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))*(pow(m_pi,2) - t2)) -
                (8*pow(-2 + delta,2)*pow(m_pi,2)*t2)/(pow(mrho,2)*pow(pow(m_pi,2) - s,2)) -
                (8*pow(-2 + delta,2)*pow(m_pi,2)*t2)/(pow(mrho,2)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (8*(-2 + delta)*(-8*C4*pow(mrho,4) + pow(m_pi,2)*(2 + delta - 8*C4*pow(mrho,2)) - (2 + 3*delta)*s + pow(mrho,2)*(-2 + 3*delta + 16*C4*s))*t2)/
                 (pow(mrho,2)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (4*(-2 + delta)*(eta1 - eta2)*(pow(ma1,2) - s)*(eta2*(pow(m_pi,2) + pow(mrho,2) - 2*s)*(pow(m_pi,2) + s) +
                     eta1*(-2*pow(m_pi,4) + pow(mrho,4) - 3*pow(mrho,2)*s + 2*pow(s,2) + pow(m_pi,2)*(pow(mrho,2) + s)))*t2)/
                 ((pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (pow(eta1 - eta2,2)*(pow(eta1,2)*(3*pow(ma1,4) + 4*pow(m_pi,4) + pow(mrho,4) + pow(m_pi,2)*(8*pow(mrho,2) - 4*s) -
                        4*pow(ma1,2)*(2*pow(m_pi,2) + pow(mrho,2) - s) - 4*pow(mrho,2)*s + 2*pow(s,2)) +
                     pow(eta2,2)*(3*pow(ma1,4) + 4*pow(m_pi,4) + pow(mrho,4) - 2*pow(mrho,2)*s + 2*pow(s,2) - 4*pow(m_pi,2)*(pow(mrho,2) + s) +
                        4*pow(ma1,2)*(-2*pow(m_pi,2) + pow(mrho,2) + s)) -
                     2*eta1*eta2*(3*pow(ma1,4) + 4*pow(m_pi,4) - pow(mrho,4) + 2*pow(m_pi,2)*(pow(mrho,2) - 2*s) - 2*pow(mrho,2)*s + 2*pow(s,2) +
                        pow(ma1,2)*(-8*pow(m_pi,2) + 4*s)))*t2)/(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s)) +
                (8*(pow(delta,2)*(8*pow(m_pi,4) + 3*pow(mrho,4) + 4*pow(m_pi,2)*(3*pow(mrho,2) - 2*s) - 6*pow(mrho,2)*s + 2*pow(s,2)) +
                     4*pow(mrho,4)*(3 + 12*C4*(2*pow(m_pi,2) - s) + 8*pow(C4,2)*pow(-2*pow(m_pi,2) + s,2)) -
                     4*delta*pow(mrho,2)*(16*C4*pow(m_pi,4) + 2*pow(m_pi,2)*(3 + 6*C4*pow(mrho,2) - 8*C4*s) + pow(mrho,2)*(3 - 6*C4*s) + s*(-3 + 4*C4*s)))*t2)/
                 (pow(mrho,4)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (8*(-2 + delta)*(pow(m_pi,4)*(-2 + 3*delta - 8*C4*pow(mrho,2)) + (pow(mrho,2) - s)*((-2 + 3*delta)*s + pow(mrho,2)*(-2 + delta - 8*C4*s)) +
                     4*pow(m_pi,2)*(2*C4*pow(mrho,4) + delta*s - pow(mrho,2)*(-1 + delta + 4*C4*s)))*t2)/
                 (pow(mrho,2)*(pow(m_pi,2) - s)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (4*(-2 + delta)*(eta1 - eta2)*(pow(ma1,2) - s)*(eta2*(pow(m_pi,2) + s)*(pow(m_pi,4) - pow(m_pi,2)*(pow(mrho,2) - 2*s) + (pow(mrho,2) - s)*s) +
                     eta1*(-4*pow(m_pi,6) + pow(pow(mrho,2) - s,2)*s + pow(m_pi,4)*(3*pow(mrho,2) + s) -
                        pow(m_pi,2)*(pow(mrho,4) - pow(mrho,2)*s + 2*pow(s,2))))*t2)/
                 ((pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,2) - s)*
                   (pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (pow(eta1 - eta2,2)*(-2*eta1*eta2*(pow(m_pi,8) - pow(mrho,4)*pow(s,2) + pow(s,4) -
                        pow(m_pi,4)*(pow(mrho,4) + 2*pow(mrho,2)*s - 4*pow(s,2)) + 2*pow(m_pi,2)*s*(pow(mrho,4) + pow(mrho,2)*s - 2*pow(s,2))) +
                     pow(eta2,2)*(pow(m_pi,8) - 2*pow(m_pi,6)*pow(mrho,2) + pow(s,2)*pow(pow(mrho,2) + s,2) + pow(m_pi,4)*pow(pow(mrho,2) + 2*s,2) -
                        2*pow(m_pi,2)*s*(pow(mrho,4) + 2*pow(mrho,2)*s + 2*pow(s,2))) +
                     pow(eta1,2)*(pow(m_pi,8) - 2*pow(m_pi,6)*pow(mrho,2) - 4*pow(m_pi,2)*pow(pow(mrho,2) - s,2)*s + pow(pow(mrho,2) - s,2)*pow(s,2) +
                        pow(m_pi,4)*(3*pow(mrho,4) - 6*pow(mrho,2)*s + 4*pow(s,2))))*t2)/
                 ((pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (2*pow(eta1 - eta2,2)*(pow(ma1,2) - s)*(pow(eta1,2)*(pow(ma1,4)*s + pow(m_pi,4)*(-3*pow(mrho,2) + 2*s) +
                        s*(2*pow(mrho,4) - 3*pow(mrho,2)*s + pow(s,2)) - 2*pow(m_pi,2)*(pow(mrho,4) - 4*pow(mrho,2)*s + 2*pow(s,2)) +
                        pow(ma1,2)*(2*pow(m_pi,2)*(pow(mrho,2) - 2*s) + 3*s*(-pow(mrho,2) + s))) -
                     2*eta1*eta2*(pow(ma1,4)*s + s*(2*pow(m_pi,4) + 4*pow(m_pi,2)*(pow(mrho,2) - s) + s*(-2*pow(mrho,2) + s)) +
                        pow(ma1,2)*(pow(m_pi,2)*(pow(mrho,2) - 4*s) + s*(-2*pow(mrho,2) + 3*s))) +
                     pow(eta2,2)*(-4*pow(m_pi,2)*s*(pow(ma1,2) + pow(mrho,2) + s) + pow(m_pi,4)*(pow(mrho,2) + 2*s) +
                        s*(pow(ma1,4) + s*(pow(mrho,2) + s) + pow(ma1,2)*(pow(mrho,2) + 3*s))))*t2)/
                 ((pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (4*(eta1 - eta2)*(pow(ma1,2) - s)*(eta1*(-4*pow(m_pi,4)*(6*C4*pow(mrho,4) + 2*delta*s + pow(mrho,2)*(1 - 2*delta - 8*C4*s)) +
                        2*pow(m_pi,2)*(4*delta*pow(s,2) + pow(mrho,2)*s*(6 - 7*delta - 16*C4*s) + 2*pow(mrho,4)*(-2 + delta + 8*C4*s)) -
                        (pow(mrho,2) - s)*s*(-2*delta*s + pow(mrho,2)*(-6 + 3*delta + 8*C4*s))) +
                     eta2*(delta*(2*pow(m_pi,4)*(pow(mrho,2) + 4*s) + pow(m_pi,2)*(2*pow(mrho,4) + pow(mrho,2)*s - 8*pow(s,2)) +
                           s*(-2*pow(mrho,4) - pow(mrho,2)*s + 2*pow(s,2))) -
                        2*pow(mrho,2)*(4*C4*pow(m_pi,4)*(pow(mrho,2) + 4*s) + pow(m_pi,2)*(s*(5 - 16*C4*s) + pow(mrho,2)*(2 - 8*C4*s)) +
                           s*(s*(-3 + 4*C4*s) + pow(mrho,2)*(-2 + 4*C4*s)))))*t2)/
                 (pow(mrho,2)*(pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*
                   (pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (8*(eta1 - eta2)*(delta*(eta1*(4*pow(m_pi,6) + pow(m_pi,4)*(7*pow(mrho,2) - 8*s) + pow(ma1,4)*(pow(m_pi,2) - s) -
                           pow(ma1,2)*(2*pow(m_pi,2) + pow(mrho,2) - 2*s)*(2*pow(m_pi,2) - s) + pow(m_pi,2)*s*(-8*pow(mrho,2) + 5*s) +
                           s*(pow(mrho,4) + pow(mrho,2)*s - pow(s,2))) +
                        eta2*(-4*pow(m_pi,6) - pow(m_pi,4)*(pow(mrho,2) - 8*s) + pow(ma1,4)*(-pow(m_pi,2) + s) +
                           pow(m_pi,2)*(2*pow(mrho,4) - 5*pow(s,2)) + s*(-2*pow(mrho,4) + pow(mrho,2)*s + pow(s,2)) +
                           pow(ma1,2)*(4*pow(m_pi,4) - 6*pow(m_pi,2)*s + s*(pow(mrho,2) + 2*s)))) -
                     2*pow(mrho,2)*(eta1*(8*C4*pow(m_pi,6) + pow(m_pi,4)*(3 + 8*C4*(pow(mrho,2) - 2*s)) + 2*C4*pow(ma1,4)*(pow(m_pi,2) - s) +
                           2*pow(m_pi,2)*s*(-1 - 6*C4*pow(mrho,2) + 5*C4*s) -
                           pow(ma1,2)*(8*C4*pow(m_pi,4) + pow(m_pi,2)*(1 + 2*C4*(pow(mrho,2) - 6*s)) + 2*C4*s*(-pow(mrho,2) + 2*s)) +
                           s*(-(s*(1 + 2*C4*s)) + pow(mrho,2)*(1 + 4*C4*s))) +
                        eta2*(2*C4*pow(ma1,4)*(-pow(m_pi,2) + s) - (pow(m_pi,2) - s)*
                            (8*C4*pow(m_pi,4) - 2*pow(mrho,2) + s + 2*C4*pow(s,2) + pow(m_pi,2)*(3 - 4*C4*(pow(mrho,2) + 2*s))) +
                           pow(ma1,2)*(8*C4*pow(m_pi,4) + 2*C4*s*(pow(mrho,2) + 2*s) + pow(m_pi,2)*(1 - 2*C4*(pow(mrho,2) + 6*s))))))*t2)/
                 (pow(mrho,2)*(pow(m_pi,2) - s)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (8*(-2 + delta)*(delta - 4*C4*pow(mrho,2))*pow(t2,2))/
                 (pow(mrho,2)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (16*(-2 + delta)*(delta - 4*C4*pow(mrho,2))*s*pow(t2,2))/
                 (pow(mrho,2)*(pow(m_pi,2) - s)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (pow(eta1 - eta2,2)*(pow(eta1,2)*(pow(mrho,2) - s) + 2*eta1*eta2*s - pow(eta2,2)*s)*
                   (pow(m_pi,4) - pow(m_pi,2)*(pow(mrho,2) - 2*s) + (pow(mrho,2) - s)*s)*pow(t2,2))/
                 ((pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (2*(-2 + delta)*(eta1 - eta2)*(pow(ma1,2) - s)*(-(eta1*(pow(m_pi,2) + 2*pow(mrho,2) - 3*s)) - eta2*(pow(m_pi,2) + s))*pow(t2,2))/
                 ((pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (2*(-2 + delta)*(eta1 - eta2)*(pow(ma1,2) - s)*(pow(m_pi,2) + s)*(-2*eta2*s + eta1*(pow(m_pi,2) - pow(mrho,2) + s))*pow(t2,2))/
                 ((pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,2) - s)*
                   (pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (pow(eta1 - eta2,3)*(eta1*(pow(ma1,2) - 2*pow(m_pi,2) - pow(mrho,2) + s) - eta2*(pow(ma1,2) - 2*pow(m_pi,2) + pow(mrho,2) + s))*
                   pow(t2,2))/(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s)) -
                (8*(delta - 4*C4*pow(mrho,2))*(delta*(4*pow(m_pi,2) + 3*pow(mrho,2) - 2*s) - 2*pow(mrho,2)*(3 + 8*C4*pow(m_pi,2) - 4*C4*s))*pow(t2,2))/
                 (pow(mrho,4)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (pow(eta1 - eta2,2)*(pow(ma1,2) - s)*(pow(eta2,2)*s*(pow(ma1,2) - 4*pow(m_pi,2) + pow(mrho,2) + 3*s) +
                     pow(eta1,2)*(2*pow(m_pi,2)*(pow(mrho,2) - 2*s) + s*(pow(ma1,2) - 3*pow(mrho,2) + 3*s)) -
                     2*eta1*eta2*(pow(m_pi,2)*(pow(mrho,2) - 4*s) + s*(pow(ma1,2) - 2*pow(mrho,2) + 3*s)))*pow(t2,2))/
                 ((pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (2*(eta1 - eta2)*(pow(ma1,2) - s)*(eta1*(4*delta*pow(s,2) - 2*pow(mrho,2)*s*(-2 + 3*delta + 8*C4*s) + pow(mrho,4)*(-2 + delta + 16*C4*s) -
                        2*pow(m_pi,2)*(8*C4*pow(mrho,4) + 4*delta*s + pow(mrho,2)*(2 - 3*delta - 16*C4*s))) +
                     eta2*(pow(m_pi,2)*(8*delta*s + pow(mrho,2)*(-2 + delta - 32*C4*s)) + s*(-4*delta*s + pow(mrho,2)*(-2 + delta + 16*C4*s))))*pow(t2,2))/
                 (pow(mrho,2)*(pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*
                   (pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (4*(eta1 - eta2)*(delta*(eta1*(pow(ma1,2)*(pow(m_pi,2) - s) - (2*pow(m_pi,2) + pow(mrho,2) - 2*s)*(2*pow(m_pi,2) - s)) +
                        eta2*(4*pow(m_pi,4) - 6*pow(m_pi,2)*s + pow(ma1,2)*(-pow(m_pi,2) + s) + s*(pow(mrho,2) + 2*s))) +
                     2*pow(mrho,2)*(eta1*(8*C4*pow(m_pi,4) + 2*C4*s*(pow(ma1,2) - pow(mrho,2) + 2*s) +
                           pow(m_pi,2)*(1 - 2*C4*(pow(ma1,2) - pow(mrho,2) + 6*s))) -
                        eta2*(8*C4*pow(m_pi,4) + 2*C4*s*(pow(ma1,2) + pow(mrho,2) + 2*s) - pow(m_pi,2)*(-1 + 2*C4*(pow(ma1,2) + pow(mrho,2) + 6*s)))))*
                   pow(t2,2))/(pow(mrho,2)*(pow(m_pi,2) - s)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (pow(eta1 - eta2,4)*pow(t2,3))/(3.*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (8*pow(eta1 - eta2,2)*(delta - 4*C4*pow(mrho,2))*pow(t2,3))/
                 (3.*pow(mrho,2)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (16*pow(delta - 4*C4*pow(mrho,2),2)*pow(t2,3))/
                 (3.*pow(mrho,4)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (4*(-2 + delta)*eta1*(eta1 - eta2)*(pow(ma1,2) - s)*pow(t2,3))/
                 (3.*(pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (2*pow(eta1 - eta2,4)*(pow(ma1,2) - s)*s*pow(t2,3))/
                 (3.*(pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (2*pow(eta1 - eta2,2)*s*(-2*eta1*eta2*s + pow(eta2,2)*s + pow(eta1,2)*(-pow(mrho,2) + s))*pow(t2,3))/
                 (3.*(pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (4*(eta1 - eta2)*(pow(ma1,2) - s)*(2*eta2*(delta - 4*C4*pow(mrho,2))*s + eta1*(-2*delta*s + pow(mrho,2)*(-2 + delta + 8*C4*s)))*pow(t2,3))/
                 (3.*pow(mrho,2)*(pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*
                   (pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (pow(eta1 - eta2,2)*(-2*eta1*eta2*(pow(ma1,8) + pow(m_pi,8) - pow(m_pi,4)*pow(mrho,4) -
                        2*pow(ma1,2)*pow(m_pi,2)*(pow(m_pi,2) - pow(mrho,2))*(pow(mrho,2) + s) + pow(ma1,6)*(-4*pow(m_pi,2) + 2*s) +
                        pow(ma1,4)*(4*pow(m_pi,4) - pow(mrho,4) + 2*pow(m_pi,2)*(pow(mrho,2) - 2*s) - 2*pow(mrho,2)*s + 2*pow(s,2))) +
                     pow(eta2,2)*(pow(ma1,8) + pow(m_pi,4)*pow(pow(m_pi,2) - pow(mrho,2),2) + 2*pow(ma1,6)*(-2*pow(m_pi,2) + pow(mrho,2) + s) +
                        2*pow(ma1,2)*pow(m_pi,2)*(-pow(mrho,4) + pow(m_pi,2)*(2*pow(mrho,2) - s) + pow(mrho,2)*s) +
                        pow(ma1,4)*(4*pow(m_pi,4) + pow(mrho,4) - 2*pow(mrho,2)*s + 2*pow(s,2) - 4*pow(m_pi,2)*(pow(mrho,2) + s))) +
                     pow(eta1,2)*(pow(ma1,8) + pow(m_pi,8) - 2*pow(m_pi,6)*pow(mrho,2) - 2*pow(ma1,6)*(2*pow(m_pi,2) + pow(mrho,2) - s) -
                        2*pow(m_pi,2)*pow(mrho,4)*s + pow(m_pi,4)*(3*pow(mrho,4) + 2*pow(mrho,2)*s) +
                        pow(ma1,4)*(4*pow(m_pi,4) + pow(mrho,4) + pow(m_pi,2)*(8*pow(mrho,2) - 4*s) - 4*pow(mrho,2)*s + 2*pow(s,2)) -
                        2*pow(ma1,2)*(pow(mrho,2)*s*(-pow(mrho,2) + s) + pow(m_pi,4)*(3*pow(mrho,2) + s) + pow(m_pi,2)*(2*pow(mrho,4) - 3*pow(mrho,2)*s)))))
                  /((pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))*(pow(ma1,2) - t1)) -
                (8*pow(-2 + delta,2)*pow(m_pi,2)*(4*pow(m_pi,2) - pow(mrho,2)))/
                 ((pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))*(pow(m_pi,2) - t1)) +
                (8*pow(-2 + delta,2)*pow(m_pi,2)*t1)/(pow(mrho,2)*pow(pow(m_pi,2) - s,2)) +
                (8*pow(-2 + delta,2)*pow(m_pi,2)*t1)/(pow(mrho,2)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (8*(-2 + delta)*(-8*C4*pow(mrho,4) + pow(m_pi,2)*(2 + delta - 8*C4*pow(mrho,2)) - (2 + 3*delta)*s + pow(mrho,2)*(-2 + 3*delta + 16*C4*s))*t1)/
                 (pow(mrho,2)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (4*(-2 + delta)*(eta1 - eta2)*(pow(ma1,2) - s)*(eta2*(pow(m_pi,2) + pow(mrho,2) - 2*s)*(pow(m_pi,2) + s) +
                     eta1*(-2*pow(m_pi,4) + pow(mrho,4) - 3*pow(mrho,2)*s + 2*pow(s,2) + pow(m_pi,2)*(pow(mrho,2) + s)))*t1)/
                 ((pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (pow(eta1 - eta2,2)*(pow(eta1,2)*(3*pow(ma1,4) + 4*pow(m_pi,4) + pow(mrho,4) + pow(m_pi,2)*(8*pow(mrho,2) - 4*s) -
                        4*pow(ma1,2)*(2*pow(m_pi,2) + pow(mrho,2) - s) - 4*pow(mrho,2)*s + 2*pow(s,2)) +
                     pow(eta2,2)*(3*pow(ma1,4) + 4*pow(m_pi,4) + pow(mrho,4) - 2*pow(mrho,2)*s + 2*pow(s,2) - 4*pow(m_pi,2)*(pow(mrho,2) + s) +
                        4*pow(ma1,2)*(-2*pow(m_pi,2) + pow(mrho,2) + s)) -
                     2*eta1*eta2*(3*pow(ma1,4) + 4*pow(m_pi,4) - pow(mrho,4) + 2*pow(m_pi,2)*(pow(mrho,2) - 2*s) - 2*pow(mrho,2)*s + 2*pow(s,2) +
                        pow(ma1,2)*(-8*pow(m_pi,2) + 4*s)))*t1)/(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s)) -
                (8*(pow(delta,2)*(8*pow(m_pi,4) + 3*pow(mrho,4) + 4*pow(m_pi,2)*(3*pow(mrho,2) - 2*s) - 6*pow(mrho,2)*s + 2*pow(s,2)) +
                     4*pow(mrho,4)*(3 + 12*C4*(2*pow(m_pi,2) - s) + 8*pow(C4,2)*pow(-2*pow(m_pi,2) + s,2)) -
                     4*delta*pow(mrho,2)*(16*C4*pow(m_pi,4) + 2*pow(m_pi,2)*(3 + 6*C4*pow(mrho,2) - 8*C4*s) + pow(mrho,2)*(3 - 6*C4*s) + s*(-3 + 4*C4*s)))*t1)/
                 (pow(mrho,4)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (8*(-2 + delta)*(pow(m_pi,4)*(-2 + 3*delta - 8*C4*pow(mrho,2)) + (pow(mrho,2) - s)*((-2 + 3*delta)*s + pow(mrho,2)*(-2 + delta - 8*C4*s)) +
                     4*pow(m_pi,2)*(2*C4*pow(mrho,4) + delta*s - pow(mrho,2)*(-1 + delta + 4*C4*s)))*t1)/
                 (pow(mrho,2)*(pow(m_pi,2) - s)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (4*(-2 + delta)*(eta1 - eta2)*(pow(ma1,2) - s)*(eta2*(pow(m_pi,2) + s)*(pow(m_pi,4) - pow(m_pi,2)*(pow(mrho,2) - 2*s) + (pow(mrho,2) - s)*s) +
                     eta1*(-4*pow(m_pi,6) + pow(pow(mrho,2) - s,2)*s + pow(m_pi,4)*(3*pow(mrho,2) + s) -
                        pow(m_pi,2)*(pow(mrho,4) - pow(mrho,2)*s + 2*pow(s,2))))*t1)/
                 ((pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,2) - s)*
                   (pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (pow(eta1 - eta2,2)*(-2*eta1*eta2*(pow(m_pi,8) - pow(mrho,4)*pow(s,2) + pow(s,4) -
                        pow(m_pi,4)*(pow(mrho,4) + 2*pow(mrho,2)*s - 4*pow(s,2)) + 2*pow(m_pi,2)*s*(pow(mrho,4) + pow(mrho,2)*s - 2*pow(s,2))) +
                     pow(eta2,2)*(pow(m_pi,8) - 2*pow(m_pi,6)*pow(mrho,2) + pow(s,2)*pow(pow(mrho,2) + s,2) + pow(m_pi,4)*pow(pow(mrho,2) + 2*s,2) -
                        2*pow(m_pi,2)*s*(pow(mrho,4) + 2*pow(mrho,2)*s + 2*pow(s,2))) +
                     pow(eta1,2)*(pow(m_pi,8) - 2*pow(m_pi,6)*pow(mrho,2) - 4*pow(m_pi,2)*pow(pow(mrho,2) - s,2)*s + pow(pow(mrho,2) - s,2)*pow(s,2) +
                        pow(m_pi,4)*(3*pow(mrho,4) - 6*pow(mrho,2)*s + 4*pow(s,2))))*t1)/
                 ((pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (2*pow(eta1 - eta2,2)*(pow(ma1,2) - s)*(pow(eta1,2)*(pow(ma1,4)*s + pow(m_pi,4)*(-3*pow(mrho,2) + 2*s) +
                        s*(2*pow(mrho,4) - 3*pow(mrho,2)*s + pow(s,2)) - 2*pow(m_pi,2)*(pow(mrho,4) - 4*pow(mrho,2)*s + 2*pow(s,2)) +
                        pow(ma1,2)*(2*pow(m_pi,2)*(pow(mrho,2) - 2*s) + 3*s*(-pow(mrho,2) + s))) -
                     2*eta1*eta2*(pow(ma1,4)*s + s*(2*pow(m_pi,4) + 4*pow(m_pi,2)*(pow(mrho,2) - s) + s*(-2*pow(mrho,2) + s)) +
                        pow(ma1,2)*(pow(m_pi,2)*(pow(mrho,2) - 4*s) + s*(-2*pow(mrho,2) + 3*s))) +
                     pow(eta2,2)*(-4*pow(m_pi,2)*s*(pow(ma1,2) + pow(mrho,2) + s) + pow(m_pi,4)*(pow(mrho,2) + 2*s) +
                        s*(pow(ma1,4) + s*(pow(mrho,2) + s) + pow(ma1,2)*(pow(mrho,2) + 3*s))))*t1)/
                 ((pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (4*(eta1 - eta2)*(pow(ma1,2) - s)*(eta1*(4*pow(m_pi,4)*(6*C4*pow(mrho,4) + 2*delta*s + pow(mrho,2)*(1 - 2*delta - 8*C4*s)) -
                        2*pow(m_pi,2)*(4*delta*pow(s,2) + pow(mrho,2)*s*(6 - 7*delta - 16*C4*s) + 2*pow(mrho,4)*(-2 + delta + 8*C4*s)) +
                        (pow(mrho,2) - s)*s*(-2*delta*s + pow(mrho,2)*(-6 + 3*delta + 8*C4*s))) +
                     eta2*(-(delta*(2*pow(m_pi,4)*(pow(mrho,2) + 4*s) + pow(m_pi,2)*(2*pow(mrho,4) + pow(mrho,2)*s - 8*pow(s,2)) +
                             s*(-2*pow(mrho,4) - pow(mrho,2)*s + 2*pow(s,2)))) +
                        2*pow(mrho,2)*(4*C4*pow(m_pi,4)*(pow(mrho,2) + 4*s) + pow(m_pi,2)*(s*(5 - 16*C4*s) + pow(mrho,2)*(2 - 8*C4*s)) +
                           s*(s*(-3 + 4*C4*s) + pow(mrho,2)*(-2 + 4*C4*s)))))*t1)/
                 (pow(mrho,2)*(pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*
                   (pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (8*(eta1 - eta2)*(delta*(eta1*(4*pow(m_pi,6) + pow(m_pi,4)*(7*pow(mrho,2) - 8*s) + pow(ma1,4)*(pow(m_pi,2) - s) -
                           pow(ma1,2)*(2*pow(m_pi,2) + pow(mrho,2) - 2*s)*(2*pow(m_pi,2) - s) + pow(m_pi,2)*s*(-8*pow(mrho,2) + 5*s) +
                           s*(pow(mrho,4) + pow(mrho,2)*s - pow(s,2))) +
                        eta2*(-4*pow(m_pi,6) - pow(m_pi,4)*(pow(mrho,2) - 8*s) + pow(ma1,4)*(-pow(m_pi,2) + s) +
                           pow(m_pi,2)*(2*pow(mrho,4) - 5*pow(s,2)) + s*(-2*pow(mrho,4) + pow(mrho,2)*s + pow(s,2)) +
                           pow(ma1,2)*(4*pow(m_pi,4) - 6*pow(m_pi,2)*s + s*(pow(mrho,2) + 2*s)))) -
                     2*pow(mrho,2)*(eta1*(8*C4*pow(m_pi,6) + pow(m_pi,4)*(3 + 8*C4*(pow(mrho,2) - 2*s)) + 2*C4*pow(ma1,4)*(pow(m_pi,2) - s) +
                           2*pow(m_pi,2)*s*(-1 - 6*C4*pow(mrho,2) + 5*C4*s) -
                           pow(ma1,2)*(8*C4*pow(m_pi,4) + pow(m_pi,2)*(1 + 2*C4*(pow(mrho,2) - 6*s)) + 2*C4*s*(-pow(mrho,2) + 2*s)) +
                           s*(-(s*(1 + 2*C4*s)) + pow(mrho,2)*(1 + 4*C4*s))) +
                        eta2*(2*C4*pow(ma1,4)*(-pow(m_pi,2) + s) - (pow(m_pi,2) - s)*
                            (8*C4*pow(m_pi,4) - 2*pow(mrho,2) + s + 2*C4*pow(s,2) + pow(m_pi,2)*(3 - 4*C4*(pow(mrho,2) + 2*s))) +
                           pow(ma1,2)*(8*C4*pow(m_pi,4) + 2*C4*s*(pow(mrho,2) + 2*s) + pow(m_pi,2)*(1 - 2*C4*(pow(mrho,2) + 6*s))))))*t1)/
                 (pow(mrho,2)*(pow(m_pi,2) - s)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (8*(-2 + delta)*(delta - 4*C4*pow(mrho,2))*pow(t1,2))/
                 (pow(mrho,2)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (16*(-2 + delta)*(delta - 4*C4*pow(mrho,2))*s*pow(t1,2))/
                 (pow(mrho,2)*(pow(m_pi,2) - s)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (pow(eta1 - eta2,2)*(pow(eta1,2)*(pow(mrho,2) - s) + 2*eta1*eta2*s - pow(eta2,2)*s)*
                   (pow(m_pi,4) - pow(m_pi,2)*(pow(mrho,2) - 2*s) + (pow(mrho,2) - s)*s)*pow(t1,2))/
                 ((pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (2*(-2 + delta)*(eta1 - eta2)*(pow(ma1,2) - s)*(-(eta1*(pow(m_pi,2) + 2*pow(mrho,2) - 3*s)) - eta2*(pow(m_pi,2) + s))*pow(t1,2))/
                 ((pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (2*(-2 + delta)*(eta1 - eta2)*(pow(ma1,2) - s)*(pow(m_pi,2) + s)*(-2*eta2*s + eta1*(pow(m_pi,2) - pow(mrho,2) + s))*pow(t1,2))/
                 ((pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,2) - s)*
                   (pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (pow(eta1 - eta2,3)*(-(eta1*(pow(ma1,2) - 2*pow(m_pi,2) - pow(mrho,2) + s)) + eta2*(pow(ma1,2) - 2*pow(m_pi,2) + pow(mrho,2) + s))*
                   pow(t1,2))/(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s)) +
                (8*(delta - 4*C4*pow(mrho,2))*(delta*(4*pow(m_pi,2) + 3*pow(mrho,2) - 2*s) - 2*pow(mrho,2)*(3 + 8*C4*pow(m_pi,2) - 4*C4*s))*pow(t1,2))/
                 (pow(mrho,4)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (pow(eta1 - eta2,2)*(pow(ma1,2) - s)*(pow(eta2,2)*s*(pow(ma1,2) - 4*pow(m_pi,2) + pow(mrho,2) + 3*s) +
                     pow(eta1,2)*(2*pow(m_pi,2)*(pow(mrho,2) - 2*s) + s*(pow(ma1,2) - 3*pow(mrho,2) + 3*s)) -
                     2*eta1*eta2*(pow(m_pi,2)*(pow(mrho,2) - 4*s) + s*(pow(ma1,2) - 2*pow(mrho,2) + 3*s)))*pow(t1,2))/
                 ((pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (2*(eta1 - eta2)*(pow(ma1,2) - s)*(eta1*(4*delta*pow(s,2) - 2*pow(mrho,2)*s*(-2 + 3*delta + 8*C4*s) + pow(mrho,4)*(-2 + delta + 16*C4*s) -
                        2*pow(m_pi,2)*(8*C4*pow(mrho,4) + 4*delta*s + pow(mrho,2)*(2 - 3*delta - 16*C4*s))) +
                     eta2*(pow(m_pi,2)*(8*delta*s + pow(mrho,2)*(-2 + delta - 32*C4*s)) + s*(-4*delta*s + pow(mrho,2)*(-2 + delta + 16*C4*s))))*pow(t1,2))/
                 (pow(mrho,2)*(pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*
                   (pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (4*(eta1 - eta2)*(delta*(eta1*(pow(ma1,2)*(pow(m_pi,2) - s) - (2*pow(m_pi,2) + pow(mrho,2) - 2*s)*(2*pow(m_pi,2) - s)) +
                        eta2*(4*pow(m_pi,4) - 6*pow(m_pi,2)*s + pow(ma1,2)*(-pow(m_pi,2) + s) + s*(pow(mrho,2) + 2*s))) +
                     2*pow(mrho,2)*(eta1*(8*C4*pow(m_pi,4) + 2*C4*s*(pow(ma1,2) - pow(mrho,2) + 2*s) +
                           pow(m_pi,2)*(1 - 2*C4*(pow(ma1,2) - pow(mrho,2) + 6*s))) -
                        eta2*(8*C4*pow(m_pi,4) + 2*C4*s*(pow(ma1,2) + pow(mrho,2) + 2*s) - pow(m_pi,2)*(-1 + 2*C4*(pow(ma1,2) + pow(mrho,2) + 6*s)))))*
                   pow(t1,2))/(pow(mrho,2)*(pow(m_pi,2) - s)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (pow(eta1 - eta2,4)*pow(t1,3))/(3.*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (8*pow(eta1 - eta2,2)*(delta - 4*C4*pow(mrho,2))*pow(t1,3))/
                 (3.*pow(mrho,2)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (16*pow(delta - 4*C4*pow(mrho,2),2)*pow(t1,3))/
                 (3.*pow(mrho,4)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (4*(-2 + delta)*eta1*(eta1 - eta2)*(pow(ma1,2) - s)*pow(t1,3))/
                 (3.*(pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (2*pow(eta1 - eta2,4)*(pow(ma1,2) - s)*s*pow(t1,3))/
                 (3.*(pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (2*pow(eta1 - eta2,2)*s*(-2*eta1*eta2*s + pow(eta2,2)*s + pow(eta1,2)*(-pow(mrho,2) + s))*pow(t1,3))/
                 (3.*(pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (4*(eta1 - eta2)*(pow(ma1,2) - s)*(2*eta2*(delta - 4*C4*pow(mrho,2))*s + eta1*(-2*delta*s + pow(mrho,2)*(-2 + delta + 8*C4*s)))*pow(t1,3))/
                 (3.*pow(mrho,2)*(pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*
                   (pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (2*pow(eta1 - eta2,2)*(pow(eta1,2)*(2*pow(ma1,6) - 3*pow(ma1,4)*(2*pow(m_pi,2) + pow(mrho,2) - s) + pow(mrho,2)*(pow(mrho,2) - s)*s -
                        pow(m_pi,4)*(3*pow(mrho,2) + s) + pow(m_pi,2)*(-2*pow(mrho,4) + 3*pow(mrho,2)*s) +
                        pow(ma1,2)*(4*pow(m_pi,4) + pow(mrho,4) + pow(m_pi,2)*(8*pow(mrho,2) - 4*s) - 4*pow(mrho,2)*s + 2*pow(s,2))) -
                     2*eta1*eta2*(2*pow(ma1,6) - pow(m_pi,2)*(pow(m_pi,2) - pow(mrho,2))*(pow(mrho,2) + s) + pow(ma1,4)*(-6*pow(m_pi,2) + 3*s) +
                        pow(ma1,2)*(4*pow(m_pi,4) - pow(mrho,4) + 2*pow(m_pi,2)*(pow(mrho,2) - 2*s) - 2*pow(mrho,2)*s + 2*pow(s,2))) +
                     pow(eta2,2)*(2*pow(ma1,6) + 3*pow(ma1,4)*(-2*pow(m_pi,2) + pow(mrho,2) + s) +
                        pow(m_pi,2)*(-pow(mrho,4) + pow(m_pi,2)*(2*pow(mrho,2) - s) + pow(mrho,2)*s) +
                        pow(ma1,2)*(4*pow(m_pi,4) + pow(mrho,4) - 2*pow(mrho,2)*s + 2*pow(s,2) - 4*pow(m_pi,2)*(pow(mrho,2) + s))))*log(fabs(-pow(ma1,2) + t2)))
                  /(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s)) -
                (2*pow(eta1 - eta2,2)*(pow(ma1,2) - s)*(-2*eta1*eta2*(pow(m_pi,8) - 2*pow(m_pi,6)*pow(mrho,2) + 2*pow(ma1,2)*pow(m_pi,4)*s +
                        pow(m_pi,2)*(pow(ma1,4)*(pow(mrho,2) - 4*s) + 4*pow(ma1,2)*(pow(mrho,2) - s)*s + pow(mrho,2)*pow(s,2)) +
                        pow(ma1,2)*s*(pow(ma1,4) + s*(-2*pow(mrho,2) + s) + pow(ma1,2)*(-2*pow(mrho,2) + 3*s))) +
                     pow(eta2,2)*(pow(m_pi,8) - 4*pow(ma1,2)*pow(m_pi,2)*s*(pow(ma1,2) + pow(mrho,2) + s) +
                        pow(m_pi,4)*(pow(mrho,2)*s + pow(ma1,2)*(pow(mrho,2) + 2*s)) +
                        pow(ma1,2)*s*(pow(ma1,4) + s*(pow(mrho,2) + s) + pow(ma1,2)*(pow(mrho,2) + 3*s))) +
                     pow(eta1,2)*(pow(m_pi,8) + pow(ma1,2)*s*(pow(ma1,4) + 2*pow(mrho,4) - 3*pow(ma1,2)*(pow(mrho,2) - s) - 3*pow(mrho,2)*s + pow(s,2)) +
                        pow(m_pi,4)*(2*pow(mrho,4) - 3*pow(mrho,2)*s + pow(ma1,2)*(-3*pow(mrho,2) + 2*s)) +
                        2*pow(m_pi,2)*(pow(ma1,4)*(pow(mrho,2) - 2*s) + pow(mrho,2)*s*(-pow(mrho,2) + s) -
                           pow(ma1,2)*(pow(mrho,4) - 4*pow(mrho,2)*s + 2*pow(s,2)))))*log(fabs(-pow(ma1,2) + t2)))/
                 ((pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (8*(eta1 - eta2)*(delta*(eta2*(pow(m_pi,6)*pow(mrho,2)*(2*pow(m_pi,2) - s) + pow(ma1,8)*(-pow(m_pi,2) + s) +
                           pow(ma1,6)*(5*pow(m_pi,4) - 7*pow(m_pi,2)*s + s*(pow(mrho,2) + 2*s)) +
                           pow(ma1,4)*(-8*pow(m_pi,6) - pow(m_pi,4)*(pow(mrho,2) - 14*s) + pow(m_pi,2)*(2*pow(mrho,4) - pow(mrho,2)*s - 7*pow(s,2)) +
                              s*(-2*pow(mrho,4) + pow(mrho,2)*s + pow(s,2))) +
                           pow(ma1,2)*pow(m_pi,2)*(4*pow(m_pi,6) + pow(m_pi,4)*(pow(mrho,2) - 8*s) + s*(2*pow(mrho,4) + pow(mrho,2)*s - pow(s,2)) +
                              pow(m_pi,2)*(-2*pow(mrho,4) - 3*pow(mrho,2)*s + 5*pow(s,2)))) +
                        eta1*(pow(ma1,8)*(pow(m_pi,2) - s) + pow(ma1,6)*(-5*pow(m_pi,4) + (pow(mrho,2) - 2*s)*s + pow(m_pi,2)*(-2*pow(mrho,2) + 7*s)) +
                           pow(m_pi,2)*pow(mrho,2)*(2*pow(m_pi,6) + pow(m_pi,4)*(4*pow(mrho,2) - 5*s) + pow(mrho,4)*s -
                              pow(m_pi,2)*(pow(mrho,4) + 3*pow(mrho,2)*s - 2*pow(s,2))) +
                           pow(ma1,4)*(8*pow(m_pi,6) + pow(m_pi,4)*(9*pow(mrho,2) - 14*s) + pow(m_pi,2)*s*(-9*pow(mrho,2) + 7*s) +
                              s*(pow(mrho,4) + pow(mrho,2)*s - pow(s,2))) +
                           pow(ma1,2)*(-4*pow(m_pi,8) + pow(mrho,4)*s*(-pow(mrho,2) + s) + pow(m_pi,6)*(-11*pow(mrho,2) + 8*s) +
                              pow(m_pi,4)*(-3*pow(mrho,4) + 17*pow(mrho,2)*s - 5*pow(s,2)) + pow(m_pi,2)*(pow(mrho,6) - 5*pow(mrho,2)*pow(s,2) + pow(s,3))
                              ))) - 2*pow(mrho,2)*(eta2*(pow(m_pi,8)*(1 + 2*C4*pow(mrho,2)) - 2*C4*pow(m_pi,6)*pow(mrho,2)*s +
                           2*C4*pow(ma1,8)*(-pow(m_pi,2) + s) + pow(ma1,4)*
                            (-16*C4*pow(m_pi,6) + pow(m_pi,4)*(-4 + 6*C4*pow(mrho,2) + 28*C4*s) +
                              2*pow(m_pi,2)*(pow(mrho,2) + s - 3*C4*pow(mrho,2)*s - 7*C4*pow(s,2)) + s*(-2*pow(mrho,2) + s + 2*C4*pow(s,2))) +
                           pow(ma1,6)*(10*C4*pow(m_pi,4) + 2*C4*s*(pow(mrho,2) + 2*s) + pow(m_pi,2)*(1 - 2*C4*(pow(mrho,2) + 7*s))) +
                           pow(ma1,2)*pow(m_pi,2)*(8*C4*pow(m_pi,6) - 2*pow(m_pi,4)*(-2 + 3*C4*pow(mrho,2) + 8*C4*s) +
                              s*(2*pow(mrho,2) + s - 2*C4*pow(s,2)) + 2*pow(m_pi,2)*(pow(mrho,2)*(-1 + 3*C4*s) + s*(-3 + 5*C4*s)))) +
                        eta1*(pow(m_pi,8)*(-1 + 6*C4*pow(mrho,2)) + 2*C4*pow(ma1,8)*(pow(m_pi,2) - s) + pow(m_pi,2)*pow(mrho,4)*s +
                           2*pow(m_pi,6)*pow(mrho,2)*(2 - 5*C4*s) - pow(ma1,6)*
                            (10*C4*pow(m_pi,4) + pow(m_pi,2)*(1 + 2*C4*(pow(mrho,2) - 7*s)) + 2*C4*s*(-pow(mrho,2) + 2*s)) -
                           pow(m_pi,4)*pow(mrho,2)*(pow(mrho,2) + s*(3 - 4*C4*s)) +
                           pow(ma1,4)*(16*C4*pow(m_pi,6) + 2*pow(m_pi,4)*(2 + 5*C4*pow(mrho,2) - 14*C4*s) + 2*pow(m_pi,2)*s*(-1 - 7*C4*pow(mrho,2) + 7*C4*s) +
                              s*(-(s*(1 + 2*C4*s)) + pow(mrho,2)*(1 + 4*C4*s))) -
                           pow(ma1,2)*(8*C4*pow(m_pi,8) + pow(mrho,2)*(pow(mrho,2) - s)*s + 2*pow(m_pi,6)*(2 + 7*C4*pow(mrho,2) - 8*C4*s) +
                              pow(m_pi,2)*(-pow(mrho,4) + pow(s,2) + 8*C4*pow(mrho,2)*pow(s,2) - 2*C4*pow(s,3)) +
                              pow(m_pi,4)*(pow(mrho,2)*(3 - 22*C4*s) + 2*s*(-3 + 5*C4*s))))))*log(fabs(-pow(ma1,2) + t2)))/
                 ((pow(ma1,2) - pow(m_pi,2))*pow(mrho,2)*(pow(m_pi,2) - s)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (16*pow(-2 + delta,2)*pow(m_pi,2)*log(fabs(-pow(m_pi,2) + t2)))/(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s)) -
                (8*pow(-2 + delta,2)*(3*pow(m_pi,4) - 4*pow(m_pi,2)*(pow(mrho,2) - s) + pow(pow(mrho,2) - s,2))*log(fabs(-pow(m_pi,2) + t2)))/
                 ((pow(m_pi,2) - s)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (8*(-2 + delta)*(eta1 - eta2)*pow(m_pi,2)*(2*eta1*pow(m_pi,2) - 2*eta2*pow(m_pi,2) - eta1*pow(mrho,2))*(pow(m_pi,2) - s)*
                   log(fabs(-pow(m_pi,2) + t2)))/((pow(ma1,2) - pow(m_pi,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (8*(-2 + delta)*(eta1 - eta2)*pow(m_pi,2)*(pow(ma1,2) - s)*(pow(m_pi,2) - s)*
                   (-(eta2*(pow(m_pi,2) + s)) + eta1*(pow(m_pi,2) - pow(mrho,2) + s))*log(fabs(-pow(m_pi,2) + t2)))/
                 ((pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (8*(-2 + delta)*(-(delta*(4*pow(m_pi,2) - pow(mrho,2))*(pow(m_pi,2) + pow(mrho,2) - s)) +
                     2*pow(mrho,2)*(8*C4*pow(m_pi,4) - pow(mrho,2) + s + pow(m_pi,2)*(3 - 8*C4*s)))*log(fabs(-pow(m_pi,2) + t2)))/
                 (pow(mrho,2)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (2*pow(eta1 - eta2,2)*(pow(eta1,2)*(2*pow(ma1,6) - 3*pow(ma1,4)*(2*pow(m_pi,2) + pow(mrho,2) - s) + pow(mrho,2)*(pow(mrho,2) - s)*s -
                        pow(m_pi,4)*(3*pow(mrho,2) + s) + pow(m_pi,2)*(-2*pow(mrho,4) + 3*pow(mrho,2)*s) +
                        pow(ma1,2)*(4*pow(m_pi,4) + pow(mrho,4) + pow(m_pi,2)*(8*pow(mrho,2) - 4*s) - 4*pow(mrho,2)*s + 2*pow(s,2))) -
                     2*eta1*eta2*(2*pow(ma1,6) - pow(m_pi,2)*(pow(m_pi,2) - pow(mrho,2))*(pow(mrho,2) + s) + pow(ma1,4)*(-6*pow(m_pi,2) + 3*s) +
                        pow(ma1,2)*(4*pow(m_pi,4) - pow(mrho,4) + 2*pow(m_pi,2)*(pow(mrho,2) - 2*s) - 2*pow(mrho,2)*s + 2*pow(s,2))) +
                     pow(eta2,2)*(2*pow(ma1,6) + 3*pow(ma1,4)*(-2*pow(m_pi,2) + pow(mrho,2) + s) +
                        pow(m_pi,2)*(-pow(mrho,4) + pow(m_pi,2)*(2*pow(mrho,2) - s) + pow(mrho,2)*s) +
                        pow(ma1,2)*(4*pow(m_pi,4) + pow(mrho,4) - 2*pow(mrho,2)*s + 2*pow(s,2) - 4*pow(m_pi,2)*(pow(mrho,2) + s))))*log(fabs(-pow(ma1,2) + t1)))
                  /(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s)) +
                (2*pow(eta1 - eta2,2)*(pow(ma1,2) - s)*(-2*eta1*eta2*(pow(m_pi,8) - 2*pow(m_pi,6)*pow(mrho,2) + 2*pow(ma1,2)*pow(m_pi,4)*s +
                        pow(m_pi,2)*(pow(ma1,4)*(pow(mrho,2) - 4*s) + 4*pow(ma1,2)*(pow(mrho,2) - s)*s + pow(mrho,2)*pow(s,2)) +
                        pow(ma1,2)*s*(pow(ma1,4) + s*(-2*pow(mrho,2) + s) + pow(ma1,2)*(-2*pow(mrho,2) + 3*s))) +
                     pow(eta2,2)*(pow(m_pi,8) - 4*pow(ma1,2)*pow(m_pi,2)*s*(pow(ma1,2) + pow(mrho,2) + s) +
                        pow(m_pi,4)*(pow(mrho,2)*s + pow(ma1,2)*(pow(mrho,2) + 2*s)) +
                        pow(ma1,2)*s*(pow(ma1,4) + s*(pow(mrho,2) + s) + pow(ma1,2)*(pow(mrho,2) + 3*s))) +
                     pow(eta1,2)*(pow(m_pi,8) + pow(ma1,2)*s*(pow(ma1,4) + 2*pow(mrho,4) - 3*pow(ma1,2)*(pow(mrho,2) - s) - 3*pow(mrho,2)*s + pow(s,2)) +
                        pow(m_pi,4)*(2*pow(mrho,4) - 3*pow(mrho,2)*s + pow(ma1,2)*(-3*pow(mrho,2) + 2*s)) +
                        2*pow(m_pi,2)*(pow(ma1,4)*(pow(mrho,2) - 2*s) + pow(mrho,2)*s*(-pow(mrho,2) + s) -
                           pow(ma1,2)*(pow(mrho,4) - 4*pow(mrho,2)*s + 2*pow(s,2)))))*log(fabs(-pow(ma1,2) + t1)))/
                 ((pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (8*(eta1 - eta2)*(delta*(eta2*(pow(m_pi,6)*pow(mrho,2)*(2*pow(m_pi,2) - s) + pow(ma1,8)*(-pow(m_pi,2) + s) +
                           pow(ma1,6)*(5*pow(m_pi,4) - 7*pow(m_pi,2)*s + s*(pow(mrho,2) + 2*s)) +
                           pow(ma1,4)*(-8*pow(m_pi,6) - pow(m_pi,4)*(pow(mrho,2) - 14*s) + pow(m_pi,2)*(2*pow(mrho,4) - pow(mrho,2)*s - 7*pow(s,2)) +
                              s*(-2*pow(mrho,4) + pow(mrho,2)*s + pow(s,2))) +
                           pow(ma1,2)*pow(m_pi,2)*(4*pow(m_pi,6) + pow(m_pi,4)*(pow(mrho,2) - 8*s) + s*(2*pow(mrho,4) + pow(mrho,2)*s - pow(s,2)) +
                              pow(m_pi,2)*(-2*pow(mrho,4) - 3*pow(mrho,2)*s + 5*pow(s,2)))) +
                        eta1*(pow(ma1,8)*(pow(m_pi,2) - s) + pow(ma1,6)*(-5*pow(m_pi,4) + (pow(mrho,2) - 2*s)*s + pow(m_pi,2)*(-2*pow(mrho,2) + 7*s)) +
                           pow(m_pi,2)*pow(mrho,2)*(2*pow(m_pi,6) + pow(m_pi,4)*(4*pow(mrho,2) - 5*s) + pow(mrho,4)*s -
                              pow(m_pi,2)*(pow(mrho,4) + 3*pow(mrho,2)*s - 2*pow(s,2))) +
                           pow(ma1,4)*(8*pow(m_pi,6) + pow(m_pi,4)*(9*pow(mrho,2) - 14*s) + pow(m_pi,2)*s*(-9*pow(mrho,2) + 7*s) +
                              s*(pow(mrho,4) + pow(mrho,2)*s - pow(s,2))) +
                           pow(ma1,2)*(-4*pow(m_pi,8) + pow(mrho,4)*s*(-pow(mrho,2) + s) + pow(m_pi,6)*(-11*pow(mrho,2) + 8*s) +
                              pow(m_pi,4)*(-3*pow(mrho,4) + 17*pow(mrho,2)*s - 5*pow(s,2)) + pow(m_pi,2)*(pow(mrho,6) - 5*pow(mrho,2)*pow(s,2) + pow(s,3))
                              ))) - 2*pow(mrho,2)*(eta2*(pow(m_pi,8)*(1 + 2*C4*pow(mrho,2)) - 2*C4*pow(m_pi,6)*pow(mrho,2)*s +
                           2*C4*pow(ma1,8)*(-pow(m_pi,2) + s) + pow(ma1,4)*
                            (-16*C4*pow(m_pi,6) + pow(m_pi,4)*(-4 + 6*C4*pow(mrho,2) + 28*C4*s) +
                              2*pow(m_pi,2)*(pow(mrho,2) + s - 3*C4*pow(mrho,2)*s - 7*C4*pow(s,2)) + s*(-2*pow(mrho,2) + s + 2*C4*pow(s,2))) +
                           pow(ma1,6)*(10*C4*pow(m_pi,4) + 2*C4*s*(pow(mrho,2) + 2*s) + pow(m_pi,2)*(1 - 2*C4*(pow(mrho,2) + 7*s))) +
                           pow(ma1,2)*pow(m_pi,2)*(8*C4*pow(m_pi,6) - 2*pow(m_pi,4)*(-2 + 3*C4*pow(mrho,2) + 8*C4*s) +
                              s*(2*pow(mrho,2) + s - 2*C4*pow(s,2)) + 2*pow(m_pi,2)*(pow(mrho,2)*(-1 + 3*C4*s) + s*(-3 + 5*C4*s)))) +
                        eta1*(pow(m_pi,8)*(-1 + 6*C4*pow(mrho,2)) + 2*C4*pow(ma1,8)*(pow(m_pi,2) - s) + pow(m_pi,2)*pow(mrho,4)*s +
                           2*pow(m_pi,6)*pow(mrho,2)*(2 - 5*C4*s) - pow(ma1,6)*
                            (10*C4*pow(m_pi,4) + pow(m_pi,2)*(1 + 2*C4*(pow(mrho,2) - 7*s)) + 2*C4*s*(-pow(mrho,2) + 2*s)) -
                           pow(m_pi,4)*pow(mrho,2)*(pow(mrho,2) + s*(3 - 4*C4*s)) +
                           pow(ma1,4)*(16*C4*pow(m_pi,6) + 2*pow(m_pi,4)*(2 + 5*C4*pow(mrho,2) - 14*C4*s) + 2*pow(m_pi,2)*s*(-1 - 7*C4*pow(mrho,2) + 7*C4*s) +
                              s*(-(s*(1 + 2*C4*s)) + pow(mrho,2)*(1 + 4*C4*s))) -
                           pow(ma1,2)*(8*C4*pow(m_pi,8) + pow(mrho,2)*(pow(mrho,2) - s)*s + 2*pow(m_pi,6)*(2 + 7*C4*pow(mrho,2) - 8*C4*s) +
                              pow(m_pi,2)*(-pow(mrho,4) + pow(s,2) + 8*C4*pow(mrho,2)*pow(s,2) - 2*C4*pow(s,3)) +
                              pow(m_pi,4)*(pow(mrho,2)*(3 - 22*C4*s) + 2*s*(-3 + 5*C4*s))))))*log(fabs(-pow(ma1,2) + t1)))/
                 ((pow(ma1,2) - pow(m_pi,2))*pow(mrho,2)*(pow(m_pi,2) - s)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (16*pow(-2 + delta,2)*pow(m_pi,2)*log(fabs(-pow(m_pi,2) + t1)))/(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s)) +
                (8*pow(-2 + delta,2)*(3*pow(m_pi,4) - 4*pow(m_pi,2)*(pow(mrho,2) - s) + pow(pow(mrho,2) - s,2))*log(fabs(-pow(m_pi,2) + t1)))/
                 ((pow(m_pi,2) - s)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (8*(-2 + delta)*(eta1 - eta2)*pow(m_pi,2)*(2*eta1*pow(m_pi,2) - 2*eta2*pow(m_pi,2) - eta1*pow(mrho,2))*(pow(m_pi,2) - s)*
                   log(fabs(-pow(m_pi,2) + t1)))/((pow(ma1,2) - pow(m_pi,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) -
                (8*(-2 + delta)*(eta1 - eta2)*pow(m_pi,2)*(pow(ma1,2) - s)*(pow(m_pi,2) - s)*
                   (-(eta2*(pow(m_pi,2) + s)) + eta1*(pow(m_pi,2) - pow(mrho,2) + s))*log(fabs(-pow(m_pi,2) + t1)))/
                 ((pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
                (8*(-2 + delta)*(delta*(4*pow(m_pi,2) - pow(mrho,2))*(pow(m_pi,2) + pow(mrho,2) - s) -
                     2*pow(mrho,2)*(8*C4*pow(m_pi,4) - pow(mrho,2) + s + pow(m_pi,2)*(3 - 8*C4*s)))*log(fabs(-pow(m_pi,2) + t1)))/
                 (pow(mrho,2)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s)))))/(512.*Pi);

          process_list.push_back(make_unique<CollisionBranch>(
              *part_out, *photon_out, xsection, ProcessType::TwoToTwo));
          break;

        case ReactionType::pi_rho:
          part_out = pi0_particle;
          m3 = part_out->mass();

          xsection = -alpha * g_rho_2 / (48 * s * p_cm_2);
          t1 += -m_pi_2;
          t2 += -m_pi_2;
          u1 += -m_rho_2;
          u2 += -m_rho_2;
          e = 4 * (pow_int(m2, 2) - 4 * m_pi_2) *
              (pow_int(m2, 2) * (t2 - t1) / (u1 * u2) +
               m_pi_2 * (t2 - t1) / (t2 * t1) + std::log(u1 / u2 * t2 / t1) -
               pow_int(m2, 2) / (s - m_pi_2) * std::log(t2 / t1 * u1 / u2));
          e += (s - m_pi_2) * (3.0 + (s - m_pi_2) / pow_int(m2, 2)) *
               std::log(u1 / u2);
          e += (t2 - t1) *
               (s / pow_int(m2, 2) - 0.5 - pow_int(s - m_pi_2, 2) / (u1 * u2));
          xsection = xsection * e * to_mb;
          process_list.push_back(make_unique<CollisionBranch>(
              *part_out, *photon_out, xsection, ProcessType::TwoToTwo));
          break;

        case ReactionType::pi0_rho:
          if (part_b.type().pdgcode() == pdg::rho_p) {
            part_out = pi_plus_particle;
          } else {
            part_out = pi_minus_particle;
          }
          m3 = part_out->mass();

          xsection = alpha * g_rho_2 / (48 * s * p_cm_2);
          u1 += -pow_int(m2, 2);  // is u+
          u2 += -pow_int(m2, 2);
          e = (t2 - t1) *
              (4.5 - s / pow_int(m2, 2) -
               4 * s * (pow_int(m2, 2) - 4 * m_pi_2) / pow_int(s - m_pi_2, 2) +
               (pow_int(s - m_pi_2, 2) -
                4 * pow_int(m2, 2) * (pow_int(m2, 2) - 4 * m_pi_2)) /
                   (u1 * u2));
          e += std::log(u1 / u2) *
               (5 * (s - m_pi_2) - pow_int(s - m_pi_2, 2) / pow_int(m2, 2) -
                4 * (pow_int(m2, 2) - 4 * m_pi_2) * (s - m_pi_2 +
                    pow_int(m2, 2)) / (s - m_pi_2));
          xsection = xsection * e * to_mb;
          process_list.push_back(make_unique<CollisionBranch>(
              *part_out, *photon_out, xsection, ProcessType::TwoToTwo));
          break;

        case ReactionType::pi_eta:
          if (part_a.type().pdgcode() == pdg::pi_p) {
            part_out = pi_plus_particle;
          } else {
            part_out = pi_minus_particle;
          }
          m3 = part_out->mass();

          xsection = M_PI * alpha * 4.7 / (16 * m_eta_2 * s * p_cm_2);
          I0 = 1 / (m_rho * gamma_rho_tot) *
               (atan((u1 - m_rho_2) / (m_rho * gamma_rho_tot)) -
                atan((u2 - m_rho_2) / (m_rho * gamma_rho_tot)));
          I1 = std::log(
              (pow_int(u2 - m_rho_2, 2) + m_rho_2 * pow_int(gamma_rho_tot, 2)) /
              (pow_int(u1 - m_rho_2, 2) + m_rho_2 * pow_int(gamma_rho_tot, 2)));
          e = -m_pi_2 * ((t2 + u2) * (s - m_pi_2) + pow_int(2 * m_pi_2 - s, 2))
              * I0;
          e += ((s - m_pi_2) * (m_pi_2 + t2 + u2) -
                2 * m_pi_2 * (s - 2 * m_pi_2)) *
               ((t2 + u2 - m_rho_2) * I0 + 0.5 * I1);
          e += -s * (t2 - t1 + (t2 + u2 - m_rho_2) * I1 +
                     pow_int(t2 + u2 - m_rho_2, 2) * I0 -
                     m_rho_2 * pow_int(gamma_rho_tot, 2) * I0);
          xsection = xsection * e * to_mb;
          process_list.push_back(make_unique<CollisionBranch>(
              *part_out, *photon_out, xsection, ProcessType::TwoToTwo));
          break;

        case ReactionType::pi0_rho0:
          part_out = pi0_particle;
          m3 = part_out->mass();

          if (part_a.pdgcode().is_rho()) {
            m_rho_case = part_a.effective_mass();
          } else if (part_b.pdgcode().is_rho()) {
            m_rho_case = part_b.effective_mass();
          }

          xsection = ((pow(Const,2)*pow(g_POR,4)*((pow(pow(m_omega,2) - s,2)*(pow(m_pi,8) - 2*pow(m_pi,6)*pow(m_rho_case,2) + pow(m_pi,4)*(pow(m_rho_case,4) + 4*pow(m_omega,4) - 2*pow(m_omega,2)*s) +
                      pow(m_omega,4)*(pow(m_rho_case,4) + pow(m_omega,4) + 2*pow(m_omega,2)*s + 2*pow(s,2) - 2*pow(m_rho_case,2)*(pow(m_omega,2) + s)) -
                      2*pow(m_pi,2)*pow(m_omega,2)*(pow(m_rho_case,4) + 2*pow(m_omega,2)*(pow(m_omega,2) + s) - pow(m_rho_case,2)*(2*pow(m_omega,2) + s))))/(pow(m_omega,2) - t2) +
                      (pow(m_pi,8) - 2*pow(m_pi,6)*pow(m_rho_case,2) + 3*pow(m_omega,8) - 4*pow(m_omega,6)*s - 7*pow(m_omega,4)*pow(s,2) + 4*pow(m_omega,2)*pow(s,3) + 5*pow(s,4) +
                      pow(m_rho_case,4)*(pow(m_omega,4) - 2*pow(m_omega,2)*s + 2*pow(s,2)) + pow(m_rho_case,2)*(-4*pow(m_omega,6) + 8*pow(m_omega,4)*s - 6*pow(s,3)) -
                      2*pow(m_pi,2)*(4*pow(m_omega,6) - 2*pow(m_rho_case,2)*pow(pow(m_omega,2) - 2*s,2) + pow(m_rho_case,4)*s - 10*pow(m_omega,4)*s + 8*pow(s,3)) +
                      pow(m_pi,4)*(pow(m_rho_case,4) + 2*pow(m_rho_case,2)*(pow(m_omega,2) - s) + 4*(pow(m_omega,4) - 3*pow(m_omega,2)*s + 3*pow(s,2))))*t2 -
                      2*pow(m_pi,2)*pow(m_omega,4)*pow(t2,2) - pow(m_rho_case,2)*pow(m_omega,4)*pow(t2,2) + pow(m_omega,6)*pow(t2,2) - pow(m_pi,4)*s*pow(t2,2) +
                      pow(m_pi,2)*pow(m_rho_case,2)*s*pow(t2,2) + 8*pow(m_pi,2)*pow(m_omega,2)*s*pow(t2,2) + 3*pow(m_rho_case,2)*pow(m_omega,2)*s*pow(t2,2) -
                      2*pow(m_omega,4)*s*pow(t2,2) - 8*pow(m_pi,2)*pow(s,2)*pow(t2,2) - 3*pow(m_rho_case,2)*pow(s,2)*pow(t2,2) - 3*pow(m_omega,2)*pow(s,2)*pow(t2,2) +
                      5*pow(s,3)*pow(t2,2) + ((pow(m_omega,4) - 4*pow(m_omega,2)*s + 5*pow(s,2))*pow(t2,3))/3. -
                      (pow(pow(m_omega,2) - s,2)*(pow(m_pi,8) - 2*pow(m_pi,6)*pow(m_rho_case,2) + pow(m_pi,4)*(pow(m_rho_case,4) + 4*pow(m_omega,4) - 2*pow(m_omega,2)*s) +
                      pow(m_omega,4)*(pow(m_rho_case,4) + pow(m_omega,4) + 2*pow(m_omega,2)*s + 2*pow(s,2) - 2*pow(m_rho_case,2)*(pow(m_omega,2) + s)) -
                      2*pow(m_pi,2)*pow(m_omega,2)*(pow(m_rho_case,4) + 2*pow(m_omega,2)*(pow(m_omega,2) + s) - pow(m_rho_case,2)*(2*pow(m_omega,2) + s))))/(pow(m_omega,2) - t1) -
                      (pow(m_pi,8) - 2*pow(m_pi,6)*pow(m_rho_case,2) + 3*pow(m_omega,8) - 4*pow(m_omega,6)*s - 7*pow(m_omega,4)*pow(s,2) + 4*pow(m_omega,2)*pow(s,3) + 5*pow(s,4) +
                      pow(m_rho_case,4)*(pow(m_omega,4) - 2*pow(m_omega,2)*s + 2*pow(s,2)) + pow(m_rho_case,2)*(-4*pow(m_omega,6) + 8*pow(m_omega,4)*s - 6*pow(s,3)) -
                      2*pow(m_pi,2)*(4*pow(m_omega,6) - 2*pow(m_rho_case,2)*pow(pow(m_omega,2) - 2*s,2) + pow(m_rho_case,4)*s - 10*pow(m_omega,4)*s + 8*pow(s,3)) +
                      pow(m_pi,4)*(pow(m_rho_case,4) + 2*pow(m_rho_case,2)*(pow(m_omega,2) - s) + 4*(pow(m_omega,4) - 3*pow(m_omega,2)*s + 3*pow(s,2))))*t1 +
                      2*pow(m_pi,2)*pow(m_omega,4)*pow(t1,2) + pow(m_rho_case,2)*pow(m_omega,4)*pow(t1,2) - pow(m_omega,6)*pow(t1,2) + pow(m_pi,4)*s*pow(t1,2) -
                      pow(m_pi,2)*pow(m_rho_case,2)*s*pow(t1,2) - 8*pow(m_pi,2)*pow(m_omega,2)*s*pow(t1,2) - 3*pow(m_rho_case,2)*pow(m_omega,2)*s*pow(t1,2) +
                      2*pow(m_omega,4)*s*pow(t1,2) + 8*pow(m_pi,2)*pow(s,2)*pow(t1,2) + 3*pow(m_rho_case,2)*pow(s,2)*pow(t1,2) + 3*pow(m_omega,2)*pow(s,2)*pow(t1,2) -
                      5*pow(s,3)*pow(t1,2) - ((pow(m_omega,4) - 4*pow(m_omega,2)*s + 5*pow(s,2))*pow(t1,3))/3. +
                      2*(pow(m_omega,2) - s)*(-pow(m_pi,8) + pow(m_pi,4)*(4*pow(m_omega,4) - 7*pow(m_omega,2)*s + pow(s,2) + pow(m_rho_case,2)*(pow(m_omega,2) + s)) +
                      pow(m_pi,2)*(-6*pow(m_omega,6) + 6*pow(m_omega,4)*s + 8*pow(m_omega,2)*pow(s,2) + pow(m_rho_case,4)*(-pow(m_omega,2) + s) +
                      pow(m_rho_case,2)*(4*pow(m_omega,4) - 7*pow(m_omega,2)*s - pow(s,2))) +
                      pow(m_omega,2)*(2*pow(m_omega,6) + pow(m_rho_case,4)*(pow(m_omega,2) - s) - 4*pow(m_omega,2)*pow(s,2) - 3*pow(s,3) +
                      pow(m_rho_case,2)*(-3*pow(m_omega,4) + 2*pow(m_omega,2)*s + 3*pow(s,2))))*log((-pow(m_omega,2) + t2)/(-pow(m_omega,2) + t1))))/
                      (128.0*M_PI*pow(pow(m_omega,2) - s,2)*(pow(m_pi,4) + pow(pow(m_rho_case,2) - s,2) - 2*pow(m_pi,2)*(pow(m_rho_case,2) + s))))*to_mb;

            process_list.push_back(make_unique<CollisionBranch>(
            *part_out, *photon_out, xsection, ProcessType::TwoToTwo));
            break;

        case ReactionType::no_reaction:
          // never reached
          break;
       }
    }
  }
  return process_list;
}

float ScatterActionPhoton::pi_pi_rho0(const float M, const float s) const {
  const float to_mb = 0.3894;
  const float m_pi = ParticleType::find(pdg::pi_z).mass();
  const float m_pi_2 = pow_int(m_pi, 2);
  const float m_rho = ParticleType::find(pdg::rho_z).mass();
  const float gamma_rho_tot = ParticleType::find(pdg::rho_z).width_at_pole();
//  const float g_rho_2 = 24 * twopi * gamma_rho_tot * pow_int(m_rho, 2) /
//                       pow(pow_int(m_rho, 2) - 4 * pow_int(m_pi, 2), 3.0 / 2.0);
  const float g_rho_2 = 2.9*4.0*M_PI;
  const float DM = pow_int(M, 2) - 4 * pow_int(m_pi, 2);
  const float sqrts = sqrt(s);
  const float p_cm_2 = 0.25 * s - m_pi_2;
  if (sqrts <= M) {
    return 0;
  }
  std::array<float, 2> mandelstam_t = get_t_range(sqrts, m_pi, m_pi, M, 0.0f);
  float t1 = mandelstam_t[1];
  float t2 = mandelstam_t[0];
  float u1 = 2 * m_pi_2 + pow_int(M, 2) - s - t1;
  float u2 = 2 * m_pi_2 + pow_int(M, 2) - s - t2;
  float xsection = alpha * g_rho_2 / (4 * s * p_cm_2);

  t1 += -m_pi_2;
  t2 += -m_pi_2;
  if (std::abs(t1) < really_small) {
    t1 = -really_small;
  }
  if (t2 / t1 <= 0) {
    return 0;
  }
  u1 += -m_pi_2;
  u2 += -m_pi_2;
  if (std::abs(u2) < really_small) {
    return 0;
  }
  if (u1 / u2 <= 0) {
    return 0;
  }

  xsection = xsection *
             (2 * (t2 - t1) -
              DM * ((s - 2 * m_pi_2) / (s - pow_int(M, 2)) * std::log(t2 / t1) +
                    m_pi_2 * (t2 - t1) / (t2 * t1) +
                    (s - 2 * m_pi_2) / (s - pow_int(M, 2)) * std::log(u1 / u2) +
                    m_pi_2 * (u1 - u2) / (u1 * u2))) *
             to_mb;
  if (xsection > 0) {
    return xsection;
  } else {
    return really_small;
  }
}

float ScatterActionPhoton::pi_pi0_rho(const float M, const float s) const {
  const float to_mb = 0.3894;
  const float m_pi = ParticleType::find(pdg::pi_z).mass();
  const float m_pi_2 = pow_int(m_pi, 2);
  const float m_rho = ParticleType::find(pdg::rho_z).mass();
  const float gamma_rho_tot = ParticleType::find(pdg::rho_z).width_at_pole();
//  const float g_rho_2 = 24 * twopi * gamma_rho_tot * pow_int(m_rho, 2) /
//                       pow(pow_int(m_rho, 2) - 4 * pow_int(m_pi, 2), 3.0 / 2.0);
  const float g_rho_2 = 2.9*4.0*M_PI;
  const float DM = pow_int(M, 2) - 4 * pow_int(m_pi, 2);
  const float sqrts = sqrt(s);
  const float p_cm_2 = 0.25 * s - m_pi_2;
  if (sqrts <= M) {
    return 0;
  }
  std::array<float, 2> mandelstam_t = get_t_range(sqrts, m_pi, m_pi, M, 0.0f);
  float t1 = mandelstam_t[1];
  float t2 = mandelstam_t[0];
  float xsection = -alpha * g_rho_2 / (16 * s * p_cm_2);
  float e = 1.0 / 3.0 * (s - 2 * pow_int(M, 2)) / pow_int(M, 2) /
            pow_int(s - pow_int(M, 2), 2) * (pow_int(t2, 3) - pow_int(t1, 3));
  e += 0.5 * (s - 6 * pow_int(M, 2)) / pow_int(M, 2) / (s - pow_int(M, 2)) *
       (pow_int(t2, 2) - pow_int(t1, 2));
  t1 += -m_pi_2;
  t2 += -m_pi_2;
  if (std::abs(t1) < really_small) {
    t1 = -really_small;
  }
  if (t2 / t1 <= 0) {
    return 0;
  }
  e += (4 * s * DM / pow_int(s - pow_int(M, 2), 2) + m_pi_2 /pow_int(M, 2)
        - 4.5) * (t2 - t1);
  e += 4 * s * DM / (s - pow_int(M, 2)) * std::log(t2 / t1);
  e += 4 * m_pi_2 * DM * ((t2 - t1) / (t2 * t1));
  xsection = xsection * e * to_mb;
  if (xsection > 0) {
    return xsection;
  } else {
    return really_small;
  }
}

float ScatterActionPhoton::diff_cross_section(float t, float m3) const {
  const float to_mb = 0.3894;
  const float m_rho = ParticleType::find(pdg::rho_z).mass();
  const float m_rho_2 = pow_int(m_rho, 2);
  const float m_pi = ParticleType::find(pdg::pi_z).mass();
  const float m_pi_2 = pow_int(m_pi, 2);
  const float m_eta = ParticleType::find(pdg::eta).mass();
  const float m_eta_2 = pow_int(m_eta, 2);
  const float gamma_rho_tot = ParticleType::find(pdg::rho_z).width_at_pole();
//  const float g_rho_2 = 24 * twopi * gamma_rho_tot * pow_int(m_rho, 2) /
//                       pow(pow_int(m_rho, 2) - 4 * pow_int(m_pi, 2), 3.0 / 2.0);
  const float g_rho_2 = 2.9*4.0*M_PI;
  float s = mandelstam_s();
  const float p_cm_2 = cm_momentum_squared();
  const float m1 = incoming_particles_[0].effective_mass();
  const float m2 = incoming_particles_[1].effective_mass();
  const float m3_2 = pow_int(m3, 2);
  const float DM = pow_int(m3, 2) - 4 * pow_int(m_pi, 2);
  float u = pow_int(m1, 2) + pow_int(m2, 2) + pow_int(m3, 2) - s - t;
  float diff_xsection = 0.0;
  float e = 0.0;

  const float Const = 0.059;
  const float g_POR = 25.8;
  const float ma1 = 1.26;
  const float ghat = 6.4483;
  const float eta1 = 2.3920;
  const float eta2 = 1.9430;
  const float delta = -0.6426;
  const float C4 = -0.14095;
  const float Gammaa1 = 0.4;
  const float Pi = M_PI;
  float m_omega = ParticleType::find(pdg::omega).mass();
  float m_rho_case = m_rho;
  float mrho = m_rho;

  switch (reac) {
    case ReactionType::pi_pi:
      if (outgoing_particles_[0].type().pdgcode().is_rho()) {
        diff_xsection = alpha * g_rho_2 / (4 * s * p_cm_2);
        diff_xsection =
            diff_xsection *
            (2 - DM / (t - m_pi_2) *
            ((s - 2 * m_pi_2) / (s - pow_int(m3, 2)) + m_pi_2 / (t - m_pi_2))
            - DM / (u - m_pi_2) *
            ((s - 2 * m_pi_2) / (s - pow_int(m3, 2)) + m_pi_2 / (u - m_pi_2)));
      } else if (outgoing_particles_[0].type().pdgcode() == pdg::eta) {
        diff_xsection =
            twopi * alpha * 4.7 * pow_int(m_rho, 4) /
            (pow_int(s - m_rho_2, 2) + pow_int(gamma_rho_tot, 2) * m_rho_2) /
            (32 * m_eta_2 * pow_int(m_rho, 4) * s * p_cm_2);
        diff_xsection = diff_xsection * (s * (u - m_pi_2) * (t - m_pi_2) -
                                         m_pi_2 * pow_int(s - m_eta_2, 2));
      } else if (outgoing_particles_[0].type().pdgcode() == pdg::photon) {
        diff_xsection = twopi * pow_int(alpha, 2) / (s * p_cm_2);
        u += -m_pi_2;
        t += -m_pi_2;
        diff_xsection =
            diff_xsection * (1 + 2 * m_pi_2 * (1 / t + 1 / u) +
                             2 * pow_int(m_pi, 4) * pow_int(1 / t + 1 / u, 2));
      }
      break;
    case ReactionType::pi0_pi:
      diff_xsection = -alpha * g_rho_2 / (16 * s * p_cm_2);
      e = (s - 2 * m3_2) * pow_int(t - m_pi_2, 2) / m3_2 / pow_int(s - m3_2, 2)
          + (s - 2 * m3_2) *
          pow_int(u - m_pi_2, 2) / m3_2 / pow_int(s - m3_2, 2);
      e += (s - 6 * m3_2) * (t - m_pi_2) / m3_2 / (s - m3_2) +
           (s - 6 * m3_2) * (u - m_pi_2) / m3_2 / (s - m3_2);
      e += 2 * 4 * s * DM / pow_int(s - m3_2, 2);
      e += 4 * DM / (t - m_pi_2) * (s / (s - m3_2) + m_pi_2 / (t - m_pi_2)) +
           4 * DM / (u - m_pi_2) * (s / (s - m3_2) + m_pi_2 / (u - m_pi_2));
      e += 2 * (m_pi_2 / m3_2 - 4.5);
      diff_xsection = diff_xsection * e;
      break;
    case ReactionType::pi_rho0:
      if (incoming_particles_[0].pdgcode().is_rho()){
        mrho = incoming_particles_[0].effective_mass();
      } else {
        mrho = incoming_particles_[1].effective_mass();
      }

    /*  diff_xsection = alpha * g_rho_2 / (12 * s * p_cm_2);
      diff_xsection =
          diff_xsection *
          (2 - s * (pow_int(m2, 2) -
          4*pow_int(m_pi, 2)) / pow_int(s - m_pi_2, 2) -
           (pow_int(m2, 2) - 4 * pow_int(m_pi, 2)) / (t - m_pi_2) *
               ((s - pow_int(m2, 2) + m_pi_2) / (s - m_pi_2) +
                m_pi_2 / (t - m_pi_2))); */

      diff_xsection = (pow(Const,2)*pow(ghat,4)*((-8*pow(-2 + delta,2)*pow(m_pi,2))/(pow(mrho,2)*pow(pow(m_pi,2) - s,2)) -
       (8*pow(-2 + delta,2)*pow(m_pi,2)*(pow(m_pi,4) + pow(pow(mrho,2) - t,2) - 2*pow(m_pi,2)*(pow(mrho,2) + t)))/
        (pow(mrho,2)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))*pow(pow(m_pi,2) - t,2)) +
       (4*(-2 + delta)*(eta1 - eta2)*(pow(ma1,2) - s)*(-(eta2*(pow(m_pi,2) + s)) + eta1*(-pow(mrho,2) + s + t))*
          (-pow(m_pi,4) + pow(m_pi,2)*(pow(mrho,2) - 2*t) + t*(-pow(mrho,2) + 2*s + t)))/
        ((pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))*
          (pow(m_pi,2) - t)) - (8*(-2 + delta)*(pow(m_pi,4)*(2 - 3*delta + 8*C4*pow(mrho,2)) + pow(mrho,4)*(-2 + delta + 8*C4*t) +
            t*((2 + 3*delta)*s + 2*delta*t) + pow(m_pi,2)*(-8*C4*pow(mrho,4) + (-2 + delta)*s - (2 + 3*delta)*t + 4*pow(mrho,2)*(1 + 4*C4*t)) -
            pow(mrho,2)*(t*(-2 + 3*delta + 8*C4*t) + s*(-2 + delta + 16*C4*t))))/
        (pow(mrho,2)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))*(pow(m_pi,2) - t)) +
       (4*(-2 + delta)*(eta1 - eta2)*(pow(ma1,2) - s)*(eta2*(pow(m_pi,2) + s)*
             (pow(m_pi,4) - pow(m_pi,2)*(pow(mrho,2) - 2*s) + s*(pow(mrho,2) - s - 2*t)) +
            eta1*(-4*pow(m_pi,6) + s*(-pow(mrho,2) + s)*(-pow(mrho,2) + s + t) + pow(m_pi,4)*(3*pow(mrho,2) + s + t) -
               pow(m_pi,2)*(pow(mrho,4) + 2*s*(s - t) + pow(mrho,2)*(-s + t)))))/
        ((pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,2) - s)*
          (pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
       (pow(eta1 - eta2,2)*(pow(eta2,2)*(pow(m_pi,8) - 2*pow(m_pi,6)*pow(mrho,2) + pow(m_pi,4)*(pow(pow(mrho,2) + 2*s,2) - 2*s*t) +
               pow(s,2)*(pow(pow(mrho,2) + s,2) + 2*(-pow(mrho,2) + s)*t + 2*pow(t,2)) -
               2*pow(m_pi,2)*s*(pow(mrho,4) + pow(mrho,2)*(2*s - t) + 2*s*(s + t))) +
            2*eta1*eta2*(-pow(m_pi,8) + pow(m_pi,4)*(pow(mrho,4) + 2*pow(mrho,2)*s + 2*s*(-2*s + t)) -
               2*pow(m_pi,2)*s*(pow(mrho,4) + pow(mrho,2)*(s + t) - 2*s*(s + t)) + pow(s,2)*(pow(mrho,4) - pow(s,2) + 2*pow(mrho,2)*t - 2*t*(s + t)))\
             + pow(eta1,2)*(pow(m_pi,8) - 2*pow(m_pi,6)*pow(mrho,2) + pow(m_pi,4)*(3*pow(mrho,4) + 2*s*(2*s - t) + 2*pow(mrho,2)*(-3*s + t)) -
               2*pow(m_pi,2)*(pow(mrho,2) - s)*(-2*s*(s + t) + pow(mrho,2)*(2*s + t)) +
               s*(-pow(mrho,2) + s)*(pow(s,2) + 2*s*t + 2*pow(t,2) - pow(mrho,2)*(s + 2*t)))))/
        ((pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
       (pow(eta1 - eta2,2)*(-2*eta1*eta2*(pow(m_pi,8) - pow(m_pi,4)*(pow(mrho,4) + 2*(pow(mrho,2) + s)*t - 4*pow(t,2)) +
               pow(t,2)*(-pow(mrho,4) - 2*pow(mrho,2)*s + 2*pow(s,2) + 2*s*t + pow(t,2)) +
               2*pow(m_pi,2)*t*(pow(mrho,4) + pow(mrho,2)*(s + t) - 2*t*(s + t))) +
            pow(eta2,2)*(pow(m_pi,8) - 2*pow(m_pi,6)*pow(mrho,2) + pow(m_pi,4)*(pow(mrho,4) + 4*pow(mrho,2)*t - 2*(s - 2*t)*t) +
               pow(t,2)*(pow(mrho,4) + 2*pow(s,2) + 2*s*t + pow(t,2) + 2*pow(mrho,2)*(-s + t)) -
               2*pow(m_pi,2)*t*(pow(mrho,4) - pow(mrho,2)*(s - 2*t) + 2*t*(s + t))) +
            pow(eta1,2)*(pow(m_pi,8) - 2*pow(m_pi,6)*pow(mrho,2) + pow(m_pi,4)*(3*pow(mrho,4) + 2*pow(mrho,2)*(s - 3*t) - 2*(s - 2*t)*t) +
               t*(-pow(mrho,2) + t)*(2*pow(s,2) + 2*s*t + pow(t,2) - pow(mrho,2)*(2*s + t)) -
               2*pow(m_pi,2)*(-pow(mrho,2) + t)*(2*t*(s + t) - pow(mrho,2)*(s + 2*t)))))/
        ((pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))*pow(pow(ma1,2) - t,2)) +
       (8*(-2 + delta)*((-2 + delta)*pow(mrho,6) + pow(m_pi,6)*(-2 + 3*delta - 8*C4*pow(mrho,2)) + s*t*((-2 + 3*delta)*s + 4*delta*t) +
            pow(m_pi,4)*(8*C4*pow(mrho,4) + 4*delta*s + 2*t - 3*delta*t - pow(mrho,2)*(2 + delta + 16*C4*s - 8*C4*t)) +
            pow(mrho,4)*(-((-2 + delta)*t) + s*(4 - 2*delta + 8*C4*t)) + pow(mrho,2)*s*(s*(-2 + delta - 8*C4*t) - 2*t*(delta + 8*C4*t)) +
            pow(m_pi,2)*(s*((2 - 3*delta)*s - 8*delta*t) - pow(mrho,4)*(-6 + 3*delta + 8*C4*(s + t)) +
               pow(mrho,2)*(8*C4*pow(s,2) + 4*(-1 + delta)*t + s*(-8 + 6*delta + 32*C4*t)))))/
        (pow(mrho,2)*(pow(m_pi,2) - s)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))*(pow(m_pi,2) - t)) +
       (2*pow(eta1 - eta2,2)*(pow(ma1,2) - s)*(pow(eta1,2)*(pow(m_pi,8) + pow(m_pi,4)*(2*pow(mrho,4) + 2*s*t - 3*pow(mrho,2)*(s + t)) +
               s*t*(2*pow(mrho,4) + pow(s,2) + 3*s*t + pow(t,2) - 3*pow(mrho,2)*(s + t)) -
               2*pow(m_pi,2)*(pow(mrho,2) - s - t)*(-2*s*t + pow(mrho,2)*(s + t))) +
            pow(eta2,2)*(pow(m_pi,8) - 4*pow(m_pi,2)*s*t*(pow(mrho,2) + s + t) + pow(m_pi,4)*(2*s*t + pow(mrho,2)*(s + t)) +
               s*t*(pow(s,2) + 3*s*t + pow(t,2) + pow(mrho,2)*(s + t))) +
            2*eta1*eta2*(-pow(m_pi,8) + 2*pow(m_pi,6)*pow(mrho,2) - 2*pow(m_pi,4)*s*t - s*t*(pow(s,2) + 3*s*t + pow(t,2) - 2*pow(mrho,2)*(s + t)) -
               pow(m_pi,2)*(-4*s*t*(s + t) + pow(mrho,2)*(pow(s,2) + 4*s*t + pow(t,2))))))/
        ((pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))*
          (pow(ma1,2) - t)) + (8*(pow(delta,2)*(8*pow(m_pi,4) + 3*pow(mrho,4) - 6*pow(mrho,2)*(s + t) + 2*pow(s + t,2) +
               4*pow(m_pi,2)*(3*pow(mrho,2) - 2*(s + t))) - 4*delta*pow(mrho,2)*
             (16*C4*pow(m_pi,4) + pow(mrho,2)*(3 - 6*C4*(s + t)) + (s + t)*(-3 + 4*C4*(s + t)) + 2*pow(m_pi,2)*(3 + C4*(6*pow(mrho,2) - 8*(s + t)))) +
            4*pow(mrho,4)*(3 + 4*C4*(2*pow(m_pi,2) - s - t)*(3 + C4*(4*pow(m_pi,2) - 2*(s + t))))))/
        (pow(mrho,4)*(pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
       (4*(eta1 - eta2)*(-pow(ma1,2) + s)*(eta2*(-2*pow(m_pi,4)*(delta - 4*C4*pow(mrho,2))*(pow(mrho,2) + 4*s) +
               pow(m_pi,2)*(-2*pow(mrho,4)*(-2 + delta + 8*C4*s) + 8*delta*s*(s + t) - pow(mrho,2)*((-10 + delta)*s - (-2 + delta)*t + 32*C4*s*(s + t))) +
               s*(2*pow(mrho,4)*(-2 + delta + 4*C4*s) - 2*delta*pow(s + t,2) + pow(mrho,2)*((-6 + delta)*s + (-2 + delta)*t + 8*C4*pow(s + t,2)))) +
            eta1*(4*pow(m_pi,4)*(6*C4*pow(mrho,4) + 2*delta*s + pow(mrho,2)*(1 - 2*delta - 8*C4*s)) + 2*delta*s*pow(s + t,2) -
               pow(mrho,2)*((-6 + 5*delta)*pow(s,2) + 2*(-2 + 3*delta)*s*t + (-2 + delta)*pow(t,2) + 8*C4*s*pow(s + t,2)) +
               pow(mrho,4)*((-2 + delta)*(3*s + t) + 8*C4*s*(s + 2*t)) -
               2*pow(m_pi,2)*(4*delta*s*(s + t) - pow(mrho,2)*(-6*s + 7*delta*s - 2*t + 3*delta*t + 16*C4*s*(s + t)) +
                  2*pow(mrho,4)*(-2 + delta + 4*C4*(2*s + t))))))/
        (pow(mrho,2)*(pow(Gammaa1,2)*pow(ma1,2) + pow(pow(ma1,2) - s,2))*
          (pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))) +
       (4*(eta1 - eta2)*(((-2 + delta)*(pow(m_pi,4) - pow(m_pi,2)*(pow(mrho,2) - 2*s) + s*(pow(mrho,2) - s - 2*t))*
               (eta1*(pow(mrho,2) - s - t) + eta2*(pow(m_pi,2) + t)))/((pow(m_pi,2) - s)*(pow(ma1,2) - t)) +
            ((-2 + delta)*(eta2*(pow(m_pi,2) + t)*(pow(m_pi,4) - pow(m_pi,2)*(pow(mrho,2) - 2*t) + (pow(mrho,2) - 2*s - t)*t) +
                 eta1*(-4*pow(m_pi,6) + (pow(mrho,2) - t)*(pow(mrho,2) - s - t)*t + pow(m_pi,4)*(3*pow(mrho,2) + s + t) -
                    pow(m_pi,2)*(pow(mrho,4) + pow(mrho,2)*(s - t) + 2*t*(-s + t)))))/((-pow(ma1,2) + t)*(-pow(m_pi,2) + t)) +
            (eta2*(-2*pow(m_pi,4)*(delta - 4*C4*pow(mrho,2))*(pow(mrho,2) + 4*t) +
                  pow(m_pi,2)*(8*delta*t*(s + t) - 2*pow(mrho,4)*(-2 + delta + 8*C4*t) -
                     pow(mrho,2)*(-((-2 + delta)*s) + (-10 + delta)*t + 32*C4*t*(s + t))) +
                  t*(-2*delta*pow(s + t,2) + 2*pow(mrho,4)*(-2 + delta + 4*C4*t) + pow(mrho,2)*((-2 + delta)*s + (-6 + delta)*t + 8*C4*pow(s + t,2)))) +
               eta1*(2*delta*t*pow(s + t,2) - pow(mrho,2)*((-2 + delta)*pow(s,2) + 2*(-2 + 3*delta)*s*t + (-6 + 5*delta)*pow(t,2) + 8*C4*t*pow(s + t,2)) +
                  pow(mrho,4)*(8*C4*t*(2*s + t) + (-2 + delta)*(s + 3*t)) +
                  4*pow(m_pi,4)*(6*C4*pow(mrho,4) + 2*delta*t + pow(mrho,2)*(1 - 2*delta - 8*C4*t)) -
                  2*pow(m_pi,2)*(4*delta*t*(s + t) - pow(mrho,2)*(-2*s + 3*delta*s - 6*t + 7*delta*t + 16*C4*t*(s + t)) +
                     2*pow(mrho,4)*(-2 + delta + 4*C4*(s + 2*t)))))/(pow(mrho,2)*(-pow(ma1,2) + t))))/
        (pow(m_pi,4) + pow(pow(mrho,2) - s,2) - 2*pow(m_pi,2)*(pow(mrho,2) + s))))/(512.*Pi);

      break;
    case ReactionType::pi_rho:
      diff_xsection = -alpha * g_rho_2 / (48 * s * p_cm_2);
      e = 4 * (pow_int(m2, 2) - 4 * pow_int(m_pi, 2)) *
          (t / pow_int(t - m_pi_2, 2) + u / pow_int(u - m2, 2) -
           m2 / (s - m_pi_2) * (1 / (t - m_pi_2) + 1 / (u - m2)));
      e += (3.0 + (s - m_pi_2) / m2) * (s - m_pi_2) / (u - m2) - 0.5 + s / m2 -
           pow_int((s - m_pi_2) / (u - m2), 2);
      diff_xsection = diff_xsection * e;
      break;
    case ReactionType::pi0_rho:
      diff_xsection = alpha * g_rho_2 / (48 * s * p_cm_2);
      e = 4.5 - s / m2 -
          4 * s * (pow_int(m2, 2) - 4*pow_int(m_pi, 2)) / pow_int(s - m_pi_2, 2)
          + (pow_int(s - m_pi_2, 2) - 4*m2*(pow_int(m2, 2)
          - 4*pow_int(m_pi, 2))) /
              pow_int(u - m2, 2);
      e += 1 / (u - m2) * (5 * (s - m_pi_2) - pow_int(s - m_pi_2, 2) / m2 -
                           4 * (pow_int(m2, 2) - 4 * pow_int(m_pi, 2)) *
                               (s - m_pi_2 + m2) / (s - m_pi_2));
      diff_xsection = diff_xsection * e;
      break;
    case ReactionType::pi_eta:
      diff_xsection = twopi * alpha * 4.7 *
                      (pow_int(m_rho, 4) / (pow_int(u - m_rho_2, 2) +
                                        pow_int(gamma_rho_tot, 2) * m_rho_2)) /
                      (32 * m_eta_2 * pow_int(m_rho, 4) * s * p_cm_2);
      diff_xsection = diff_xsection * (u * (s - m_pi_2) * (t - m_pi_2) -
                                       m_pi_2 * pow_int(u - m_eta_2, 2));
      break;
    case ReactionType::pi0_rho0:

    if (incoming_particles_[0].pdgcode().is_rho()) {
      m_rho_case = incoming_particles_[0].effective_mass();
    } else if (incoming_particles_[1].pdgcode().is_rho()) {
      m_rho_case = incoming_particles_[1].effective_mass();
    }

      diff_xsection = (pow(Const,2)*pow(g_POR,4)*(pow(m_omega,4)*pow(s,4) + 4*pow(m_omega,4)*pow(s,3)*t - 4*pow(m_omega,2)*pow(s,4)*t + 10*pow(m_omega,4)*pow(s,2)*pow(t,2) -
                      16*pow(m_omega,2)*pow(s,3)*pow(t,2) + 5*pow(s,4)*pow(t,2) + 4*pow(m_omega,4)*s*pow(t,3) - 16*pow(m_omega,2)*pow(s,2)*pow(t,3) +
                      10*pow(s,3)*pow(t,3) + pow(m_omega,4)*pow(t,4) - 4*pow(m_omega,2)*s*pow(t,4) + 5*pow(s,2)*pow(t,4) + pow(m_pi,8)*pow(-2*pow(m_omega,2) + s + t,2) -
                      2*pow(m_pi,6)*pow(m_rho_case,2)*(2*pow(m_omega,4) + pow(s,2) + pow(t,2) - 2*pow(m_omega,2)*(s + t)) +
                      pow(m_rho_case,4)*(2*pow(s,2)*pow(t,2) - 2*pow(m_omega,2)*s*t*(s + t) + pow(m_omega,4)*(pow(s,2) + pow(t,2))) -
                      2*pow(m_rho_case,2)*(3*pow(s,2)*pow(t,2)*(s + t) - 3*pow(m_omega,2)*s*t*pow(s + t,2) +
                      pow(m_omega,4)*(pow(s,3) + 2*pow(s,2)*t + 2*s*pow(t,2) + pow(t,3))) +
                      pow(m_pi,4)*(-2*pow(m_rho_case,2)*(pow(m_omega,2) - s)*(pow(m_omega,2) - t)*(s + t) - 8*pow(m_omega,2)*s*t*(s + t) + 4*pow(m_omega,4)*(pow(s,2) + pow(t,2)) -
                      2*s*t*(pow(s,2) - 6*s*t + pow(t,2)) + pow(m_rho_case,4)*(2*pow(m_omega,4) + pow(s,2) + pow(t,2) - 2*pow(m_omega,2)*(s + t))) -
                      2*pow(m_pi,2)*(2*(s + t)*pow(-2*s*t + pow(m_omega,2)*(s + t),2) + pow(m_rho_case,4)*(-4*pow(m_omega,2)*s*t + pow(m_omega,4)*(s + t) + s*t*(s + t)) -
                      pow(m_rho_case,2)*(-10*pow(m_omega,2)*s*t*(s + t) + 2*pow(m_omega,4)*(pow(s,2) + 3*s*t + pow(t,2)) + s*t*(pow(s,2) + 8*s*t + pow(t,2))))))/
                      (128.0*M_PI*pow(pow(m_omega,2) - s,2)*(pow(pow(m_pi,2) - pow(m_rho_case,2),2) - 2*(pow(m_pi,2) + pow(m_rho_case,2))*s + pow(s,2))*pow(pow(m_omega,2) - t,2));
      break;
    case ReactionType::no_reaction:
      // never reached
      break;
  }
  return diff_xsection*to_mb;
}

}  // namespace Smash
