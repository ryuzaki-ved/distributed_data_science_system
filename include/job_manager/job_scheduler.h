#pragma once

#include "utils/types.h"
#include "communication/mpi_communicator.h"
#include <queue>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <memory>

namespace dds {

// Forward declarations
class WorkerNode;
class StorageManager;

struct JobStatus {
    JobID job_id;
    JobState state;
    std::string message;
    double progress;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    std::vector<int> assigned_workers;
    std::string error_message;
    JobResult result;
};

struct WorkerInfo {
    int worker_id;
    int rank;
    std::string hostname;
    bool is_available;
    std::vector<JobID> assigned_jobs;
    double cpu_usage;
    double memory_usage;
    double network_usage;
    std::chrono::system_clock::time_point last_heartbeat;
    int num_cores;
    size_t available_memory;
};

struct SchedulingPolicy {
    enum class Type {
        ROUND_ROBIN,
        LEAST_LOADED,
        RESOURCE_AWARE,
        AFFINITY_BASED,
        ADAPTIVE
    };
    
    Type type = Type::LEAST_LOADED;
    double cpu_weight = 0.4;
    double memory_weight = 0.3;
    double network_weight = 0.3;
    bool enable_work_stealing = true;
    int max_jobs_per_worker = 2;
    double load_balancing_threshold = 0.2;
};

class JobScheduler {
public:
    JobScheduler();
    ~JobScheduler();
    
    // Initialization
    bool initialize(const SchedulingPolicy& policy = SchedulingPolicy{});
    void shutdown();
    bool is_initialized() const { return initialized_; }
    
    // Job management
    JobID submit_job(const JobConfig& config);
    bool cancel_job(JobID job_id);
    bool pause_job(JobID job_id);
    bool resume_job(JobID job_id);
    JobStatus get_job_status(JobID job_id) const;
    std::vector<JobStatus> get_all_job_status() const;
    
    // Worker management
    bool register_worker(int worker_id, int rank, const std::string& hostname);
    bool unregister_worker(int worker_id);
    bool update_worker_status(int worker_id, double cpu_usage, double memory_usage, double network_usage);
    std::vector<WorkerInfo> get_worker_info() const;
    bool assign_job_to_worker(JobID job_id, int worker_id);
    bool remove_job_from_worker(JobID job_id, int worker_id);
    
    // Scheduling
    int select_worker_for_job(const JobConfig& config);
    bool schedule_job(JobID job_id);
    bool reschedule_job(JobID job_id);
    void run_scheduler_loop();
    void stop_scheduler_loop();
    
    // Monitoring and metrics
    double get_system_load() const;
    double get_job_queue_length() const;
    double get_average_job_completion_time() const;
    double get_worker_utilization() const;
    std::vector<double> get_performance_metrics() const;
    
    // Fault tolerance
    bool handle_worker_failure(int worker_id);
    bool redistribute_failed_jobs(JobID failed_job_id);
    bool checkpoint_job_state(JobID job_id);
    bool restore_job_state(JobID job_id);
    
    // Configuration
    void set_scheduling_policy(const SchedulingPolicy& policy);
    const SchedulingPolicy& get_scheduling_policy() const { return policy_; }
    void set_max_concurrent_jobs(int max_jobs);
    int get_max_concurrent_jobs() const { return max_concurrent_jobs_; }

private:
    bool initialized_;
    SchedulingPolicy policy_;
    std::atomic<bool> running_;
    std::thread scheduler_thread_;
    
    // Job management
    std::unordered_map<JobID, JobConfig> pending_jobs_;
    std::unordered_map<JobID, JobStatus> job_status_;
    std::queue<JobID> job_queue_;
    std::mutex job_mutex_;
    std::condition_variable job_cv_;
    
    // Worker management
    std::unordered_map<int, WorkerInfo> workers_;
    std::mutex worker_mutex_;
    
    // Performance tracking
    std::vector<double> job_completion_times_;
    std::vector<double> worker_utilizations_;
    std::mutex metrics_mutex_;
    
