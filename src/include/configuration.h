/*
 *
 *    Copyright (c) 2014-2017
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#ifndef SRC_INCLUDE_CONFIGURATION_H_
#define SRC_INCLUDE_CONFIGURATION_H_

#include <array>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <yaml-cpp/yaml.h>  // NOLINT(build/include_order)

#include "density.h"
#include "forwarddeclarations.h"

namespace YAML {
template <typename T>
struct convert {
  static Node encode(const T &x) { return Node{static_cast<std::string>(x)}; }
  static bool decode(const Node &node, T &x) {
    if (!node.IsScalar()) {
      return false;
    } else {
      x = static_cast<T>(node.Scalar());
      return true;
    }
  }
};
}  // namespace YAML

namespace smash {
/*!\Userguide
 * \page inputoptions Input file Options
 *
 * SMASH is configured via an input file in YAML format. Typically you will
 * start from the supplied `config.yaml` file and modify it according to your
 * needs. If you ever make a mistake there and specify a configuration key that
 * SMASH does not recognize, then on startup it will tell you about the keys it
 * could not make any sense of.
 *
 * \par The available keys are documented on the following pages:
 * \li \subpage input_general_
 * \li \subpage input_logging_
 * \li \subpage input_collision_term_
 * \li \subpage input_modi_
 * \li \subpage input_output_options_
 * \li \subpage input_lattice_
 * \li \subpage input_potentials_
 *
 * \ifnot user
 * \par The relevant functions and classes for input are:
 * \li \ref Configuration
 * \li \ref ExperimentBase::create()
 * \li \ref ColliderModus
 * \li \ref BoxModus
 * \li \ref SphereModus
 * \li \ref ListModus
 * \endif
 */

/*!\Userguide
 * \page input_general_ General
 * This section in the `config.yaml` file contains all general/global
 * configuration options to SMASH.
 *
 * Available Settings
 * ------------------
 */

/**
 * Interface to the SMASH configuration files.
 *
 * The configuration is created from a YAML file and then stores a nested map of
 * maps (normally a tree, but YAML allows it to be cyclic - even though we don't
 * want that feature).
 *
 * Typical usage in SMASH needs to read the value once. In that case use the
 * Configuration::take function:
 * \code
 * double sigma = config.take({"General", "SIGMA"});
 * \endcode
 * Note the curly braces in the function call. It's a std::initializer_list of
 * strings. This allows an arbitrary nesting depth in all via the same function.
 * But as a consequence the keys must all be given as constant strings at
 * compile time.
 *
 * If you need to access configuration values from a run-time string you can use
 * Configuration::operator[]. This returns a Configuration object that
 * references the respective sub-tree.
 *
 * By taking values (instead of just reading), the configuration object should
 * be empty at the end of initialization. If the object is not empty then SMASH
 * will print a warning (using Configuration::unused_values_report). This can be
 * important for the user to discover typos in his configuration file (or
 * command line parameters).
 */
class Configuration {
 public:
  /// \ingroup exception
  /// Thrown when the types in the config file and C++ don't match.
  struct IncorrectTypeInAssignment : public std::runtime_error {
    using std::runtime_error::runtime_error;
  };

  /// \ingroup exception
  /// Thrown for YAML parse errors
  struct ParseError : public std::runtime_error {
    using std::runtime_error::runtime_error;
  };

  /// \ingroup exception
  /// Thrown if the file does not exist
  struct FileDoesNotExist : public std::runtime_error {
    using std::runtime_error::runtime_error;
  };

  /**
   * Return type of Configuration::take that automatically determines the target
   * type.
   *
   * This class is an implementation detail of Configuration and can be ignored
   * by users of Configuration.
   */
  class Value {
    friend class Configuration;

    /// a YAML leaf node
    const YAML::Node node_;
    const char *const key_;

    /** Constructs the Value wrapper from a YAML::Node.
     *
     * \note This constructor must be implicit, otherwise it's impossible to
     * return an rvalue Value object - because the copy constructor is deleted.
     */
    Value(const YAML::Node &n, const char *key) : node_(n), key_(key) {
      if (!(n.IsScalar() || n.IsSequence() || n.IsMap())) {
        std::stringstream err;
        err << "Configuration value for \"" << key
            << "\" is missing or invalid";
        throw std::runtime_error(err.str());
      }
    }

