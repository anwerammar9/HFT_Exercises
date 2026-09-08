#include "observer_market_data.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

namespace {

class RecordingObserver : public MarketDataObserver {
 public:
  void on_quote(const Quote& q) override {
    std::lock_guard<std::mutex> g(mu);
    quotes.push_back(q);
  }
  void on_trade(const Trade& t) override {
    std::lock_guard<std::mutex> g(mu);
    trades.push_back(t);
  }

  std::mutex mu;
  std::vector<Quote> quotes;
  std::vector<Trade> trades;
};

// Tagged observer that appends to a shared log in callback order.
class TaggedObserver : public MarketDataObserver {
 public:
  TaggedObserver(std::string tag, std::mutex& log_mu, std::vector<std::string>& log)
      : tag_(std::move(tag)), log_mu_(log_mu), log_(log) {}
  void on_quote(const Quote&) override { record(); }
  void on_trade(const Trade&) override { record(); }

 private:
  void record() {
    std::lock_guard<std::mutex> g(log_mu_);
    log_.push_back(tag_);
  }

  std::string tag_;
  std::mutex& log_mu_;
  std::vector<std::string>& log_;
};

class ImpatientObserver : public MarketDataObserver {
 public:
  void on_quote(const Quote&) override { throw std::runtime_error("boom"); }
  void on_trade(const Trade&) override {}
};

TEST(ObserverMarketDataTest, AllSubscribersReceiveQuote) {
  MarketDataFeed feed;
  auto a = std::make_shared<RecordingObserver>();
  auto b = std::make_shared<RecordingObserver>();
  feed.subscribe(a);
  feed.subscribe(b);

  Quote q;
  q.symbol = 1;
  q.bid = 1'000'000;
  q.seq = 1;
  feed.publish_quote(q);

  ASSERT_EQ(a->quotes.size(), 1u);  // stub: never notified -> red
  ASSERT_EQ(b->quotes.size(), 1u);
  EXPECT_EQ(a->quotes[0].bid, 1'000'000);
}

TEST(ObserverMarketDataTest, EventsRouteByType) {
  MarketDataFeed feed;
  auto r = std::make_shared<RecordingObserver>();
  feed.subscribe(r);

  Quote q;
  q.symbol = 1;
  q.bid = 1000;
  q.seq = 1;
  Trade t;
  t.symbol = 1;
  t.price = 1000;
  t.qty = 5;
  t.seq = 2;
  feed.publish_quote(q);
  feed.publish_trade(t);

  ASSERT_EQ(r->quotes.size(), 1u);  // stub: never notified -> red
  ASSERT_EQ(r->trades.size(), 1u);
  EXPECT_EQ(r->quotes[0].bid, 1000);
  EXPECT_EQ(r->trades[0].qty, 5);
}

TEST(ObserverMarketDataTest, SubscriptionOrderPreserved) {
  MarketDataFeed feed;
  std::mutex m;
  std::vector<std::string> log;
  auto a = std::make_shared<TaggedObserver>("a", m, log);
  auto b = std::make_shared<TaggedObserver>("b", m, log);
  auto c = std::make_shared<TaggedObserver>("c", m, log);
  feed.subscribe(a);
  feed.subscribe(b);
  feed.subscribe(c);

  Quote q;
  q.symbol = 1;
  feed.publish_quote(q);

  {
    std::lock_guard<std::mutex> g(m);
    ASSERT_EQ(log.size(), 3u);  // stub: never notified -> red
    EXPECT_EQ(log[0], "a");
    EXPECT_EQ(log[1], "b");
    EXPECT_EQ(log[2], "c");
  }
}

TEST(ObserverMarketDataTest, UnsubscribeSkipsAndReportsUnknown) {
  MarketDataFeed feed;
  auto a = std::make_shared<RecordingObserver>();
  auto b = std::make_shared<RecordingObserver>();
  ObserverId a_id = feed.subscribe(a);
  ObserverId b_id = feed.subscribe(b);

  EXPECT_NE(a_id, 0u);  // stub: ids are 0 -> red
  EXPECT_NE(b_id, 0u);
  EXPECT_EQ(feed.subscriber_count(), 2u);

  EXPECT_TRUE(feed.unsubscribe(a_id));       // stub: always false -> red
  EXPECT_FALSE(feed.unsubscribe(a_id));      // already removed -> false
  EXPECT_FALSE(feed.unsubscribe(12345));     // unknown id -> false
  EXPECT_EQ(feed.subscriber_count(), 1u);

  Quote q;
  q.symbol = 1;
  feed.publish_quote(q);
  ASSERT_EQ(a->quotes.size(), 0u);  // dropped observer sees nothing
  ASSERT_EQ(b->quotes.size(), 1u);  // stub: never notified -> red
}

TEST(ObserverMarketDataTest, SubscribeDuringPublishDefersDelivery) {
  MarketDataFeed feed;
  auto early = std::make_shared<RecordingObserver>();
  auto late = std::make_shared<RecordingObserver>();
  feed.subscribe(early);

  Quote q;
  q.symbol = 1;
  feed.publish_quote(q);
  ASSERT_EQ(early->quotes.size(), 1u);  // stub: never notified -> red

  // Now subscribe a second observer from WITHIN a callback and confirm it is
  // not notified by that same publish but is from the next one.
  class AdderObserver : public MarketDataObserver {
   public:
    AdderObserver(MarketDataFeed& feed, std::shared_ptr<MarketDataObserver> late)
        : feed_(feed), late_(std::move(late)) {}
    void on_quote(const Quote&) override {
      if (added_) return;
      added_ = true;
      feed_.subscribe(late_);
    }
    void on_trade(const Trade&) override {}
    bool added_ = false;
    MarketDataFeed& feed_;
    std::shared_ptr<MarketDataObserver> late_;
  };

  MarketDataFeed sub_feed;
  auto adder = std::make_shared<AdderObserver>(sub_feed, late);
  sub_feed.subscribe(adder);
  sub_feed.publish_quote(q);  // adder subscribes `late` mid-publish
  EXPECT_EQ(late->quotes.size(), 0u);  // same-publish add is deferred
  EXPECT_EQ(sub_feed.subscriber_count(), 2u);

  sub_feed.publish_quote(q);  // now `late` is subscribed for real
  ASSERT_EQ(late->quotes.size(), 1u);  // stub: never notified -> red
}

TEST(ObserverMarketDataTest, SymbolFilterRestrictsFanOut) {
  MarketDataFeed feed;
  auto spy = std::make_shared<RecordingObserver>();
  feed.subscribe(spy, /*symbol=*/42);

  Quote q;
  q.symbol = 7;
  feed.publish_quote(q);
  EXPECT_EQ(spy->quotes.size(), 0u);  // filtered out

  q.symbol = 42;
  feed.publish_quote(q);
  ASSERT_EQ(spy->quotes.size(), 1u);  // stub: never notified -> red
  EXPECT_EQ(spy->quotes[0].symbol, 42u);
}

TEST(ObserverMarketDataTest, ThrowingObserverIsDropped) {
  MarketDataFeed feed;
  auto healthy = std::make_shared<RecordingObserver>();
  auto sick = std::make_shared<ImpatientObserver>();
  feed.subscribe(healthy);
  feed.subscribe(sick);
  EXPECT_EQ(feed.subscriber_count(), 2u);

  Quote q;
  q.symbol = 1;
  feed.publish_quote(q);  // `sick` throws -> feed must drop it

  ASSERT_EQ(healthy->quotes.size(), 1u);  // stub: never notified -> red
  EXPECT_EQ(feed.subscriber_count(), 1u);  // the throwing observer is gone

  feed.publish_quote(q);  // fan-out still works afterwards
  ASSERT_EQ(healthy->quotes.size(), 2u);
}

TEST(ObserverMarketDataTest, ConcurrentPublishersNoLostEvents) {
  MarketDataFeed feed;
  auto counter = std::make_shared<RecordingObserver>();
  feed.subscribe(counter);

  constexpr int kThreads = 4;
  constexpr int kPerThread = 500;
  std::vector<std::thread> workers;
  for (int t = 0; t < kThreads; ++t) {
    workers.emplace_back([&feed] {
      for (int i = 0; i < kPerThread; ++i) {
        Quote q;
        q.symbol = 1;
        q.seq = static_cast<std::uint64_t>(i);
        feed.publish_quote(q);
      }
    });
  }
  for (auto& w : workers) w.join();

  EXPECT_EQ(counter->quotes.size(), kThreads * kPerThread);  // stub: 0 -> red
}

}  // namespace