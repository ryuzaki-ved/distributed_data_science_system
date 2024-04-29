#pragma once

#include <chrono>
#include <mutex>
#include <atomic>

namespace dds {
namespace utils {

// Simple thread-safe token-bucket rate limiter (header-only)
class RateLimiter {
private:
    double tokens_per_second_;
    double burst_size_;
    double available_tokens_;
    std::chrono::steady_clock::time_point last_refill_;
    mutable std::mutex mutex_;

public:
    // rate: tokens per second, burst: max bucket size
    RateLimiter(double rate = 10.0, double burst = 20.0)
        : tokens_per_second_(rate), burst_size_(burst), available_tokens_(burst),
          last_refill_(std::chrono::steady_clock::now()) {}

    // Try to consume N tokens; returns true if allowed
    bool allow(double tokens = 1.0) {
        std::lock_guard<std::mutex> lock(mutex_);
        refill();
        if (available_tokens_ >= tokens) {
            available_tokens_ -= tokens;
            return true;
        }
        return false;
    }

    // Adjust rate/burst at runtime
    void set_rate(double rate) {
        std::lock_guard<std::mutex> lock(mutex_);
        refill();
        tokens_per_second_ = rate;
    }

    void set_burst(double burst) {
        std::lock_guard<std::mutex> lock(mutex_);
        refill();
        burst_size_ = burst;
        if (available_tokens_ > burst_size_) available_tokens_ = burst_size_;
    }

    double get_available() const {
        std::lock_guard<std::mutex> lock(mutex_);
        const_cast<RateLimiter*>(this)->refill();
        return available_tokens_;
    }

private:
    void refill() {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - last_refill_;
        double added = elapsed.count() * tokens_per_second_;
        if (added > 0.0) {
            available_tokens_ = std::min(burst_size_, available_tokens_ + added);
            last_refill_ = now;
        }
    }
};

} // namespace utils
} // namespace dds