   public:
    /// if you want to copy this you're doing it wrong
    Value(const Value &) = delete;
    /// if you want to copy this you're doing it wrong
    Value &operator=(const Value &) = delete;

    /**
     * Convert the value to the type of the supplied argument.
     *
     * The argument itself is not used other than to determine its type. This
     * function is necessary because in some situations the overload resolution
     * rules lead to the correct conversion becoming hidden. Then you'll see a
     * compiler error with a list of ambiguous constructor calls as candidates.
     * Use this function as a workaround.
     * Example:
     * \code
     * // this doesn't compile:
     * const PdgCode code0(config.take({"key"}));
     * // this compiles (because PdgCode::operator= is not overloaded), but note
     * // that it cannot be used in constructor initializer lists:
     * const PdgCode code1 = config.take({"key"});
     *
     * // Thus, for class member variables use the following pattern:
     * class X {
     *  public:
     *   X() : code_(config.take({"key"}).convert_for(code_)) {}
     *
     *  private:
     *   const PdgCode code_;
     * };
     * \endcode
     */
    template <typename T>
    T convert_for(const T &) const {
      return operator T();
    }

    /**
     * This function determines the type it is assigned to and calls
     * YAML::Node::as<T>() with this type.
     *
     * This makes reading values more convenient than calling as<type>()
     * explicitly.
     */
    template <typename T>
    operator T() const {
      try {
        return node_.as<T>();
      } catch (YAML::TypedBadConversion<T> &e) {
        throw IncorrectTypeInAssignment(
            "The value for key \"" + std::string(key_) +
            "\" cannot be converted to the requested type.");
      }
    }

    template <typename T>
    operator std::vector<T>() const {
      try {
        return node_.as<std::vector<T>>();
      } catch (YAML::TypedBadConversion<T> &e) {
        throw IncorrectTypeInAssignment(
            "One of the values in the sequence for key \"" + std::string(key_) +
            "\" failed to convert to the requested type. E.g. [1 2] is a "
            "sequence of one string \"1 2\" and [1, 2] is a sequence of two "
            "integers. Often there is just a comma missing in the config "
            "file.");
      } catch (YAML::TypedBadConversion<std::vector<T>> &e) {
        throw IncorrectTypeInAssignment(
            "The value for key \"" + std::string(key_) +
            "\" cannot be converted to the requested type. A sequence was "
            "expected but apparently not found.");
      }
    }

    template <typename T, size_t N>
    operator std::array<T, N>() const {
      const std::vector<T> vec = operator std::vector<T>();
      const size_t n_read = vec.size();
      // Alert if size does not match
      if (n_read != N) {
        throw IncorrectTypeInAssignment("Wrong number of values in array \"" +
                                        std::string(key_) + "\". Expected " +
                                        std::to_string(N) +
                                        " values,"
                                        " found " +
                                        std::to_string(n_read) + ".");
      }
      std::array<T, N> arr;
      std::copy_n(vec.begin(), N, arr.begin());
      return arr;
    }

    operator ReactionsBitSet() const {
      const std::vector<std::string> v = operator std::vector<std::string>();
      ReactionsBitSet s;
      for (const auto &x : v) {
        if (x == "All") {
          s.flip();
        } else if (x == "Elastic") {
          s.set(IncludedReactions::Elastic);
        } else if (x == "NN_to_NR") {
          s.set(IncludedReactions::NN_to_NR);
        } else if (x == "NN_to_DR") {
          s.set(IncludedReactions::NN_to_DR);
        } else if (x == "KN_to_KN") {
          s.set(IncludedReactions::KN_to_KN);
        } else if (x == "KN_to_KDelta") {
          s.set(IncludedReactions::KN_to_KDelta);
        } else if (x == "Strangeness_exchange") {
          s.set(IncludedReactions::Strangeness_exchange);
        } else {
          throw IncorrectTypeInAssignment(
              "The value for key \"" + std::string(key_) +
              "\" should be \"All\", \"Elastic\", \"NN_to_NR\", \"NN_to_DR\","
              "\"KN_to_KN\", \"KN_to_KDelta\" or \"strangeness_exchange\","
              " or any combination of these.");
        }
      }
      return s;
    }

