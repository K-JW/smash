/*
 *
 *    Copyright (c) 2013-2015
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */
#include "include/resonances.h"

#include <gsl/gsl_sf_coupling.h>

#include "include/kinematics.h"
#include "include/logging.h"
#include "include/random.h"

namespace Smash {


double clebsch_gordan(const int j_a, const int j_b, const int j_c,
                      const int m_a, const int m_b, const int m_c) {
  const auto &log = logger<LogArea::Resonances>();
  double wigner_3j =  gsl_sf_coupling_3j(j_a, j_b, j_c, m_a, m_b, -m_c);
  double result = 0.;
  if (std::abs(wigner_3j) > really_small)
    result = std::pow(-1, (j_a-j_b+m_c)/2.) * std::sqrt(j_c + 1) * wigner_3j;

  log.debug("CG: ", result, " I1: ", j_a, " I2: ", j_b, " IR: ", j_c,
            " iz1: ", m_a, " iz2: ", m_b, " izR: ", m_c);

  return result;
}

/* Integrand for spectral-function integration */
float spectral_function_integrand(float resonance_mass, float srts,
                                  float stable_mass, const ParticleType &type) {
  if (srts <= stable_mass + resonance_mass) {
    return 0.;
  }

  /* Integrand is the spectral function weighted by the CM momentum of the
   * final state. */
  return type.spectral_function(resonance_mass)
         * pCM(srts, stable_mass, resonance_mass);
}

/* Resonance mass sampling for 2-particle final state */
float sample_resonance_mass(const ParticleType &type_res,
                            const float mass_stable, const float cms_energy) {
  /* largest possible mass: Use 'nextafter' to make sure it is not above the
   * physical limit by numerical error. */
  const float max_mass = std::nextafter(cms_energy - mass_stable, 0.f);
  // largest possible cm momentum (from smallest mass)
  const float pcm_max = pCM(cms_energy, mass_stable, type_res.minimum_mass());
  /* The maximum of the spectral-function ratio 'usually' happens at the
   * largest mass. However, this is not always the case, therefore we need
   * an additional fudge factor (empirically 2.5 happens to be sufficient). */
  const float q_max = type_res.spectral_function(max_mass)
                    / type_res.spectral_function_simple(max_mass) * 2.5;
  const float max = pcm_max * q_max;  // maximum value for rejection sampling
  float mass_res, pcm, q;
  // Loop: rejection sampling
  do {
    // sample mass from a simple Breit-Wigner (aka Cauchy) distribution
    mass_res = Random::cauchy(type_res.mass(), type_res.width_at_pole()/2.f,
                              type_res.minimum_mass(), max_mass);
    // determine cm momentum for this case
    pcm = pCM(cms_energy, mass_stable, mass_res);
    // determine ratio of full to simple spectral function
    q = type_res.spectral_function(mass_res)
      / type_res.spectral_function_simple(mass_res);
  } while (q*pcm < Random::uniform(0.f, max));

  // check that we are using the proper maximum value
  if (q*pcm > max) {
    const auto &log = logger<LogArea::Resonances>();
    log.fatal("maximum not correct in sample_resonance_mass: ",
              q*pcm, " ", max, " ", type_res.pdgcode(), " ",
              mass_stable, " ", cms_energy, " ", mass_res);
    throw std::runtime_error("Maximum not correct in sample_resonance_mass!");
  }

  return mass_res;
}


}  // namespace Smash
