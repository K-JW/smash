/*
 *    Copyright (c) 2012-2013
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 */
#ifndef SRC_INCLUDE_PARTICLES_H_
#define SRC_INCLUDE_PARTICLES_H_

#include <iterator>
#include <map>
#include <string>
#include <utility>

#include "include/decaymodes.h"
#include "include/particledata.h"
#include "include/particletype.h"

namespace Smash {

/**
 * The Particles class abstracts the storage and manipulation of particles.
 *
 * There should be only one Particles object per Experiment. This object stores
 * the data about all existing particles in the experiment (ParticleData). Each
 * particle is of a predefined type (ParticleType). The types are immutable and
 * need not be copied into each ParticleData object. The type information can
 * easily be retrieved via the PDG code.
 *
 * \note
 * The Particles object cannot be copied, because it does not make sense
 * semantically. Move semantics make sense and can be implemented when needed.
 */
class Particles {
  /**
   * Adapter class for making iterations over particle data and types easier to
   * use.
   *
   * This is an implementation detail and need not be understood for use of the
   * Particles class.
   */
  template <typename T>
  class MapIterationAdapter {
    /// the type T::begin() returns
    using map_iterator = decltype(std::declval<T &>().begin());
    /// the value type of T (with const added if T is const)
    using mapped_type = typename std::conditional<
        std::is_const<T>::value, const typename T::mapped_type,
        typename T::mapped_type>::type;

   public:
    MapIterationAdapter(T *map) : map_(map) {}  // NOLINT(runtime/explicit)

    /**
     * Adapter class that modifies the normal map iterator to return only the
     * value instead of the key/value pair.
     */
    class iterator : public map_iterator {
     public:
      iterator(map_iterator it)  // NOLINT(runtime/explicit)
          : map_iterator(it) {}

      /**
       * overwritten dereference operator to return only the value instead of
       * the
       * key/value pair.
       */
      mapped_type &operator*() { return map_iterator::operator*().second; }
      /// const overload of the above
      const mapped_type &operator*() const {
        return map_iterator::operator*().second;
      }
    };

    /// returns an adapted iterator to the begin iterator of the map
    iterator begin() const { return map_->begin(); }
    /// returns an adapted iterator to the end iterator of the map
    iterator end() const { return map_->end(); }

    // const_iterator cbegin() const { return map_.cbegin(); }
    // const_iterator cend() const { return map_.cend(); }

   private:
    /// points to the map the adapter class wraps; needed for calls to begin/end
    T *map_;
  };

  using ParticleDataMap = std::map<int, ParticleData>;
  using ParticleTypeMap = std::map<int, ParticleType>;
  using DecayModesMap = std::map<int, DecayModes>;

 public:
  /// Use improbable values for default constructor
  Particles(const std::string &particles, const std::string &decaymodes) {
    load_particle_types(particles);
    load_decaymodes(decaymodes);
  }

  /// Cannot be copied
  Particles(const Particles &) = delete;
  /// Cannot be copied
  Particles &operator=(const Particles &) = delete;

  /**
   * Use the returned object for iterating over all ParticleData objects.
   *
   * You can use this object for use with range-based for:
   * \code
   * for (ParticleData &data : particles->data()) {
   *   ...
   * }
   * \endcode
   *
   * \returns Opaque object that provides begin/end functions for iteration of
   * all ParticleData objects.
   */
  MapIterationAdapter<ParticleDataMap> data() { return &data_; }
  /// const overload of the above
  MapIterationAdapter<const ParticleDataMap> data() const { return &data_; }

  /**
   * Use returned object for iterating over all ParticleType objects.
   *
   * You can use this object for use with range-based for:
   * \code
   * for (const ParticleType &type : particles->types()) {
   *   ...
   * }
   * \endcode
   *
   * \returns Opaque object that provides begin/end functions for iteration of
   * all ParticleType objects.
   */
  MapIterationAdapter<const ParticleTypeMap> types() const { return &types_; }

  // Iterating the DecayModes map would be easy, but not useful.
  // MapIterationAdapter<const DecayModesMap> decay_modes() const { return
  // &decay_modes_; }

