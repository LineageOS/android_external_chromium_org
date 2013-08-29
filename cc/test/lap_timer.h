// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TEST_LAP_TIMER_H_
#define CC_TEST_LAP_TIMER_H_

#include "base/time/time.h"

namespace cc {

// LapTimer is used to calculate average times per "Lap" in perf tests.
// Current() reports the time since the last call to Start().
// Store() adds the time since the last call to Start() to the accumulator, and
// resets the start time to now. Stored() returns the accumulated time.
// NextLap increments the lap counter, used in counting the per lap averages.
// If you initialize the LapTimer with a non zero warmup_laps, it will ignore
// the times for that many laps at the start.
// If you set the time_limit then you can use HasTimeLimitExpired() to see if
// the current accumulated time has crossed that threshold, with an optimization
// that it only tests this every check_interval laps.
class LapTimer {
 public:
  LapTimer(int warmup_laps, base::TimeDelta time_limit, int check_interval);
  // Resets the timer back to it's starting state.
  void Reset();
  // Sets the start point to now.
  void Start();
  // Returns true if there are no more warmup laps to do.
  bool IsWarmedUp();
  // Advance the lap counter and update the accumulated time.
  // The accumulated time is only updated every check_interval laps.
  // If accumulating then the start point will also be updated.
  void NextLap();
  // Returns true if the stored time has exceeded the time limit specified.
  // May cause a call to Store().
  bool HasTimeLimitExpired();
  // The average milliseconds per lap.
  float MsPerLap();
  // The number of laps per second.
  float LapsPerSecond();
  // The number of laps recorded.
  int NumLaps();

 private:
  base::TimeTicks start_time_;
  base::TimeDelta accumulator_;
  int num_laps_;
  int warmup_laps_;
  int remaining_warmups_;
  base::TimeDelta time_limit_;
  int check_interval_;
  bool accumulated_;

  DISALLOW_COPY_AND_ASSIGN(LapTimer);
};

}  // namespace cc

#endif  // CC_TEST_LAP_TIMER_H_
