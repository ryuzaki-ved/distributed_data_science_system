#pragma once

#include "../utils/types.h"
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <mutex>

namespace dds {
namespace database {

// Forward declaration
struct sqlite3;

// Database table structures
struct JobRecord {
    int id;
    std::string job_id;
    std::string job_name;
    std::string job_type;
    std::string status;
    std::string input_path;
    std::string output_path;
    std::string parameters;
    time_t created_at;
    time_t started_at;
    time_t completed_at;
    double execution_time;
    std::string error_message;
};

struct ModelRecord {
    int id;
    std::string model_id;
    std::string model_name;
    std::string algorithm_type;
    std::string model_path;
    std::string parameters;
    double accuracy;
    double loss;
    time_t created_at;
    time_t last_updated;
    bool is_active;
};

struct DatasetRecord {
    int id;
    std::string dataset_id;
    std::string dataset_name;
    std::string file_path;
    size_t file_size;
    int num_samples;
    int num_features;
    std::string description;
    time_t created_at;
    time_t last_accessed;
};

struct ExperimentRecord {
    int id;
    std::string experiment_id;
    std::string experiment_name;
    std::string description;
    std::string parameters;
    std::string results;
    time_t created_at;
    time_t completed_at;
    std::string status;
};

// Database manager for the DDS system
class DatabaseManager {
private:
    sqlite3* db_;
    std::string db_path_;
    bool initialized_;

public:
    DatabaseManager(const std::string& db_path = "dds_system.db");
    ~DatabaseManager();
    
    // Initialization
    bool initialize();
    bool create_tables();
    void close();
    
    // Job management
    bool insert_job(const JobRecord& job);
    bool update_job_status(const std::string& job_id, const std::string& status);
    bool update_job_completion(const std::string& job_id, time_t completed_at, double execution_time);
    JobRecord get_job(const std::string& job_id);
    std::vector<JobRecord> get_jobs_by_status(const std::string& status);
    std::vector<JobRecord> get_jobs_by_type(const std::string& job_type);
    std::vector<JobRecord> get_recent_jobs(int limit = 10);
    bool delete_job(const std::string& job_id);
    
    // Model management
    bool insert_model(const ModelRecord& model);
    bool update_model_metrics(const std::string& model_id, double accuracy, double loss);
    bool deactivate_model(const std::string& model_id);
    ModelRecord get_model(const std::string& model_id);
    std::vector<ModelRecord> get_active_models();
    std::vector<ModelRecord> get_models_by_type(const std::string& algorithm_type);
    bool delete_model(const std::string& model_id);
    
    // Dataset management
    bool insert_dataset(const DatasetRecord& dataset);
    bool update_dataset_access(const std::string& dataset_id);
    DatasetRecord get_dataset(const std::string& dataset_id);
    std::vector<DatasetRecord> get_all_datasets();
    bool delete_dataset(const std::string& dataset_id);
    
    // Experiment management
    bool insert_experiment(const ExperimentRecord& experiment);
    bool update_experiment_results(const std::string& experiment_id, const std::string& results, const std::string& status);
    ExperimentRecord get_experiment(const std::string& experiment_id);
    std::vector<ExperimentRecord> get_experiments_by_status(const std::string& status);
    std::vector<ExperimentRecord> get_recent_experiments(int limit = 10);
    bool delete_experiment(const std::string& experiment_id);
    
    // Analytics and reporting
    std::map<std::string, int> get_job_statistics();
    std::map<std::string, double> get_performance_metrics();
    std::vector<std::pair<std::string, int>> get_most_used_algorithms();
    std::vector<std::pair<std::string, double>> get_best_performing_models();
    
    // Utility functions
    bool backup_database(const std::string& backup_path);
    bool restore_database(const std::string& backup_path);
    bool vacuum_database();
    std::string get_database_info();
    
private:
    // Helper methods
    bool execute_query(const std::string& query);
    bool execute_query_with_params(const std::string& query, const std::vector<std::string>& params);
    std::string escape_string(const std::string& str);
    time_t get_current_timestamp();
    std::string timestamp_to_string(time_t timestamp);
    time_t string_to_timestamp(const std::string& timestamp_str);
};

// Database connection pool for high-performance access
class DatabaseConnectionPool {
private:
    std::vector<std::unique_ptr<DatabaseManager>> connections_;
    std::mutex pool_mutex_;
    size_t max_connections_;
    size_t current_connections_;

public:
    DatabaseConnectionPool(size_t max_connections = 10);
    ~DatabaseConnectionPool();
    
    std::shared_ptr<DatabaseManager> get_connection();
    void return_connection(std::shared_ptr<DatabaseManager> connection);
    void close_all_connections();
    
private:
    std::shared_ptr<DatabaseManager> create_connection();
};

// Database migration manager
class DatabaseMigrationManager {
private:
    std::shared_ptr<DatabaseManager> db_;
    std::vector<std::string> migration_scripts_;

public:
    DatabaseMigrationManager(std::shared_ptr<DatabaseManager> db);
    
    bool run_migrations();
    bool add_migration(const std::string& migration_script);
    int get_current_version();
    bool set_version(int version);
    
private:
    bool create_migrations_table();
    bool is_migration_applied(const std::string& migration_name);
    bool mark_migration_applied(const std::string& migration_name);
};

} // namespace database
} // namespace dds 