  /// Return the specific data of a particle according to its id
  inline const ParticleData &data(int id) const;
  /// Return the specific datapointer of a particle according to its id
  inline ParticleData * data_pointer(int id);
  /**
   * Return the type of a specific particle given its id
   *
   * \warning This function has a high cost. Prefer to call \ref particle_type
   *          instead.
   */
  inline const ParticleType &type(int id) const;
  /// Return the type for a specific pdgcode
  inline const ParticleType &particle_type(int pdgcode) const;
  /// Return decay modes of this particle type
  inline const DecayModes &decay_modes(int pdg) const;
  /// return the highest used id
  inline int id_max(void) const;
  /// inserts a new particle and returns its id
  inline int add_data(const ParticleData &particle_data);
  /// add a range of particles
  inline void create(size_t number, int pdg);
  /* add one particle and return pointer to it */
  inline ParticleData& create(const int pdg);
  /// remove a specific particle
  inline void remove(int id);
  /// size() of the ParticleData map
  inline size_t size(void) const;
  /// empty() check of the ParticleData map
  inline bool empty(void) const;
  /// check the existence of an element in the ParticleData map
  inline size_t count(int id) const;
  /// size() check of the ParticleType map
  inline size_t types_size(void) const;
  /// empty() check of the ParticleType map
  inline bool types_empty(void) const;
  /// return time of the computational frame
  inline double time(void) const;

  /** Check whether a particle type with the given \p pdg code is known.
   *
   * \param pdg The pdg code of the particle in question.
   * \return \c true  If a ParticleType of the given \p pdg code is registered.
   * \return \c false otherwise.
   */
  bool is_particle_type_registered(int pdgcode) const {
    return types_.count(pdgcode) == 1;
  }

  /* iterators */
  inline std::map<int, ParticleData>::iterator begin(void);
  inline std::map<int, ParticleData>::iterator end(void);
  inline std::map<int, ParticleData>::const_iterator begin() const { return data_.begin(); }
  inline std::map<int, ParticleData>::const_iterator end() const { return data_.end(); }

  inline std::map<int, ParticleData>::const_iterator cbegin(void) const;
  inline std::map<int, ParticleData>::const_iterator cend(void) const;
  inline std::map<int, ParticleType>::const_iterator types_cbegin(void) const;
  inline std::map<int, ParticleType>::const_iterator types_cend(void) const;

  struct LoadFailure : public std::runtime_error {
    using std::runtime_error::runtime_error;
  };
  struct ReferencedParticleNotFound : public LoadFailure {
    using LoadFailure::LoadFailure;
  };
  struct MissingDecays : public LoadFailure {
    using LoadFailure::LoadFailure;
  };
  struct ParseError : public LoadFailure {
    using LoadFailure::LoadFailure;
  };

  /** Reset member data to the state the object had when the constructor
   * returned.
   */
  void reset();

 private:
  void load_particle_types(const std::string &input);
  void load_decaymodes(const std::string &input);
  /// inserts a new particle type
  inline void add_type(const ParticleType &particle_type, int pdg);
  /// adds decay modes for a particle type
  inline void add_decaymodes(const DecayModes &new_decay_modes, int pdg);

