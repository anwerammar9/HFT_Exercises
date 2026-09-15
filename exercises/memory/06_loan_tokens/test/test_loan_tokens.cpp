#include "loan_tokens.h"

#include <gtest/gtest.h>

#include <optional>
#include <string>

TEST(LoanTokensTest, StartsFree) {
  LoanBox<int> box(1);
  EXPECT_EQ(box.loans(), 0u);
  EXPECT_FALSE(box.writing());
}

TEST(LoanTokensTest, SharedLoanGrantsReadAccess) {
  LoanBox<int> box(41);
  auto t = box.borrow();
  ASSERT_TRUE(t.has_value()) << "first shared loan must succeed";
  EXPECT_EQ(box.access(*t), 41);
  EXPECT_EQ(box.loans(), 1u);
  EXPECT_FALSE(box.writing());
  EXPECT_TRUE(box.repay(*t));
  EXPECT_EQ(box.loans(), 0u);
}

TEST(LoanTokensTest, ManySharedLoansCoexist) {
  LoanBox<std::string> box("book");
  auto a = box.borrow();
  auto b = box.borrow();
  ASSERT_TRUE(a.has_value());
  ASSERT_TRUE(b.has_value()) << "shared loans are unlimited";
  EXPECT_EQ(box.loans(), 2u);
  EXPECT_EQ(box.access(*a), "book");
  EXPECT_EQ(box.access(*b), "book");
  EXPECT_TRUE(box.repay(*a));
  EXPECT_EQ(box.loans(), 1u);
  EXPECT_EQ(box.access(*b), "book") << "surviving loan still resolves";
  EXPECT_TRUE(box.repay(*b));
}

TEST(LoanTokensTest, ExclusiveLoanBlocksEverything) {
  LoanBox<int> box(5);
  auto w = box.borrow_mut();
  ASSERT_TRUE(w.has_value()) << "free box must grant the exclusive loan";
  EXPECT_TRUE(box.writing());
  EXPECT_FALSE(box.borrow().has_value()) << "no shared while writing";
  EXPECT_FALSE(box.borrow_mut().has_value()) << "no second writer";
  EXPECT_TRUE(box.repay(*w));
  EXPECT_FALSE(box.writing());
}

TEST(LoanTokensTest, SharedLoanBlocksExclusive) {
  LoanBox<int> box(5);
  auto r = box.borrow();
  ASSERT_TRUE(r.has_value());
  EXPECT_FALSE(box.borrow_mut().has_value())
      << "exclusive must wait for readers";
  EXPECT_TRUE(box.repay(*r));
  auto w = box.borrow_mut();
  EXPECT_TRUE(w.has_value()) << "exclusive proceeds once readers repay";
  EXPECT_TRUE(box.repay(*w));
}

TEST(LoanTokensTest, MutationThroughTokenPersists) {
  LoanBox<int> box(1);
  auto w = box.borrow_mut();
  ASSERT_TRUE(w.has_value());
  box.access_mut(*w) = 77;
  EXPECT_TRUE(box.repay(*w));
  auto r = box.borrow();
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(box.access(*r), 77);
  EXPECT_TRUE(box.repay(*r));
}

TEST(LoanTokensTest, DoubleRepayFailsSafely) {
  LoanBox<int> box(1);
  auto r = box.borrow();
  ASSERT_TRUE(r.has_value());
  EXPECT_TRUE(box.repay(*r));
  EXPECT_FALSE(box.repay(*r)) << "second repay of the same token fails";
  EXPECT_EQ(box.loans(), 0u);
}

TEST(LoanTokensTest, StaleAccessThrows) {
  LoanBox<int> box(1);
  auto r = box.borrow();
  ASSERT_TRUE(r.has_value());
  LoanBox<int>::Token copy = *r;  // tokens alias: copies share one loan
  EXPECT_TRUE(box.repay(copy));
  EXPECT_THROW(box.access(*r), std::invalid_argument)
      << "original is dead once the alias repays";
  EXPECT_THROW(box.access(copy), std::invalid_argument);
}

TEST(LoanTokensTest, UnknownTokenIsRejected) {
  LoanBox<int> box(1);
  LoanBox<int>::Token forged{9999, false};
  LoanBox<int>::Token invalid;  // id 0: never issued
  EXPECT_THROW(box.access(forged), std::invalid_argument);
  EXPECT_THROW(box.access(invalid), std::invalid_argument);
  EXPECT_THROW(box.access_mut(forged), std::invalid_argument);
  EXPECT_FALSE(box.repay(forged));
  EXPECT_FALSE(box.repay(invalid));
  EXPECT_EQ(box.loans(), 0u) << "rejected tokens change nothing";
}

TEST(LoanTokensTest, KindMismatchIsRejected) {
  LoanBox<int> box(1);
  auto r = box.borrow();
  ASSERT_TRUE(r.has_value());
  EXPECT_THROW(box.access_mut(*r), std::invalid_argument)
      << "shared token cannot grant mutable access";
  EXPECT_TRUE(box.repay(*r));
}

TEST(LoanTokensTest, FullLifecycleInterleave) {
  LoanBox<int> box(0);
  for (int i = 0; i < 10; ++i) {
    auto r1 = box.borrow();
    auto r2 = box.borrow();
    ASSERT_TRUE(r1.has_value());
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(box.access(*r1), i);
    EXPECT_TRUE(box.repay(*r1));
    EXPECT_TRUE(box.repay(*r2));
    auto w = box.borrow_mut();
    ASSERT_TRUE(w.has_value());
    box.access_mut(*w) = i + 1;
    EXPECT_TRUE(box.repay(*w));
  }
  EXPECT_EQ(box.loans(), 0u);
  EXPECT_FALSE(box.writing());
}
