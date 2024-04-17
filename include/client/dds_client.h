#pragma once

#include "utils/types.h"
#include "job_manager/job_scheduler.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace dds {

// Forward declarations
class JobManager;

struct ClientConfig {
    std::string server_host;
    int server_port;
    std::string username;
    std::string password;
    bool enable_ssl;
    std::string cert_file;
    std::string key_file;
    int connection_timeout;
    int request_timeout;
    bool enable_retry;
    int max_retries;
    double retry_delay;
    LogLevel log_level;
    std::string log_file;
};

struct ClientJobStatus {
    JobID job_id;
    std::string job_name;
    JobState state;
    double progress;
    std::string message;
    std::chrono::system_clock::time_point submit_time;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    std::string error_message;
    JobResult result;
};

struct ClientMetrics {
    int total_jobs_submitted;
    int completed_jobs;
    int failed_jobs;
    int pending_jobs;
    double average_job_time;
    double total_execution_time;
    std::vector<double> job_completion_times;
    std::chrono::system_clock::time_point session_start;
};

class DDSClient {
public:
    DDSClient();
    ~DDSClient();
    
    // Initialization and connection
    bool initialize(const ClientConfig& config);
    bool connect();
    void disconnect();
    bool is_connected() const { return connected_; }
    bool is_initialized() const { return initialized_; }
    
    // Job submission and management
    JobID submit_job(const JobConfig& config);
    JobID submit_linear_regression(const std::string& data_path, const std::string& output_path,
                                   const LinearRegressionParams& params = LinearRegressionParams{});
    JobID submit_logistic_regression(const std::string& data_path, const std::string& output_path,
                                     const LogisticRegressionParams& params = LogisticRegressionParams{});
    JobID submit_kmeans(const std::string& data_path, const std::string& output_path,
                        const KMeansParams& params = KMeansParams{});
    JobID submit_dbscan(const std::string& data_path, const std::string& output_path,
                        const DBSCANParams& params = DBSCANParams{});
    
    // Batch operations
    std::vector<JobID> submit_batch_jobs(const std::vector<JobConfig>& configs);
    std::vector<JobID> submit_parameter_sweep(const JobConfig& base_config,
                                              const std::vector<std::pair<std::string, std::vector<double>>>& parameters);
    
    // Job monitoring and control
    ClientJobStatus get_job_status(JobID job_id);
    std::vector<ClientJobStatus> get_all_job_status();
    bool cancel_job(JobID job_id);
    bool pause_job(JobID job_id);
    bool resume_job(JobID job_id);
    bool wait_for_job_completion(JobID job_id, int timeout_seconds = -1);
    JobResult get_job_result(JobID job_id);
    
    // Data management
    bool upload_data(const std::string& local_path, const std::string& remote_path);
    bool download_data(const std::string& remote_path, const std::string& local_path);
    bool list_data_files(const std::string& directory, std::vector<std::string>& files);
    bool delete_data_file(const std::string& file_path);
    
    // System information
    std::vector<WorkerInfo> get_worker_status();
    std::vector<double> get_system_metrics();
    bool get_system_info(SystemInfo& info);
    
    // Configuration and settings
    const ClientConfig& get_config() const { return config_; }
    void set_config(const ClientConfig& config);
    void set_log_level(LogLevel level);
    void set_callback(std::function<void(JobID, const ClientJobStatus&)> callback);
    
    // Metrics and monitoring
    ClientMetrics get_client_metrics() const;
    void reset_metrics();
    void generate_report(const std::string& output_file);
    
    // Utility functions
    bool validate_job_config(const JobConfig& config);
    std::string get_last_error() const { return last_error_; }
    void clear_error() { last_error_.clear(); }

private:
    bool initialized_;
    bool connected_;
    ClientConfig config_;
    std::unique_ptr<JobManager> job_manager_;
    ClientMetrics metrics_;
    std::function<void(JobID, const ClientJobStatus&)> status_callback_;
    std::string last_error_;
    
    // Helper methods
    bool setup_connection();
    bool authenticate();
    bool send_request(const std::string& request, std::string& response);
    bool parse_response(const std::string& response, Json& json);
    void update_metrics(JobID job_id, const ClientJobStatus& status);
    void log_client_event(const std::string& event, LogLevel level = LogLevel::INFO);
    void handle_connection_error();
    bool retry_operation(const std::function<bool()>& operation);
};

