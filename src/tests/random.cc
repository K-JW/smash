/*
 *
 *    Copyright (c) 2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "unittest.h"
#include "histogram.h"

#include "../include/random.h"

using namespace Smash;

TEST(set_random_seed) {
    std::random_device rd;
    int64_t seed = rd();
    Random::set_seed(seed);
    printf("Random number seed: %ld\n", seed);
}

int tst_cnt = 0;  // test_counter

/**
 * Compare a random distribution to an analytical function.
 *
 * The distribution 'get_chi' is sampled 'n_test' times and put into a histogram
 * with bin size 'dx'. Then it is compared to the analytical function
 * 'get_analyt'.
 */
template <typename Chi, typename Analytical>
void test_distribution(int n_test, double dx,
                       Chi get_chi, Analytical get_analyt) {
  Histogram1d hist(dx);
  // sample distribution and populate histogram
  hist.populate(n_test, get_chi);
  // test with the analytical function
  hist.test(get_analyt
           // ,"random_" + std::to_string(tst_cnt) + ".dat"
           );
  tst_cnt++;
}


constexpr int N_TEST = 1E7;  // number of samples


TEST(canonical) {
  test_distribution(N_TEST, 0.0001,
                    []() { return Random::canonical(); },
                    [](double) { return 1.0; });
}

TEST(uniform) {
  auto random_4_6 = Random::make_uniform_distribution(-4.0, 6.0);
  test_distribution(N_TEST, 0.01,
                    [&]() { return random_4_6(); },
                    [](double) { return 1.0; });
}

TEST(exponential) {
  test_distribution(N_TEST, 0.1,
                    []() { return Random::exponential(1.0); },
                    [](double x) { return exp(-x); });
}

TEST(x_exponential) {
  test_distribution(N_TEST, 0.05,
      []() { return Random::exponential(1.0) + Random::exponential(1.0); },
      [](double x) { return x*exp(-x); });
}

TEST(xsquared_exponential) {
  test_distribution(N_TEST, 0.05,
      []() {
        return Random::exponential(1.0) + Random::exponential(1.0) +
               Random::exponential(1.0);
      },
      [](double x) { return x*x*exp(-x); });
}

TEST(expo) {
  test_distribution(N_TEST, 0.001,
                    []() { return Random::expo(2., -1., -3.); },
                    [](double x) { return exp(2.*x); });
}

TEST(power) {
  test_distribution(N_TEST, 0.001,
                    []() { return Random::power(3., 0.5, 1.5); },
                    [](double x) { return x*x*x; });
}

TEST(cauchy) {
  const double m0 = 0.770;
  const double Gamma = 0.150;
  test_distribution(N_TEST, 0.001,
                    [&]() { return Random::cauchy(m0, Gamma, 0.280, 1.5); },
                    [&](double x) { return Gamma/(Gamma*Gamma + (x-m0)*(x-m0)); });
}
