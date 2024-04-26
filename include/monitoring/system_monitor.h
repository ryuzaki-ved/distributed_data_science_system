#pragma once

#include "../utils/types.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>

namespace dds {
namespace monitoring {

// System metrics
struct SystemMetrics {
    double cpu_usage;
    double memory_usage;
    double disk_usage;
    double gpu_usage;
    double network_io;
    double disk_io;
    int active_jobs;
    int completed_jobs;
    int failed_jobs;
    int queue_size;
    double response_time_avg;
    int active_connections;
    std::chrono::system_clock::time_point timestamp;
};

// Alert types
enum class AlertType {
    CPU_HIGH,
    MEMORY_HIGH,
    DISK_FULL,
    GPU_HIGH,
    JOB_FAILED,
    RESPONSE_TIME_HIGH,
    CONNECTION_LIMIT,
    SYSTEM_ERROR
};

// Alert severity
enum class AlertSeverity {
    LOW,
    MEDIUM,
    HIGH,
    CRITICAL
};

// Alert structure
struct Alert {
    AlertType type;
    AlertSeverity severity;
    std::string message;
    std::string details;
    std::chrono::system_clock::time_point timestamp;
    bool acknowledged;
    std::string acknowledged_by;
};

// Alert handler function type
using AlertHandler = std::function<void(const Alert&)>;

// Performance threshold
struct PerformanceThreshold {
    double cpu_warning = 80.0;
    double cpu_critical = 95.0;
    double memory_warning = 85.0;
    double memory_critical = 95.0;
    double disk_warning = 90.0;
    double disk_critical = 98.0;
    double gpu_warning = 85.0;
    double gpu_critical = 95.0;
    double response_time_warning = 1000.0; // ms
    double response_time_critical = 5000.0; // ms
};

// Log levels
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

// System Monitor
class SystemMonitor {
private:
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> monitor_thread_;
    std::chrono::milliseconds monitoring_interval_;
    std::vector<SystemMetrics> metrics_history_;
    std::mutex metrics_mutex_;

public:
    SystemMonitor(std::chrono::milliseconds interval = std::chrono::milliseconds(1000));
    ~SystemMonitor();
    
    void start();
    void stop();
    bool is_running() const { return running_; }
    
    SystemMetrics get_current_metrics();
    std::vector<SystemMetrics> get_metrics_history(int limit = 100);
    
private:
    void monitoring_loop();
    SystemMetrics collect_system_metrics();
    double get_cpu_usage();
    double get_memory_usage();
    double get_disk_usage();
};

// Logger
class Logger {
private:
    std::string log_file_;
    LogLevel min_level_;
    std::mutex log_mutex_;
    bool console_output_;
    bool file_output_;

public:
    Logger(const std::string& log_file = "dds_system.log", 
           LogLevel min_level = LogLevel::INFO);
    
    void log(LogLevel level, const std::string& message, 
             const std::string& component = "");
    
    void info(const std::string& message, const std::string& component = "");
    void warning(const std::string& message, const std::string& component = "");
    void error(const std::string& message, const std::string& component = "");
    
private:
    std::string format_log_entry(LogLevel level, const std::string& message, 
                                 const std::string& component);
    std::string level_to_string(LogLevel level);
};

} // namespace monitoring
} // namespace dds 