    operator std::set<ThermodynamicQuantity>() const {
      const std::vector<std::string> v = operator std::vector<std::string>();
      std::set<ThermodynamicQuantity> s;
      for (const auto &x : v) {
        if (x == "rho_eckart") {
          s.insert(ThermodynamicQuantity::EckartDensity);
        } else if (x == "tmn") {
          s.insert(ThermodynamicQuantity::Tmn);
        } else if (x == "tmn_landau") {
          s.insert(ThermodynamicQuantity::TmnLandau);
        } else if (x == "landau_velocity") {
          s.insert(ThermodynamicQuantity::LandauVelocity);
        } else {
          throw IncorrectTypeInAssignment(
              "The value for key \"" + std::string(key_) +
              "\" should be \"rho_eckart\", \"tmn\""
              ", \"tmn_landau\" or \"landau_velocity\".");
        }
      }
      return s;
    }

    operator CalculationFrame() const {
      const std::string s = operator std::string();
      if (s == "center of velocity") {
        return CalculationFrame::CenterOfVelocity;
      }
      if (s == "center of mass") {
        return CalculationFrame::CenterOfMass;
      }
      if (s == "fixed target") {
        return CalculationFrame::FixedTarget;
      }
      throw IncorrectTypeInAssignment(
          "The value for key \"" + std::string(key_) +
          "\" should be \"center of velocity\" or \"center of mass\" "
          "or \"fixed target\".");
    }

    operator FermiMotion() const {
      const std::string s = operator std::string();
      if (s == "off") {
        return FermiMotion::Off;
      }
      if (s == "on") {
        return FermiMotion::On;
      }
      if (s == "frozen") {
        return FermiMotion::Frozen;
      }
      throw IncorrectTypeInAssignment(
          "The value for key \"" + std::string(key_) +
          "\" should be \"off\" or \"on\" or \"frozen\".");
    }

    operator DensityType() const {
      const std::string s = operator std::string();
      if (s == "hadron") {
        return DensityType::Hadron;
      }
      if (s == "baryon") {
        return DensityType::Baryon;
      }
      if (s == "baryonic isospin") {
        return DensityType::BaryonicIsospin;
      }
      if (s == "pion") {
        return DensityType::Pion;
      }
      if (s == "none") {
        return DensityType::None;
      }
      throw IncorrectTypeInAssignment("The value for key \"" +
                                      std::string(key_) +
                                      "\" should be \"hadron\" or \"baryon\" "
                                      "or \"baryonic isospin\" or \"pion\" "
                                      "or \"none\".");
    }

    operator ExpansionMode() const {
      const std::string s = operator std::string();
      if (s == "NoExpansion") {
        return ExpansionMode::NoExpansion;
      }
      if (s == "MasslessFRW") {
        return ExpansionMode::MasslessFRW;
      }
      if (s == "MassiveFRW") {
        return ExpansionMode::MassiveFRW;
      }
      if (s == "Exponential") {
        return ExpansionMode::Exponential;
      }
      throw IncorrectTypeInAssignment(
          "The value for key \"" + std::string(key_) +
          "\" should be \"NoExpansion\", \"MasslessFRW\"," +
          "\"MassiveFRW\" or \"Exponential\".");
    }

    operator TimeStepMode() const {
      const std::string s = operator std::string();
      if (s == "None") {
        return TimeStepMode::None;
      }
      if (s == "Fixed") {
        return TimeStepMode::Fixed;
      }
      if (s == "Adaptive") {
        return TimeStepMode::Adaptive;
      }
      throw IncorrectTypeInAssignment(
          "The value for key \"" + std::string(key_) +
          "\" should be \"None\", \"Fixed\" or \"Adaptive\".");
    }