    // Configuration
    int max_concurrent_jobs_;
    int next_job_id_;
    
    // Helper methods
    JobID generate_job_id();
    bool validate_job_config(const JobConfig& config);
    double calculate_worker_score(const WorkerInfo& worker, const JobConfig& config);
    bool check_worker_availability(int worker_id);
    void update_job_progress(JobID job_id, double progress);
    void handle_job_completion(JobID job_id, const JobResult& result);
    void handle_job_failure(JobID job_id, const std::string& error);
    void cleanup_completed_jobs();
    void monitor_worker_health();
    void log_scheduler_event(const std::string& event, JobID job_id = -1);
};

class JobManager {
public:
    JobManager();
    ~JobManager();
    
    // Initialization
    bool initialize(const std::string& config_file = "");
    void shutdown();
    bool is_initialized() const { return initialized_; }
    
    // High-level job management
    JobID submit_job(const JobConfig& config);
    bool wait_for_job_completion(JobID job_id, int timeout_seconds = -1);
    JobResult get_job_result(JobID job_id);
    bool cancel_job(JobID job_id);
    
    // Batch operations
    std::vector<JobID> submit_batch_jobs(const std::vector<JobConfig>& configs);
    std::vector<JobResult> wait_for_batch_completion(const std::vector<JobID>& job_ids);
    
    // System management
    bool start_workers(int num_workers);
    bool stop_workers();
    bool restart_workers();
    std::vector<WorkerInfo> get_worker_status();
    
    // Monitoring
    void start_monitoring();
    void stop_monitoring();
    std::vector<double> get_system_metrics();
    void generate_performance_report(const std::string& output_file);
    
    // Configuration
    bool load_configuration(const std::string& config_file);
    bool save_configuration(const std::string& config_file);
    void set_log_level(LogLevel level);
    
    // Access to components
    JobScheduler* get_scheduler() { return scheduler_.get(); }
    StorageManager* get_storage_manager() { return storage_manager_.get(); }

private:
    bool initialized_;
    std::unique_ptr<JobScheduler> scheduler_;
    std::unique_ptr<StorageManager> storage_manager_;
    std::thread monitoring_thread_;
    std::atomic<bool> monitoring_running_;
    
    // Configuration
    std::string config_file_;
    LogLevel log_level_;
    int num_workers_;
    
    // Helper methods
    bool setup_mpi_environment();
    bool setup_storage_system();
    bool validate_system_requirements();
    void monitoring_loop();
    void handle_system_events();
    void log_manager_event(const std::string& event, LogLevel level = LogLevel::INFO);
};

// Utility functions for job management
namespace job_manager_utils {
    
    // Job validation
    bool validate_job_config(const JobConfig& config);
    std::string get_job_config_error(const JobConfig& config);
    
    // Resource estimation
    ResourceRequirements estimate_job_resources(const JobConfig& config);
    bool check_resource_availability(const ResourceRequirements& requirements, const WorkerInfo& worker);
    
    // Performance analysis
    struct PerformanceMetrics {
        double throughput;
        double latency;
        double utilization;
        double efficiency;
        std::vector<double> response_times;
    };
    
    PerformanceMetrics analyze_scheduler_performance(const std::vector<JobStatus>& job_history);
    
    // Load balancing
    struct LoadBalancingMetrics {
        double load_imbalance;
        double migration_overhead;
        double convergence_time;
        std::vector<double> worker_loads;
    };
    
    LoadBalancingMetrics analyze_load_balancing(const std::vector<WorkerInfo>& workers);
    
    // Fault tolerance analysis
    struct FaultToleranceMetrics {
        double availability;
        double reliability;
        double mean_time_to_failure;
        double mean_time_to_recovery;
        std::vector<std::string> failure_modes;
    };
    
    FaultToleranceMetrics analyze_fault_tolerance(const std::vector<JobStatus>& job_history);
    
} // namespace job_manager_utils

} // namespace dds 