# Exercise 33 — Factory (Order Construction) (Reference Solution)

**What you implement:** an abstract factory for venue-specific order objects.
Construction is centralized so validation and price-tick rounding happen once,
in the factory, instead of being re-implemented (and forgotten) by every caller.

**Approach**
- `VenueOrderFactory` is the abstract factory; `IexOrderFactory` and
  `CmeOrderFactory` differ only in their price tick:
  `kIexTickMicros = 10'000` (0.01) vs `kCmeTickMicros = 2'500` (0.0025).
- One shared `build_order(...)` helper: **validate first** (`id < 0`, `qty <= 0`,
  non-positive price for limit/stop → `nullptr`), then fill the struct. Every
  `create_*` override is a two-line wrapper passing its `OrderKind` + tick.
- Price rounding is exact **integer** math (`(price + tick/2) / tick * tick`,
  round-half-up), so `12.344444` quantizes deterministically to 12.34 (IEX)
  and 12.345 (CME) — the discriminator test.
- The factory owns the invariants the tests assert: a market order always has
  `price == 0` and `post_only == false`; a stop is never `post_only`.
- `make_order_factory(venue)` is the tiny factory-of-factories on top.

## Reference API — `include/factory_orders.h`
#ifndef EXERCISE33_FACTORY_ORDERS_H_
#define EXERCISE33_FACTORY_ORDERS_H_

#include <cstdint>
#include <memory>

// Abstract factory for venue-specific order objects.
//
// Contract:
//   - The concrete factories (IexOrderFactory / CmeOrderFactory) are the ONLY
//     way to build an Order here. A request is validated before any Order is
//     produced; invalid requests return nullptr (qty <= 0, a missing/zero
//     price for limit & stop, negative ids).
//   - Each venue enforces its own price grid: IEX trades in whole cents
//     (tick = 10,000 micros), CME in a minimum tick of 0.0025 (2,500 micros).
//     create_limit/create_stop round the requested price up-half to the venue
//     tick, so the same request lands on different grids for different venues.
//   - A market order always carries price == 0; a stop order carries its
//     trigger price as `price`.
//
// TODO(anwer): implement (see SOLUTION.md). Stub: every create_* returns
// nullptr and make_order_factory returns venue-correct factories whose
// create_* still return nullptr -> the rounding and validation tests run RED.

using PriceMicros = std::int64_t;  // price in 1e-6 units ("micros")

enum class Venue { kIex, kCme };
enum class Side { kBuy, kSell };
enum class OrderKind { kLimit, kMarket, kStop };
enum class TimeInForce { kDay, kIoc, kFok };

struct Order {
  OrderKind kind = OrderKind::kLimit;
  Venue venue = Venue::kIex;
  Side side = Side::kBuy;
  std::int64_t id = 0;
  PriceMicros price = 0;  // limit: limit price; stop: trigger; market: 0
  std::int64_t qty = 0;
  TimeInForce tif = TimeInForce::kDay;
  bool post_only = false;
};

// Prices are quantized to these venue ticks at construction time.
inline constexpr PriceMicros kIexTickMicros = 10'000;  // 0.01 (one cent)
inline constexpr PriceMicros kCmeTickMicros = 2'500;   // 0.0025

class VenueOrderFactory {
 public:
  virtual ~VenueOrderFactory() = default;

  virtual std::unique_ptr<Order> create_limit(std::int64_t id, Side side,
                                              PriceMicros price,
                                              std::int64_t qty,
                                              TimeInForce tif,
                                              bool post_only) const = 0;
  virtual std::unique_ptr<Order> create_market(std::int64_t id, Side side,
                                               std::int64_t qty) const = 0;
  virtual std::unique_ptr<Order> create_stop(std::int64_t id, Side side,
                                             PriceMicros trigger,
                                             std::int64_t qty) const = 0;
  virtual Venue venue() const noexcept = 0;
};

class IexOrderFactory final : public VenueOrderFactory {
 public:
  std::unique_ptr<Order> create_limit(std::int64_t id, Side side,
                                      PriceMicros price, std::int64_t qty,
                                      TimeInForce tif,
                                      bool post_only) const override;
  std::unique_ptr<Order> create_market(std::int64_t id, Side side,
                                       std::int64_t qty) const override;
  std::unique_ptr<Order> create_stop(std::int64_t id, Side side,
                                     PriceMicros trigger,
                                     std::int64_t qty) const override;
  Venue venue() const noexcept override { return Venue::kIex; }
};

