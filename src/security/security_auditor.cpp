#include "../../include/security/security_auditor.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <regex>
#include <iomanip>
#include <sstream>

namespace dds {
namespace security {

void SecurityAuditor::log_event(SecurityEventType type, SecuritySeverity severity,
                                const std::string& user_id, const std::string& ip_address,
                                const std::string& resource, const std::string& description,
                                const std::string& user_agent) {
    if (!enabled_) return;

    std::lock_guard<std::mutex> lock(audit_mutex_);

    SecurityEvent event{
        type, severity, user_id, ip_address, resource, description, user_agent,
        std::chrono::system_clock::now(), false
    };

    audit_log_.push_back(event);

    detect_threats(event);

    if (audit_log_.size() > max_log_size_) {
        audit_log_.erase(audit_log_.begin(), audit_log_.begin() + (audit_log_.size() - max_log_size_));
    }

    if (severity == SecuritySeverity::HIGH || severity == SecuritySeverity::CRITICAL) {
        std::cout << "🚨 SECURITY ALERT [" << severity_to_string(severity) << "]: "
                  << event_type_to_string(type) << " from " << ip_address << std::endl;
    }
}

void SecurityAuditor::log_login_attempt(const std::string& user_id, const std::string& ip_address, bool success) {
    if (success) {
        log_event(SecurityEventType::LOGIN_SUCCESS, SecuritySeverity::LOW, user_id, ip_address,
                  "", "Successful login");
        failed_login_attempts_[ip_address] = 0;
    } else {
        int& attempts = failed_login_attempts_[ip_address];
        ++attempts;
        SecuritySeverity severity = (attempts >= brute_force_threshold_)
                                   ? SecuritySeverity::HIGH : SecuritySeverity::MEDIUM;

        log_event(SecurityEventType::LOGIN_FAILURE, severity, user_id, ip_address,
                  "", "Failed login attempt #" + std::to_string(attempts));

        if (attempts >= brute_force_threshold_) {
            log_event(SecurityEventType::BRUTE_FORCE_ATTEMPT, SecuritySeverity::CRITICAL, user_id, ip_address,
                      "", "Brute force attack detected");
        }
    }
}

void SecurityAuditor::log_unauthorized_access(const std::string& user_id, const std::string& resource, const std::string& ip_address) {
    log_event(SecurityEventType::UNAUTHORIZED_ACCESS, SecuritySeverity::HIGH, user_id, ip_address,
              resource, "Attempted access to restricted resource");
}

void SecurityAuditor::log_configuration_change(const std::string& user_id, const std::string& config_item, const std::string& ip_address) {
    log_event(SecurityEventType::CONFIGURATION_CHANGE, SecuritySeverity::MEDIUM, user_id, ip_address,
              config_item, "Configuration item modified");
}

void SecurityAuditor::log_suspicious_activity(const std::string& description, const std::string& ip_address) {
    log_event(SecurityEventType::SUSPICIOUS_ACTIVITY, SecuritySeverity::MEDIUM, "unknown", ip_address,
              "", description);
}

std::vector<SecurityEvent> SecurityAuditor::get_events_by_type(SecurityEventType type) const {
    std::lock_guard<std::mutex> lock(audit_mutex_);
    std::vector<SecurityEvent> filtered_events;
    filtered_events.reserve(audit_log_.size());
    for (const auto& event : audit_log_) {
        if (event.event_type == type) filtered_events.push_back(event);
    }
    filtered_events.shrink_to_fit();
    return filtered_events;
}

std::vector<SecurityEvent> SecurityAuditor::get_events_by_severity(SecuritySeverity severity) const {
    std::lock_guard<std::mutex> lock(audit_mutex_);
    std::vector<SecurityEvent> filtered_events;
    filtered_events.reserve(audit_log_.size());
    for (const auto& event : audit_log_) {
        if (event.severity == severity) filtered_events.push_back(event);
    }
    filtered_events.shrink_to_fit();
    return filtered_events;
}

std::vector<SecurityEvent> SecurityAuditor::get_events_by_ip(const std::string& ip_address) const {
    std::lock_guard<std::mutex> lock(audit_mutex_);
    std::vector<SecurityEvent> filtered_events;
    filtered_events.reserve(audit_log_.size());
    for (const auto& event : audit_log_) {
        if (event.ip_address == ip_address) filtered_events.push_back(event);
    }
    filtered_events.shrink_to_fit();
    return filtered_events;
}

std::vector<SecurityEvent> SecurityAuditor::get_recent_events(int hours) const {
    std::lock_guard<std::mutex> lock(audit_mutex_);
    std::vector<SecurityEvent> recent_events;
    auto cutoff_time = std::chrono::system_clock::now() - std::chrono::hours(hours);
    recent_events.reserve(audit_log_.size());
    for (const auto& event : audit_log_) {
        if (event.timestamp >= cutoff_time) recent_events.push_back(event);
    }
    recent_events.shrink_to_fit();
    return recent_events;
}

std::vector<ThreatAssessment> SecurityAuditor::get_active_threats() const {
    std::lock_guard<std::mutex> lock(audit_mutex_);
    std::vector<ThreatAssessment> active_threats;
    active_threats.reserve(threats_.size());
    for (const auto& pair : threats_) {
        if (pair.second.risk_score >= 50) active_threats.push_back(pair.second);
    }
    std::sort(active_threats.begin(), active_threats.end(),
        [](const ThreatAssessment& a, const ThreatAssessment& b) {
            return a.risk_score > b.risk_score;
        });
    active_threats.shrink_to_fit();
    return active_threats;
}

bool SecurityAuditor::is_brute_force_attack(const std::string& ip_address) const {
    auto it = failed_login_attempts_.find(ip_address);
    return it != failed_login_attempts_.end() && it->second >= brute_force_threshold_;
}

std::vector<std::string> SecurityAuditor::get_suspicious_ips() const {
    std::vector<std::string> suspicious_ips;
    suspicious_ips.reserve(failed_login_attempts_.size());
    for (const auto& pair : failed_login_attempts_) {
        if (pair.second >= brute_force_threshold_ / 2) suspicious_ips.push_back(pair.first);
    }
    suspicious_ips.shrink_to_fit();
    return suspicious_ips;
}

int SecurityAuditor::get_events_count_by_severity(SecuritySeverity severity) const {
    std::lock_guard<std::mutex> lock(audit_mutex_);
    return std::count_if(audit_log_.begin(), audit_log_.end(),
        [severity](const SecurityEvent& event) { return event.severity == severity; });
}

double SecurityAuditor::get_security_score() const {
    if (audit_log_.empty()) return 100.0;
    int critical_events = get_events_count_by_severity(SecuritySeverity::CRITICAL);
    int high_events = get_events_count_by_severity(SecuritySeverity::HIGH);
    int total_events = static_cast<int>(audit_log_.size());
    double score = 100.0 - ((critical_events * 10.0 + high_events * 5.0) / total_events * 100.0);
    return std::clamp(score, 0.0, 100.0);
}

void SecurityAuditor::print_security_summary() const {
    std::cout << "\n🔒 Security Audit Summary\n==========================\n";
    std::cout << "Total events: " << get_total_events() << std::endl;
    std::cout << "Security score: " << std::fixed << std::setprecision(1) << get_security_score() << "/100\n";
    std::cout << "\nEvents by severity:\n";
    std::cout << "  Critical: " << get_events_count_by_severity(SecuritySeverity::CRITICAL) << std::endl;
    std::cout << "  High: " << get_events_count_by_severity(SecuritySeverity::HIGH) << std::endl;
    std::cout << "  Medium: " << get_events_count_by_severity(SecuritySeverity::MEDIUM) << std::endl;
    std::cout << "  Low: " << get_events_count_by_severity(SecuritySeverity::LOW) << std::endl;

    auto active_threats = get_active_threats();
    std::cout << "\nActive threats: " << active_threats.size() << std::endl;

    auto suspicious_ips = get_suspicious_ips();
    std::cout << "Suspicious IPs: " << suspicious_ips.size() << std::endl;

    for (const auto& ip : suspicious_ips) {
        std::cout << "  • " << ip << " (" << failed_login_attempts_.at(ip) << " failed attempts)" << std::endl;
    }
}

void SecurityAuditor::print_threat_report() const {
    auto threats = get_active_threats();
    std::cout << "\n🎯 Active Threats Report\n=========================\n";
    if (threats.empty()) {
        std::cout << "No active threats detected." << std::endl;
        return;
    }
    for (const auto& threat : threats) {
        std::cout << "\nThreat ID: " << threat.threat_id << std::endl;
        std::cout << "Source IP: " << threat.source_ip << std::endl;
        std::cout << "Risk Score: " << threat.risk_score << "/100" << std::endl;
        std::cout << "Event Count: " << threat.event_count << std::endl;
        std::cout << "First Seen: " << std::put_time(std::localtime(&std::chrono::system_clock::to_time_t(threat.first_seen)), "%Y-%m-%d %H:%M:%S") << std::endl;
        std::cout << "Last Seen: " << std::put_time(std::localtime(&std::chrono::system_clock::to_time_t(threat.last_seen)), "%Y-%m-%d %H:%M:%S") << std::endl;
    }
}

void SecurityAuditor::export_audit_log(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cout << "Failed to open file: " << filename << std::endl;
        return;
    }
    file << "Timestamp,EventType,Severity,UserID,IPAddress,Resource,Description,UserAgent,Investigated\n";
    for (const auto& event : audit_log_) {
        auto time_t = std::chrono::system_clock::to_time_t(event.timestamp);
        file << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << ","
             << event_type_to_string(event.event_type) << ","
             << severity_to_string(event.severity) << ","
             << event.user_id << ","
             << event.ip_address << ","
             << event.resource << ","
             << event.description << ","
             << event.user_agent << ","
             << (event.investigated ? "Yes" : "No") << "\n";
    }
    std::cout << "Audit log exported to: " << filename << std::endl;
}

void SecurityAuditor::detect_threats(const SecurityEvent& event) {
    update_threat_assessment(event.ip_address, event);
    if (is_injection_attempt(event.description) || is_injection_attempt(event.resource)) {
        if (event.event_type != SecurityEventType::SQL_INJECTION_ATTEMPT &&
            event.event_type != SecurityEventType::XSS_ATTEMPT) {
            log_event(SecurityEventType::SQL_INJECTION_ATTEMPT, SecuritySeverity::HIGH,
                      event.user_id, event.ip_address, event.resource, "Potential injection attempt detected");
        }
    }
}

void SecurityAuditor::update_threat_assessment(const std::string& ip_address, const SecurityEvent& event) {
    auto& threat = threats_[ip_address];
    if (threat.threat_id.empty()) {
        threat.threat_id = "THREAT_" + ip_address;
        threat.source_ip = ip_address;
        threat.first_seen = event.timestamp;
        threat.event_count = 0;
        threat.risk_score = 0;
    }
    threat.last_seen = event.timestamp;
    ++threat.event_count;
    threat.event_pattern.push_back(event.event_type);

    int severity_score = 0;
    switch (event.severity) {
        case SecuritySeverity::LOW: severity_score = 1; break;
        case SecuritySeverity::MEDIUM: severity_score = 3; break;
        case SecuritySeverity::HIGH: severity_score = 7; break;
        case SecuritySeverity::CRITICAL: severity_score = 15; break;
    }
    threat.risk_score = std::min(100, threat.risk_score + severity_score);
}

bool SecurityAuditor::is_injection_attempt(const std::string& input) const {
    static const std::vector<std::regex> injection_patterns = {
        std::regex(R"(\b(SELECT|INSERT|UPDATE|DELETE|DROP|UNION)\b)", std::regex::icase),
        std::regex(R"(<script[^>]*>)", std::regex::icase),
        std::regex(R"(javascript:)", std::regex::icase),
        std::regex(R"(\'\s*(OR|AND)\s*\'\s*=\s*\')", std::regex::icase)
    };
    for (const auto& pattern : injection_patterns) {
        if (std::regex_search(input, pattern)) return true;
    }
    return false;
}

std::string SecurityAuditor::event_type_to_string(SecurityEventType type) const {
    switch (type) {
        case SecurityEventType::LOGIN_SUCCESS: return "LOGIN_SUCCESS";
        case SecurityEventType::LOGIN_FAILURE: return "LOGIN_FAILURE";
        case SecurityEventType::UNAUTHORIZED_ACCESS: return "UNAUTHORIZED_ACCESS";
        case SecurityEventType::PRIVILEGE_ESCALATION: return "PRIVILEGE_ESCALATION";
        case SecurityEventType::DATA_ACCESS: return "DATA_ACCESS";
        case SecurityEventType::CONFIGURATION_CHANGE: return "CONFIGURATION_CHANGE";
        case SecurityEventType::SUSPICIOUS_ACTIVITY: return "SUSPICIOUS_ACTIVITY";
        case SecurityEventType::BRUTE_FORCE_ATTEMPT: return "BRUTE_FORCE_ATTEMPT";
        case SecurityEventType::SQL_INJECTION_ATTEMPT: return "SQL_INJECTION_ATTEMPT";
        case SecurityEventType::XSS_ATTEMPT: return "XSS_ATTEMPT";
        default: return "UNKNOWN";
    }
}

std::string SecurityAuditor::severity_to_string(SecuritySeverity severity) const {
    switch (severity) {
        case SecuritySeverity::LOW: return "LOW";
        case SecuritySeverity::MEDIUM: return "MEDIUM";
        case SecuritySeverity::HIGH: return "HIGH";
        case SecuritySeverity::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

} // namespace security
} // namespace dds
