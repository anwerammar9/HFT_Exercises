#include "timer_wheel.h"

#include <gtest/gtest.h>

#include <chrono>
#include <vector>

using namespace std::chrono_literals;

TEST(TimerWheelTest, SingleTimerFiresExactlyOnceAfterDeadlineNeverBefore) {
  TimerWheel w;
  int fired = 0;

  w.schedule(100ms, [&] { ++fired; });  // deadline 100ms

  w.tick(50ms);
  EXPECT_EQ(fired, 0);  // not before its deadline
  w.tick(99ms);
  EXPECT_EQ(fired, 0);
  w.tick(100ms);
  EXPECT_EQ(fired, 1);
  w.tick(200ms);
  EXPECT_EQ(fired, 1);  // exactly once, no re-fire
}

TEST(TimerWheelTest, MultipleTimersFireInRelativeOrder) {
  TimerWheel w;
  std::vector<int> order;

  w.schedule(30ms, [&] { order.push_back(30); });
  w.schedule(10ms, [&] { order.push_back(10); });
  w.schedule(20ms, [&] { order.push_back(20); });

  w.tick(10ms);
  w.tick(20ms);
  w.tick(30ms);

  EXPECT_EQ(order, (std::vector<int>{10, 20, 30}));
}

TEST(TimerWheelTest, CancelBeforeDeadlinePreventsFiring) {
  TimerWheel w;
  int fired = 0;

  auto id = w.schedule(100ms, [&] { ++fired; });
  EXPECT_TRUE(w.cancel(id));

  w.tick(500ms);
  EXPECT_EQ(fired, 0);

  EXPECT_FALSE(w.cancel(id));      // already cancelled -> false, no throw
  EXPECT_FALSE(w.cancel(0xffff));  // unknown id -> false, no throw
}

TEST(TimerWheelTest, ReentrantScheduleFromCallbackFiresOnLaterTick) {
  TimerWheel w;
  int outer = 0;
  int inner = 0;

  // Outer fires at 50ms; while firing it schedules an inner timer whose
  // deadline is computed from the current wheel time => 50 + 30 = 80ms.
  w.schedule(50ms, [&] {
    ++outer;
    w.schedule(30ms, [&] { ++inner; });
  });

  w.tick(50ms);
  EXPECT_EQ(outer, 1);
  EXPECT_EQ(inner, 0);  // the child must NOT fire during the same tick

  w.tick(60ms);
  EXPECT_EQ(inner, 0);  // ...nor before its own deadline
  w.tick(79ms);
  EXPECT_EQ(inner, 0);
  w.tick(80ms);
  EXPECT_EQ(inner, 1);  // fires on a later tick, exactly once
  w.tick(500ms);
  EXPECT_EQ(inner, 1);
}

TEST(TimerWheelTest, WrapAroundBeyondOneFullRotation) {
  // kWheelSize == 256 slots => one rotation. A delay of 300ms spans more than
  // a full rotation; the classic buggy wheel fires it a rotation early.
  TimerWheel w;
  int f300 = 0;
  int f1500 = 0;

  w.schedule(300ms, [&] { ++f300; });
  w.schedule(1500ms, [&] { ++f1500; });

  w.tick(100ms);
  w.tick(255ms);
  EXPECT_EQ(f300, 0);  // not yet: 300 > 255
  EXPECT_EQ(f1500, 0);

  w.tick(300ms);  // crossing 256 must NOT have fired the 300ms timer early
  EXPECT_EQ(f300, 1);
  EXPECT_EQ(f1500, 0);

  w.tick(1499ms);
  EXPECT_EQ(f1500, 0);
  w.tick(1500ms);
  EXPECT_EQ(f1500, 1);

  w.tick(5000ms);  // far future: nothing re-fires
  EXPECT_EQ(f300, 1);
  EXPECT_EQ(f1500, 1);
}