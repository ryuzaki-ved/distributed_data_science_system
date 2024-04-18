#pragma once

#include "../utils/types.h"
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <functional>
#include <fstream>

namespace dds {
namespace config {

// Configuration value types
enum class ConfigValueType {
    STRING,
    INTEGER,
    DOUBLE,
    BOOLEAN,
    ARRAY,
    OBJECT
};

// Configuration value
class ConfigValue {
private:
    ConfigValueType type_;
    std::string string_value_;
    int int_value_;
    double double_value_;
    bool bool_value_;
    std::vector<std::string> array_value_;
    std::map<std::string, std::string> object_value_;

public:
    ConfigValue();
    ConfigValue(const std::string& value);
    ConfigValue(int value);
    ConfigValue(double value);
    ConfigValue(bool value);
    ConfigValue(const std::vector<std::string>& value);
    ConfigValue(const std::map<std::string, std::string>& value);
    
    // Type conversion
    std::string as_string() const;
    int as_int() const;
    double as_double() const;
    bool as_bool() const;
    std::vector<std::string> as_array() const;
    std::map<std::string, std::string> as_object() const;
    
    // Type checking
    ConfigValueType get_type() const { return type_; }
    bool is_string() const { return type_ == ConfigValueType::STRING; }
    bool is_int() const { return type_ == ConfigValueType::INTEGER; }
    bool is_double() const { return type_ == ConfigValueType::DOUBLE; }
    bool is_bool() const { return type_ == ConfigValueType::BOOLEAN; }
    bool is_array() const { return type_ == ConfigValueType::ARRAY; }
    bool is_object() const { return type_ == ConfigValueType::OBJECT; }
    
    // Default values
    std::string as_string(const std::string& default_value) const;
    int as_int(int default_value) const;
    double as_double(double default_value) const;
    bool as_bool(bool default_value) const;
};

// Configuration section
class ConfigSection {
private:
    std::string name_;
    std::map<std::string, ConfigValue> values_;
    std::map<std::string, std::shared_ptr<ConfigSection>> subsections_;

public:
    explicit ConfigSection(const std::string& name);
    
    // Value management
    void set_value(const std::string& key, const ConfigValue& value);
    ConfigValue get_value(const std::string& key) const;
    bool has_value(const std::string& key) const;
    void remove_value(const std::string& key);
    
    // Subsection management
    std::shared_ptr<ConfigSection> get_subsection(const std::string& name);
    std::shared_ptr<ConfigSection> create_subsection(const std::string& name);
    bool has_subsection(const std::string& name) const;
    
    // Getters
    const std::string& get_name() const { return name_; }
    const std::map<std::string, ConfigValue>& get_values() const { return values_; }
    const std::map<std::string, std::shared_ptr<ConfigSection>>& get_subsections() const { return subsections_; }
    
    // Utility
    std::vector<std::string> get_keys() const;
    void clear();
};

// Configuration Manager
class ConfigurationManager {
private:
    std::shared_ptr<ConfigSection> root_section_;
    std::string config_file_;
    std::map<std::string, std::string> environment_variables_;
    std::vector<std::string> config_search_paths_;
    bool auto_reload_;
    std::chrono::system_clock::time_point last_modified_;

public:
    ConfigurationManager();
    explicit ConfigurationManager(const std::string& config_file);
    ~ConfigurationManager();
    
    // Configuration loading
    bool load_config(const std::string& config_file);
    bool load_config_from_string(const std::string& config_content);
    bool reload_config();
    bool save_config(const std::string& config_file = "");
    
    // Value access
    ConfigValue get_value(const std::string& key, const ConfigValue& default_value = ConfigValue()) const;
    std::string get_string(const std::string& key, const std::string& default_value = "") const;
    int get_int(const std::string& key, int default_value = 0) const;
    double get_double(const std::string& key, double default_value = 0.0) const;
    bool get_bool(const std::string& key, bool default_value = false) const;
    std::vector<std::string> get_array(const std::string& key) const;
    std::map<std::string, std::string> get_object(const std::string& key) const;
    
    // Value setting
    void set_value(const std::string& key, const ConfigValue& value);
    void set_string(const std::string& key, const std::string& value);
    void set_int(const std::string& key, int value);
    void set_double(const std::string& key, double value);
    void set_bool(const std::string& key, bool value);
    void set_array(const std::string& key, const std::vector<std::string>& value);
    void set_object(const std::string& key, const std::map<std::string, std::string>& value);
    
    // Section access
    std::shared_ptr<ConfigSection> get_section(const std::string& section_name);
    std::shared_ptr<ConfigSection> create_section(const std::string& section_name);
    
    // Environment variables
    void load_environment_variables();
    void set_environment_variable(const std::string& key, const std::string& value);
    std::string get_environment_variable(const std::string& key, const std::string& default_value = "") const;
    
    // Configuration paths
    void add_config_search_path(const std::string& path);
    std::string find_config_file(const std::string& filename);
    
    // Validation
    bool validate_config();
    std::vector<std::string> get_validation_errors();
    
    // Configuration templates
    void create_default_config();
    void create_sample_config();
    
    // Configuration merging
    void merge_config(const ConfigurationManager& other);
    void merge_config_file(const std::string& config_file);
    
    // Configuration export
    std::string export_as_json() const;
    std::string export_as_yaml() const;
    std::string export_as_ini() const;
    
    // Configuration monitoring
    void enable_auto_reload(bool enable = true);
    bool check_for_changes();
    
    // Utility
    void clear();
    std::vector<std::string> get_all_keys() const;
    bool has_key(const std::string& key) const;
    
private:
    // Internal parsing methods
    bool parse_json_config(const std::string& content);
    bool parse_yaml_config(const std::string& content);
    bool parse_ini_config(const std::string& content);
    bool parse_env_config();
    
    // Internal utility methods
    std::vector<std::string> split_key(const std::string& key) const;
    std::shared_ptr<ConfigSection> get_or_create_section_path(const std::vector<std::string>& path);
    std::shared_ptr<ConfigSection> get_section_path(const std::vector<std::string>& path) const;
    void validate_section(const std::shared_ptr<ConfigSection>& section, std::vector<std::string>& errors);
    
    // File monitoring
    std::chrono::system_clock::time_point get_file_modification_time(const std::string& filepath);
};

// Configuration validators
class ConfigValidator {
public:
    static bool validate_required_fields(const ConfigurationManager& config, 
                                       const std::vector<std::string>& required_fields);
    static bool validate_field_types(const ConfigurationManager& config,
                                   const std::map<std::string, ConfigValueType>& field_types);
    static bool validate_field_ranges(const ConfigurationManager& config,
                                    const std::map<std::string, std::pair<double, double>>& ranges);
    static bool validate_field_patterns(const ConfigurationManager& config,
                                      const std::map<std::string, std::string>& patterns);
};

// Configuration templates
class ConfigTemplates {
public:
    static std::string get_default_config_template();
    static std::string get_hadoop_config_template();
    static std::string get_web_server_config_template();
    static std::string get_database_config_template();
    static std::string get_monitoring_config_template();
    static std::string get_algorithm_config_template();
};

} // namespace config
} // namespace dds 