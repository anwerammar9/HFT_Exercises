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