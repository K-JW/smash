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
#include "../include/Modus.h"
#include "../include/Parameters.h"
#include "../include/Particles.h"

class Experiment {
 public:
    Experiment() {}
    /* Virtual base class destructor
     * to avoid undefined behavior when destroying derived objects
     */
    virtual ~Experiment() {}
    static std::unique_ptr<Experiment> create(std::string modus_chooser);
    virtual void configure(std::list<Parameters> configuration) = 0;
    virtual void commandline_arg(int steps) = 0;
    virtual void initialize(char *path) = 0;
    virtual void run_time_evolution() = 0;
    virtual void end() = 0;
};

template <typename Modus> class ExperimentImplementation : public Experiment {
 public:
    ExperimentImplementation(): particles_(nullptr), cross_sections_(nullptr) {}
    virtual void configure(std::list<Parameters> configuration);
    virtual void commandline_arg(int steps);
    virtual void initialize(char *path);
    virtual void run_time_evolution();
    virtual void end();

 private:
    Modus bc_;
    Particles *particles_;
    CrossSections *cross_sections_;
};

#endif  // SRC_INCLUDE_EXPERIMENT_H_
