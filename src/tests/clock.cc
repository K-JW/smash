/*
 *
 *    Copyright (c) 2014-2015
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include "unittest.h"
#include "../include/clock.h"

using namespace Smash;

TEST(size) {
  // if this fails, then either the internal structure in Clock is
  // changed (using other types or the addition of new variables) or the
  // alignment of the internal structure is somehow different. In both
  // cases, this test is meant to warn future developers to check if the
  // new behaviour is wanted or an unintendet side effect that should be
  // corrected.
  COMPARE(sizeof(Clock), 3 * sizeof(Clock::Representation));
}

TEST(set_clock) {
  Clock labtime(0.123f, 0.234f);
  COMPARE(labtime.current_time(), 0.123f);
  COMPARE(labtime.timestep_duration(), 0.234f);
}

TEST(run_clock) {
  Clock labtime(0.0f, 0.1f);
  COMPARE(labtime.current_time(), 0.0f);
  ++labtime;
  COMPARE(labtime.current_time(), 0.1f);
  labtime += 0.5f;
  COMPARE(labtime.current_time(), 0.6f);
  labtime += 2;
  COMPARE(labtime.current_time(), 0.8f);
  Clock endtime(1.0f, 0.0f);
  while (labtime < endtime) {
    ++labtime;
  }
  COMPARE(labtime.current_time(), 1.0f);
}

TEST(reset_timestep) {
  Clock labtime(0.0f, 0.1f);
  ++labtime;
  ++labtime;
  labtime.set_timestep_duration(0.2f);
  ++labtime;
  COMPARE(labtime.current_time(), 0.4f);
}

TEST(compare) {
  Clock labtime(0.0f, 0.0f);
  Clock comtime(1.0f, 0.0f);
  VERIFY(labtime < comtime);
  VERIFY(labtime < 0.1f);
  VERIFY(comtime > 0.1f);
}

TEST(multiple) {
  Clock labtime(7.0f, 1.0f);
  float interval = 2.1f;
  // time is 7, next multiple is 8.4
  VERIFY(!labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
  ++labtime;  // 8; 8.4
  VERIFY( labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
  ++labtime;  // 9; 10.5
  VERIFY(!labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
  ++labtime;  // 10; 10.5
  VERIFY( labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
  ++labtime;  // 11; 12.6
  VERIFY(!labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
  ++labtime;  // 12; 12.6
  VERIFY( labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
  ++labtime;  // 13
  ++labtime;  // 14
  ++labtime;  // 15
  ++labtime;  // 16
  ++labtime;  // 17
  ++labtime;  // 18; 18.9
  VERIFY( labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
  ++labtime;  // 19; 21.0
  VERIFY(!labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
  ++labtime;  // 20; 21.0 (NO!)
  VERIFY(!labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
  ++labtime;  // 21; 21.0 (YES!)
  VERIFY( labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
  ++labtime;  // 22; 23.1 (NO!)
  VERIFY(!labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
}

TEST(multiple_small_interval) {
  Clock labtime(6.5f, 1.0f);
  float interval = 0.1f;
  for (int ticks = 0; ticks < 20; ++ticks) {
    VERIFY(labtime.multiple_is_in_next_tick(interval)) << ticks;
  }
}

TEST(multiple_small_increment) {
  Clock labtime(0.0f, 0.002f);
  for (int i = 0; i < 30; ++i) {
    while (++labtime < i + 0.997f) {
      VERIFY(!labtime.multiple_is_in_next_tick(1.0f)) << labtime.current_time();
    }
    // check that one of this or next tick (but one only!) fits.
    VERIFY((labtime.multiple_is_in_next_tick(1.0f)
       ^ (++labtime).multiple_is_in_next_tick(1.0f))) << labtime.current_time();
  }
}

TEST(multiple_negative_times) {
  {
    Clock labtime(-12.0f, 1.0f);
    float interval = 2.2f;
    VERIFY(!labtime.multiple_is_in_next_tick(interval)) << labtime.current_time() << ' ' << labtime.next_multiple(interval);
    ++labtime;  // time is -11, next multiple is -11.0: YES
    VERIFY(labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
    ++labtime;  // -10, -8.8: NO
    VERIFY(!labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
    ++labtime;  // -9; -8.8: YES
    VERIFY(labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
    ++labtime;  // -8; -6.6: NO
    VERIFY(!labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
    ++labtime;  // -7; -6.6: YES
    VERIFY(labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
    ++labtime;  // -6; -4.4: NO
    VERIFY(!labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
    ++labtime;  // -5; -4.4: YES
    VERIFY(labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
    ++labtime;  // -4; -2.2: NO
    VERIFY(!labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
    ++labtime;  // -3; -2.2: YES
    VERIFY(labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
    ++labtime;  // -2;  0.0: NO
    VERIFY(!labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
    ++labtime;  // -1;  0.0: NO
    VERIFY(!labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
    ++labtime;  // 0;  0.0: YES
    VERIFY(labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
    ++labtime;  // 1;  2.2: NO
    VERIFY(!labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
    ++labtime;  // 2;  2.2: YES
    VERIFY(labtime.multiple_is_in_next_tick(interval)) << labtime.current_time();
  }
  {
    Clock labtime(-1.4, 0.4);
    const float interval = 1.3;
    // t = -1.4, -1.3: YES
    VERIFY( labtime.multiple_is_in_next_tick(interval)) << labtime.current_time() << ' ' << labtime.next_multiple(interval);
    ++labtime;  // t = -1.0, 0.0: NO
    VERIFY(!labtime.multiple_is_in_next_tick(interval)) << labtime.current_time() << ' ' << labtime.next_multiple(interval);
    ++labtime;  // t = -0.6, 0.0: NO
    VERIFY(!labtime.multiple_is_in_next_tick(interval)) << labtime.current_time() << ' ' << labtime.next_multiple(interval);
    ++labtime;  // t = -0.2, 0.0: YES
    VERIFY( labtime.multiple_is_in_next_tick(interval)) << labtime.current_time() << ' ' << labtime.next_multiple(interval);
    ++labtime;  // t = +0.2, 1.3: NO
    VERIFY(!labtime.multiple_is_in_next_tick(interval)) << labtime.current_time() << ' ' << labtime.next_multiple(interval);
    ++labtime;  // t = +0.6, 1.3: NO
    VERIFY(!labtime.multiple_is_in_next_tick(interval)) << labtime.current_time() << ' ' << labtime.next_multiple(interval);
    ++labtime;  // t = +1.0, 1.3: YES
    VERIFY( labtime.multiple_is_in_next_tick(interval)) << labtime.current_time() << ' ' << labtime.next_multiple(interval);
    ++labtime;  // t = +1.4, 2.6: NO
    VERIFY(!labtime.multiple_is_in_next_tick(interval)) << labtime.current_time() << ' ' << labtime.next_multiple(interval);
    ++labtime;  // t = +1.8, 2.6: NO
    VERIFY(!labtime.multiple_is_in_next_tick(interval)) << labtime.current_time() << ' ' << labtime.next_multiple(interval);
    ++labtime;  // t = +2.2, 2.6: NO
    VERIFY(!labtime.multiple_is_in_next_tick(interval)) << labtime.current_time() << ' ' << labtime.next_multiple(interval);
    ++labtime;  // t = +2.6, 2.6: YES
    VERIFY( labtime.multiple_is_in_next_tick(interval)) << labtime.current_time() << ' ' << labtime.next_multiple(interval);
  }
}

TEST(next_multiple) {
  Clock labtime(-1.4, 0.4);
  float interval = 1.3;
  COMPARE(labtime.next_multiple(interval), -interval) << labtime.current_time();
  ++labtime;  // t = -1.0
  COMPARE(labtime.next_multiple(interval), 0.f) << labtime.current_time();
  ++labtime;  // t = -0.6
  COMPARE(labtime.next_multiple(interval), 0.f) << labtime.current_time();
  ++labtime;  // t = -0.2
  COMPARE(labtime.next_multiple(interval), 0.f) << labtime.current_time();
  ++labtime;  // t = +0.2
  COMPARE(labtime.next_multiple(interval), interval) << labtime.current_time();
  ++labtime;  // t = +0.6
  COMPARE(labtime.next_multiple(interval), interval) << labtime.current_time();
  ++labtime;  // t = +1.0
  COMPARE(labtime.next_multiple(interval), interval) << labtime.current_time();
  ++labtime;  // t = +1.4
  COMPARE(labtime.next_multiple(interval), 2.6f) << labtime.current_time();
  ++labtime;  // t = +1.8
  ++labtime;  // t = +2.2
  COMPARE(labtime.next_multiple(interval), 2.6f) << labtime.current_time();
  ++labtime;  // t = +2.6
  COMPARE(labtime.next_multiple(interval), 3.9f) << labtime.current_time();
  ++labtime;  // t = +3.0
  COMPARE(labtime.next_multiple(interval), 3.9f) << labtime.current_time();
  ++labtime;  // t = +3.4
  ++labtime;  // t = +3.8
  ++labtime;  // t = +4.2
  ++labtime;  // t = +4.6
  ++labtime;  // t = +5.0
  COMPARE(labtime.next_multiple(interval), 5.2f) << labtime.current_time();
  ++labtime;  // t = +5.4
  ++labtime;  // t = +5.8
  ++labtime;  // t = +6.2
  ++labtime;  // t = +6.6
  ++labtime;  // t = +7.0
  ++labtime;  // t = +7.4
  COMPARE(labtime.next_multiple(interval), 7.8f) << labtime.current_time();
  ++labtime;  // t = +7.8
  COMPARE(labtime.next_multiple(interval), 9.1f) << labtime.current_time();
  ++labtime;  // t = +8.2
  COMPARE(labtime.next_multiple(interval), 9.1f) << labtime.current_time();
}
TEST(end_tick_on_multiple) {
  Clock labtime(-1.4f, 0.4);
  constexpr float interval = 1.3f;
  labtime.end_tick_on_multiple(interval);
  ++labtime;  // t = -1.3
  COMPARE(labtime.current_time(), -interval) << labtime.timestep_duration();
  labtime.reset(0.1f); // t = 0.1
  labtime.end_tick_on_multiple(interval);
  ++labtime;  // t = 1.3
  COMPARE(labtime.current_time(), interval);
  labtime.set_timestep_duration(0.2f);
  VERIFY(labtime.multiple_is_in_next_tick(interval));
  ++labtime; // t = 1.5
  labtime.end_tick_on_multiple(interval);
  ++labtime; // t = 2.6
  COMPARE(labtime.current_time(), 2 * interval) << labtime.timestep_duration();
  labtime.end_tick_on_multiple(interval);
  ++labtime; // t = 3.9
  COMPARE(labtime.current_time(), 3.9f) << labtime.timestep_duration();
  labtime.end_tick_on_multiple(interval);
  ++labtime; // t = 5.2
  COMPARE(labtime.current_time(), 5.2f) << labtime.timestep_duration();
  COMPARE(labtime.next_multiple(interval), 6.5f) << labtime.current_time();
  labtime.set_timestep_duration(0.2f);
  VERIFY(labtime.multiple_is_in_next_tick(interval));
}

TEST(longtime_test) {
  // this test is added after assessing Issue #697: At very long times,
  // the code for Clock::multiple_is_in_next_tick() failed.
  Clock labtime(0.0f, 0.01f);
  while (labtime < 100*1000) {
    for (int i = 0; i < 10; ++i) {
      ++labtime;
    }
    Clock nexttime = labtime;
    ++nexttime;
    VERIFY(labtime.multiple_is_in_next_tick(0.1f));
  }
}

TEST(assignment) {
  Clock labtime(4.2f, 0.3f);
  Clock resettime = labtime;
  ++labtime;
  COMPARE(labtime.current_time(), 4.5f);
  labtime = std::move(resettime);
  COMPARE(labtime.current_time(), 4.2f);
}

TEST_CATCH(init_negative_dt, std::range_error) {
  Clock labtime(4.4f, -0.2f);
}
TEST_CATCH(set_negative_dt, std::range_error) {
  Clock labtime(4.4f, 0.2f);
  labtime.set_timestep_duration(-0.3f);
}
TEST_CATCH(negative_interval, std::range_error) {
  Clock labtime(4.4f, 0.2f);
  labtime.multiple_is_in_next_tick(-0.8f);
}
TEST_CATCH(big_timestep_negative, std::range_error) {
  Clock labtime(4.4f, 0.2f);
  labtime += -0.8f;
}

TEST(no_overflow_single_increment) {
  Clock labtime(0.0f, 1.0f);
  labtime += (std::numeric_limits<Clock::Representation>::max() - 3);
  ++labtime;
  ++labtime;
}

TEST_CATCH(overflow_single_increment, std::overflow_error) {
  Clock labtime(0.0f, 1.0f);
  labtime += (std::numeric_limits<Clock::Representation>::max() - 3);
  ++labtime;
  ++labtime;
  ++labtime;
}

TEST(no_overflow_large_increment) {
  Clock labtime(0.0f, 1.0f);
  ++labtime;
  ++labtime;
  labtime += (std::numeric_limits<Clock::Representation>::max() - 3);
}

TEST_CATCH(overflow_large_increment, std::overflow_error) {
  Clock labtime(0.0f, 1.0f);
  ++labtime;
  ++labtime;
  ++labtime;
  labtime += (std::numeric_limits<Clock::Representation>::max() - 3);
}
