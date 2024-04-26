#include "../../include/monitoring/system_monitor.h"
#include <iostream>

namespace dds {
namespace monitoring {

SystemMonitor::SystemMonitor(std::chrono::milliseconds interval)
    : running_(false), monitoring_interval_(interval), alerting_enabled_(true), 
      system_healthy_(true), last_health_check_(std::chrono::system_clock::now()) {
}

SystemMonitor::~SystemMonitor() {
    stop();
}

void SystemMonitor::start() {
    if (running_) return;
    running_ = true;
    std::cout << "System monitoring started" << std::endl;
}

void SystemMonitor::stop() {
    running_ = false;
    std::cout << "System monitoring stopped" << std::endl;
}

SystemMetrics SystemMonitor::get_current_metrics() {
    SystemMetrics metrics;
    metrics.cpu_usage = get_cpu_usage();
    metrics.memory_usage = get_memory_usage();
    metrics.disk_usage = get_disk_usage();
    metrics.gpu_usage = get_gpu_usage();
    metrics.network_io = get_network_io();
    metrics.disk_io = get_disk_io();
    metrics.active_jobs = 2;
    metrics.completed_jobs = 15;
    metrics.failed_jobs = 0;
    metrics.queue_size = 3;
    metrics.response_time_avg = 125.5;
    metrics.active_connections = 8;
    metrics.timestamp = std::chrono::system_clock::now();
    
    // Process alerts based on metrics
    if (alerting_enabled_) {
        process_alerts(metrics);
    }
    
    return metrics;
}

std::vector<SystemMetrics> SystemMonitor::get_metrics_history(int limit) {
    return {};
}

void SystemMonitor::monitoring_loop() {
    // Stub implementation
}

SystemMetrics SystemMonitor::collect_system_metrics() {
    return get_current_metrics();
}

double SystemMonitor::get_cpu_usage() {
    return 25.5;
}

double SystemMonitor::get_memory_usage() {
    return 45.2;
}

double SystemMonitor::get_disk_usage() {
    return 30.1;
}

// Logger implementation
Logger::Logger(const std::string& log_file, LogLevel min_level)
    : log_file_(log_file), min_level_(min_level), console_output_(true), file_output_(true) {
}

void Logger::log(LogLevel level, const std::string& message, const std::string& component) {
    std::string formatted = format_log_entry(level, message, component);
    if (console_output_) {
        std::cout << formatted << std::endl;
    }
}

void Logger::info(const std::string& message, const std::string& component) {
    log(LogLevel::INFO, message, component);
}

void Logger::warning(const std::string& message, const std::string& component) {
    log(LogLevel::WARNING, message, component);
}

void Logger::error(const std::string& message, const std::string& component) {
    log(LogLevel::ERROR, message, component);
}

std::string Logger::format_log_entry(LogLevel level, const std::string& message, const std::string& component) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::string timestamp = std::ctime(&time_t);
    timestamp.pop_back(); // Remove newline
    
    std::string level_str = level_to_string(level);
    std::string component_str = component.empty() ? "SYSTEM" : component;
    
