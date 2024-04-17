#pragma once

#include "utils/types.h"
#include "communication/mpi_communicator.h"
#include "algorithms/linear_regression.h"
#include "algorithms/logistic_regression.h"
#include "algorithms/kmeans.h"
#include "algorithms/dbscan.h"
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <memory>
#include <functional>

namespace dds {

// Forward declarations
class StorageManager;

struct WorkerConfig {
    int worker_id;
    int rank;
    std::string hostname;
    int num_threads;
    size_t max_memory;
    std::string data_directory;
    bool enable_checkpointing;
    int checkpoint_interval;
    double heartbeat_interval;
    int max_jobs_per_worker;
    bool enable_work_stealing;
};

struct WorkerMetrics {
    double cpu_usage;
    double memory_usage;
    double network_usage;
    double disk_usage;
    int active_jobs;
    int completed_jobs;
    int failed_jobs;
    double average_job_time;
    std::chrono::system_clock::time_point last_update;
};

struct JobExecutionContext {
    JobID job_id;
    JobConfig config;
    std::chrono::system_clock::time_point start_time;
    std::thread execution_thread;
    std::atomic<bool> is_running;
    std::atomic<bool> should_cancel;
    double progress;
    std::string status_message;
    JobResult result;
    std::string error_message;
};

class WorkerNode {
public:
    WorkerNode();
    ~WorkerNode();
    
    // Initialization
    bool initialize(const WorkerConfig& config);
    void shutdown();
    bool is_initialized() const { return initialized_; }
    
    // Job execution
    bool execute_job(const JobConfig& config, JobID job_id);
    bool cancel_job(JobID job_id);
    bool pause_job(JobID job_id);
    bool resume_job(JobID job_id);
    JobStatus get_job_status(JobID job_id) const;
    
    // Worker management
    void start_worker_loop();
    void stop_worker_loop();
    bool send_heartbeat();
    bool update_metrics();
    WorkerMetrics get_metrics() const;
    
    // Algorithm execution
    bool execute_linear_regression(const JobConfig& config, JobResult& result);
    bool execute_logistic_regression(const JobConfig& config, JobResult& result);
    bool execute_kmeans(const JobConfig& config, JobResult& result);
    bool execute_dbscan(const JobConfig& config, JobResult& result);
    
    // Data management
    bool load_data_partition(const std::string& data_path, Matrix& data);
    bool save_results(const JobResult& result, const std::string& output_path);
    bool checkpoint_job_state(JobID job_id);
    bool restore_job_state(JobID job_id);
    
    // Communication
    bool send_job_status(JobID job_id, const JobStatus& status);
    bool receive_job_commands();
    bool broadcast_results(const JobResult& result);
    
    // Configuration
    const WorkerConfig& get_config() const { return config_; }
    void set_config(const WorkerConfig& config);
    
    // Performance monitoring
    double get_cpu_usage() const;
    double get_memory_usage() const;
    double get_network_usage() const;
    int get_active_job_count() const;
    double get_average_job_completion_time() const;

private:
    bool initialized_;
    WorkerConfig config_;
    WorkerMetrics metrics_;
    std::atomic<bool> running_;
    std::thread worker_thread_;
    std::thread heartbeat_thread_;
    std::thread metrics_thread_;
    
    // Job management
    std::unordered_map<JobID, std::unique_ptr<JobExecutionContext>> active_jobs_;
    std::queue<JobID> job_queue_;
    std::mutex job_mutex_;
    std::condition_variable job_cv_;
    
    // Algorithm instances
    std::unique_ptr<LinearRegression> linear_regression_;
    std::unique_ptr<LogisticRegression> logistic_regression_;
    std::unique_ptr<KMeans> kmeans_;
    std::unique_ptr<DBSCAN> dbscan_;
    
    // Storage
    std::unique_ptr<StorageManager> storage_manager_;
    
    // Performance tracking
    std::vector<double> job_completion_times_;
    std::mutex metrics_mutex_;
    
    // Helper methods
    void worker_loop();
    void heartbeat_loop();
    void metrics_loop();
    bool process_job_queue();
    void execute_job_internal(JobID job_id, const JobConfig& config);
    bool validate_job_config(const JobConfig& config);
    void update_job_progress(JobID job_id, double progress, const std::string& message);
    void handle_job_completion(JobID job_id, const JobResult& result);
    void handle_job_failure(JobID job_id, const std::string& error);
    void cleanup_completed_jobs();
    bool check_system_resources();
    void log_worker_event(const std::string& event, JobID job_id = -1);
    
