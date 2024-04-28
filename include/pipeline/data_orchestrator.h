#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace dds {
namespace pipeline {

// Pipeline task status
enum class TaskStatus {
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED,
    SKIPPED,
    CANCELLED
};

// Task execution context
struct TaskContext {
    std::string task_id;
    std::string pipeline_id;
    std::map<std::string, std::string> parameters;
    std::map<std::string, std::string> input_data;
    std::map<std::string, std::string> output_data;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
};

// Pipeline task definition
class PipelineTask {
private:
    std::string task_id_;
    std::string name_;
    std::string description_;
    std::vector<std::string> dependencies_;
    std::function<bool(TaskContext&)> executor_;
    TaskStatus status_;
    std::string error_message_;
    int retry_count_;
    int max_retries_;

public:
    PipelineTask(const std::string& id, const std::string& name, 
                 std::function<bool(TaskContext&)> executor);
    
    // Configuration
    void set_description(const std::string& description) { description_ = description; }
    void add_dependency(const std::string& task_id) { dependencies_.push_back(task_id); }
    void set_max_retries(int retries) { max_retries_ = retries; }
    
    // Execution
    bool execute(TaskContext& context);
    bool can_execute(const std::map<std::string, TaskStatus>& task_statuses) const;
    
    // Status management
    TaskStatus get_status() const { return status_; }
    void set_status(TaskStatus status) { status_ = status; }
    std::string get_error_message() const { return error_message_; }
    
    // Getters
    std::string get_id() const { return task_id_; }
    std::string get_name() const { return name_; }
    std::vector<std::string> get_dependencies() const { return dependencies_; }
    int get_retry_count() const { return retry_count_; }
};

// Pipeline execution result
struct PipelineResult {
    std::string pipeline_id;
    bool success;
    std::chrono::milliseconds total_duration;
    std::map<std::string, TaskStatus> task_results;
    std::vector<std::string> failed_tasks;
    std::string error_summary;
};

// Data pipeline orchestrator
class DataOrchestrator {
private:
    std::map<std::string, std::vector<std::shared_ptr<PipelineTask>>> pipelines_;
    std::map<std::string, TaskStatus> task_statuses_;
    std::map<std::string, PipelineResult> execution_history_;
    std::mutex orchestrator_mutex_;
    std::condition_variable task_cv_;
    std::atomic<bool> running_;
    int max_concurrent_tasks_;
    int active_tasks_;

public:
    DataOrchestrator(int max_concurrent = 4) 
        : running_(false), max_concurrent_tasks_(max_concurrent), active_tasks_(0) {}
    
    // Pipeline management
    void create_pipeline(const std::string& pipeline_id);
    void add_task_to_pipeline(const std::string& pipeline_id, std::shared_ptr<PipelineTask> task);
    bool remove_pipeline(const std::string& pipeline_id);
    
    // Task creation helpers
    std::shared_ptr<PipelineTask> create_data_ingestion_task(const std::string& source, const std::string& destination);
    std::shared_ptr<PipelineTask> create_data_transformation_task(const std::string& transform_type, const std::map<std::string, std::string>& config);
    std::shared_ptr<PipelineTask> create_data_validation_task(const std::vector<std::string>& validation_rules);
    std::shared_ptr<PipelineTask> create_ml_training_task(const std::string& algorithm, const std::map<std::string, std::string>& params);
    std::shared_ptr<PipelineTask> create_data_export_task(const std::string& destination, const std::string& format);
    
    // Execution
    PipelineResult execute_pipeline(const std::string& pipeline_id, const std::map<std::string, std::string>& parameters = {});
    void execute_pipeline_async(const std::string& pipeline_id, const std::map<std::string, std::string>& parameters = {});
    bool cancel_pipeline(const std::string& pipeline_id);
    
    // Monitoring
    std::vector<std::string> get_pipeline_list() const;
    std::vector<std::string> get_running_pipelines() const;
    PipelineResult get_execution_result(const std::string& pipeline_id) const;
    std::map<std::string, TaskStatus> get_pipeline_status(const std::string& pipeline_id) const;
    
    // Scheduling and triggers
    void schedule_pipeline(const std::string& pipeline_id, const std::string& cron_expression);
    void trigger_on_data_change(const std::string& pipeline_id, const std::string& data_path);
    void trigger_on_pipeline_completion(const std::string& trigger_pipeline, const std::string& target_pipeline);
    
    // Reporting
    void print_pipeline_summary(const std::string& pipeline_id) const;
    void print_execution_report(const std::string& pipeline_id) const;
    void export_pipeline_metrics(const std::string& filename) const;
    
    // Control
    void start_orchestrator() { running_ = true; }
    void stop_orchestrator() { running_ = false; }
    bool is_running() const { return running_; }

private:
    bool validate_pipeline_dependencies(const std::string& pipeline_id) const;
    std::vector<std::shared_ptr<PipelineTask>> get_executable_tasks(const std::string& pipeline_id) const;
    void execute_task_async(std::shared_ptr<PipelineTask> task, TaskContext context);
    void update_task_status(const std::string& task_id, TaskStatus status);
    std::string generate_task_id(const std::string& prefix) const;
    std::string status_to_string(TaskStatus status) const;
};

} // namespace pipeline
} // namespace dds
