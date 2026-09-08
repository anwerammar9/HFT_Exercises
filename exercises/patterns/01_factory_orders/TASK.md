# Exercise patterns/01_factory_orders (ex33) — Factory (Order Construction) (Task)

## The problem (in plain words)

An order-entry client has to manufacture many kinds of orders — limit, market,
stop — for different venues, each with its own validation rules and price grid
(IEX trades in whole cents, CME in 0.0025 minimum ticks). If every caller
builds `Order` structs by hand, they will forget to validate the qty, forget to
round the price to the venue tick, or mix up which fields a market order should
have. The **Factory** pattern makes the *construction* path explicit: you ask a
venue-specific factory for an order and it returns a validated, correctly-typed
object — or `nullptr` when the request is nonsense.

## Requirements (what the tests check)

1. `create_limit(id, side, price, qty, tif, post_only)` builds a `Limit` order
   whose price has been **quantized up-half** to the venue tick:
   - `IexOrderFactory` quantizes to `kIexTickMicros = 10'000` (0.01);
   - `CmeOrderFactory` quantizes to `kCmeTickMicros = 2'500` (0.0025).
   The same input therefore lands on *different* prices for the two venues
   (12.344444 → 12.34 at IEX but 12.345 at CME).
2. `create_stop(...)` carries the trigger price (also quantized); a stop is
   never `post_only`.
3. `create_market(id, side, qty)` yields a `Market` order with `price == 0` and
   `post_only == false`.
4. Invalid requests return `nullptr` (never a half-built order): `qty <= 0`,
   `id < 0`, and, for limit/stop only, `price <= 0`.
5. `venue()` reports the venue the factory normalizes for.
6. `make_order_factory(Venue)` is a simple factory-of-factories returning the
   concrete factory for a venue, so callers can select the grid by name.

## Public API

```cpp
using PriceMicros = std::int64_t;   // prices in 1e-6 units
enum class Venue { kIex, kCme };
enum class Side { kBuy, kSell };
enum class OrderKind { kLimit, kMarket, kStop };
enum class TimeInForce { kDay, kIoc, kFok };

struct Order {
  OrderKind kind; Venue venue; Side side; std::int64_t id;
  PriceMicros price; std::int64_t qty; TimeInForce tif; bool post_only;
};

class VenueOrderFactory {              // abstract factory interface
  virtual std::unique_ptr<Order> create_limit(std::int64_t id, Side side,
      PriceMicros price, std::int64_t qty, TimeInForce tif, bool post_only) const = 0;
  virtual std::unique_ptr<Order> create_market(std::int64_t id, Side side,
      std::int64_t qty) const = 0;
  virtual std::unique_ptr<Order> create_stop(std::int64_t id, Side side,
      PriceMicros trigger, std::int64_t qty) const = 0;
  virtual Venue venue() const noexcept = 0;
};

class IexOrderFactory final : public VenueOrderFactory { /* ... */ };
class CmeOrderFactory final : public VenueOrderFactory { /* ... */ };
std::unique_ptr<VenueOrderFactory> make_order_factory(Venue venue);
```

## How to think about it (suggested design)

- One shared `build_order(factory, kind, side, id, price, qty, ...)` helper in
  the `.cpp`: validate first, then fill the struct. Every `create_*` method is
  a thin wrapper that supplies its `OrderKind` and the venue tick.
- Rounding is **integer** math (`(price + tick/2) / tick * tick`) — never
  float, so `12.344444` quantizes deterministically.
- This is the contract-holding factory: the invariant "a market order never has
  a price" is enforced *once*, in the factory, not by every caller.

## Make it harder (optional — not covered by the tests)

- **Registry + config load:** a `make_order_factory("IEX")` name-based registry
  driven by a config file, so venues can be added without touching callers.
- **Prototype:** add `clone()` to `Order` (a per-kind copy) so child orders can
  be forked from a parent template cheaply.
- **Builder for composite orders:** a `ComboOrderBuilder` that assembles the
  legs of a spread order from simple factory products.
- **Session-aware factories:** factories bound to a session id that stamp the
  order's session and reject orders for a session that is not yet open.

## Files

- Stub: `src/factory_orders.cpp`
- Tests: `test/test_factory_orders.cpp`
- Reference: `SOLUTION.md`