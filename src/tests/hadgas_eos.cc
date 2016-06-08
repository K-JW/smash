/*
 *
 *    Copyright (c) 2016
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "unittest.h"
#include "setup.h"
#include "../include/hadgas_eos.h"
#include "../include/constants.h"

using namespace Smash;

TEST(td_simple_gas) {
  ParticleType::create_type_list(
    "# NAME MASS[GEV] WIDTH[GEV] PDG\n"
    "π⁰ 0.138 7.7e-9     111\n"
    "K⁰ 0.494 0.0        311\n"
    "N⁺ 0.938 0.0       2212\n");
  // Note that antiparticles are also created!
  const double T   = 0.1;
  const double mub = 0.8;
  const double mus = 0.1;
  COMPARE_ABSOLUTE_ERROR(HadronGasEos::net_baryon_density(T, mub, mus), 0.144397953, 1.e-6);
  COMPARE_ABSOLUTE_ERROR(HadronGasEos::net_strange_density(T, mub, mus), 0.002152896, 1.e-6);
  COMPARE_ABSOLUTE_ERROR(HadronGasEos::density(T, mub, mus), 0.15637968, 1.e-6);
  COMPARE_ABSOLUTE_ERROR(HadronGasEos::pressure(T, mub, mus), 0.015637968, 1.e-6);
  COMPARE_ABSOLUTE_ERROR(HadronGasEos::energy_density(T, mub, mus), 0.16493153, 1.e-6);
}

TEST(mu_zero_net_strangeness) {
  const double mub = 0.6;
  const double T   = 0.05;
  const double mus = HadronGasEos::mus_net_strangeness0(T, mub);
  const double ns  = HadronGasEos::net_strange_density(T, mub, mus);
  VERIFY(std::abs(ns) < 1.e-4) << ns << ", mus = " << mus;
}

TEST(solve_EoS_substitute) {
  const double mub = 0.2;
  const double mus = 0.0;
  const double T = 0.30;
  const double e  = HadronGasEos::energy_density(T, mub, mus);
  const double nb = HadronGasEos::net_baryon_density(T, mub, mus);
  const double ns = HadronGasEos::net_strange_density(T, mub, mus);
  HadronGasEos eos = HadronGasEos(false);
  const std::array<double, 3> sol = eos.solve_eos(e, nb, ns);
  COMPARE_ABSOLUTE_ERROR(sol[0], T, 1.e-4);
  COMPARE_ABSOLUTE_ERROR(sol[1], mub, 1.e-4);
  COMPARE_ABSOLUTE_ERROR(sol[2], mus, 1.e-4);
}
/*
TEST(make_test_table) {
  // To switch on these tests, comment out the previous ones.
  // Otherwise the particle table confilct emerges.
  Test::create_actual_particletypes();
  const double ns = 0.0;
  const double nb = 0.3;
  HadronGasEos eos = HadronGasEos(false);
  for (int ie = 0; ie < 1000; ie++) {
    const double e = nb + 0.001 + 0.001 * ie;
    const std::array<double, 3> sol = eos.solve_eos(e, nb, ns);
    std::cout << e << " " << HadronGasEos::pressure(sol[0], sol[1], sol[2]) << std::endl;
  }
}

TEST(make_test_table2) {
  const double mub = 0.0;
  const double mus = 0.0;
  for (int it = 0; it < 1000; it++) {
    const double T = 0.070 + 0.001 * it;
    const double e  = HadronGasEos::energy_density(T, mub, mus);
    const double p  = HadronGasEos::pressure(T, mub, mus);
    std::cout << T << " " << (e-3*p)/(T*T*T*T) << std::endl;
  }
}*/
