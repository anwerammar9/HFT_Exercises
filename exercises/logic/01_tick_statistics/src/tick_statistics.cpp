#include "tick_statistics.h"

void TickStatistics::add_tick(double price, std::uint64_t qty)
{
    if (count_ == 0)
    {
        min_ = price;
        max_ = price;
    }
    else
    {
        if (price < min_) min_ = price;
        if (price > max_) max_ = price;
    }
    ++count_;
    last_ = price;
    sum_ += price;
    sum_sq_ += price * price;
    pq_sum_ += price * static_cast<double>(qty);
    q_sum_ += qty;
    prices_.push_back(price);
}

double TickStatistics::mean() const noexcept
{
    if (count_ == 0) return 0.0;
    return sum_ / static_cast<double>(count_);
}

double TickStatistics::variance() const noexcept
{
    if (count_ == 0) return 0.0;
    const double mu = mean();
    double var = (sum_sq_ / static_cast<double>(count_)) - (mu * mu);
    // Clamp tiny negative values caused by floating-point cancellation.
    if (var < 0.0 && var > -1e-12) var = 0.0;
    return var;
}

double TickStatistics::vwap() const noexcept
{
    if (q_sum_ == 0) return 0.0;
    return pq_sum_ / static_cast<double>(q_sum_);
}

double TickStatistics::ewma(double alpha) const noexcept
{
    if (prices_.empty()) return 0.0;
    double ema = prices_.front();
    for (std::size_t i = 1; i < prices_.size(); ++i)
    {
        ema = alpha * prices_[i] + (1.0 - alpha) * ema;
    }
    return ema;
}