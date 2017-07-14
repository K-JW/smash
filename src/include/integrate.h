/*
 *
 *    Copyright (c) 2015
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#ifndef SRC_INCLUDE_INTEGRATE_H_
#define SRC_INCLUDE_INTEGRATE_H_

#include <cuba.h>
#include <gsl/gsl_integration.h>
#include <gsl/gsl_monte_plain.h>
#include <gsl/gsl_monte_vegas.h>

#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>

#include "cxx14compat.h"
#include "fpenvironment.h"
#include "random.h"

//void Cuhre(const int ndim, const int ncomp,
//  integrand_t integrand, void *userdata, const int nvec,
//  const cubareal epsrel, const cubareal epsabs,
//  const int flags, const int mineval, const int maxeval,
//  const int key,
//  const char *statefile, void *spin,
//  int *nregions, int *neval, int *fail,
//  cubareal integral[], cubareal error[], cubareal prob[]);

namespace Smash {

/**
 * A deleter type for std::unique_ptr to be used with
 * `gsl_integration_workspace` pointers. This will call
 * `gsl_integration_workspace_free` instead of `delete`.
 */
struct GslWorkspaceDeleter {
  /// The class has no members, so this is a noop.
  constexpr GslWorkspaceDeleter() = default;

  /// Frees the gsl_integration_cquad_workspace resource if it is non-zero.
  void operator()(gsl_integration_cquad_workspace *ptr) const {
    if (ptr == nullptr) {
      return;
    }
    gsl_integration_cquad_workspace_free(ptr);
  }
};

/** The result type returned from integrations,
 * containing the value and an error. */
class Result : public std::pair<double, double> {
  using Base = std::pair<double, double>;

 public:
  /// forward the pair constructors
  using Base::pair;

  /// conversion to double yields the value of the integral
  operator double() const { return Base::first; }

  /// access the first entry in the pair as the value
  double value() const { return Base::first; }

  /// access the second entry in the pair as the absolute error
  double error() const { return Base::second; }

  /// check whether the relative error is small
  ///
  /// Returns an empty string if it is small and an error message if it is
  /// large.
  std::string check_error(double relative_tolerance=1.0) const {
    if (value() == 0) {
      return "";
    }
    if (std::abs(value()) < 1e-12) {
      // For small values the relative error can be very large.
      // The threshold was chosen large enough so that the tests pass.
      return "";
    }
    const auto relative_error = std::abs(error() / value());
    if (relative_error < relative_tolerance) {
      return "";
    } else {
      std::stringstream error_msg;
      error_msg << "Integration error = " << relative_error*100
                << "% > " << relative_tolerance*100 << "%: "
                << value() << " +- " << error();
      return error_msg.str();
    }
  }
};

/**
 * A C++ interface for numerical integration in one dimension
 * with the GSL CQUAD integration functions.
 *
 * Example:
 * \code
 * Integrator integrate;
 * const auto result = integrate(0.1, 0.9, [](double x) { return x * x; });
 * \endcode
 */
class Integrator {
 public:
  /**
   * Construct an integration functor with the given \p workspace_size.
   *
   * \note Since the workspace is allocated in the constructor and deallocated
   * on destruction, you should not recreate Integrator objects unless required.
   * Thus, if you want to calculate multiple integrals with the same \p
   * workspace_size, keep the Integrator object around.
   *
   * \param workspace_size The internal workspace is allocated such that it can
   *                       hold the given number of double precision intervals,
   *                       their integration results, and error estimates.
   *                       It also determines the maximum number of subintervals
   *                       the integration algorithm will use.
   */
  explicit Integrator(size_t workspace_size)
      : workspace_(gsl_integration_cquad_workspace_alloc(workspace_size)) {}

  /// Convenience overload of the above with a workspace size of 1000.
  Integrator() : Integrator(1000) {}

