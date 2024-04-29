#include "../../include/config/configuration_manager.h"
#include <iostream>

namespace dds {
namespace config {

// =====================
// ConfigValue
// =====================

ConfigValue::ConfigValue() : type_(ConfigValueType::STRING) {}

ConfigValue::ConfigValue(const std::string& value)
    : type_(ConfigValueType::STRING), string_value_(value) {}

ConfigValue::ConfigValue(int value)
    : type_(ConfigValueType::INTEGER), int_value_(value) {}

ConfigValue::ConfigValue(double value)
    : type_(ConfigValueType::DOUBLE), double_value_(value) {}

ConfigValue::ConfigValue(bool value)
    : type_(ConfigValueType::BOOLEAN), bool_value_(value) {}

ConfigValue::ConfigValue(const std::vector<std::string>& value)
    : type_(ConfigValueType::ARRAY), array_value_(value) {}

ConfigValue::ConfigValue(const std::map<std::string, std::string>& value)
    : type_(ConfigValueType::OBJECT), object_value_(value) {}

std::string ConfigValue::as_string() const {
    switch (type_) {
        case ConfigValueType::STRING:  return string_value_;
        case ConfigValueType::INTEGER: return std::to_string(int_value_);
        case ConfigValueType::DOUBLE:  return std::to_string(double_value_);
        case ConfigValueType::BOOLEAN: return bool_value_ ? "true" : "false";
        default: return "";
    }
}

int ConfigValue::as_int() const {
    switch (type_) {
        case ConfigValueType::INTEGER: return int_value_;
        case ConfigValueType::DOUBLE:  return static_cast<int>(double_value_);
        case ConfigValueType::BOOLEAN: return bool_value_ ? 1 : 0;
        default: return 0;
    }
}

double ConfigValue::as_double() const {
    switch (type_) {
        case ConfigValueType::DOUBLE:  return double_value_;
        case ConfigValueType::INTEGER: return static_cast<double>(int_value_);
        default: return 0.0;
    }
}

bool ConfigValue::as_bool() const {
    switch (type_) {
        case ConfigValueType::BOOLEAN: return bool_value_;
        case ConfigValueType::INTEGER: return int_value_ != 0;
        case ConfigValueType::STRING:  return string_value_ == "true" || string_value_ == "1";
        default: return false;
    }
}

std::vector<std::string> ConfigValue::as_array() const {
    return array_value_;
}

std::map<std::string, std::string> ConfigValue::as_object() const {
    return object_value_;
}

std::string ConfigValue::as_string(const std::string& default_value) const {
    return (type_ == ConfigValueType::STRING) ? string_value_ : default_value;
}

int ConfigValue::as_int(int default_value) const {
    return (type_ == ConfigValueType::INTEGER) ? int_value_ : default_value;
}

double ConfigValue::as_double(double default_value) const {
    return (type_ == ConfigValueType::DOUBLE) ? double_value_ : default_value;
}

bool ConfigValue::as_bool(bool default_value) const {
    return (type_ == ConfigValueType::BOOLEAN) ? bool_value_ : default_value;
}

// =====================
// ConfigSection
// =====================

ConfigSection::ConfigSection(const std::string& name) : name_(name) {}

void ConfigSection::set_value(const std::string& key, const ConfigValue& value) {
    values_[key] = value;
}

ConfigValue ConfigSection::get_value(const std::string& key) const {
    auto it = values_.find(key);
    return (it != values_.end()) ? it->second : ConfigValue();
}

bool ConfigSection::has_value(const std::string& key) const {
    return values_.find(key) != values_.end();
}

void ConfigSection::remove_value(const std::string& key) {
    values_.erase(key);
}

std::shared_ptr<ConfigSection> ConfigSection::get_subsection(const std::string& name) {
    auto it = subsections_.find(name);
    return (it != subsections_.end()) ? it->second : nullptr;
}

std::shared_ptr<ConfigSection> ConfigSection::create_subsection(const std::string& name) {
    auto subsection = std::make_shared<ConfigSection>(name);
    subsections_[name] = subsection;
    return subsection;
}

bool ConfigSection::has_subsection(const std::string& name) const {
    return subsections_.find(name) != subsections_.end();
}

std::vector<std::string> ConfigSection::get_keys() const {
    std::vector<std::string> keys;
    for (const auto& [key, _] : values_) {
        keys.push_back(key);
    }
    return keys;
}

void ConfigSection::clear() {
    values_.clear();
    subsections_.clear();
}

// =====================
// ConfigurationManager
// =====================

ConfigurationManager::ConfigurationManager()
    : root_section_(std::make_shared<ConfigSection>("root")), auto_reload_(false) {}

ConfigurationManager::ConfigurationManager(const std::string& config_file)
    : root_section_(std::make_shared<ConfigSection>("root")), config_file_(config_file), auto_reload_(false) {}

ConfigurationManager::~ConfigurationManager() = default;

bool ConfigurationManager::load_config(const std::string& config_file) {
    config_file_ = config_file;
    std::cout << "Loading configuration from: " << config_file << std::endl;
    // TODO: Add file parsing logic
    return true;
}

bool ConfigurationManager::load_config_from_string(const std::string& config_content) {
    std::cout << "Loading configuration from string" << std::endl;
    // TODO: Add string parsing logic
    return true;
}

bool ConfigurationManager::reload_config() {
    return load_config(config_file_);
}

bool ConfigurationManager::save_config(const std::string& config_file) {
    const std::string& file = config_file.empty() ? config_file_ : config_file;
    std::cout << "Saving configuration to: " << file << std::endl;
    // TODO: Add save logic
    return true;
}

// Getters with defaults
ConfigValue ConfigurationManager::get_value(const std::string& key, const ConfigValue& default_value) const {
    return default_value; // TODO: Implement actual lookup logic
}

std::string ConfigurationManager::get_string(const std::string& key, const std::string& default_value) const {
    return default_value;
}

int ConfigurationManager::get_int(const std::string& key, int default_value) const {
    return default_value;
}

double ConfigurationManager::get_double(const std::string& key, double default_value) const {
    return default_value;
}

bool ConfigurationManager::get_bool(const std::string& key, bool default_value) const {
    return default_value;
}

std::vector<std::string> ConfigurationManager::get_array(const std::string& key) const {
    return {};
}

std::map<std::string, std::string> ConfigurationManager::get_object(const std::string& key) const {
    return {};
}

// Setters
void ConfigurationManager::set_value(const std::string& key, const ConfigValue& value) {
    // TODO: Implement value setting logic
}

void ConfigurationManager::set_string(const std::string& key, const std::string& value) {
    set_value(key, ConfigValue(value));
}

void ConfigurationManager::set_int(const std::string& key, int value) {
    set_value(key, ConfigValue(value));
}

void ConfigurationManager::set_double(const std::string& key, double value) {
    set_value(key, ConfigValue(value));
}

void ConfigurationManager::set_bool(const std::string& key, bool value) {
    set_value(key, ConfigValue(value));
}

void ConfigurationManager::set_array(const std::string& key, const std::vector<std::string>& value) {
    set_value(key, ConfigValue(value));
}

void ConfigurationManager::set_object(const std::string& key, const std::map<std::string, std::string>& value) {
    set_value(key, ConfigValue(value));
}

// Sections
std::shared_ptr<ConfigSection> ConfigurationManager::get_section(const std::string& section_name) {
    return root_section_->get_subsection(section_name);
}

std::shared_ptr<ConfigSection> ConfigurationManager::create_section(const std::string& section_name) {
    return root_section_->create_subsection(section_name);
}

// Environment variables
void ConfigurationManager::load_environment_variables() {
    // TODO: Implement loading from environment
}

void ConfigurationManager::set_environment_variable(const std::string& key, const std::string& value) {
    environment_variables_[key] = value;
}

std::string ConfigurationManager::get_environment_variable(const std::string& key, const std::string& default_value) const {
    auto it = environment_variables_.find(key);
    return (it != environment_variables