class CmeOrderFactory final : public VenueOrderFactory {
 public:
  std::unique_ptr<Order> create_limit(std::int64_t id, Side side,
                                      PriceMicros price, std::int64_t qty,
                                      TimeInForce tif,
                                      bool post_only) const override;
  std::unique_ptr<Order> create_market(std::int64_t id, Side side,
                                       std::int64_t qty) const override;
  std::unique_ptr<Order> create_stop(std::int64_t id, Side side,
                                     PriceMicros trigger,
                                     std::int64_t qty) const override;
  Venue venue() const noexcept override { return Venue::kCme; }
};

// Simple factory-of-factories: returns the concrete factory for a venue.
std::unique_ptr<VenueOrderFactory> make_order_factory(Venue venue);

#endif  // EXERCISE33_FACTORY_ORDERS_H_

## Reference implementation — `src/factory_orders.cpp`
#include "factory_orders.h"

namespace {

// Round half up to the nearest tick. Inputs are non-negative integers, so the
// classic (value + tick/2) / tick * tick integer formula is exact.
PriceMicros quantize(PriceMicros price, PriceMicros tick) {
  return ((price + tick / 2) / tick) * tick;
}

// Shared construction path: validate, then produce a venue-consistent Order.
// The factory pattern invites exactly this enforced-invariant creation; the
// caller cannot forget to round a price, because they never build an Order.
std::unique_ptr<Order> build_order(const VenueOrderFactory& factory,
                                   OrderKind kind, Side side,
                                   std::int64_t id, PriceMicros price,
                                   std::int64_t qty, TimeInForce tif,
                                   bool post_only, PriceMicros tick) {
  if (id < 0 || qty <= 0) return nullptr;
  if (kind != OrderKind::kMarket && price <= 0) return nullptr;

  auto order = std::make_unique<Order>();
  order->kind = kind;
  order->venue = factory.venue();
  order->side = side;
  order->id = id;
  order->price =
      (kind == OrderKind::kMarket) ? PriceMicros{0} : quantize(price, tick);
  order->qty = qty;
  order->tif = tif;
  order->post_only = post_only && kind != OrderKind::kStop;
  return order;
}

}  // namespace

std::unique_ptr<Order> IexOrderFactory::create_limit(std::int64_t id,
                                                     Side side,
                                                     PriceMicros price,
                                                     std::int64_t qty,
                                                     TimeInForce tif,
                                                     bool post_only) const {
  return build_order(*this, OrderKind::kLimit, side, id, price, qty, tif,
                     post_only, kIexTickMicros);
}

std::unique_ptr<Order> IexOrderFactory::create_market(std::int64_t id,
                                                      Side side,
                                                      std::int64_t qty) const {
  return build_order(*this, OrderKind::kMarket, side, id, 0, qty,
                     TimeInForce::kDay, false, kIexTickMicros);
}

std::unique_ptr<Order> IexOrderFactory::create_stop(std::int64_t id, Side side,
                                                    PriceMicros trigger,
                                                    std::int64_t qty) const {
  return build_order(*this, OrderKind::kStop, side, id, trigger, qty,
                     TimeInForce::kDay, false, kIexTickMicros);
}

std::unique_ptr<Order> CmeOrderFactory::create_limit(std::int64_t id,
                                                     Side side,
                                                     PriceMicros price,
                                                     std::int64_t qty,
                                                     TimeInForce tif,
                                                     bool post_only) const {
  return build_order(*this, OrderKind::kLimit, side, id, price, qty, tif,
                     post_only, kCmeTickMicros);
}

std::unique_ptr<Order> CmeOrderFactory::create_market(std::int64_t id,
                                                      Side side,
                                                      std::int64_t qty) const {
  return build_order(*this, OrderKind::kMarket, side, id, 0, qty,
                     TimeInForce::kDay, false, kCmeTickMicros);
}

std::unique_ptr<Order> CmeOrderFactory::create_stop(std::int64_t id, Side side,
                                                    PriceMicros trigger,
                                                    std::int64_t qty) const {
  return build_order(*this, OrderKind::kStop, side, id, trigger, qty,
                     TimeInForce::kDay, false, kCmeTickMicros);
}

std::unique_ptr<VenueOrderFactory> make_order_factory(Venue venue) {
  if (venue == Venue::kIex) return std::make_unique<IexOrderFactory>();
  return std::make_unique<CmeOrderFactory>();
}

**How the tests verify you:** the rounding tests assert a *different* price for
the same input across IEX/CME (the integer half-up quantizer is the
discriminator), the validation tests demand `nullptr` for qty/price garbage,
and the market/stop tests check the invariants (`price == 0`, trigger carried,
no post-only stop). `make_order_factory` must hand back venue-correct rounding.
The stub returns `nullptr` everywhere it matters, keeping the suite RED until
the factory is real.