    // Algorithm-specific helpers
    bool prepare_data_for_algorithm(const JobConfig& config, Matrix& X, Vector& y);
    bool partition_data_for_distributed(const Matrix& X, const Vector& y,
                                       std::vector<Matrix>& X_partitions,
                                       std::vector<Vector>& y_partitions);
    bool collect_distributed_results(const JobResult& local_result, JobResult& global_result);
};

class WorkerPool {
public:
    WorkerPool();
    ~WorkerPool();
    
    // Initialization
    bool initialize(int num_workers, const WorkerConfig& base_config);
    void shutdown();
    bool is_initialized() const { return initialized_; }
    
    // Worker management
    bool start_workers();
    bool stop_workers();
    bool restart_workers();
    std::vector<WorkerMetrics> get_all_worker_metrics();
    
    // Job distribution
    bool distribute_job(const JobConfig& config, JobID job_id);
    bool distribute_batch_jobs(const std::vector<JobConfig>& configs, 
                              const std::vector<JobID>& job_ids);
    
    // Load balancing
    int select_worker_for_job(const JobConfig& config);
    bool rebalance_workload();
    double get_load_imbalance() const;
    
    // Monitoring
    void start_monitoring();
    void stop_monitoring();
    std::vector<double> get_pool_metrics();
    void generate_worker_report(const std::string& output_file);
    
    // Configuration
    void set_worker_config(const WorkerConfig& config);
    const WorkerConfig& get_worker_config() const { return base_config_; }

private:
    bool initialized_;
    std::vector<std::unique_ptr<WorkerNode>> workers_;
    WorkerConfig base_config_;
    std::thread monitoring_thread_;
    std::atomic<bool> monitoring_running_;
    
    // Load balancing
    std::vector<double> worker_loads_;
    std::mutex load_mutex_;
    
    // Helper methods
    bool setup_worker_environment();
    void monitoring_loop();
    void update_worker_loads();
    double calculate_worker_load(const WorkerMetrics& metrics);
    void log_pool_event(const std::string& event);
};

// Utility functions for worker management
namespace worker_utils {
    
    // Resource monitoring
    struct SystemResources {
        double cpu_usage;
        double memory_usage;
        double disk_usage;
        double network_usage;
        int num_processes;
        size_t available_memory;
        size_t total_memory;
    };
    
    SystemResources get_system_resources();
    bool check_resource_availability(const SystemResources& current, const SystemResources& required);
    
    // Performance profiling
    struct PerformanceProfile {
        double execution_time;
        double memory_peak;
        double cpu_peak;
        double io_operations;
        std::vector<double> time_breakdown;
    };
    
    PerformanceProfile profile_job_execution(const std::function<void()>& job_function);
    
    // Fault tolerance
    struct FaultToleranceConfig {
        bool enable_checkpointing;
        int checkpoint_interval;
        bool enable_replication;
        int replication_factor;
        bool enable_retry;
        int max_retries;
        double retry_delay;
    };
    
    bool setup_fault_tolerance(const FaultToleranceConfig& config);
    bool handle_worker_failure(int worker_id, const std::string& failure_reason);
    
    // Load balancing algorithms
    enum class LoadBalancingAlgorithm {
        ROUND_ROBIN,
        LEAST_CONNECTIONS,
        WEIGHTED_ROUND_ROBIN,
        LEAST_RESPONSE_TIME,
        CONSISTENT_HASHING
    };
    
    int select_worker_round_robin(const std::vector<WorkerMetrics>& workers);
    int select_worker_least_connections(const std::vector<WorkerMetrics>& workers);
    int select_worker_weighted_round_robin(const std::vector<WorkerMetrics>& workers, 
                                          const std::vector<double>& weights);
    int select_worker_least_response_time(const std::vector<WorkerMetrics>& workers);
    
    // Data partitioning
    std::vector<Matrix> partition_data_round_robin(const Matrix& data, int num_partitions);
    std::vector<Matrix> partition_data_hash_based(const Matrix& data, int num_partitions);
    std::vector<Matrix> partition_data_range_based(const Matrix& data, int num_partitions);
    
    // Communication optimization
    struct CommunicationOptimization {
        bool enable_compression;
        bool enable_batching;
        int batch_size;
        bool enable_pipelining;
        int pipeline_depth;
    };
    
    bool optimize_communication(const CommunicationOptimization& config);
    
} // namespace worker_utils

} // namespace dds 