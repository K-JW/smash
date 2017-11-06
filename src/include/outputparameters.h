/*
 *    Copyright (c) 2017-
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 */
#ifndef SRC_INCLUDE_OUTPUTPARAMETERS_H_
#define SRC_INCLUDE_OUTPUTPARAMETERS_H_

#include <set>

#include "configuration.h"
#include "density.h"
#include "forwarddeclarations.h"
#include "logging.h"

namespace Smash {

/**
 * Helper structure for Experiment to hold output options and parameters.
 * Experiment has one member of this struct.
 */
struct OutputParameters {
  /// Default constructor, useful for tests
  OutputParameters()
      : td_position(ThreeVector()),
        td_dens_type(DensityType::None),
        td_rho_eckart(false),
        td_tmn(false),
        td_tmn_landau(false),
        td_v_landau(false),
        td_smearing(true),
        part_extended(false),
        part_only_final(true),
        coll_extended(false),
        coll_printstartend(false) {}

  /// Constructor from configuration
  explicit OutputParameters(Configuration&& conf) : OutputParameters() {
    const auto& log = logger<LogArea::Experiment>();
    log.trace(source_location);

    if (conf.has_value({"Thermodynamics"})) {
      auto subcon = conf["Thermodynamics"];
      if (subcon.has_value({"Position"})) {
        const std::array<double, 3> a = subcon.take({"Position"});
        td_position = ThreeVector(a[0], a[1], a[2]);
      }
      std::set<ThermodynamicQuantity> quan = subcon.take({"Quantities"});
      td_rho_eckart = (quan.count(ThermodynamicQuantity::EckartDensity) > 0);
      td_tmn = (quan.count(ThermodynamicQuantity::Tmn) > 0);
      td_tmn_landau = (quan.count(ThermodynamicQuantity::TmnLandau) > 0);
      td_v_landau = (quan.count(ThermodynamicQuantity::LandauVelocity) > 0);
      td_dens_type = subcon.take({"Type"}, DensityType::None);
      td_smearing = subcon.take({"Smearing"}, true);
    }

    if (conf.has_value({"Particles"})) {
      part_extended = conf.take({"Particles", "Extended"}, false);
      part_only_final = conf.take({"Particles", "Only_Final"}, true);
    }

    if (conf.has_value({"Collisions"})) {
      coll_extended = conf.take({"Collisions", "Extended"}, false);
      coll_printstartend = conf.take({"Collisions", "Print_Start_End"}, false);
    }
  }
  /// Point, where thermodynamic quantities are calculated
  ThreeVector td_position;
  /// Type (e.g., baryon/pion/hadron) of thermodynamic quantity
  DensityType td_dens_type;
  /// Print out Eckart rest frame density of type td_dens_type or not?
  bool td_rho_eckart;
  /// Print out energy-momentum tensor of type td_dens_type or not?
  bool td_tmn;
  /** Print out energy-momentum tensor in Landau rest frame
   * (of type td_dens_type) or not? */
  bool td_tmn_landau;
  /// Print out Landau velocity of type td_dens_type or not?
  bool td_v_landau;
  /** Whether smearing is on or off; WARNING : if smearing is off,
      then final result is in GeV instead of GeV/fm3 */
  bool td_smearing;

  /// Extended format for particles output
  bool part_extended;
  /// Print only final particles in event
  bool part_only_final;

  /// Extended format for collisions output
  bool coll_extended;
  /// Print initial and final particles in event into collision output
  bool coll_printstartend;
};

}  // namespace Smash

#endif  // SRC_INCLUDE_OUTPUTPARAMETERS_H_
