#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <mutex>

namespace dds {
namespace profiling {

// Performance measurement data
struct PerformanceMeasurement {
    std::string function_name;
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;
    std::chrono::microseconds duration;
    size_t memory_used;
    std::string thread_id;
};

// Performance statistics
struct PerformanceStats {
    std::string function_name;
    size_t call_count;
    std::chrono::microseconds total_time;
    std::chrono::microseconds avg_time;
    std::chrono::microseconds min_time;
    std::chrono::microseconds max_time;
    size_t total_memory;
    size_t avg_memory;
};

// RAII timer for automatic profiling
class ProfileTimer {
private:
    std::string function_name_;
    std::chrono::high_resolution_clock::time_point start_time_;
    
public:
    ProfileTimer(const std::string& function_name);
    ~ProfileTimer();
};

// Main performance profiler
class PerformanceProfiler {
private:
    static PerformanceProfiler* instance_;
    std::vector<PerformanceMeasurement> measurements_;
    std::unordered_map<std::string, PerformanceStats> stats_;
    std::mutex profiler_mutex_;
    bool enabled_;
    size_t max_measurements_;

public:
    static PerformanceProfiler& getInstance();
    
    // Control profiling
    void enable() { enabled_ = true; }
    void disable() { enabled_ = false; }
    bool is_enabled() const { return enabled_; }
    
    // Record measurements
    void start_measurement(const std::string& function_name);
    void end_measurement(const std::string& function_name);
    void record_measurement(const PerformanceMeasurement& measurement);
    
    // Get statistics
    std::vector<PerformanceStats> get_function_stats() const;
    PerformanceStats get_function_stat(const std::string& function_name) const;
    std::vector<PerformanceMeasurement> get_recent_measurements(size_t count = 100) const;
    
    // Analysis
    std::vector<std::string> get_slowest_functions(size_t count = 10) const;
    std::vector<std::string> get_most_called_functions(size_t count = 10) const;
    double get_total_execution_time() const;
    
    // Reporting
    void print_performance_report() const;
    void print_function_summary(const std::string& function_name) const;
    void export_to_csv(const std::string& filename) const;
    
    // Cleanup
    void clear_measurements();
    void set_max_measurements(size_t max_count) { max_measurements_ = max_count; }

private:
    PerformanceProfiler() : enabled_(true), max_measurements_(10000) {}
    void update_stats(const PerformanceMeasurement& measurement);
    size_t get_current_memory_usage() const;
    std::string get_current_thread_id() const;
};

// Macro for easy profiling
#define PROFILE_FUNCTION() dds::profiling::ProfileTimer _prof_timer(__FUNCTION__)
#define PROFILE_SCOPE(name) dds::profiling::ProfileTimer _prof_timer(name)

} // namespace profiling
} // namespace dds
