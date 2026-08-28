#ifndef EXERCISE10_TIMER_WHEEL_H_
#define EXERCISE10_TIMER_WHEEL_H_

#include <chrono>
#include <cstdint>
#include <functional>
#include <queue>
#include <set>
#include <vector>

// Single-threaded, tick-driven timer wheel (matching-engine / market-data
// event-loop style: one thread owns the wheel and drives time explicitly).
//
// Time model (read the tests, they encode this exactly):
//   - The wheel's clock advances ONLY through tick(now). `now` is a
//     monotonically increasing std::chrono::milliseconds value; the wheel keeps
//     an internal "current time", starting at 0ms.
//   - schedule(delay, cb): fires `cb` EXACTLY ONCE at the first tick(now) for
//     which  now >= (wheel's current time when schedule() was called) + delay.
//     Returns a unique, opaque TimerId (0 is never valid).
//   - A callback firing during tick() may call schedule()/cancel() re-entrantly;
//     a new timer's deadline is computed from the wheel's time AT THAT MOMENT,
//     so it fires on a later tick, never during the tick that just ran it.
//   - cancel(id): true iff the timer was still pending (prevents the callback).
//     False (and safe) for already-fired and unknown ids. Idempotent.
//
// Contract details chosen for this exercise:
//   - Granularity: 1ms per wheel slot.
//   - Wheel size for one full rotation: kWheelSize (= 256) slots.
//   - The wrap-around edge case (delay > 256ms, i.e. more than one rotation)
//     must still fire at the right absolute tick — that's the classic bug.
//   - Single-threaded: only the driving thread may call schedule/cancel/tick.
//
// TODO(anwer): implement schedule/cancel/tick in src/timer_wheel.cpp.
//   Suggested design: std::array of buckets (std::vector<Timer> per slot),
//   a rotation counter per timer so a delay longer than one rotation lands many
//   rotations ahead, and a next-slot cursor that tick() walks forward.
//   Members below are reserved for your implementation.

class TimerWheel {
 public:
  using TimerId = std::uint64_t;
  static constexpr std::uint64_t kWheelSize = 256;  // 1ms slots => one rotation

  TimerWheel() = default;

  TimerWheel(const TimerWheel&) = delete;
  TimerWheel& operator=(const TimerWheel&) = delete;

  TimerId schedule(std::chrono::milliseconds delay, std::function<void()> cb);

  bool cancel(TimerId id);

  void tick(std::chrono::milliseconds now);

 private:
  struct Timer {
    std::chrono::milliseconds deadline;
    std::uint64_t seq;
    TimerId id;
    std::function<void()> cb;
  };
  struct TimerCmp {
    bool operator()(const Timer& a, const Timer& b) const {
      if (a.deadline != b.deadline) return a.deadline > b.deadline;  // min-heap
      return a.seq > b.seq;
    }
  };

  std::chrono::milliseconds now_{0};
  TimerId next_id_{0};
  std::uint64_t seq_{0};
  std::priority_queue<Timer, std::vector<Timer>, TimerCmp> pq_;
  std::set<TimerId> pending_;
  std::set<TimerId> done_;
};

#endif  // EXERCISE10_TIMER_WHEEL_H_