#include "../../include/profiling/performance_profiler.h"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <thread>
#include <sstream>
#include <utility>
#include <vector>
#include <string>
#include <mutex>
#include <chrono>
#include <map>

namespace dds {
namespace profiling {

PerformanceProfiler* PerformanceProfiler::instance_ = nullptr;

PerformanceProfiler& PerformanceProfiler::getInstance() {
    static std::mutex instance_mutex;
    std::lock_guard<std::mutex> lock(instance_mutex);
    if (!instance_) {
        instance_ = new PerformanceProfiler();
    }
    return *instance_;
}

ProfileTimer::ProfileTimer(const std::string& function_name)
    : function_name_(function_name), start_time_(std::chrono::high_resolution_clock::now()) {
    if (PerformanceProfiler::getInstance().is_enabled()) {
        PerformanceProfiler::getInstance().start_measurement(function_name_);
    }
}

ProfileTimer::~ProfileTimer() {
    if (PerformanceProfiler::getInstance().is_enabled()) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);

        PerformanceMeasurement measurement{
            function_name_,
            start_time_,
            end_time,
            duration,
            PerformanceProfiler::getInstance().get_current_memory_usage(),
            PerformanceProfiler::getInstance().get_current_thread_id()
        };

        PerformanceProfiler::getInstance().record_measurement(measurement);
    }
}

void PerformanceProfiler::start_measurement(const std::string& /*function_name*/) {
    if (!enabled_) return;
    // Start timing is handled by ProfileTimer
}

void PerformanceProfiler::end_measurement(const std::string& /*function_name*/) {
    if (!enabled_) return;
    // End timing is handled by ProfileTimer destructor
}

void PerformanceProfiler::record_measurement(const PerformanceMeasurement& measurement) {
    if (!enabled_) return;

    std::lock_guard<std::mutex> lock(profiler_mutex_);

    measurements_.push_back(measurement);
    update_stats(measurement);

    if (measurements_.size() > max_measurements_) {
        measurements_.erase(measurements_.begin(), measurements_.begin() + (measurements_.size() - max_measurements_));
    }
}

std::vector<PerformanceStats> PerformanceProfiler::get_function_stats() const {
    std::lock_guard<std::mutex> lock(profiler_mutex_);
    std::vector<PerformanceStats> stats;
    stats.reserve(stats_.size());
    for (const auto& pair : stats_) {
        stats.push_back(pair.second);
    }
    return stats;
}

PerformanceStats PerformanceProfiler::get_function_stat(const std::string& function_name) const {
    std::lock_guard<std::mutex> lock(profiler_mutex_);
    auto it = stats_.find(function_name);
    return (it != stats_.end()) ? it->second : PerformanceStats{};
}

std::vector<PerformanceMeasurement> PerformanceProfiler::get_recent_measurements(size_t count) const {
    std::lock_guard<std::mutex> lock(profiler_mutex_);
    if (measurements_.size() <= count) {
        return measurements_;
    }
    return std::vector<PerformanceMeasurement>(measurements_.end() - count, measurements_.end());
}