    return "[" + timestamp + "] [" + level_str + "] [" + component_str + "] " + message;
}

std::string Logger::level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

// New SystemMonitor methods implementation
double SystemMonitor::get_gpu_usage() {
    // Simulate GPU usage
    return 83.2;
}

double SystemMonitor::get_network_io() {
    // Simulate network I/O in MB/s
    return 15.7;
}

double SystemMonitor::get_disk_io() {
    // Simulate disk I/O in MB/s
    return 8.3;
}

void SystemMonitor::enable_alerting(bool enabled) {
    alerting_enabled_ = enabled;
    std::cout << "Alerting " << (enabled ? "enabled" : "disabled") << std::endl;
}

void SystemMonitor::set_thresholds(const PerformanceThreshold& thresholds) {
    thresholds_ = thresholds;
    std::cout << "Performance thresholds updated" << std::endl;
}

void SystemMonitor::add_alert_handler(AlertHandler handler) {
    alert_handlers_.push_back(handler);
}

void SystemMonitor::trigger_alert(AlertType type, AlertSeverity severity, const std::string& message, const std::string& details) {
    Alert alert;
    alert.type = type;
    alert.severity = severity;
    alert.message = message;
    alert.details = details;
    alert.timestamp = std::chrono::system_clock::now();
    alert.acknowledged = false;
    
    {
        std::lock_guard<std::mutex> lock(alerts_mutex_);
        alerts_.push_back(alert);
    }
    
    send_alert_notifications(alert);
    std::cout << "🚨 ALERT [" << static_cast<int>(severity) << "]: " << message << std::endl;
}

std::vector<Alert> SystemMonitor::get_active_alerts() {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    std::vector<Alert> active_alerts;
    for (const auto& alert : alerts_) {
        if (!alert.acknowledged) {
            active_alerts.push_back(alert);
        }
    }
    return active_alerts;
}

void SystemMonitor::acknowledge_alert(size_t alert_index, const std::string& user) {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    if (alert_index < alerts_.size()) {
        alerts_[alert_index].acknowledged = true;
        alerts_[alert_index].acknowledged_by = user;
        std::cout << "Alert acknowledged by " << user << std::endl;
    }
}

void SystemMonitor::clear_acknowledged_alerts() {
    std::lock_guard<std::mutex> lock(alerts_mutex_);
    alerts_.erase(std::remove_if(alerts_.begin(), alerts_.end(),
        [](const Alert& alert) { return alert.acknowledged; }), alerts_.end());
}

void SystemMonitor::record_performance_metric(const std::string& metric_name, double value) {
    std::lock_guard<std::mutex> lock(performance_mutex_);
    performance_history_[metric_name].push_back(value);
    
    // Keep only last 1000 samples
    if (performance_history_[metric_name].size() > 1000) {
        performance_history_[metric_name].erase(performance_history_[metric_name].begin());
    }
}

double SystemMonitor::get_average_performance(const std::string& metric_name, int samples) {
    std::lock_guard<std::mutex> lock(performance_mutex_);
    const auto& history = performance_history_[metric_name];
    
    if (history.empty()) return 0.0;
    
    int count = std::min(samples, static_cast<int>(history.size()));
    double sum = 0.0;
    
    for (int i = history.size() - count; i < history.size(); ++i) {
        sum += history[i];
    }
    
    return sum / count;
}

std::map<std::string, double> SystemMonitor::get_performance_summary() {
    std::map<std::string, double> summary;
    std::lock_guard<std::mutex> lock(performance_mutex_);
    
    for (const auto& pair : performance_history_) {
        if (!pair.second.empty()) {
            summary[pair.first] = pair.second.back(); // Latest value
        }
    }
    
    return summary;
}

bool SystemMonitor::is_system_healthy() {
    return system_healthy_.load();
}

void SystemMonitor::perform_health_check() {
    auto metrics = get_current_metrics();
    bool healthy = true;
    
    // Check critical thresholds
    if (metrics.cpu_usage > thresholds_.cpu_critical ||
        metrics.memory_usage > thresholds_.memory_critical ||
        metrics.disk_usage > thresholds_.disk_critical) {
        healthy = false;
    }
    
    system_healthy_ = healthy;
    last_health_check_ = std::chrono::system_clock::now();
    
    if (!healthy) {
        trigger_alert(AlertType::SYSTEM_ERROR, AlertSeverity::CRITICAL, 
                     "System health check failed", "Critical resource usage detected");
    }
}

std::string SystemMonitor::get_health_report() {
    auto metrics = get_current_metrics();
    std::string report = "System Health Report:\n";
    report += "CPU Usage: " + std::to_string(metrics.cpu_usage) + "%\n";
    report += "Memory Usage: " + std::to_string(metrics.memory_usage) + "%\n";
    report += "Disk Usage: " + std::to_string(metrics.disk_usage) + "%\n";
    report += "GPU Usage: " + std::to_string(metrics.gpu_usage) + "%\n";
    report += "Active Jobs: " + std::to_string(metrics.active_jobs) + "\n";
    report += "System Status: " + (is_system_healthy() ? "Healthy" : "Unhealthy") + "\n";
    return report;
}

void SystemMonitor::monitor_job_performance(const std::string& job_id, double execution_time) {
    record_performance_metric("job_execution_time_" + job_id, execution_time);
    
    if (execution_time > 300000) { // 5 minutes
        trigger_alert(AlertType::RESPONSE_TIME_HIGH, AlertSeverity::MEDIUM,
                     "Long running job detected: " + job_id,
                     "Execution time: " + std::to_string(execution_time) + "ms");
    }
}

void SystemMonitor::monitor_resource_usage(const std::string& resource, double usage) {
    record_performance_metric("resource_" + resource, usage);
}

void SystemMonitor::monitor_error_rate(const std::string& component, int errors, int total_requests) {
    if (total_requests > 0) {
        double error_rate = (static_cast<double>(errors) / total_requests) * 100.0;
        record_performance_metric("error_rate_" + component, error_rate);
        
        if (error_rate > 5.0) { // 5% error rate threshold
            trigger_alert(AlertType::SYSTEM_ERROR, AlertSeverity::HIGH,
                         "High error rate in " + component,
                         "Error rate: " + std::to_string(error_rate) + "%");
        }
    }
}

bool SystemMonitor::predict_resource_exhaustion(const std::string& resource, std::chrono::minutes time_window) {
    double trend = calculate_trend("resource_" + resource, 10);
    return trend > 0.5; // Simplified prediction
}

double SystemMonitor::calculate_trend(const std::string& metric, int samples) {
    std::lock_guard<std::mutex> lock(performance_mutex_);
    const auto& history = performance_history_[metric];
    
    if (history.size() < 2) return 0.0;
    
    int count = std::min(samples, static_cast<int>(history.size()));
    if (count < 2) return 0.0;
    
    // Simple linear trend calculation
    double first = history[history.size() - count];
    double last = history.back();
    
    return (last - first) / count;
}

std::vector<std::string> SystemMonitor::get_performance_recommendations() {
    std::vector<std::string> recommendations;
    auto metrics = get_current_metrics();
    
    if (metrics.cpu_usage > 80) {
        recommendations.push_back("Consider scaling up CPU resources or optimizing CPU-intensive tasks");
    }
    
    if (metrics.memory_usage > 85) {
        recommendations.push_back("Memory usage is high - consider increasing memory or optimizing memory usage");
    }
    
    if (metrics.gpu_usage > 90) {
        recommendations.push_back("GPU utilization is very high - consider adding more GPUs or optimizing GPU workloads");
    }
    
    if (metrics.response_time_avg > 1000) {
        recommendations.push_back("Response times are high - consider performance optimization or load balancing");
    }
    
    return recommendations;
}

void SystemMonitor::process_alerts(const SystemMetrics& metrics) {
    // Check CPU usage
    if (metrics.cpu_usage > thresholds_.cpu_critical) {
        trigger_alert(AlertType::CPU_HIGH, AlertSeverity::CRITICAL, 
                     "Critical CPU usage: " + std::to_string(metrics.cpu_usage) + "%");
    } else if (metrics.cpu_usage > thresholds_.cpu_warning) {
        trigger_alert(AlertType::CPU_HIGH, AlertSeverity::MEDIUM,
                     "High CPU usage: " + std::to_string(metrics.cpu_usage) + "%");
    }
    
    // Check memory usage
    if (metrics.memory_usage > thresholds_.memory_critical) {
        trigger_alert(AlertType::MEMORY_HIGH, AlertSeverity::CRITICAL,
                     "Critical memory usage: " + std::to_string(metrics.memory_usage) + "%");
    } else if (metrics.memory_usage > thresholds_.memory_warning) {
        trigger_alert(AlertType::MEMORY_HIGH, AlertSeverity::MEDIUM,
                     "High memory usage: " + std::to_string(metrics.memory_usage) + "%");
    }
    
    // Check disk usage
    if (metrics.disk_usage > thresholds_.disk_critical) {
        trigger_alert(AlertType::DISK_FULL, AlertSeverity::CRITICAL,
                     "Critical disk usage: " + std::to_string(metrics.disk_usage) + "%");
    }
    
    // Check response time
    if (metrics.response_time_avg > thresholds_.response_time_critical) {
        trigger_alert(AlertType::RESPONSE_TIME_HIGH, AlertSeverity::HIGH,
                     "Critical response time: " + std::to_string(metrics.response_time_avg) + "ms");
    }
}

void SystemMonitor::send_alert_notifications(const Alert& alert) {
    for (const auto& handler : alert_handlers_) {
        handler(alert);
    }
}

void SystemMonitor::analyze_performance_trends() {
    // Analyze trends in key metrics
    double cpu_trend = calculate_trend("cpu_usage", 20);
    double memory_trend = calculate_trend("memory_usage", 20);
    
    if (cpu_trend > 2.0) {
        std::cout << "📈 CPU usage trending upward" << std::endl;
    }
    
    if (memory_trend > 2.0) {
        std::cout << "📈 Memory usage trending upward" << std::endl;
    }
}

void SystemMonitor::detect_anomalies() {
    // Simple anomaly detection based on standard deviation
    auto current_metrics = get_current_metrics();
    
    // This is a simplified implementation
    if (current_metrics.cpu_usage > 95.0) {
        std::cout << "🔍 Anomaly detected: Extremely high CPU usage" << std::endl;
    }
}

} // namespace monitoring
} // namespace dds 