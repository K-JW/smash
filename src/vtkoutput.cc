/*
 *
 *    Copyright (c) 2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include <fstream>
#include <memory>

#include "include/clock.h"
#include "include/config.h"
#include "include/filedeleter.h"
#include "include/forwarddeclarations.h"
#include "include/particles.h"
#include "include/vtkoutput.h"

namespace Smash {

VtkOutput::VtkOutput(bf::path path, Configuration&& /*conf*/)
  : base_path_(std::move(path)), vtk_output_counter_(0) {}
/*!\Userguide
 * \page input_vtk Vtk
 *
 * Writes the current particle list at a specific time
 * to separate .vtk files. These fixed moments of time are event start,
 * event end and every next time interval \f$\Delta t\f$, where
 * \f$\Delta t\f$ is controlled by an option.
 * Produced output can be opened by paraview
 * and used for an easy visualization of the simulation.
 *
 * \key Enable (bool, optional, default = false):\n
 * true - VTK output enabled\n
 * false - no VTK output
 *
 * For details on VTK output format see \ref format_vtk.
 */

VtkOutput::~VtkOutput() {
}

  /*!\Userguide
   * \page format_vtk Vtk format
   * In general VTK is a very versatile format, which allows many possible
   * structures. For generic VTK format one can see http://vtk.org. Here only
   * SMASH-specific VTK format is described.
   *
   * SMASH VTK files contain a snapshot of simulation at one moment of time.
   * VTK output files are written at initialization at event start and
   * every period of time \f$ \Delta t \f$, where \f$ \Delta t \f$ is regulated
   * by option (see \ref input_general_). For every new output moment
   * a separate VTK file is written. File names are constructed as follows:
   * pos_ev<event>_tstep<output_number>.vtk.
   *
   * Files contain particle coordinates, momenta and PDG codes. VTK output is
   * known to work with paraview, a free visualization and data analysis
   * software. Files of this format are supposed to be used as a black box
   * and opened with paraview, but at the same time they are
   * human-readable text files.
   **/

void VtkOutput::at_eventstart(const Particles &particles,
                              const int event_number) {
  vtk_output_counter_ = 0;
  vtk_thermodynamics_output_counter_ = 0;
  write(particles, event_number);
  vtk_output_counter_++;
}

void VtkOutput::at_eventend(const Particles &/*particles*/,
                            const int /*event_number*/) {
}

void VtkOutput::at_intermediate_time(const Particles &particles,
                                   const int event_number,
                                   const Clock& /*clock*/) {
  write(particles, event_number);
  vtk_output_counter_++;
}

void VtkOutput::write(const Particles &particles, const int event_number) {
  char filename[32];
  snprintf(filename, sizeof(filename), "pos_ev%05i_tstep%05i.vtk", event_number,
           vtk_output_counter_);
  FilePtr file_{std::fopen((base_path_ / filename).native().c_str(), "w")};

  /* Legacy VTK file format */
  std::fprintf(file_.get(), "# vtk DataFile Version 2.0\n");
  std::fprintf(file_.get(), "Generated from molecular-offset data %s\n",
               VERSION_MAJOR);
  std::fprintf(file_.get(), "ASCII\n");

  /* Unstructured data sets are composed of points, lines, polygons, .. */
  std::fprintf(file_.get(), "DATASET UNSTRUCTURED_GRID\n");
  std::fprintf(file_.get(), "POINTS %zu double\n", particles.size());
  for (const auto &p : particles) {
    std::fprintf(file_.get(), "%g %g %g\n", p.position().x1(),
                 p.position().x2(), p.position().x3());
  }
  std::fprintf(file_.get(), "CELLS %zu %zu\n", particles.size(),
               particles.size() * 2);
  for (size_t point_index = 0; point_index < particles.size(); point_index++) {
    std::fprintf(file_.get(), "1 %zu\n", point_index);
  }
  std::fprintf(file_.get(), "CELL_TYPES %zu\n", particles.size());
  for (size_t point_index = 0; point_index < particles.size(); point_index++) {
    std::fprintf(file_.get(), "1\n");
  }
  std::fprintf(file_.get(), "POINT_DATA %zu\n", particles.size());
  std::fprintf(file_.get(), "SCALARS pdg_codes int 1\n");
  std::fprintf(file_.get(), "LOOKUP_TABLE default\n");
  for (const auto &p : particles) {
    std::fprintf(file_.get(), "%s\n", p.pdgcode().string().c_str());
  }
  std::fprintf(file_.get(), "VECTORS momentum double\n");
  for (const auto &p : particles) {
    std::fprintf(file_.get(), "%g %g %g\n", p.momentum().x1(),
                 p.momentum().x2(), p.momentum().x3());
  }
}

void VtkOutput::thermodynamics_output(const std::string varname,
                               RectangularLattice<DensityOnLattice> &lattice,
                               const int event_number) {
  std::ofstream file;
  char suffix[22];
  snprintf(suffix, sizeof(suffix), "_%05i_tstep%05i.vtk",
                        event_number, vtk_thermodynamics_output_counter_);
  const auto dim = lattice.dimensions();
  const auto cs = lattice.cell_sizes();
  const auto orig = lattice.origin();

  file.open(base_path_.string() + std::string("/") + varname +
                std::string(suffix), std::ios::out);
  file  << "# vtk DataFile Version 2.0\n" << varname << "\n" <<
           "ASCII\n" <<
           "DATASET STRUCTURED_POINTS\n"
           "DIMENSIONS " << dim[0] << " " << dim[1] << " " << dim[2] << "\n" <<
           "SPACING " << cs[0] << " " << cs[1] << " " << cs[2] << "\n" <<
           "ORIGIN " << orig[0] << " " << orig[1] << " " << orig[2] << "\n" <<
           "POINT_DATA " << lattice.size() << "\n" <<
           "SCALARS " << varname << " float 1\n" <<
           "LOOKUP_TABLE default\n";
    file << std::setprecision(3);
    file << std::fixed;
  lattice.iterate_sublattice({0, 0, 0}, dim,
    [&](DensityOnLattice &node, int ix, int, int) {
      file << node.density() << " ";
      if (ix > 0 && ix % dim[0] == 0) {
        file << "\n";
      }
    });
  file.close();
  vtk_thermodynamics_output_counter_++;
}

}  // namespace Smash
