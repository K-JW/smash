/*
 *    Copyright (c) 2013
 *      maximilian attems <attems@fias.uni-frankfurt.de>
 *      Jussi Auvinen <auvinen@fias.uni-frankfurt.de>
 *
 *    GNU General Public License (GPLv3)
 */
#ifndef SRC_INCLUDE_PARAMETERS_H_
#define SRC_INCLUDE_PARAMETERS_H_

#include "../include/constants.h"

class Parameters {
  public:
    /* default constructor with probable values */
    Parameters(): output_interval_(100), testparticles_(1), eps_(0.001),
      cross_section_(10.0), seed_(1) {}
    /* member funtions */
    int inline output_interval() const;
    void inline set_output_interval(const int &UPDATE);
    int inline testparticles() const;
    void inline set_testparticles(const int &TESTPARTICLES);
    float inline eps() const;
    void inline set_eps(const float &EPS);
    float inline cross_section() const;
    void inline set_cross_section(const float &sigma);
    int64_t inline seed() const;
    void inline set_seed(const int64_t &RANDOMSEED);

  private:
    /* number of steps before giving measurables */
    int output_interval_;
    /* number of test particle */
    int testparticles_;
    /* temporal time step */
    float eps_;
    /* cross section of the elastic scattering */
    float cross_section_;
    /* initial seed for random generator */
    int64_t seed_;
};

/* return the number on whihc interval output will be shown */
int inline Parameters::output_interval(void) const {
  return output_interval_;
}

/* set when to output physics */
void inline Parameters::set_output_interval(const int &update) {
  output_interval_ = update;
}

/* return the number of testparticles:
 * if equal to one a "testparticle" corresponds to a real particle
 */
int inline Parameters::testparticles(void) const {
  return testparticles_;
}

/* set the number of test particles */
void inline Parameters::set_testparticles(const int &TESTPARTICLES) {
  testparticles_ = TESTPARTICLES;
}

/* return the time step in use */
float inline Parameters::eps(void) const {
  return eps_;
}

/* set the time step */
void inline Parameters::set_eps(const float &EPS) {
  eps_ = EPS;
}

/* return the cross section */
float inline Parameters::cross_section(void) const {
  return cross_section_;
}

/* set the cross section */
void inline Parameters::set_cross_section(const float &sigma) {
  cross_section_ = sigma;
}

/* return the random seed */
int64_t inline Parameters::seed(void) const {
  return seed_;
}

/* set the seed */
void inline Parameters::set_seed(const int64_t &randomseed) {
  seed_ = randomseed;
}



#endif  // SRC_INCLUDE_PARAMETERS_H_