    operator BoxInitialCondition() const {
      const std::string s = operator std::string();
      if (s == "thermal momenta") {
        return BoxInitialCondition::ThermalMomenta;
      }
      if (s == "peaked momenta") {
        return BoxInitialCondition::PeakedMomenta;
      }
      throw IncorrectTypeInAssignment(
          "The value for key \"" + std::string(key_) +
          "\" should be \"thermal momenta\" or \"peaked momenta\".");
    }

    operator SphereInitialCondition() const {
      const std::string s = operator std::string();
      if (s == "thermal momenta") {
        return SphereInitialCondition::ThermalMomenta;
      }
      if (s == "IC_ES") {
        return SphereInitialCondition::IC_ES;
      }
      if (s == "IC_1M") {
        return SphereInitialCondition::IC_1M;
      }
      if (s == "IC_2M") {
        return SphereInitialCondition::IC_2M;
      }
      if (s == "IC_Massive") {
        return SphereInitialCondition::IC_Massive;
      }
      throw IncorrectTypeInAssignment(
          "The value for key \"" + std::string(key_) +
          "\" should be \"thermal momenta\", \"IC_ES\", " +
          "\"IC_1M\", \"IC_2M\" or" + "\"IC_Massive\".");
    }

    operator NNbarTreatment() const {
      const std::string s = operator std::string();
      if (s == "no annihilation") {
        return NNbarTreatment::NoAnnihilation;
      }
      if (s == "resonances") {
        return NNbarTreatment::Resonances;
      }
      if (s == "strings") {
        return NNbarTreatment::Strings;
      }
      throw IncorrectTypeInAssignment(
          "The value for key \"" + std::string(key_) + "\" should be " +
          "\"no annihilation\", \"detailed balance\", or \"strings\".");
    }

    operator Sampling() const {
      const std::string s = operator std::string();
      if (s == "quadratic") {
        return Sampling::Quadratic;
      }
      if (s == "custom") {
        return Sampling::Custom;
      }
      if (s == "uniform") {
        return Sampling::Uniform;
      }
      throw IncorrectTypeInAssignment(
          "The value for key \"" + std::string(key_) +
          "\" should be \"quadratic\", \"uniform\" or \"custom\".");
    }

    operator ThermalizationAlgorithm() const {
      const std::string s = operator std::string();
      if (s == "mode sampling") {
        return ThermalizationAlgorithm::ModeSampling;
      }
      if (s == "biased BF") {
        return ThermalizationAlgorithm::BiasedBF;
      }
      if (s == "unbiased BF") {
        return ThermalizationAlgorithm::UnbiasedBF;
      }
      throw IncorrectTypeInAssignment(
          "The value for key \"" + std::string(key_) +
          "\" should be \"mode sampling\", \"biased BF\" or \"unbiased BF\".");
    }
  };

  /**
   * Reads config.yaml from the specified \p path.
   *
   * \param path The directory where the SMASH config files are located.
   */
  explicit Configuration(const bf::path &path);

  /**
   * Reads a YAML config file from the specified \p path.
   *
   * \param path The directory where the SMASH config files are located.
   * \param filename The filename (without path) of the YAML config file, in
   *                 case you don't want the default "config.yaml".
   */
  explicit Configuration(const bf::path &path, const bf::path &filename);

#ifdef BUILD_TESTS
  /**
   * \mocking
   * Unit tests can use this constructor to get a Configuration object from a
   * built-in string.
   * This function is only available to tests and should never be used/needed in
   * actual SMASH code. The intention is to avoid creating a mock object for
   * Configuration to test other classes of SMASH.
   */
  explicit Configuration(const char *yaml) : root_node_(YAML::Load(yaml)) {}
#endif

  /// if you want to copy this you're doing it wrong
  Configuration(const Configuration &) = default;
  /// if you want to copy this you're doing it wrong
  Configuration &operator=(const Configuration &) = default;

  /// moving is fine
  Configuration(Configuration &&) = default;
  /// moving is fine
  Configuration &operator=(Configuration &&) = default;

