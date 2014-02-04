/*
 *    Copyright (c) 2013-2014
 *      SMASH Team
 * 
 *    GNU General Public License (GPLv3 or later)
 */
#ifndef SRC_INCLUDE_EXPERIMENT_H_
#define SRC_INCLUDE_EXPERIMENT_H_

#include <memory>
#include <list>
#include <string>

#include "../include/CrossSections.h"
#include "../include/ModusDefault.h"
#include "../include/Parameters.h"
#include "../include/Particles.h"

#include "outputroutines.h"

class ExperimentBase {
 public:
    ExperimentBase() = default;
    /* Virtual base class destructor
     * to avoid undefined behavior when destroying derived objects
     */
    virtual ~ExperimentBase() {}

    virtual void configure(std::list<Parameters> configuration) = 0;
    virtual void commandline_arg(int steps) = 0;

    static std::unique_ptr<ExperimentBase> create(std::string modus_chooser, int nevents);

    virtual void run(std::string path) = 0;
};

struct ExperimentParameters {
  /* number of test particle */
  int testparticles = 1;
  /* temporal time step */
  float eps = 0.001f;
  /* cross section of the elastic scattering */
  float cross_section = 10.0f;
};

template <typename Modus> class Experiment : public ExperimentBase {
 public:
  Experiment(int nevents)
      : particles_(nullptr), cross_sections_(nullptr), nevents_(nevents) {}

    virtual void configure(std::list<Parameters> configuration) override;
    virtual void commandline_arg(int steps) override;

    virtual void run(std::string path) override;

 private:
    void initialize(const char *path);
    void run_time_evolution();
    void end();

    void assign_params(std::list<Parameters> *configuration);

    void print_startup();

    float energy_total(Particles *particles);

    inline timespec set_timer_start();

    Modus modus_;
    Particles *particles_;
    CrossSections *cross_sections_;

    int nevents_;

    ExperimentParameters parameters_;

    /* number of steps */
    int steps_ = 10000;
    /* number of steps before giving measurables */
    int output_interval = 100;
    /* initial seed for random generator */
    int64_t seed = 1;
    /* initial total energy of the system */
    float energy_initial = 0.f;
    /* starting time of the simulation */
    timespec time_start = set_timer_start();
};

#endif  // SRC_INCLUDE_EXPERIMENT_H_
