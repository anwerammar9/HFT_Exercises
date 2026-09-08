#include "order_state_machine.h"

#include <atomic>
#include <thread>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace {

using State = OrderState;

// Drives a fresh machine into `s` via legal transitions; false if unreachable.
bool drive_to(OrderStateMachine& m, State s) {
  switch (s) {
    case State::New:
      return true;
    case State::PendingNew:
      return m.transition(State::PendingNew);
    case State::PartiallyFilled:
      return m.transition(State::PartiallyFilled);
    case State::PendingCancel:
      return m.transition(State::PartiallyFilled) &&
             m.transition(State::PendingCancel);
    case State::Filled:
      return m.transition(State::PartiallyFilled) &&
             m.transition(State::Filled);
    case State::Cancelled:
      return m.transition(State::Cancelled);
    case State::Rejected:
      return m.transition(State::PendingNew) && m.transition(State::Rejected);
  }
  return false;
}

TEST(OrderStateMachineTest, LegalTransitionsAllSucceed) {
  const std::vector<std::pair<State, State>> legal = {
      {State::New, State::PendingNew},
      {State::New, State::PartiallyFilled},
      {State::New, State::Cancelled},
      {State::New, State::Rejected},
      {State::PendingNew, State::New},
      {State::PendingNew, State::PartiallyFilled},
      {State::PendingNew, State::Rejected},
      {State::PartiallyFilled, State::PartiallyFilled},
      {State::PartiallyFilled, State::Filled},
      {State::PartiallyFilled, State::PendingCancel},
      {State::PartiallyFilled, State::Cancelled},
      {State::PendingCancel, State::Cancelled},
      {State::PendingCancel, State::PartiallyFilled},
      {State::PendingCancel, State::Filled},
  };
  for (const auto& [from, to] : legal) {
    OrderStateMachine m;
    ASSERT_TRUE(drive_to(m, from));  // stub: drive_to fails -> red
    EXPECT_TRUE(m.transition(to));
    EXPECT_EQ(m.state(), to);
  }
}

TEST(OrderStateMachineTest, IllegalTransitionsRejectedAndUnchanged) {
  const std::vector<std::pair<State, State>> illegal = {
      {State::New, State::New},           {State::New, State::PendingCancel},
      {State::New, State::Filled},        {State::PendingNew, State::PendingNew},
      {State::PendingNew, State::PendingCancel},
      {State::PendingNew, State::Cancelled},
      {State::PendingNew, State::Filled}, {State::PendingCancel, State::PendingCancel},
      {State::PendingCancel, State::PendingNew},
      {State::PendingCancel, State::Rejected},
      {State::PartiallyFilled, State::New},
      {State::PartiallyFilled, State::PendingNew},
      {State::PartiallyFilled, State::Rejected},
      {State::Filled, State::New},        {State::Filled, State::PendingNew},
      {State::Filled, State::PartiallyFilled},
      {State::Filled, State::Filled},     {State::Filled, State::PendingCancel},
      {State::Filled, State::Cancelled},  {State::Filled, State::Rejected},
      {State::Cancelled, State::New},     {State::Cancelled, State::PendingNew},
      {State::Cancelled, State::PartiallyFilled},
      {State::Cancelled, State::Filled},  {State::Cancelled, State::PendingCancel},
      {State::Cancelled, State::Cancelled},
      {State::Cancelled, State::Rejected}, {State::Rejected, State::New},
      {State::Rejected, State::PendingNew},
      {State::Rejected, State::PartiallyFilled},
      {State::Rejected, State::Filled},   {State::Rejected, State::PendingCancel},
      {State::Rejected, State::Cancelled},
      {State::Rejected, State::Rejected},
  };
  for (const auto& [from, to] : illegal) {
    OrderStateMachine m;
    ASSERT_TRUE(drive_to(m, from));
    State before = m.state();
    EXPECT_FALSE(m.transition(to));  // stub returns false anyway; real check below
    EXPECT_EQ(m.state(), before);    // real impl must leave state untouched
  }
}

TEST(OrderStateMachineTest, TerminalStatesRejectEverything) {
  for (State terminal : {State::Filled, State::Cancelled, State::Rejected}) {
    OrderStateMachine m;
    ASSERT_TRUE(drive_to(m, terminal));
    for (State to : {State::New, State::PendingNew, State::PartiallyFilled,
                     State::Filled, State::PendingCancel, State::Cancelled,
                     State::Rejected}) {
      EXPECT_FALSE(m.transition(to));
      EXPECT_EQ(m.state(), terminal);
    }
  }
}

TEST(OrderStateMachineTest, ConcurrentFillCancelExactlyOneWins) {
  for (int iter = 0; iter < 200; ++iter) {
    OrderStateMachine m;
    ASSERT_TRUE(drive_to(m, State::PartiallyFilled));

    std::atomic<State> winner{State::New};
    std::thread a([&] {
      if (m.transition(State::Filled)) winner.store(State::Filled);
    });
    std::thread b([&] {
      if (m.transition(State::Cancelled)) winner.store(State::Cancelled);
    });
    a.join();
    b.join();

    EXPECT_TRUE(winner == State::Filled || winner == State::Cancelled);
    EXPECT_EQ(m.state(), winner.load());  // exactly one state was committed
  }
}

TEST(OrderStateMachineTest, ReaderNeverObservesIllegalState) {
  for (int iter = 0; iter < 50; ++iter) {
    OrderStateMachine m;
    ASSERT_TRUE(drive_to(m, State::PartiallyFilled));

    std::atomic<bool> stop{false};
    std::thread reader([&] {
      while (!stop.load(std::memory_order_acquire)) {
        State s = m.state();
        if (s != State::PartiallyFilled && s != State::Filled &&
            s != State::Cancelled) {
          ADD_FAILURE();  // a torn / impossible state was observed
          return;
        }
      }
    });
    std::thread a([&] { m.transition(State::Filled); });
    std::thread b([&] { m.transition(State::Cancelled); });
    a.join();
    b.join();
    stop.store(true, std::memory_order_release);
    reader.join();
  }
}

}  // namespace