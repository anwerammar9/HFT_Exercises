#include "factory_orders.h"

#include <memory>

// TODO(anwer): implement the factories (see SOLUTION.md).
//
// Suggested shape: a single build_order(...) helper — validate (id >= 0,
// qty > 0, price > 0 for limit/stop), then fill an Order with the price
// quantized via ((price + tick/2) / tick) * tick to the venue tick
// (kIexTickMicros / kCmeTickMicros). Market orders always price == 0.
//
// Stub: every create_* returns nullptr (nothing is ever built), so the
// rounding and validation tests run RED without hanging or crashing.

std::unique_ptr<Order> IexOrderFactory::create_limit(std::int64_t /*id*/,
                                                     Side /*side*/,
                                                     PriceMicros /*price*/,
                                                     std::int64_t /*qty*/,
                                                     TimeInForce /*tif*/,
                                                     bool /*post_only*/) const {
  return nullptr;
}

std::unique_ptr<Order> IexOrderFactory::create_market(std::int64_t /*id*/,
                                                      Side /*side*/,
                                                      std::int64_t /*qty*/) const {
  return nullptr;
}

std::unique_ptr<Order> IexOrderFactory::create_stop(std::int64_t /*id*/,
                                                    Side /*side*/,
                                                    PriceMicros /*trigger*/,
                                                    std::int64_t /*qty*/) const {
  return nullptr;
}

std::unique_ptr<Order> CmeOrderFactory::create_limit(std::int64_t /*id*/,
                                                     Side /*side*/,
                                                     PriceMicros /*price*/,
                                                     std::int64_t /*qty*/,
                                                     TimeInForce /*tif*/,
                                                     bool /*post_only*/) const {
  return nullptr;
}

std::unique_ptr<Order> CmeOrderFactory::create_market(std::int64_t /*id*/,
                                                      Side /*side*/,
                                                      std::int64_t /*qty*/) const {
  return nullptr;
}

std::unique_ptr<Order> CmeOrderFactory::create_stop(std::int64_t /*id*/,
                                                    Side /*side*/,
                                                    PriceMicros /*trigger*/,
                                                    std::int64_t /*qty*/) const {
  return nullptr;
}

std::unique_ptr<VenueOrderFactory> make_order_factory(Venue venue) {
  if (venue == Venue::kIex) return std::make_unique<IexOrderFactory>();
  return std::make_unique<CmeOrderFactory>();
}