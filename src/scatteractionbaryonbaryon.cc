
/*
 *
 *    Copyright (c) 2015
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "include/scatteractionbaryonbaryon.h"

#include "include/clebschgordan.h"
#include "include/constants.h"
#include "include/cxx14compat.h"
#include "include/parametrizations.h"

namespace Smash {


float ScatterActionBaryonBaryon::total_cross_section() const {
  const PdgCode &pdg_a = incoming_particles_[0].type().pdgcode();
  const PdgCode &pdg_b = incoming_particles_[1].type().pdgcode();
  const double s = mandelstam_s();

  /* Currently all BB collisions use the nucleon-nucleon parametrizations. */
  if (pdg_a == pdg_b) {
    return pp_total(s);     // pp, nn
  } else if (pdg_a.is_antiparticle_of(pdg_b)) {
    return ppbar_total(s);  // NNbar
  } else {
    return np_total(s);     // np
  }
}


CollisionBranchList ScatterActionBaryonBaryon::two_to_two_cross_sections() {
  CollisionBranchList process_list;
  const ParticleType &type_a = incoming_particles_[0].type();
  const ParticleType &type_b = incoming_particles_[1].type();

  if (type_a.is_nucleon() || type_a.is_Delta() ||
      type_b.is_nucleon() || type_b.is_Delta()) {
    /* N R → N N, Δ R → N N */
    process_list = bar_bar_to_nuc_nuc(type_a, type_b, false);
  }
  if (type_a.is_anti_nucleon() || type_a.is_anti_Delta() ||
      type_b.is_anti_nucleon() || type_b.is_anti_Delta()) {
    /* N̅ R → N̅ N̅, Δ̅ R → N̅ N̅ */
    process_list = bar_bar_to_nuc_nuc(type_a, type_b, true);
  }

  return process_list;
}


CollisionBranchList ScatterActionBaryonBaryon::bar_bar_to_nuc_nuc(
                            const ParticleType &type_a,
                            const ParticleType &type_b,
                            const bool is_anti_particles) {
  CollisionBranchList process_list;

  const double s = mandelstam_s();
  const double sqrts = sqrt_s();
  /* CM momentum in final state */
  double p_cm_final = std::sqrt(s - 4.*nucleon_mass*nucleon_mass)/2.;

  ParticleTypePtrList nuc_or_anti_nuc;
  if (is_anti_particles) {
    nuc_or_anti_nuc = ParticleType::list_anti_nucleons();
  }
  else nuc_or_anti_nuc = ParticleType::list_nucleons();

  /* Loop over all nucleon or anti-nucleon charge states. */
  for (ParticleTypePtr nuc_a : nuc_or_anti_nuc) {
    for (ParticleTypePtr nuc_b : nuc_or_anti_nuc) {
      /* Check for charge conservation. */
      if (type_a.charge() + type_b.charge()
          != nuc_a->charge() + nuc_b->charge()) {
        continue;
      }
      // loop over total isospin
      for (const int twoI : I_tot_range(*nuc_a, *nuc_b)) {
        const float isospin_factor = isospin_clebsch_gordan_sqr_2to2(type_a,
                                                  type_b, *nuc_a, *nuc_b, twoI);
        /* If Clebsch-Gordan coefficient is zero, don't bother with the rest */
        if (std::abs(isospin_factor) < really_small) {
          continue;
        }

        /* Calculate matrix element for inverse process. */
        const float matrix_element =
            nn_to_resonance_matrix_element(sqrts, type_a, type_b, twoI);
        if (matrix_element <= 0.) {
          continue;
        }

        /** Cross section for 2->2 resonance absorption, obtained via detailed
         * balance from the inverse reaction.
         * See eqs. (B.6), (B.9) and (181) in \iref{Buss:2011mx}.
         * There are factors for spin, isospin and symmetry involved. */
        const float spin_factor = (nuc_a->spin() + 1) * (nuc_b->spin() + 1);
        const int sym_fac_in =
                    (type_a.iso_multiplet() == type_b.iso_multiplet()) ? 2 : 1;
        const int sym_fac_out =
                    (nuc_a->iso_multiplet() == nuc_b->iso_multiplet()) ? 2 : 1;
        const float xsection = isospin_factor * spin_factor
                              * sym_fac_in / sym_fac_out
                              * p_cm_final * matrix_element / (s*cm_momentum());

        if (xsection > really_small) {
          process_list.push_back(make_unique<CollisionBranch>
                                (*nuc_a, *nuc_b, xsection,
                                  ProcessType::TwoToTwo));
          const auto &log = logger<LogArea::ScatterAction>();
          log.debug("2->2 absorption with original particles: ",
                    type_a, type_b);
        }
      }
    }
  }
  return process_list;
}


float ScatterActionBaryonBaryon::nn_to_resonance_matrix_element(double sqrts,
      const ParticleType &type_a, const ParticleType &type_b, const int twoI) {
  const float m_a = type_a.mass();
  const float m_b = type_b.mass();
  const float msqr = 2. * (m_a*m_a + m_b*m_b);

  /** NN → NΔ: fit sqrt(s)-dependence to OBE model [\iref{Dmitriev:1986st}] */
  if ((type_a.is_Delta() && type_b.is_nucleon()) ||
      (type_b.is_Delta() && type_a.is_nucleon()) ||
      (type_a.is_anti_Delta() && type_b.is_anti_nucleon()) ||
      (type_b.is_anti_Delta() && type_a.is_anti_nucleon())) {
    return 68. / std::pow(sqrts - 1.104, 1.951);
  /** All other processes use a constant matrix element,
   *  similar to \iref{Bass:1998ca}, equ. (3.35). */
  } else if ((type_a.is_Nstar() && (type_b.is_nucleon() || type_b.is_anti_nucleon())) ||
             (type_b.is_Nstar() && (type_a.is_nucleon() || type_a.is_anti_nucleon()))) {
    // NN → NN*
    if (twoI == 2) {
      return 7. / msqr;
    } else if (twoI == 0) {
      return 14. / msqr;
    }
  } else if ((type_a.is_Deltastar() && (type_b.is_nucleon() || type_b.is_anti_nucleon())) ||
             (type_b.is_Deltastar() && (type_a.is_nucleon() || type_a.is_anti_nucleon()))) {
    // NN → NΔ*
    return 15. / msqr;
  } else if ((type_a.is_Delta() && type_b.is_Delta()) ||
             (type_a.is_anti_Delta() && type_b.is_anti_Delta())) {
    // NN → ΔΔ
    if (twoI == 2) {
      return 45. / msqr;
    } else if (twoI == 0) {
      return 120. / msqr;
    }
  } else if ((type_a.is_Nstar() && (type_b.is_Delta() || type_b.is_anti_Delta())) ||
             (type_b.is_Nstar() && (type_a.is_Delta() || type_a.is_anti_Delta()))) {
    // NN → ΔN*
    return 7. / msqr;
  } else if ((type_a.is_Deltastar() && (type_b.is_Delta()  || type_b.is_anti_Delta())) ||
             (type_b.is_Deltastar() && (type_a.is_Delta()  || type_a.is_anti_Delta()))) {
    // NN → ΔΔ*
    if (twoI == 2) {
      return 15. / msqr;
    } else if (twoI == 0) {
      return 25. / msqr;
    }
  }
  // all cases not listed: zero!
  return 0.;
}


void ScatterActionBaryonBaryon::format_debug_output(std::ostream &out) const {
  out << "Baryon-Baryon ";
  ScatterAction::format_debug_output(out);
}


}  // namespace Smash
