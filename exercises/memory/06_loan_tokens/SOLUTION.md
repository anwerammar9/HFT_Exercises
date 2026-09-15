# Exercise memory/06_loan_tokens (ex47) — Explicit Loan Tokens (Reference Solution)

**What you implement:** the manual counterpart to ex46 — a borrow box where
every loan is an explicit, copyable `Token` (generation id + kind) that the
caller repays by hand. Same shared-XOR-mutable rule, but forgotten repays,
double repays, stale access, and forged tokens are all testable runtime
events.

**Approach**
- Members: `T value_`, `vector<Record{id, mut, live}>`, `next_id_` from 1
  (0 is never issued, so it is always invalid), `bool writer_`.
- `borrow()` refuses iff `writer_`; `borrow_mut()` refuses iff `writer_` or
  any record is live (the vector holds live records only — see prune).
- `find(id)` is a linear scan (loan counts are tiny; a map is overkill).
- `repay()` validates id + kind + liveness, marks dead, clears the writer
  flag for mut loans, and `prune()`s dead records so `loans()` counts live
  loans exactly.
- `access()` validates and returns `value_`; `access_mut()` additionally
  requires a mut-kind token. Both throw `std::invalid_argument` on dead,
  unknown, or mismatched tokens. Token copies alias one loan by id, so
  double repay and repay-through-alias are caught by the live flag.
- Single-threaded by contract.

## Reference API + implementation — `include/loan_tokens.h`

The class is a template, so the implementation is inline in the header:

```cpp
#ifndef EXERCISE47_LOAN_TOKENS_H_
#define EXERCISE47_LOAN_TOKENS_H_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

// Explicit-loan borrow checker: the manual counterpart to ex46's RAII guards.
// Instead of scope-bound guards, the caller holds an explicit Token for every
// outstanding loan and repays it by hand. Same shared-XOR-mutable rule, but
// misuse (forgetting repay, double repay, stale access) is a *testable runtime
// event* rather than a scope exit — which is exactly why RAII guards exist.
//
// Contract:
//   - borrow() issues a shared token iff no writer is active (else nullopt).
//   - borrow_mut() issues an exclusive token iff no loan of any kind is
//     active (else nullopt).
//   - access(t) / access_mut(t) resolve a live token to the object; a dead,
//     unknown, or kind-mismatched token throws std::invalid_argument.
//   - repay(t) ends the loan and returns true; repaying an unknown or already
//     repaid token returns false (no state change).
//   - loans() counts live loans; writing() reports an active exclusive loan.
//   - Tokens are plain values (copyable): identity is the generation id, so
//     copies of a token alias the same loan and double repay is caught.
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

  std::optional<Token> borrow() {
    if (writer_) return std::nullopt;
    const Token t{next_id_++, false};
    loans_.push_back({t.id, false, true});
    return t;
  }

  std::optional<Token> borrow_mut() {
    if (writer_ || !loans_.empty()) return std::nullopt;
    const Token t{next_id_++, true};
    loans_.push_back({t.id, true, true});
    writer_ = true;
    return t;
  }

  const T& access(const Token& t) const {
    const Record* r = find(t);
    if (r == nullptr || !r->live || r->mut != t.mut || t.id == 0)
      throw std::invalid_argument("LoanBox: dead or unknown token");
    return value_;
  }

  T& access_mut(const Token& t) {
    if (!t.mut) throw std::invalid_argument("LoanBox: not a mutable token");
    Record* r = find(t);
    if (r == nullptr || !r->live)
      throw std::invalid_argument("LoanBox: dead or unknown token");
    return value_;
  }

  bool repay(const Token& t) {
    Record* r = find(t);
    if (r == nullptr || !r->live || r->mut != t.mut || t.id == 0) return false;
    r->live = false;
    if (t.mut) {
      writer_ = false;
    }
    prune();
    return true;
  }

  std::size_t loans() const noexcept { return loans_.size(); }
  bool writing() const noexcept { return writer_; }

 private:
  struct Record {
    std::uint64_t id;
    bool mut;
    bool live;
  };

  Record* find(const Token& t) noexcept {
    for (auto& r : loans_)
      if (r.id == t.id) return &r;
    return nullptr;
  }
  const Record* find(const Token& t) const noexcept {
    for (const auto& r : loans_)
      if (r.id == t.id) return &r;
    return nullptr;
  }
  // Drop dead records so loans() counts *live* loans only.
  void prune() {
    std::size_t kept = 0;
    for (std::size_t i = 0; i < loans_.size(); ++i) {
      if (loans_[i].live) loans_[kept++] = loans_[i];
    }
    loans_.resize(kept);
  }

  T value_;
  std::vector<Record> loans_;
  std::uint64_t next_id_ = 1;
  bool writer_ = false;
};

#endif  // EXERCISE47_LOAN_TOKENS_H_
```