  /// Highest id of a given particle
  int id_max_ = -1;
  /**
   * dynamic data of the particles a map between its id and data
   *
   * A map structure is used as particles decay and hence this
   * a swiss cheese over the runtime of SMASH. Also we want direct
   * lookup of the corresponding particle id with its data.
   */
  ParticleDataMap data_;
  /**
   * a map between pdg and correspoding static data of the particles
   *
   * PDG ids are scattered in a large range of values, hence it is a map.
   */
  ParticleTypeMap types_;
  /// a map between pdg and corresponding decay modes
  DecayModesMap all_decay_modes_;
};

/* return the data of a specific particle */
inline const ParticleData &Particles::data(int particle_id) const {
  return data_.at(particle_id);
}

/* return the pointer to the data of a specific particle */
inline ParticleData* Particles::data_pointer(int particle_id) {
  return &data_.at(particle_id);
}

/* return the type of a specific particle */
inline const ParticleType &Particles::type(int particle_id) const {
  return types_.at(data_.at(particle_id).pdgcode());
}

/* return a specific type */
inline const ParticleType &Particles::particle_type(int pdgcode) const {
  return types_.at(pdgcode);
}

/* return the decay modes of specific type */
inline const DecayModes &Particles::decay_modes(int pdg) const {
  return all_decay_modes_.at(pdg);
}

/* add a new particle data and return the id of the new particle */
inline int Particles::add_data(ParticleData const &particle_data) {
  id_max_++;
  data_.insert(std::pair<int, ParticleData>(id_max_, particle_data));
  data_.at(id_max_).set_id(id_max_);
  return id_max_;
}

/* create a bunch of particles */
inline void Particles::create(size_t number, int pdgcode) {
  ParticleData particle;
  /* fixed pdgcode and no collision yet */
  particle.set_pdgcode(pdgcode);
  particle.set_collision(-1, 0, -1);
  for (size_t i = 0; i < number; i++) {
    id_max_++;
    particle.set_id(id_max_);
    data_.insert(std::pair<int, ParticleData>(id_max_, particle));
  }
}

/* create a bunch of particles */
inline ParticleData& Particles::create(int pdgcode) {
  ParticleData particle;
  /* fixed pdgcode and no collision yet */
  particle.set_pdgcode(pdgcode);
  particle.set_collision(-1, 0, -1);
  id_max_++;
  particle.set_id(id_max_);
  data_.insert(std::pair<int, ParticleData>(id_max_, particle));
  return data_[id_max_];
}

/* return the highest used id */
inline int Particles::id_max() const {
  return id_max_;
}

/* remove a particle */
inline void Particles::remove(int id) {
  data_.erase(id);
}

/* total number of particles */
inline size_t Particles::size() const {
  return data_.size();
}

/* check if we have particles */
inline bool Particles::empty() const {
  return data_.empty();
}

/* total number particle types */
inline size_t Particles::types_size() const {
  return types_.size();
}

/* check if we have particle types */
inline bool Particles::types_empty() const {
  return types_.empty();
}

inline std::map<int, ParticleData>::iterator Particles::begin() {
  return data_.begin();
}

inline std::map<int, ParticleData>::iterator Particles::end() {
  return data_.end();
}

inline std::map<int, ParticleData>::const_iterator Particles::cbegin() const {
  return data_.begin();
}

inline std::map<int, ParticleData>::const_iterator Particles::cend() const {
  return data_.end();
}

/* we only provide const ParticleType iterators as this shouldn't change */
inline std::map<int, ParticleType>::const_iterator Particles::types_cbegin()
  const {
  return types_.begin();
}

inline std::map<int, ParticleType>::const_iterator Particles::types_cend()
  const {
  return types_.end();
}

/* check the existence of an element in the ParticleData map */
inline size_t Particles::count(int id) const {
  return data_.count(id);
}

/* return computation time which is reduced by the start up time */
inline double Particles::time() const {
  return data_.begin()->second.position().x0() - 1.0;
}

/* boost_CM - boost to center of momentum */
void boost_CM(ParticleData *particle1, ParticleData *particle2,
  FourVector *velocity);

/* boost_from_CM - boost back from center of momentum */
void boost_back_CM(ParticleData *particle1, ParticleData *particle2,
  FourVector *velocity_orig);

/* particle_distance - measure distance between two particles */
double particle_distance(ParticleData *particle_orig1,
  ParticleData *particle_orig2);

/* time_collision - measure collision time of two particles */
double collision_time(const ParticleData &particle1,
  const ParticleData &particle2);

/* momenta_exchange - soft scattering */
void momenta_exchange(ParticleData *particle1, ParticleData *particle2);

/* Sample final state momenta in general 2->2 process */
void sample_cms_momenta(ParticleData *particle1, ParticleData *particle2,
  const double cms_energy, const double mass1, const double mass2);

}  // namespace Smash

#endif  // SRC_INCLUDE_PARTICLES_H_
