/*
 *
 *    Copyright (c) 2014-2015
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "include/binaryoutputcollisions.h"

#include <string>

#include <boost/filesystem.hpp>

#include "include/action.h"
#include "include/clock.h"
#include "include/config.h"
#include "include/particles.h"

namespace Smash {

BinaryOutputCollisions::BinaryOutputCollisions(const bf::path &path,
                                               std::string name,
                                               const OutputParameters &out_par)
    : BinaryOutputBase(
          std::fopen(
              (path /
               ((name == "Collisions" ? "collisions_binary" : name) + ".bin"))
                  .native().c_str(), "wb"), name, out_par.coll_extended),
      print_start_end_(out_par.coll_printstartend) {
  /*!\Userguide
   * \page input_binary_collisions Binary_collisions
   * Saves information about every collision, decay and box
   * wall crossing in a binary format. Optionally initial and
   * final particle configurations can be written out.
   *
   * \key Enable (bool, optional, default = false):\n
   * true - binary collision output enabled\n
   * false - no binary collision output
   *
   * \key Print_Start_End (bool, optional, default = false): \n
   * false - only information about collisions, decays and
   * box wall crossings during the whole evolution \n
   * true - initial and final configuration are written in addition
   *
   * \key Extended (bool, optional, default = false): \n
   * true - additional information is written out for each particle
   * false - default information for each particle
   *
   * Detailed specification of the binary format can be found here:
   * \ref format_binary_
   */
}

/*!\Userguide
 * \page format_binary_ Binary format
 *
 * Collisions output
 * -----------------
 * Written to \c collisions_binary.bin file. Contains interactions
 * (collisions, decays, box wall crossings) and optionally initial
 * and final configuration. Interactions are written in comp. frame
 * time-ordered fashion, in 'i' blocks. Initial and final states
 * are written as 'p' blocks. For options of this output see
 * \ref input_general_, \ref input_binary_collisions.
 *
 * See also \ref collisions_output_in_box_modus_.
 **/

void BinaryOutputCollisions::at_eventstart(const Particles &particles,
                                           const int /*event_number*/) {
  char pchar = 'p';
  if (print_start_end_) {
    std::fwrite(&pchar, sizeof(char), 1, file_.get());
    write(particles.size());
    write(particles);
  }
}

void BinaryOutputCollisions::at_eventend(const Particles &particles,
                                         const int event_number,
                                         double impact_parameter) {
  char pchar = 'p';
  if (print_start_end_) {
    std::fwrite(&pchar, sizeof(char), 1, file_.get());
    write(particles.size());
    write(particles);
  }

  // Event end line
  char fchar = 'f';
  std::fwrite(&fchar, sizeof(char), 1, file_.get());
  write(event_number);
  write(impact_parameter);

  /* Flush to disk */
  std::fflush(file_.get());
}

void BinaryOutputCollisions::at_interaction(const Action &action,
                                            const double density) {
  char ichar = 'i';
  std::fwrite(&ichar, sizeof(char), 1, file_.get());
  write(action.incoming_particles().size());
  write(action.outgoing_particles().size());
  std::fwrite(&density, sizeof(double), 1, file_.get());
  const double weight = action.raw_weight_value();
  std::fwrite(&weight, sizeof(double), 1, file_.get());
  const double partial_weight = action.partial_weight();
  std::fwrite(&partial_weight, sizeof(double), 1, file_.get());
  const auto type = static_cast<uint32_t>(action.get_type());
  std::fwrite(&type, sizeof(uint32_t), 1, file_.get());
  write(action.incoming_particles());
  write(action.outgoing_particles());
}

BinaryOutputBase::BinaryOutputBase(FILE *f, std::string name, bool extended)
    : OutputInterface(name), file_{f}, extended_(extended) {
  std::fwrite("SMSH", 4, 1, file_.get());  // magic number
  write(format_version_);                  // file format version number
  std::uint16_t format_variant = static_cast<uint16_t>(extended_);
  write(format_variant);
  write(VERSION_MAJOR);  // SMASH version
}

// write functions:
void BinaryOutputBase::write(const std::string &s) {
  const auto size = boost::numeric_cast<uint32_t>(s.size());
  std::fwrite(&size, sizeof(std::uint32_t), 1, file_.get());
  std::fwrite(s.c_str(), s.size(), 1, file_.get());
}

void BinaryOutputBase::write(const double x) {
  std::fwrite(&x, sizeof(x), 1, file_.get());
}

void BinaryOutputBase::write(const FourVector &v) {
  std::fwrite(v.begin(), sizeof(*v.begin()), 4, file_.get());
}

void BinaryOutputBase::write(const Particles &particles) {
  for (const auto &p : particles) {
    write_particledata(p);
  }
}

void BinaryOutputBase::write(const ParticleList &particles) {
  for (const auto &p : particles) {
    write_particledata(p);
  }
}

void BinaryOutputBase::write_particledata(const ParticleData &p) {
  write(p.position());
  double mass = p.effective_mass();
  std::fwrite(&mass, sizeof(mass), 1, file_.get());
  write(p.momentum());
  write(p.pdgcode().get_decimal());
  write(p.id());
  if (extended_) {
    write(p.get_history().collisions_per_particle);
    write(p.formation_time());
    write(p.cross_section_scaling_factor());
    write(p.get_history().id_process);
    write(static_cast<uint32_t>((p.get_history().process_type)));
    write(p.get_history().time_of_origin);
    write(p.get_history().p1.get_decimal());
    write(p.get_history().p2.get_decimal());
  }
}

}  // namespace Smash
