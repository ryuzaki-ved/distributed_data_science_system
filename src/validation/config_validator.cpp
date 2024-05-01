#include "../../include/validation/config_validator.h"
#include <iostream>
#include <regex>
#include <fstream>
#include <filesystem>

namespace dds {
namespace validation {

void ConfigValidator::add_required_field(const std::string& field_name) {
    field_rules_[field_name].push_back(ValidationRule::REQUIRED);
}

void ConfigValidator::add_numeric_range(const std::string& field_name, double min_val, double max_val) {
    field_rules_[field_name].push_back(ValidationRule::NUMERIC_RANGE);
    numeric_ranges_[field_name] = {min_val, max_val};
}

void ConfigValidator::add_string_length(const std::string& field_name, int min_len, int max_len) {
    field_rules_[field_name].push_back(ValidationRule::STRING_LENGTH);
    string_lengths_[field_name] = {min_len, max_len};
}

void ConfigValidator::add_file_exists(const std::string& field_name) {
    field_rules_[field_name].push_back(ValidationRule::FILE_EXISTS);
}

void ConfigValidator::add_positive_number(const std::string& field_name) {
    field_rules_[field_name].push_back(ValidationRule::POSITIVE_NUMBER);
}

void ConfigValidator::add_email_validation(const std::string& field_name) {
    field_rules_[field_name].push_back(ValidationRule::VALID_EMAIL);
}

void ConfigValidator::add_url_validation(const std::string& field_name) {
    field_rules_[field_name].push_back(ValidationRule::VALID_URL);
}

bool ConfigValidator::validate_config(const std::map<std::string, std::string>& config) {
    clear_errors();
    
    for (const auto& [field_name, rules] : field_rules_) {
        auto it = config.find(field_name);
        const std::string& value = (it != config.end()) ? it->second : std::string();
        for (const auto& rule : rules) {
            if (rule == ValidationRule::REQUIRED && value.empty()) {
                add_error(field_name, "Field is required", rule);
                continue;
            }
            if (!value.empty()) {
                validate_field(field_name, value);
            }
        }
    }
    
    return !has_errors();
}

bool ConfigValidator::validate_field(const std::string& field_name, const std::string& value) {
    auto it = field_rules_.find(field_name);
    if (it == field_rules_.end()) return true;

    bool valid = true;
    const auto& rules = it->second;
    for (const auto& rule : rules) {
        switch (rule) {
            case ValidationRule::NUMERIC_RANGE: {
                if (!is_valid_number(value)) {
                    add_error(field_name, "Must be a valid number", rule);
                    valid = false;
                } else {
                    double num_val = std::stod(value);
                    auto range = numeric_ranges_[field_name];
                    if (num_val < range.first || num_val > range.second) {
                        add_error(field_name, "Value out of range [" + std::to_string(range.first) + 
                                 ", " + std::to_string(range.second) + "]", rule);
                        valid = false;
                    }
                }
                break;
            }
            case ValidationRule::STRING_LENGTH: {
                auto length = string_lengths_[field_name];
                if (value.length() < length.first || value.length() > length.second) {
                    add_error(field_name, "String length must be between " + std::to_string(length.first) + 
                             " and " + std::to_string(length.second), rule);
                    valid = false;
                }
                break;
            }
            case ValidationRule::FILE_EXISTS: {
                if (!file_exists(value)) {
                    add_error(field_name, "File does not exist: " + value, rule);
                    valid = false;
                }
                break;
            }
            case ValidationRule::POSITIVE_NUMBER: {
                if (!is_valid_number(value) || std::stod(value) <= 0) {
                    add_error(field_name, "Must be a positive number", rule);
                    valid = false;
                }
                break;
            }
            case ValidationRule::VALID_EMAIL: {
                if (!is_valid_email(value)) {
                    add_error(field_name, "Invalid email format", rule);
                    valid = false;
                }
                break;
            }
            case ValidationRule::VALID_URL: {
                if (!is_valid_url(value)) {
                    add_error(field_name, "Invalid URL format", rule);
                    valid = false;
                }
                break;
            }
        }
    }
    
    return valid;
}

void ConfigValidator::print_validation_report() const {
    if (!has_errors()) {
        std::cout << "✅ Configuration validation passed!" << std::endl;
        return;
    }
    
    std::cout << "❌ Configuration validation failed:" << std::endl;
    for (const auto& error : errors_) {
        std::cout << "  • " << error.field_name << ": " << error.error_message << std::endl;
    }
}

bool ConfigValidator::is_valid_number(const std::string& value) const {
    if (value.empty()) return false;
    char* endptr = nullptr;
    std::strtod(value.c_str(), &endptr);
    return endptr != value.c_str() && *endptr == '\0';
}

bool ConfigValidator::is_valid_email(const std::string& email) const {
    const std::regex email_pattern(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    return std::regex_match(email, email_pattern);
}

bool ConfigValidator::is_valid_url(const std::string& url) const {
    const std::regex url_pattern(R"(https?://[^\s/$.?#].[^\s]*)");
    return std::regex_match(url, url_pattern);
}

bool ConfigValidator::file_exists(const std::string& path) const {
    return std::filesystem::exists(path);
}

void ConfigValidator::add_error(const std::string& field, const std::string& message, ValidationRule rule) {
    ValidationError error;
    error.field_name = field;
    error.error_message = message;
    error.rule_type = rule;
    errors_.push_back(error);
}

} // namespace validation
} // namespace dds