std::vector<std::string> PerformanceProfiler::get_slowest_functions(size_t count) const {
    std::lock_guard<std::mutex> lock(profiler_mutex_);
    std::vector<std::pair<std::string, std::chrono::microseconds>> function_times;
    function_times.reserve(stats_.size());
    for (const auto& pair : stats_) {
        function_times.emplace_back(pair.first, pair.second.avg_time);
    }
    std::sort(function_times.begin(), function_times.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    std::vector<std::string> result;
    result.reserve(std::min(count, function_times.size()));
    for (size_t i = 0; i < std::min(count, function_times.size()); ++i) {
        result.push_back(function_times[i].first);
    }
    return result;
}

std::vector<std::string> PerformanceProfiler::get_most_called_functions(size_t count) const {
    std::lock_guard<std::mutex> lock(profiler_mutex_);
    std::vector<std::pair<std::string, size_t>> function_calls;
    function_calls.reserve(stats_.size());
    for (const auto& pair : stats_) {
        function_calls.emplace_back(pair.first, pair.second.call_count);
    }
    std::sort(function_calls.begin(), function_calls.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    std::vector<std::string> result;
    result.reserve(std::min(count, function_calls.size()));
    for (size_t i = 0; i < std::min(count, function_calls.size()); ++i) {
        result.push_back(function_calls[i].first);
    }
    return result;
}

double PerformanceProfiler::get_total_execution_time() const {
    std::lock_guard<std::mutex> lock(profiler_mutex_);
    std::chrono::microseconds total{0};
    for (const auto& pair : stats_) {
        total += pair.second.total_time;
    }
    return static_cast<double>(total.count()) / 1000.0;
}

void PerformanceProfiler::print_performance_report() const {
    std::cout << "\n🔍 Performance Profiler Report\n";
    std::cout << "================================\n";
    std::cout << "Total measurements: " << measurements_.size() << '\n';
    std::cout << "Total execution time: " << get_total_execution_time() << " ms\n";
    std::cout << "\nTop 5 slowest functions:\n";
    auto slowest = get_slowest_functions(5);
    for (const auto& func : slowest) {
        auto stat = get_function_stat(func);
        std::cout << "  • " << func << ": " << stat.avg_time.count() << " μs avg ("
                  << stat.call_count << " calls)\n";
    }
    std::cout << "\nTop 5 most called functions:\n";
    auto most_called = get_most_called_functions(5);
    for (const auto& func : most_called) {
        auto stat = get_function_stat(func);
        std::cout << "  • " << func << ": " << stat.call_count << " calls ("
                  << stat.avg_time.count() << " μs avg)\n";
    }
}

void PerformanceProfiler::print_function_summary(const std::string& function_name) const {
    auto stat = get_function_stat(function_name);
    if (stat.call_count == 0) {
        std::cout << "No data for function: " << function_name << '\n';
        return;
    }
    std::cout << "\n📊 Function: " << function_name << '\n';
    std::cout << "Calls: " << stat.call_count << '\n';
    std::cout << "Total time: " << stat.total_time.count() << " μs\n";
    std::cout << "Average time: " << stat.avg_time.count() << " μs\n";
    std::cout << "Min time: " << stat.min_time.count() << " μs\n";
    std::cout << "Max time: " << stat.max_time.count() << " μs\n";
    std::cout << "Average memory: " << stat.avg_memory << " bytes\n";
}

void PerformanceProfiler::export_to_csv(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cout << "Failed to open file: " << filename << '\n';
        return;
    }
    file << "Function,Calls,TotalTime(μs),AvgTime(μs),MinTime(μs),MaxTime(μs),AvgMemory(bytes)\n";
    for (const auto& pair : stats_) {
        const auto& stat = pair.second;
        file << stat.function_name << ',' << stat.call_count << ','
             << stat.total_time.count() << ',' << stat.avg_time.count() << ','
             << stat.min_time.count() << ',' << stat.max_time.count() << ','
             << stat.avg_memory << '\n';
    }
    std::cout << "Performance data exported to: " << filename << '\n';
}

void PerformanceProfiler::clear_measurements() {
    std::lock_guard<std::mutex> lock(profiler_mutex_);
    measurements_.clear();
    stats_.clear();
}

void PerformanceProfiler::update_stats(const PerformanceMeasurement& measurement) {
    auto& stat = stats_[measurement.function_name];
    if (stat.call_count == 0) {
        stat.function_name = measurement.function_name;
        stat.min_time = measurement.duration;
        stat.max_time = measurement.duration;
    }
    stat.call_count++;
    stat.total_time += measurement.duration;
    stat.avg_time = std::chrono::microseconds(stat.total_time.count() / stat.call_count);
    stat.min_time = std::min(stat.min_time, measurement.duration);
    stat.max_time = std::max(stat.max_time, measurement.duration);
    stat.total_memory += measurement.memory_used;
    stat.avg_memory = stat.total_memory / stat.call_count;
}

size_t PerformanceProfiler::get_current_memory_usage() const {
    return 1024; // Placeholder
}

std::string PerformanceProfiler::get_current_thread_id() const {
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    return oss.str();
}

} // namespace profiling
} // namespace dds