  /**
   * Merge the configuration in \p yaml into the existing tree.
   *
   * The function parses the string in \p yaml into its internal tree
   * representation. Then it merges the nodes from the new tree into the
   * existing tree.
   * The merge resolves conflicts by taking the value from \p yaml.
   *
   * \param yaml A string with YAML (or JSON) content that is to be merged.
   */
  void merge_yaml(const std::string &yaml);

  std::vector<std::string> list_upmost_nodes();

  /**
   * The default interface for SMASH to read configuration values.
   *
   * The function returns the value at the specified \p keys and removes it from
   * the Configuration object. Therefore, a subsequent call to take or has_value
   * with the same \p keys returns an undefined value / \c false.
   * By removing the value, the Configuration object keeps track which settings
   * were never read.
   *
   * \param keys You can pass an arbitrary number of keys inside curly braces,
   *             following the nesting structure in the config file. Example:
                 \verbatim
     Group:
         Key: Value
                 \endverbatim
   *             Call \code string value = config.take({"Group", "Key"});
   *             \endcode to read the value.
   *
   * \return A proxy object that converts to the correct type automatically on
   *         assignment.
   */
  Value take(std::initializer_list<const char *> keys);

  template <typename T>
  T take(std::initializer_list<const char *> keys, T default_value) {
    if (has_value(keys)) {
      return take(keys).operator T();
    }
    return default_value;
  }

  /**
   * Additional interface for SMASH to read configuration values without
   * removing them.
   *
   * The function returns the value at the specified \p keys but does not remove
   * it from the Configuration object. Semantically, this means the value was
   * not used.
   *
   * \param keys You can pass an arbitrary number of keys inside curly braces,
   *             following the nesting structure in the config file.
   *
   * \return A proxy object that converts to the correct type automatically on
   *         assignment.
   */
  Value read(std::initializer_list<const char *> keys) const;

  template <typename T>
  T read(std::initializer_list<const char *> keys, T default_value) {
    if (has_value(keys)) {
      return read(keys).operator T();
    }
    return default_value;
  }

  /**
   * Removes all entries in the map except for \p key.
   *
   * \param key The key of the map entry to keep.
   */
  void remove_all_but(const std::string &key);

  /**
   * Access to the YAML::Node behind the requested \p keys.
   *
   * If you want to read a value use the \ref read function above. Use the
   * subscript operator if you want to assign a new value. The YAML::Node class
   * will automatically convert the data you assign to a string representation
   * suitable for the YAML file.
   *
   * \return An opaque object that can be assigned to.
   *
   * \see take
   * \see read
   */
  template <typename T>
  Configuration operator[](T &&key) {
    return root_node_[std::forward<T>(key)];
  }

  /**
   * Assignment overwrites the value of the current YAML node.
   *
   * \param value An arbitrary value that yaml-cpp can convert into YAML
   * representation. Any builtin type, strings, maps, and vectors can be used
   * here.
   */
  template <typename T>
  Configuration &operator=(T &&value) {
    root_node_ = std::forward<T>(value);
    return *this;
  }

  /**
   *  Returns if there is a (maybe empty) value behind the requested \p keys.
   */
  bool has_value_including_empty(
      std::initializer_list<const char *> keys) const;
  /**
   * Returns whether there is a non-empty value behind the requested \p keys.
   */
  bool has_value(std::initializer_list<const char *> keys) const;

  /**
   * Returns a string listing the key/value pairs that have not been taken yet.
   */
  std::string unused_values_report() const;

  /**
   * Returns a YAML string of the current tree.
   *
   * This differs from the above in that it does not remove empty maps.
   */
  std::string to_string() const;

 private:
  /** Creates a subobject that has its root node at the given node.
   *
   * \note This constructor is not explicit because it can be called only from
   * inside Configuration and by making it explicit a return would require the
   * copy constructor.
   */
  Configuration(const YAML::Node &node)  // NOLINT(runtime/explicit) : see above
      : root_node_(node) {}

  /// the general_config.yaml contents - fully parsed
  YAML::Node root_node_;
};

}  // namespace smash

#endif  // SRC_INCLUDE_CONFIGURATION_H_
