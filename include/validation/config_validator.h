#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace dds {
namespace validation {

// Validation rule types
enum class ValidationRule {
    REQUIRED,
    NUMERIC_RANGE,
    STRING_LENGTH,
    FILE_EXISTS,
    DIRECTORY_EXISTS,
    POSITIVE_NUMBER,
    VALID_EMAIL,
    VALID_URL
};

// Validation error
struct ValidationError {
    std::string field_name;
    std::string error_message;
    ValidationRule rule_type;
};

// Configuration validator
class ConfigValidator {
private:
    std::map<std::string, std::vector<ValidationRule>> field_rules_;
    std::map<std::string, std::pair<double, double>> numeric_ranges_;
    std::map<std::string, std::pair<int, int>> string_lengths_;
    std::vector<ValidationError> errors_;

public:
    ConfigValidator() = default;
    
    // Add validation rules
    void add_required_field(const std::string& field_name);
    void add_numeric_range(const std::string& field_name, double min_val, double max_val);
    void add_string_length(const std::string& field_name, int min_len, int max_len);
    void add_file_exists(const std::string& field_name);
    void add_positive_number(const std::string& field_name);
    void add_email_validation(const std::string& field_name);
    void add_url_validation(const std::string& field_name);
    
    // Validate configuration
    bool validate_config(const std::map<std::string, std::string>& config);
    bool validate_field(const std::string& field_name, const std::string& value);
    
    // Get validation results
    std::vector<ValidationError> get_errors() const { return errors_; }
    bool has_errors() const { return !errors_.empty(); }
    void clear_errors() { errors_.clear(); }
    
    // Print validation report
    void print_validation_report() const;
    
private:
    bool is_valid_number(const std::string& value) const;
    bool is_valid_email(const std::string& email) const;
    bool is_valid_url(const std::string& url) const;
    bool file_exists(const std::string& path) const;
    void add_error(const std::string& field, const std::string& message, ValidationRule rule);
};

} // namespace validation
} // namespace dds
