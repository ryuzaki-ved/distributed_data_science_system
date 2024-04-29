#pragma once

#include <chrono>
#include <thread>
#include <random>
#include <stdexcept>
#include <functional>

namespace dds {
namespace utils {

// Header-only retry helper with exponential backoff and optional jitter
struct RetryOptions {
    int max_attempts = 3;
    std::chrono::milliseconds initial_delay{100};
    double backoff_multiplier = 2.0;  // Delay *= multiplier after each failure
    bool jitter = true;               // Add small random jitter to avoid thundering herd
};

inline std::chrono::milliseconds apply_jitter(std::chrono::milliseconds base, bool jitter_enabled) {
    if (!jitter_enabled || base.count() == 0) return base;
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(-static_cast<int>(base.count()) / 10,
                                            static_cast<int>(base.count()) / 10);
    return std::chrono::milliseconds(base.count() + dist(rng));
}

// Runs the callable and retries on exceptions up to max_attempts
// Returns the callable's return value or rethrows the last exception
// Usage: auto result = run_with_retry([&]{ return might_throw(); }, {5, 200ms, 2.0, true});
template <typename Callable>
auto run_with_retry(Callable&& callable, RetryOptions options = {}) -> decltype(callable()) {
    using namespace std::chrono;

    std::exception_ptr last_exception;
    milliseconds delay = options.initial_delay;

    for (int attempt = 1; attempt <= options.max_attempts; ++attempt) {
        try {
            return callable();
        } catch (...) {
            last_exception = std::current_exception();
            if (attempt == options.max_attempts) break;
            auto sleep_for = apply_jitter(delay, options.jitter);
            std::this_thread::sleep_for(sleep_for);
            delay = milliseconds(static_cast<long long>(delay.count() * options.backoff_multiplier));
        }
    }
    std::rethrow_exception(last_exception);
}

// Convenience overload to accept a function and an on-retry callback (receives attempt index)
template <typename Callable, typename OnRetry>
auto run_with_retry(Callable&& callable, OnRetry&& on_retry, RetryOptions options) -> decltype(callable()) {
    using namespace std::chrono;

    std::exception_ptr last_exception;
    milliseconds delay = options.initial_delay;

    for (int attempt = 1; attempt <= options.max_attempts; ++attempt) {
        try {
            return callable();
        } catch (...) {
            last_exception = std::current_exception();
            if (attempt == options.max_attempts) break;
            on_retry(attempt);
            auto sleep_for = apply_jitter(delay, options.jitter);
            std::this_thread::sleep_for(sleep_for);
            delay = milliseconds(static_cast<long long>(delay.count() * options.backoff_multiplier));
        }
    }
    std::rethrow_exception(last_exception);
}

} // namespace utils
} // namespace dds
