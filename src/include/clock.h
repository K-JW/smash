/*
 *
 *    Copyright (c) 2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#ifndef SRC_INCLUDE_CLOCK_H_
#define SRC_INCLUDE_CLOCK_H_

#include <cmath>
#include <stdexcept>

namespace Smash {

/** Clock tracks the simulation time, i.e., the time IN the simulation.
 *
 * Usage:
 * ------
 * \code
 *   Clock labtime(0.f, 0.1f);
 *   Clock endtime(10.f, 0.f);
 *   while (labtime < endtime) {
 *     // do something
 *     ++labtime;
 *   }
 * \endcode
 *
 **/
class Clock {
 public:
  /// default initializer: Timestep size is set to 0!
  Clock() : counter_(0), timestep_size_(0.f), reset_time_(0.f) {};
  /** initialize with base time and time step size.
   *
   * \param time base time
   * \param dt step size
   *
   */
  Clock(const float time, const float dt)
                : counter_(0)
                , timestep_size_(dt)
                , reset_time_(time) {
    if (dt < 0.f) {
      throw std::range_error("No negative time increment allowed");
    }
  }
  inline float current_time() const {
    return reset_time_ + timestep_size_ * counter_;
  }
  /// returns the time step size.
  float timestep_size() const {
    return timestep_size_;
  }
  /** sets the time step size (and resets the counter)
   *
   * \param dt new time step size
   *
   */
  void set_timestep_size(const float dt) {
    if (dt < 0.f) {
      throw std::range_error("No negative time increment allowed");
    }
    reset_time_ = current_time();
    counter_ = 0;
    timestep_size_ = dt;
  }
  /** checks if a multiple of a given interval is reached within the
   * next tick.
   *
   * \param interval The interval \f$t_i\f$ at which, for instance,
   * output is expected to happen
   *
   * \return is there a natural number n so that \f$n \cdot t_i\f$ is
   * between the current time and the next time: \f$\exists n \in
   * \mathbb{N}: t \le n \cdot t_i < t + \Delta t\f$.
   * 
   */
  bool multiple_is_in_next_tick(const float interval) const {
    if (interval < 0.f) {
      throw std::range_error("Negative interval makes no sense for clock");
    }
    // if the interval is less than or equal to the time step size, one
    // multiple will surely be within the next tick!
    if (interval <= timestep_size_) {
      return true;
    }
    // else, let's first see what n is in question:
    float n = floor((current_time() + timestep_size_) / interval);
    return (current_time() <= n * interval
         && n * interval < current_time() + timestep_size_);
  }
  /** resets the time to a pre-defined value
   * 
   * This is the only way of turning the clock back. It is needed so
   * that the time can be adjusted after initialization (different
   * initial conditions may require different starting times).
   *
   **/
  void reset(const float reset_time) {
    if (reset_time < current_time()) {
      printf("Resetting clock from %g fm/c to %g fm/c\n",
              current_time(), reset_time);
    }
    reset_time_ = reset_time;
    counter_ = 0.0;
  }
  /// advances the clock by one tick (\f$\Delta t\f$)
  Clock& operator++() {
    counter_++;
    return *this;
  }
  /// advances the clock by an arbitrary timestep
  Clock& operator+=(const float& big_timestep) {
    if (big_timestep < 0.f) {
      throw std::range_error("Alas, the clock cannot be turned back.");
    }
    reset_time_ += big_timestep;
    return *this;
  }
  /// advances the clock by an arbitrary number of ticks.
  Clock& operator+=(const int& advance_several_timesteps) {
    if (advance_several_timesteps < 0) {
      throw std::range_error("Alas, the clock cannot be turned back.");
    }
    counter_ += advance_several_timesteps;
    return *this;
  }
  /// compares the times between two clocks.
  bool operator<(const Clock& rhs) const {
    return current_time() < rhs.current_time();
  }
  /// compares the time of the clock against a fixed time.
  bool operator<(const float& time) const {
    return current_time() < time;
  }
  /// compares the time of the clock against a fixed time.
  bool operator>(const float& time) const {
    return current_time() > time;
  }
 private:
  /// clock tick. This is purely internal and will be reset when the
  /// timestep size is changed
  int counter_ = 0;
  /// The time step size \f$\Delta t\f$
  float timestep_size_ = 0.0f;
  /// The time of last reset (when counter_ was set to 0).
  float reset_time_ = 0.0f;
};

}  // namespace Smash

#endif  // SRC_INCLUDE_CLOCK_H_
