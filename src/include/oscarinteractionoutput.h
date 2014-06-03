/*
 *
 *    Copyright (c) 2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#ifndef SRC_INCLUDE_OSCARINTERACTIONOUTPUT_H_
#define SRC_INCLUDE_OSCARINTERACTIONOUTPUT_H_

#include "outputinterface.h"

#include "filedeleter.h"
#include "forwarddeclarations.h"

namespace Smash {

class OscarInteractionOutput : public OutputInterface {
 public:
  OscarInteractionOutput(bf::path path);
  ~OscarInteractionOutput();

  /// writes the initial particle information of an event
  void at_eventstart(const Particles &particles,
                     const int event_number) override;

  /// writes the final particle information of an event
  void at_eventend(const Particles &particles, const int event_number) override;

  /**
   * Write a prefix line and a line per particle to OSCAR output.
   */
  void write_interaction(const ParticleList &incoming_particles,
                         const ParticleList &outgoing_particles) override;

  void after_Nth_timestep(const Particles &particles, const int event_number,
                          const Clock &clock) override;

 private:
  void write(const Particles &particles);

  FilePtr file_;
};
}  // namespace Smash

#endif  // SRC_INCLUDE_OSCARINTERACTIONOUTPUT_H_
