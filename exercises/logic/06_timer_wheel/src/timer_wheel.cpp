#include "timer_wheel.h"

#include <stdexcept>
#include <utility>
#include <vector>

// TODO(anwer): implement schedule/cancel/tick (see SOLUTION.md).
//
// Suggested shape (used by the reference):
//   schedule():  pin id = ++next_id_ (0 never valid), deadline = now_ + delay,
//                push Timer{deadline, seq_++, id, cb} onto pq_, mark pending.
//   cancel():    erase from pending_, mark done_, return true iff it was
//                pending (unknown/fired -> false).
//   tick():      clamp now to monotonic; snapshot all timers with
//                deadline <= now BEFORE running callbacks (so a re-entrant
//                schedule() during a callback is never fired by the same
//                tick); run each snapshot timer unless it landed in done_.

TimerWheel::TimerId TimerWheel::schedule(std::chrono::milliseconds /*delay*/,
                                         std::function<void()> /*cb*/) {
  throw std::logic_error("not implemented");
}

bool TimerWheel::cancel(TimerId /*id*/) { return false; }

void TimerWheel::tick(std::chrono::milliseconds /*now*/) {}