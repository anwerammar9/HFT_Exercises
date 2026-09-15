#ifndef EXERCISE47_LOAN_TOKENS_H_
#define EXERCISE47_LOAN_TOKENS_H_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

// TODO(anwer): implement explicit-loan borrow checking (see SOLUTION.md).
//
// Contract:
//   - borrow() issues a shared token iff no writer is active.
//   - borrow_mut() issues an exclusive token iff fully free.
//   - access()/access_mut() resolve live tokens, throw invalid_argument on
//     dead/unknown/mismatched tokens.
//   - repay() ends the loan (true) or reports unknown/dead (false).
//   - Tokens alias by generation id: copies share one loan.
//
// Suggested shape (see SOLUTION.md):
//   - Record { id, mut, live } in a vector; next_id_ from 1 (0 invalid).
//   - repay() marks dead + prunes so loans() counts live loans only.
template <typename T>
class LoanBox {
 public:
  struct Token {
    std::uint64_t id = 0;  // 0 == invalid (never issued)
    bool mut = false;
  };

  template <typename... Args>
  explicit LoanBox(Args&&... args) : value_(std::forward<Args>(args)...) {}

  LoanBox(const LoanBox&) = delete;
  LoanBox& operator=(const LoanBox&) = delete;

  std::optional<Token> borrow() {  // TODO(anwer)
    return std::nullopt;
  }

  std::optional<Token> borrow_mut() {  // TODO(anwer)
    return std::nullopt;
  }

  const T& access(const Token& /*t*/) const {  // TODO(anwer)
    throw std::invalid_argument("LoanBox: not implemented");
  }

  T& access_mut(const Token& /*t*/) {  // TODO(anwer)
    throw std::invalid_argument("LoanBox: not implemented");
  }

  bool repay(const Token& /*t*/) {  // TODO(anwer)
    return false;
  }

  std::size_t loans() const noexcept { return 0; }  // TODO(anwer)
  bool writing() const noexcept { return writer_; }

 private:
  T value_;
  bool writer_ = false;
};

#endif  // EXERCISE47_LOAN_TOKENS_H_
