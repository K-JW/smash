/*
 *
 *    Copyright (c) 2012-2013
 *      Hannah Petersen <petersen@fias.uni-frankfurt.de>
 *      
 *    GNU General Public License (GPLv3)
 *
 */

#include <list>

#include "include/Experiment.h"
#include "include/BoundaryConditions.h"
#include "include/Parameters.h"
#include "include/param-reader.h"
#include "include/Box.h"
//#include "include/Sphere.h"

std::unique_ptr<Experiment> Experiment::create(const int &modus)
{
  typedef std::unique_ptr<Experiment> ExperimentPointer;
  if (modus == 1) {
    return ExperimentPointer{new ExperimentImplementation<BoxBoundaryConditions>};
  }
//  else if (modus == 2) {
//    return ExperimentPointer{new ExperimentImplementation<SphereBoundaryConditions>};
//  }
    else {
    throw std::string("Invalid boundaryCondition requested from Experiment::create.");
  }
}



template <typename BoundaryConditions>
void ExperimentImplementation<BoundaryConditions>::config(char *path)
{
    std::list<Parameters> configuration;
    process_config(&configuration, path);
    BoundaryConditions bc;
    bc.assign_params_general(&configuration);
    bc.assign_params_specific(&configuration);
    
}

