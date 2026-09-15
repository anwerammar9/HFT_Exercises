# Exercise memory/06_loan_tokens (ex47) — Explicit Loan Tokens (Task)

## The problem (in plain words)

ex46's RAII guards release automatically at scope exit — convenient, but the
*discipline* is invisible. Strip the guards away: the caller now holds an
explicit `Token` per loan and must `repay()` it by hand, like checking books
out of a library. The same shared-XOR-mutable rule applies, but every misuse
(forgotten repay, double repay, stale access, forged tokens) becomes a
testable runtime event. That pain is the point: it motivates why RAII guards
exist.

## Requirements (what the tests check)

1. A fresh box is free: `loans() == 0`, `writing()` false.
2. `borrow()` issues a shared token iff no writer is active; any number may
   coexist and each resolves via `access()`.
3. `borrow_mut()` issues an exclusive token iff no loan of any kind is
   active; while it lives, both `borrow()` and `borrow_mut()` fail.
4. A live shared loan makes `borrow_mut()` fail; after repay it succeeds.
5. Mutation via `access_mut()` persists and is visible to later loans.
6. Double `repay()` of one token: first true, second false, counts exact.
7. Token copies alias one loan: repaying through a copy kills the original
   (stale `access()` throws `std::invalid_argument`).
8. Forged (`id 9999`) and never-issued (`id 0`) tokens: `access` /
   `access_mut` throw, `repay` returns false, state untouched.
9. Kind mismatch: `access_mut()` on a shared token throws.
10. A 10-round borrow/borrow/write interleave keeps exact counts throughout.

## Public API

```cpp
template <typename T> class LoanBox {
  struct Token { std::uint64_t id = 0; bool mut = false; };  // id 0 invalid
  template <typename... Args> explicit LoanBox(Args&&... args);
  LoanBox(const LoanBox&) = delete;
  std::optional<Token> borrow();        // nullopt if writing
  std::optional<Token> borrow_mut();    // nullopt unless fully free
  const T& access(const Token&) const;  // throws invalid_argument if dead
  T& access_mut(const Token&);          // throws if dead or shared-kind
  bool repay(const Token&);             // false if unknown/already repaid
  std::size_t loans() const noexcept;   // live loans only
  bool writing() const noexcept;
};
```

The stub lives in `include/loan_tokens.h` (a template — implement inline,
replacing the `TODO(anwer)` bodies).

## How to think about it (suggested design)

- Members: `T value_`, `vector<Record{id, mut, live}>`, `next_id_` from 1,
  `bool writer_`.
- `borrow()`: refuse if `writer_`, else push a live shared record.
  `borrow_mut()`: refuse if `writer_` *or any record is live*, else push an
  exclusive record and set the flag.
- `find(id)` linear scan (loan counts are tiny; a map is overkill).
- `repay()`: validate id + kind + liveness → mark dead, clear the writer
  flag for mut, `prune()` dead records so `loans()` stays exact.
- `access()` validates then returns `value_`; `access_mut()` additionally
  requires `t.mut`.
- Single-threaded by contract.

## Make it harder (optional — not covered by the tests)

- **Lease expiry:** stamp each record with a logical-clock deadline
  (`advance()` like ex05's bucket); `access()` on an expired lease throws and
  `collect_expired()` reaps them (leases for market-data snapshots).
- **Read-vs-write counts:** split `loans()` into `readers()` / `is_writing()`
  introspection without changing the token protocol.
- **Guard adapter:** build ex46-style RAII guards *on top of* `LoanBox`
  (repay in the destructor) and prove the same test suite passes — the
  adapter is the argument for RAII.

## Files

- Stub: `include/loan_tokens.h` (template — implement inline)
- Tests: `test/test_loan_tokens.cpp`
- Reference: `SOLUTION.md`
