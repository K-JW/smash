/*
 *    Copyright (c) 2012-2013
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 */
#ifndef SRC_INCLUDE_PARTICLETYPE_H_
#define SRC_INCLUDE_PARTICLETYPE_H_

#include <string>

namespace Smash {

/**
 * Particle type contains the static properties of a particle
 *
 * SMASH reads in first the list of particles with their properties
 * and they don't change from this time on.
 * They are looked up according to their PDG code.
 */
class ParticleType {
 public:
  /// Use improbable values for default constructor
  ParticleType()
      : name_("unknown"),
        mass_(-1),
        width_(-1),
        pdgcode_(-1),
        isospin_(100),
        charge_(100),
        spin_(100) {}
  /// Explicit constructor
  ParticleType(std::string n, float m, float w, int id, int isosp, int ch,
               int sp)
      : name_(n),
        mass_(m),
        width_(w),
        pdgcode_(id),
        isospin_(isosp),
        charge_(ch),
        spin_(sp) {}
  /// set particle type
  inline void set(const std::string &n, float m, float w, int id, int isosp,
                  int ch, int sp);
  /// return particle name
  inline std::string name(void) const;
  /// return particle mass
  inline float mass(void) const;
  /// return particle width
  inline float width(void) const;
  /// return particle pdgcode
  inline int pdgcode(void) const;
  /// Isospin is 2 * particle data book value
  inline int isospin(void) const;
  /// return particle charge
  inline int charge(void) const;
  /// Spin is 2 * particle data book value
  inline int spin(void) const;

 private:
  /// name of the particle
  std::string name_;
  /// mass of the particle
  float mass_;
  /// width of the particle
  float width_;
  /// pdgcode of the particle
  int pdgcode_;
  /// isospin of the particle
  int isospin_;
  /// charge of the particle
  int charge_;
  /// spin of the particle
  int spin_;
};

inline void ParticleType::set(const std::string &NAME, float MASS,
     float WIDTH, int ID, int ISOSPIN, int CHARGE, int SPIN) {
  mass_ = MASS;
  width_ = WIDTH;
  pdgcode_ = ID;
  name_ = NAME;
  isospin_ = ISOSPIN;
  charge_ = CHARGE;
  spin_ = SPIN;
}

inline int ParticleType::charge(void) const {
  return charge_;
}

inline int ParticleType::isospin(void) const {
  return isospin_;
}

inline float ParticleType::mass(void) const {
  return mass_;
}

inline std::string ParticleType::name(void) const {
  return name_;
}

inline int ParticleType::pdgcode(void) const {
  return pdgcode_;
}

inline int ParticleType::spin(void) const {
  return spin_;
}

inline float ParticleType::width(void) const {
  return width_;
}

}  // namespace Smash

#endif  // SRC_INCLUDE_PARTICLETYPE_H_
