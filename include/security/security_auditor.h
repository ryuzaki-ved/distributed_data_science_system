#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <mutex>

namespace dds {
namespace security {

// Security event types
enum class SecurityEventType {
    LOGIN_SUCCESS,
    LOGIN_FAILURE,
    UNAUTHORIZED_ACCESS,
    PRIVILEGE_ESCALATION,
    DATA_ACCESS,
    CONFIGURATION_CHANGE,
    SUSPICIOUS_ACTIVITY,
    BRUTE_FORCE_ATTEMPT,
    SQL_INJECTION_ATTEMPT,
    XSS_ATTEMPT
};

// Security severity levels
enum class SecuritySeverity {
    LOW,
    MEDIUM,
    HIGH,
    CRITICAL
};

// Security audit event
struct SecurityEvent {
    SecurityEventType event_type;
    SecuritySeverity severity;
    std::string user_id;
    std::string ip_address;
    std::string resource;
    std::string description;
    std::string user_agent;
    std::chrono::system_clock::time_point timestamp;
    bool investigated;
};

// Security threat assessment
struct ThreatAssessment {
    std::string threat_id;
    std::string source_ip;
    int risk_score;
    std::vector<SecurityEventType> event_pattern;
    std::chrono::system_clock::time_point first_seen;
    std::chrono::system_clock::time_point last_seen;
    int event_count;
};

// Security auditor main class
class SecurityAuditor {
private:
    std::vector<SecurityEvent> audit_log_;
    std::unordered_map<std::string, ThreatAssessment> threats_;
    std::unordered_map<std::string, int> failed_login_attempts_;
    std::mutex audit_mutex_;
    bool enabled_;
    size_t max_log_size_;
    int brute_force_threshold_;

public:
    SecurityAuditor() : enabled_(true), max_log_size_(10000), brute_force_threshold_(5) {}
    
    // Control auditing
    void enable() { enabled_ = true; }
    void disable() { enabled_ = false; }
    bool is_enabled() const { return enabled_; }
    
    // Log security events
    void log_event(SecurityEventType type, SecuritySeverity severity, 
                   const std::string& user_id, const std::string& ip_address,
                   const std::string& resource = "", const std::string& description = "",
                   const std::string& user_agent = "");
    
    void log_login_attempt(const std::string& user_id, const std::string& ip_address, bool success);
    void log_unauthorized_access(const std::string& user_id, const std::string& resource, const std::string& ip_address);
    void log_configuration_change(const std::string& user_id, const std::string& config_item, const std::string& ip_address);
    void log_suspicious_activity(const std::string& description, const std::string& ip_address);
    
    // Analysis and reporting
    std::vector<SecurityEvent> get_events_by_type(SecurityEventType type) const;
    std::vector<SecurityEvent> get_events_by_severity(SecuritySeverity severity) const;
    std::vector<SecurityEvent> get_events_by_user(const std::string& user_id) const;
    std::vector<SecurityEvent> get_events_by_ip(const std::string& ip_address) const;
    std::vector<SecurityEvent> get_recent_events(int hours = 24) const;
    
    // Threat detection
    std::vector<ThreatAssessment> get_active_threats() const;
    ThreatAssessment analyze_ip_behavior(const std::string& ip_address) const;
    bool is_brute_force_attack(const std::string& ip_address) const;
    std::vector<std::string> get_suspicious_ips() const;
    
    // Security metrics
    int get_total_events() const { return audit_log_.size(); }
    int get_events_count_by_severity(SecuritySeverity severity) const;
    double get_security_score() const;
    
    // Reporting
    void print_security_summary() const;
    void print_threat_report() const;
    void export_audit_log(const std::string& filename) const;
    
    // Maintenance
    void clear_old_events(int days_old = 30);
    void mark_event_investigated(size_t event_index);
    void set_brute_force_threshold(int threshold) { brute_force_threshold_ = threshold; }

private:
    void detect_threats(const SecurityEvent& event);
    void update_threat_assessment(const std::string& ip_address, const SecurityEvent& event);
    int calculate_risk_score(const std::vector<SecurityEvent>& events) const;
    std::string event_type_to_string(SecurityEventType type) const;
    std::string severity_to_string(SecuritySeverity severity) const;
    bool is_injection_attempt(const std::string& input) const;
};

} // namespace security
} // namespace dds
