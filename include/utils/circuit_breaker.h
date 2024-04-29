#pragma once

#include <chrono>
#include <mutex>

namespace dds {
namespace utils {

// Simple thread-safe circuit breaker (header-only)
class CircuitBreaker {
public:
    enum class State { CLOSED, OPEN, HALF_OPEN };

private:
    int failure_threshold_;
    int half_open_success_threshold_;
    std::chrono::milliseconds open_timeout_;

    int consecutive_failures_;
    int half_open_successes_;
    State state_;

    std::chrono::steady_clock::time_point opened_at_;
    mutable std::mutex mutex_;

public:
    // failure_threshold: trips breaker after N consecutive failures
    // open_timeout: time to stay OPEN before moving to HALF_OPEN
    // half_open_success_threshold: successes needed to close after HALF_OPEN
    CircuitBreaker(int failure_threshold = 5,
                   std::chrono::milliseconds open_timeout = std::chrono::milliseconds(5000),
                   int half_open_success_threshold = 2)
        : failure_threshold_(failure_threshold),
          half_open_success_threshold_(half_open_success_threshold),
          open_timeout_(open_timeout),
          consecutive_failures_(0),
          half_open_successes_(0),
          state_(State::CLOSED) {}

    // Check if a call is allowed at this moment
    bool allow() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ == State::OPEN) {
            auto now = std::chrono::steady_clock::now();
            if (now - opened_at_ >= open_timeout_) {
                state_ = State::HALF_OPEN;
                half_open_successes_ = 0;
                return true; // probe request
            }
            return false;
        }
        return true; // CLOSED or HALF_OPEN allow calls
    }

    // Inform breaker of a successful call
    void on_success() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ == State::HALF_OPEN) {
            if (++half_open_successes_ >= half_open_success_threshold_) {
                reset();
            }
        } else {
            consecutive_failures_ = 0;
        }
    }

    // Inform breaker of a failed call
    void on_failure() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ == State::HALF_OPEN) {
            trip_open();
            return;
        }
        if (++consecutive_failures_ >= failure_threshold_) {
            trip_open();
        }
    }

    State state() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    void reset() {
        state_ = State::CLOSED;
        consecutive_failures_ = 0;
        half_open_successes_ = 0;
    }

private:
    void trip_open() {
        state_ = State::OPEN;
        opened_at_ = std::chrono::steady_clock::now();
        half_open_successes_ = 0;
    }
};

} // namespace utils
} // namespace dds