  /**
   * The function call operator implements the integration functionality.
   *
   * \param a The lower limit of the integral.
   * \param b The upper limit of the integral.
   * \param fun The callable to integrate over. This callable may be a function
   *            pointer, lambda, or a functor object. In any case, the callable
   *            must return a `double` and take a single `double` argument. If
   *            you want to pass additional data to the callable you can e.g.
   *            use lambda captures.
   */
  template <typename F>
  Result operator()(double a, double b, F &&fun) {
    Result result;
    const gsl_function gslfun{
        // important! The lambda cannot use captures, otherwise the
        // conversion to a function pointer type is impossible.
        [](double x, void *type_erased) -> double {
          auto &&f = *static_cast<F *>(type_erased);
          return f(x);
        },
        &fun};
    // We disable float traps when calling GSL code we cannot control.
    DisableFloatTraps guard;
    const int error_code = gsl_integration_cquad(&gslfun, a, b,
                          accuracy_absolute_, accuracy_relative_,
                          workspace_.get(),
                          &result.first, &result.second,
                          nullptr /* Don't store the number of evaluations */);
    if (error_code) {
      std::stringstream err;
      err << "GSL integration: " << gsl_strerror(error_code);
      throw std::runtime_error(err.str());
    }
    return result;
  }

 private:
  /// Holds the workspace pointer.
  std::unique_ptr<gsl_integration_cquad_workspace,
                  GslWorkspaceDeleter> workspace_;

  /// Parameter to the GSL integration function: desired absolute error limit
  const double accuracy_absolute_ = 1.0e-5;

  /// Parameter to the GSL integration function: desired relative error limit
  const double accuracy_relative_ = 5.0e-4;
};

/**
 * A C++ interface for numerical integration in one dimension
 * with the GSL Monte-Carlo integration functions.
 *
 * Example:
 * \code
 * Integrator integrate;
 * const auto result = integrate(0.1, 0.9,
 *                               [](double x) { return x * x; });
 * \endcode
 */
class Integrator1dMonte {
 public:
  /**
   * Construct an integration functor.
   *
   * \param num_calls The desired number of calls to the integrand function
   *                  (defaults to 1E6 if omitted), i.e. how often the integrand
   *                  is sampled in the Monte-Carlo integration. Larger numbers
   *                  lead to a more precise result, but also to increased
   *                  runtime.
   *
   * \note Since the workspace is allocated in the constructor and deallocated
   * on destruction, you should not recreate Integrator objects unless required.
   * Thus, if you want to calculate multiple integrals with the same \p
   * workspace_size, keep the Integrator object around.
   */
  explicit Integrator1dMonte(size_t num_calls = 1E6)
      : state_(gsl_monte_plain_alloc(1)),
        rng_(gsl_rng_alloc(gsl_rng_mt19937)),
        number_of_calls_(num_calls) {
    gsl_monte_plain_init(state_);
    // initialize the GSL RNG with a random seed
    const uint32_t seed = Random::uniform_int(0ul, ULONG_MAX);
    gsl_rng_set(rng_, seed);
  }

  /**
   * Destructor: Clean up internal state and RNG.
   */
  ~Integrator1dMonte() {
    gsl_monte_plain_free(state_);
    gsl_rng_free(rng_);
  }

  /**
   * The function call operator implements the integration functionality.
   *
   * \param min The lower limit of the integration.
   * \param max The upper limit of the integration.
   * \param fun The callable to integrate over. This callable may be a function
   *            pointer, lambda, or a functor object. In any case, the callable
   *            must return a `double` and take two `double` arguments. If you
   *            want to pass additional data to the callable you can e.g. use
   *            lambda captures.
   */
  template <typename F>
  Result operator()(double min, double max, F &&fun) {
    Result result = {0, 0};

    const double lower[1] = {min};
    const double upper[1] = {max};

    if (max <= min)
      return result;

    const gsl_monte_function monte_fun{
        // trick: pass integrand function as 'params'
        [](double *x, size_t /*dim*/, void *params) -> double {
          auto &&f = *static_cast<F *>(params);
          return f(x[0]);
        },
        1, &fun};

    gsl_monte_plain_integrate(&monte_fun, lower, upper, 1, number_of_calls_,
                              rng_, state_, &result.first, &result.second);

    return result;
  }