class DDSClientCLI {
public:
    DDSClientCLI();
    ~DDSClientCLI();
    
    // CLI interface
    int run(int argc, char* argv[]);
    void show_help();
    void show_version();
    
    // Command handlers
    int handle_submit_command(const std::vector<std::string>& args);
    int handle_status_command(const std::vector<std::string>& args);
    int handle_cancel_command(const std::vector<std::string>& args);
    int handle_list_command(const std::vector<std::string>& args);
    int handle_upload_command(const std::vector<std::string>& args);
    int handle_download_command(const std::vector<std::string>& args);
    int handle_workers_command(const std::vector<std::string>& args);
    int handle_metrics_command(const std::vector<std::string>& args);
    int handle_config_command(const std::vector<std::string>& args);
    
    // Interactive mode
    void run_interactive();
    std::string read_command();
    std::vector<std::string> parse_command(const std::string& command);
    void execute_command(const std::vector<std::string>& args);
    
    // Configuration
    bool load_config_file(const std::string& config_file);
    bool save_config_file(const std::string& config_file);
    void set_default_config();

private:
    std::unique_ptr<DDSClient> client_;
    std::string config_file_;
    bool interactive_mode_;
    bool verbose_mode_;
    
    // Helper methods
    void print_job_status(const ClientJobStatus& status);
    void print_worker_info(const std::vector<WorkerInfo>& workers);
    void print_metrics(const ClientMetrics& metrics);
    void print_system_metrics(const std::vector<double>& metrics);
    std::string format_duration(std::chrono::system_clock::duration duration);
    std::string format_file_size(size_t bytes);
    void show_progress_bar(double progress, int width = 50);
};

// Utility functions for client applications
namespace client_utils {
    
    // Configuration management
    bool validate_client_config(const ClientConfig& config);
    std::string get_config_error(const ClientConfig& config);
    ClientConfig load_config_from_file(const std::string& file_path);
    bool save_config_to_file(const ClientConfig& config, const std::string& file_path);
    
    // Job configuration helpers
    JobConfig create_linear_regression_config(const std::string& data_path, const std::string& output_path,
                                             const LinearRegressionParams& params);
    JobConfig create_logistic_regression_config(const std::string& data_path, const std::string& output_path,
                                               const LogisticRegressionParams& params);
    JobConfig create_kmeans_config(const std::string& data_path, const std::string& output_path,
                                  const KMeansParams& params);
    JobConfig create_dbscan_config(const std::string& data_path, const std::string& output_path,
                                  const DBSCANParams& params);
    
    // Data validation
    bool validate_data_file(const std::string& file_path, JobType job_type);
    bool check_data_format(const std::string& file_path, const std::string& expected_format);
    size_t estimate_data_size(const std::string& file_path);
    
    // Result processing
    struct ResultSummary {
        JobType job_type;
        std::string algorithm_name;
        double execution_time;
        double accuracy;
        double loss;
        std::vector<double> metrics;
        std::string model_path;
        std::string visualization_path;
    };
    
    ResultSummary process_job_result(const JobResult& result);
    bool save_result_summary(const ResultSummary& summary, const std::string& output_file);
    
    // Visualization helpers
    bool generate_result_visualization(const JobResult& result, const std::string& output_path);
    bool create_performance_chart(const std::vector<double>& metrics, const std::string& output_path);
    bool create_comparison_chart(const std::vector<ResultSummary>& results, const std::string& output_path);
    
    // Report generation
    struct ClientReport {
        std::string session_id;
        std::chrono::system_clock::time_point session_start;
        std::chrono::system_clock::time_point session_end;
        ClientMetrics metrics;
        std::vector<ClientJobStatus> job_history;
        std::vector<WorkerInfo> worker_snapshots;
        std::vector<double> system_metrics_history;
    };
    
    ClientReport generate_session_report(const DDSClient& client);
    bool save_report(const ClientReport& report, const std::string& output_file);
    
    // Error handling
    std::string get_error_description(JobState state, const std::string& error_message);
    std::string suggest_solution(const std::string& error_type);
    bool is_recoverable_error(const std::string& error_message);
    
} // namespace client_utils

} // namespace dds 