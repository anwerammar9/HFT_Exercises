#include "twap_vwap_slicer.h"

#include <chrono>

#include <gtest/gtest.h>

namespace {

using std::chrono::milliseconds;

TEST(TwapSlicerTest, SplitsEvenlyAndAbsorbsRemainderIntoLast) {
  TwapSlicer slicer(100, milliseconds(4000), 3);
  auto p = slicer.plan();
  ASSERT_EQ(p.size(), 3u);  // stub: empty -> red
  EXPECT_EQ(p[0].qty, 33);
  EXPECT_EQ(p[1].qty, 33);
  EXPECT_EQ(p[2].qty, 34);                       // remainder -> last slice
  EXPECT_EQ(p[0].send_at, milliseconds(1333));   // 4000 * 1/3
  EXPECT_EQ(p[1].send_at, milliseconds(2666));   // 4000 * 2/3
  EXPECT_EQ(p[2].send_at, milliseconds(4000));
}

TEST(TwapSlicerTest, SliceSumExact) {
  TwapSlicer slicer(10000, milliseconds(1000), 7);
  auto p = slicer.plan();
  ASSERT_EQ(p.size(), 7u);  // stub: empty -> red
  Qty sum = 0;
  for (const auto& s : p) sum += s.qty;
  EXPECT_EQ(sum, 10000);
}

TEST(TwapSlicerTest, EvenDivisionPerfectlySpaced) {
  TwapSlicer slicer(200, milliseconds(8000), 4);
  auto p = slicer.plan();
  ASSERT_EQ(p.size(), 4u);  // stub: empty -> red
  for (int i = 0; i < 4; ++i) {
    EXPECT_EQ(p[i].qty, 50);
    EXPECT_EQ(p[i].send_at, milliseconds(2000 * (i + 1)));
  }
}

TEST(TwapSlicerTest, SingleSliceIsFullOrderImmediate) {
  TwapSlicer slicer(100, milliseconds(5000), 1);
  auto p = slicer.plan();
  ASSERT_EQ(p.size(), 1u);  // stub: empty -> red
  EXPECT_EQ(p[0].qty, 100);
  EXPECT_EQ(p[0].send_at, milliseconds(0));
}

TEST(TwapSlicerTest, DegenerateInputsEmpty) {
  EXPECT_TRUE(TwapSlicer(0, milliseconds(1000), 3).plan().empty());
  EXPECT_TRUE(TwapSlicer(100, milliseconds(1000), 0).plan().empty());
}

TEST(VwapSlicerTest, WeightsSlicesByVolumeCurve) {
  VwapSlicer slicer(100, milliseconds(1000), {0.25, 0.50, 0.25});
  auto p = slicer.plan();
  ASSERT_EQ(p.size(), 3u);  // stub: empty -> red
  EXPECT_EQ(p[0].qty, 25);
  EXPECT_EQ(p[1].qty, 50);
  EXPECT_EQ(p[2].qty, 25);
  EXPECT_EQ(p[0].send_at, milliseconds(250));   // end of bucket 1
  EXPECT_EQ(p[1].send_at, milliseconds(750));   // end of bucket 2 (cumulative)
  EXPECT_EQ(p[2].send_at, milliseconds(1000));
}

TEST(VwapSlicerTest, NormalizesFractionsNotSummingToOne) {
  VwapSlicer slicer(200, milliseconds(1000), {0.5, 1.0, 0.5});  // sums to 2.0
  auto p = slicer.plan();
  ASSERT_EQ(p.size(), 3u);  // stub: empty -> red
  EXPECT_EQ(p[0].qty, 50);   // 200 * 0.25
  EXPECT_EQ(p[1].qty, 100);  // 200 * 0.50
  EXPECT_EQ(p[2].qty, 50);   // 200 * 0.25
  Qty sum = 0;
  for (const auto& s : p) sum += s.qty;
  EXPECT_EQ(sum, 200);
}

TEST(VwapSlicerTest, SingleFractionIsFullOrderImmediate) {
  VwapSlicer slicer(100, milliseconds(1000), {1.0});
  auto p = slicer.plan();
  ASSERT_EQ(p.size(), 1u);  // stub: empty -> red
  EXPECT_EQ(p[0].qty, 100);
  EXPECT_EQ(p[0].send_at, milliseconds(0));
}

TEST(VwapSlicerTest, DegenerateVwapEmpty) {
  EXPECT_TRUE(VwapSlicer(100, milliseconds(1000), {}).plan().empty());
  EXPECT_TRUE(VwapSlicer(0, milliseconds(1000), {0.5, 0.5}).plan().empty());
}

}  // namespace