 private:
  /// internal state of the Monte-Carlo integrator
  gsl_monte_plain_state *state_;

  /// random number generator
  gsl_rng *rng_;

  /// number of calls to the integrand
  const std::size_t number_of_calls_;
};

/**
 * A C++ interface for numerical integration in two dimensions
 * with the GSL Monte-Carlo integration functions.
 *
 * Example:
 * \code
 * Integrator integrate;
 * const auto result = integrate(0.1, 0.9, 0., 0.5,
 *                               [](double x, double y) { return x * y; });
 * \endcode
 */
class Integrator2d {
 public:
  /**
   * Construct an integration functor.
   *
   * \param num_calls The desired number of calls to the integrand function
   *                  (defaults to 1E6 if omitted), i.e. how often the integrand
   *                  is sampled in the Monte-Carlo integration. Larger numbers
   *                  lead to a more precise result, but also to increased
   * runtime.
   *
   * \note Since the workspace is allocated in the constructor and deallocated
   * on destruction, you should not recreate Integrator objects unless required.
   * Thus, if you want to calculate multiple integrals with the same \p
   * workspace_size, keep the Integrator object around.
   */
  explicit Integrator2d(size_t num_calls = 1E6)
      : state_(gsl_monte_plain_alloc(2)),
        rng_(gsl_rng_alloc(gsl_rng_mt19937)),
        number_of_calls_(num_calls) {
    gsl_monte_plain_init(state_);
    // initialize the GSL RNG with a random seed
    const uint32_t seed = Random::uniform_int(0ul, ULONG_MAX);
    gsl_rng_set(rng_, seed);
  }

  /**
   * Destructor: Clean up internal state and RNG.
   */
  ~Integrator2d() {
    gsl_monte_plain_free(state_);
    gsl_rng_free(rng_);
  }

  /**
   * The function call operator implements the integration functionality.
   *
   * \param min1 The lower limit in the first dimension.
   * \param max1 The upper limit in the first dimension.
   * \param min2 The lower limit in the second dimension.
   * \param max2 The upper limit in the second dimension.
   * \param fun The callable to integrate over. This callable may be a function
   *            pointer, lambda, or a functor object. In any case, the callable
   *            must return a `double` and take two `double` arguments. If you
   *            want to pass additional data to the callable you can e.g. use
   *            lambda captures.
   */
  template <typename F>
  Result operator()(double min1, double max1, double min2, double max2,
                    F &&fun) {
    Result result = {0, 0};

    const double lower[2] = {min1, min2};
    const double upper[2] = {max1, max2};

    if (max1 <= min1 || max2 <= min2)
      return result;

    const gsl_monte_function monte_fun{
        // trick: pass integrand function as 'params'
        [](double *x, size_t /*dim*/, void *params) -> double {
          auto &&f = *static_cast<F *>(params);
          return f(x[0], x[1]);
        },
        2, &fun};

    gsl_monte_plain_integrate(&monte_fun, lower, upper, 2, number_of_calls_,
                              rng_, state_, &result.first, &result.second);

    return result;
  }

 private:
  /// internal state of the Monte-Carlo integrator
  gsl_monte_plain_state *state_;

  /// random number generator
  gsl_rng *rng_;

  /// number of calls to the integrand
  const std::size_t number_of_calls_;
};

/**
 * A C++ interface for numerical integration in two dimensions
 * with the Cuba Cuhre integration function.
 *
 * Example:
 * \code
 * Integrator integrate;
 * const auto result = integrate(0.1, 0.9, 0., 0.5,
 *                               [](double x, double y) { return x * y; });
 * \endcode
 */
