/*
 *
 *    Copyright (c) 2014-2015
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#ifndef SRC_INCLUDE_OUTPUTINTERFACE_H_
#define SRC_INCLUDE_OUTPUTINTERFACE_H_

#include "forwarddeclarations.h"
#include "density.h"
#include "energymomentumtensor.h"
#include "lattice.h"
#include "macros.h"

namespace Smash {

/// Represents thermodynamic quantities that can be printed out
enum class ThermodynamicQuantity : char {
  Density,
  Tmn,
  TmnLandau,
  LandauVelocity,
};

/**
 * \ingroup output
 *
 * \brief Abstraction of generic output
 * Any output should inherit this class. It provides virtual methods that will
 * be called at predefined moments:
 * 1) At event start and event end
 * 2) After every N'th timestep
 * 3) At each interaction
 */
class OutputInterface {
 public:
  virtual ~OutputInterface() = default;

  /**
   * Output launched at event start after initialization, when particles are
   * generated but not yet propagated.
   * \param particles List of particles.
   * \param event_number Number of the current event.
   */
  virtual void at_eventstart(const Particles &particles,
                             const int event_number) = 0;

  /**
   * Output launched at event end. Event end is determined by maximal timestep
   * option.
   * \param particles List of particles.
   * \param event_number Number of the current event.
   */
  virtual void at_eventend(const Particles &particles,
                           const int event_number) = 0;

  /**
   * Called whenever an action modified one or more particles.
   *
   * \param action The action object, containing the initial and final state etc.
   * \param density The density at the interaction point.
   *
   * \fpPrecision Why \c double?
   */
  virtual void at_interaction(const Action &action, const double density) {
    SMASH_UNUSED(action);
    SMASH_UNUSED(density);
  }

  /**
   * Output launched after every N'th timestep. N is controlled by an option.
   * \param particles List of particles.
   * \param clock System clock.
   * \param dens_param Parameters for density calculation.
   */
  virtual void at_intermediate_time(const Particles &particles,
                                    const Clock &clock,
                                    const DensityParameters &dens_param) {
    SMASH_UNUSED(particles);
    SMASH_UNUSED(clock);
    SMASH_UNUSED(dens_param);
  }

  /**
   * Output to write thermodynamics from the lattice.
   * \param tq Thermodynamic quantity to be written, used for file name etc.
   * \param dt Type of density, i.e. which particles to take into account.
   * \param lattice Lattice of tabulated values.
   */
  virtual void thermodynamics_output(const ThermodynamicQuantity tq,
                            const DensityType dt,
                            RectangularLattice<DensityOnLattice> &lattice) {
    SMASH_UNUSED(tq);
    SMASH_UNUSED(dt);
    SMASH_UNUSED(lattice);
  }

  /**
   * Output to write energy-momentum tensor and related quantities from the lattice.
   * \param tq Thermodynamic quantity to be written: Tmn, Tmn_Landau, v_Landau
   * \param dt Type of density, i.e. which particles to take into account.
   * \param lattice Lattice of tabulated values.
   */
  virtual void thermodynamics_output(const ThermodynamicQuantity tq,
                            const DensityType dt,
                            RectangularLattice<EnergyMomentumTensor> &lattice) {
    SMASH_UNUSED(tq);
    SMASH_UNUSED(dt);
    SMASH_UNUSED(lattice);
  }
};

}  // namespace Smash

#endif  // SRC_INCLUDE_OUTPUTINTERFACE_H_
