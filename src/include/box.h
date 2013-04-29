/*
 *    Copyright (c) 2012-2013
 *      maximilian attems <attems@fias.uni-frankfurt.de>
 *      Jussi Auvinen <auvinen@fias.uni-frankfurt.de>
 *    GNU General Public License (GPLv3)
 */
#ifndef SRC_INCLUDE_BOX_H_
#define SRC_INCLUDE_BOX_H_

#include <stdint.h>
#include <time.h>

class box {
  public:
    /* default constructor with probable values */
    box(): steps_(10000), update_(100), a_(10.0), eps_(0.001),
      temperature_(0.1), cross_section_(10.0), energy_initial_(0),
      seed_(1), testparticle_(1), time_start_(set_timer_start()) {}
    /* member funtions */
    float inline a() const;
    void inline set_a(const float &A);
    float inline cross_section() const;
    void inline set_cross_section(const float &sigma);
    float inline energy_initial() const;
    void inline set_energy_initial(const float &energy);
    float inline eps() const;
    void inline set_eps(const float &EPS);
    float inline number_density_initial() const;
    void inline set_number_density_inital(const float &number_density);
    float inline temperature() const;
    void inline set_temperature(const float &T);
    int inline testparticle() const;
    void inline set_testparticle(const int &TESTPARTICLE);
    int64_t inline seed() const;
    void inline set_seed(const int64_t &RANDOMSEED);
    int inline steps() const;
    void inline set_steps(const int &STEPS);
    int inline update() const;
    void inline set_update(const int &UPDATE);
    timespec inline time_start() const;
    timespec inline set_timer_start();

  private:
    /* number of steps */
    int steps_;
    /* number of steps before giving measurables */
    int update_;
    /* Cube edge length */
    float a_;
    /* temporal time step */
    float eps_;
    /* Temperature of the Boltzmann distribution for thermal initialization */
    float temperature_;
    /* cross section of the elastic scattering */
    float cross_section_;
    /* initial total energy of the box */
    float energy_initial_;
    /* initial number density of the box */
    float number_density_initial_;
    /* initial seed for random generator */
    int64_t seed_;
    /* number of test particle */
    int testparticle_;
    /* starting time of the simulation */
    timespec time_start_;
};

float inline box::a(void) const {
  return a_;
}

void inline box::set_a(const float &A) {
  a_ = A;
}

float inline box::eps(void) const {
  return eps_;
}

void inline box::set_eps(const float &EPS) {
  eps_ = EPS;
}

int inline box::steps(void) const {
  return steps_;
}

void inline box::set_steps(const int &STEPS) {
  steps_ = STEPS;
}

int inline box::testparticle(void) const {
  return testparticle_;
}

void inline box::set_testparticle(const int &TESTPARTICLE) {
  testparticle_ = TESTPARTICLE;
}

int inline box::update(void) const {
  return update_;
}

void inline box::set_update(const int &UPDATE) {
  update_ = UPDATE;
}

float inline box::temperature(void) const {
  return temperature_;
}

void inline box::set_temperature(const float &T) {
  temperature_ = T;
}

float inline box::cross_section(void) const {
  return cross_section_;
}

void inline box::set_cross_section(const float &sigma) {
  cross_section_ = sigma;
}

float inline box::energy_initial(void) const {
  return energy_initial_;
}

void inline box::set_energy_initial(const float &energy) {
  energy_initial_ = energy;
}

float inline box::number_density_initial(void) const {
  return number_density_initial_;
}

void inline box::set_number_density_inital(const float &number_density) {
  number_density_initial_ = number_density;
}

int64_t inline box::seed(void) const {
  return seed_;
}

void inline box::set_seed(const int64_t &randomseed) {
  seed_ = randomseed;
}

timespec inline box::time_start(void) const {
  return time_start_;
}

timespec inline box::set_timer_start(void) {
  timespec time;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time);
  return time;
}

/* convenience routines for periodic grids */
/* sm - site minus one */
inline int sm(int s, int num) {
  if (s != 0)
    return s - 1;
  else
    return num - 1;
}

/* sp - site plus one */
inline int sp(int s, int num) {
  if (s != num - 1)
    return s + 1;
  else
    return 0;
}

/* support for gcc branch prediction */
#ifdef __GNUC__
#define likely(x)       __builtin_expect((x), 1)
#define unlikely(x)     __builtin_expect((x), 0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

#endif  // SRC_INCLUDE_BOX_H_
