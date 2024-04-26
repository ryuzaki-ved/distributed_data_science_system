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
    
    // Alert system
    std::vector<Alert> alerts_;
    std::mutex alerts_mutex_;
    std::vector<AlertHandler> alert_handlers_;
    PerformanceThreshold thresholds_;
    bool alerting_enabled_;
    
    // Performance tracking
    std::map<std::string, std::vector<double>> performance_history_;
    std::mutex performance_mutex_;
    
    // Health checking
    std::atomic<bool> system_healthy_;
    std::chrono::system_clock::time_point last_health_check_;

public:
    SystemMonitor(std::chrono::milliseconds interval = std::chrono::milliseconds(1000));
    ~SystemMonitor();
    
    void start();
    void stop();
    bool is_running() const { return running_; }
    
    SystemMetrics get_current_metrics();
    std::vector<SystemMetrics> get_metrics_history(int limit = 100);
    
    // Alert management
    void enable_alerting(bool enabled = true);
    void set_thresholds(const PerformanceThreshold& thresholds);
    void add_alert_handler(AlertHandler handler);
    void trigger_alert(AlertType type, AlertSeverity severity, const std::string& message, const std::string& details = "");
    std::vector<Alert> get_active_alerts();
    void acknowledge_alert(size_t alert_index, const std::string& user = "system");
    void clear_acknowledged_alerts();
    
    // Performance tracking
    void record_performance_metric(const std::string& metric_name, double value);
    double get_average_performance(const std::string& metric_name, int samples = 10);
    std::map<std::string, double> get_performance_summary();
    
    // Health monitoring
    bool is_system_healthy();
    void perform_health_check();
    std::string get_health_report();
    
    // Advanced monitoring
    void monitor_job_performance(const std::string& job_id, double execution_time);
    void monitor_resource_usage(const std::string& resource, double usage);
    void monitor_error_rate(const std::string& component, int errors, int total_requests);
    
    // Predictive analytics
    bool predict_resource_exhaustion(const std::string& resource, std::chrono::minutes time_window);
    double calculate_trend(const std::string& metric, int samples = 20);
    std::vector<std::string> get_performance_recommendations();
    
private:
    void monitoring_loop();
    SystemMetrics collect_system_metrics();
    double get_cpu_usage();
    double get_memory_usage();
    double get_disk_usage();
    double get_gpu_usage();
    double get_network_io();
    double get_disk_io();
    
    // Alert processing
    void process_alerts(const SystemMetrics& metrics);
    void send_alert_notifications(const Alert& alert);
    
    // Performance analysis
    void analyze_performance_trends();
    void detect_anomalies();
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