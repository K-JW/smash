/*
 *    Copyright (c) 2012-2017
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 */
#ifndef SRC_INCLUDE_BOXMODUS_H_
#define SRC_INCLUDE_BOXMODUS_H_

#include <map>
#include <memory>

#include "forwarddeclarations.h"
#include "modusdefault.h"

namespace smash {

/**
 * \ingroup modus
 * BoxModus: Provides a modus for infinite matter calculations
 *
 * Matter is confined in a cubical box. Depending on the initial
 * condition, particles are either reflected on the boundaries
 * (not implemented now) or inserted on opposite positions.
 *
 * To use this modus, choose
    Modus:         Box
 * \code
 * General:
 *      Modus: Box
 * \endcode
 * in the configuration file.
 *
 * Options for BoxModus go in the "Modi"→"Box" section of the
 * configuration:
 *
 * \code
 * Modi:
 *      Box:
 *              # definitions here
 * \endcode
 *
 * The following configuration options are understood:
 * \ref input_modi_box_
 */
class BoxModus : public ModusDefault {
 public:
  /**
   * Gathers all configuration variables for the Box.
   *
   * \param[in] modus_config The configuration object that sets all
   *                         initial conditions of the experiment.
   * \param[in] parameters Unused, but necessary because of templated
   *                       initialization (see ColliderModus)
   */
  explicit BoxModus(Configuration modus_config,
                    const ExperimentParameters &parameters);

  /**
   * Generates initial state of the particles in the system according to
   * specified parameters: number of particles of each species, momentum
   * and coordinate space distributions. Susbsequently makes the total
   * 3-momentum 0.
   *
   * \param[out] particles An empty list that gets filled up by this function
   * \param[in] parameters The initialization parameters of the box
   * \return The starting time of the simulation
   */
  double initial_conditions(Particles *particles,
                            const ExperimentParameters &parameters);

  /**
   * Enforces that all particles are inside the box
   *
   * \param[in] particles particles to check their position and possibly
   *            move it
   * \param[in] output_list output objects
   * \return The number of particles that were put back into the box
   *
   * In BoxModus if a particle crosses the wall of the box, it is
   * inserted from the opposite side. Wall crossings are written to
   * collision output: this is where OutputsList is used.
   */
  int impose_boundary_conditions(Particles *particles,
                                 const OutputsList &output_list = {});

  /// \copydoc smash::ModusDefault::create_grid
  Grid<GridOptions::PeriodicBoundaries> create_grid(
      const Particles &particles, double min_cell_length,
      CellSizeStrategy strategy = CellSizeStrategy::Optimal) const {
    return {{{0, 0, 0}, {length_, length_, length_}},
            particles,
            min_cell_length,
            strategy};
  }

  /// \copydoc smash::ModusDefault::create_grandcan_thermalizer
  std::unique_ptr<GrandCanThermalizer> create_grandcan_thermalizer(
      Configuration &conf) const {
    const std::array<double, 3> lat_size = {length_, length_, length_};
    const std::array<double, 3> origin = {0., 0., 0.};
    const bool periodicity = true;
    return make_unique<GrandCanThermalizer>(conf, lat_size, origin,
                                            periodicity);
  }

  /// \copydoc smash::ModusDefault::max_timestep()
  double max_timestep(double max_transverse_distance_sqr) const {
    return 0.5 * std::sqrt(length_ * length_ - max_transverse_distance_sqr);
  }

  double length() const { return length_; }

 private:
  const BoxInitialCondition initial_condition_;
  /// length of the cube's edge in fm/c
  const double length_;
  /// Temperature of the Box in GeV
  const double temperature_;
  /// initial time of the box
  const double start_time_ = 0.;
  /**
   * whether to use a thermal initialization for all particles
   * instead of specific numbers
   */
  const bool use_thermal_ = false;
  /**
   * baryon chemical potential for thermal box;
   * only used if use_thermal_ is true
   */
  const double mub_;
  /**
   * strange chemical potential for thermal box;
   * only used if use_thermal_ is true
   */
  const double mus_;
  /// particle multiplicities at initialization
  const std::map<PdgCode, int> init_multipl_;

  /**\ingroup logging
   * console output on startup of box specific parameters;
   * writes the initial state for the Box to the output stream.
   *
   * \param[in] out The ostream into which to output
   * \param[in] m The BoxModus object to write into out
   */
  friend std::ostream &operator<<(std::ostream &out, const BoxModus &m);
};

}  // namespace smash

#endif  // SRC_INCLUDE_BOXMODUS_H_