class Integrator2dCuhre {
 public:
  /**
   * Construct an integration functor.
   *
   * \param num_calls The desired number of calls to the integrand function
   *                  (defaults to 1E6 if omitted), i.e. how often the integrand
   *                  is sampled in the integration. Larger numbers lead to a
   *                  more precise result, but also to increased runtime.
   * \param epsrel    The desired relative accuracy.
   * \param epsabs    The desired absolute accuracy.
   */
  explicit Integrator2dCuhre(int num_calls = 1e6,
                        double epsrel = 1e-3, double epsabs = 1e-3)
      : maxeval_(num_calls), epsrel_(epsrel), epsabs_(epsabs) {
  }

  /**
   * The function call operator implements the integration functionality.
   *
   * \param min1 The lower limit in the first dimension.
   * \param max1 The upper limit in the first dimension.
   * \param min2 The lower limit in the second dimension.
   * \param max2 The upper limit in the second dimension.
   * \param fun The callable to integrate over. This callable may be a function
   *            pointer, lambda, or a functor object. In any case, the callable
   *            must return a `double` and take two `double` arguments. If you
   *            want to pass additional data to the callable you can e.g. use
   *            lambda captures.
   */
  template <typename F>
  Result operator()(double min1, double max1, double min2, double max2,
                    F fun) {
                    //F &&fun) {
    Result result = {0., 0.};

    /*
    const double lower[2] = {min1, min2};
    const double upper[2] = {max1, max2};

    if (max1 <= min1 || max2 <= min2)
      return result;
    */

    /*
    const gsl_monte_function monte_fun{
        // trick: pass integrand function as 'params'
        [](double *x, size_t dim, void *params) -> double {
          auto &&f = *static_cast<F *>(params);
          return f(x[0], x[1]);
        },
        2, &fun};
    */

    // TODO transform integrand to unit cube

    const integrand_t cuhre_fun {
        [](const int* ndim, const cubareal xx[], const int* ncomp,
           cubareal ff[], void* userdata) -> int {
          auto &&f = *static_cast<F *>(userdata);
          ff[0] = f(xx[0], xx[1]);
          return 0;
        } };
    //gsl_monte_plain_integrate(&monte_fun, lower, upper, 2, number_of_calls_,
    //                          rng_, state_, &result.first, &result.second);

    //void Cuhre(const int ndim, const int ncomp,
    //  integrand_t integrand, void *userdata, const int nvec,
    //  const cubareal epsrel, const cubareal epsabs,
    //  const int flags, const int mineval, const int maxeval,
    //  const int key,
    //  const char *statefile, void *spin,
    //  int *nregions, int *neval, int *fail,
    //  cubareal integral[], cubareal error[], cubareal prob[]);
    //    return result;
    //  }

    const int ndim = 2;
    const int ncomp = 1;
    void* userdata = &fun;
    const int nvec = 1;
    const int flags = 0;  // Use the defaults.
    const int mineval = 0;
    const int maxeval = maxeval_;
    const int key = -1;  // Use the default.
    const char* statefile = nullptr;
    void* spin = nullptr;
    Cuhre(ndim, ncomp, cuhre_fun, userdata, nvec, epsrel_, epsabs_, flags,
          mineval, maxeval, key, statefile, spin,
          &nregions_, &neval_, &fail_,
          &result.first, &result.second, &prob_);
    return result;
  };

 private:
  /// The (approximate) maximum number of integrand evaluations allowed.
  int maxeval_;
  /// Requested relative accuracy.
  double epsrel_;
  /// Requested absolute accuracy.
  double epsabs_;
  /// Actual number of subregions needed.
  int nregions_;
  /// Actual number of integrand evaluations needed.
  int neval_;
  /// An error flag.
  ///
  /// 0 if the desired accuracy was reached, -1 if the dimension is out of
  /// range, larger than 0 if the accuracy goal was not met within the maximum
  /// number of evaluations.
  int fail_;
  /// The chi^2 probability that the error is not a reliable estimate of the
  /// true integration error.
  double prob_;
};

}  // namespace Smash

#endif  // SRC_INCLUDE_INTEGRATE_H_
