#include "../../include/pipeline/data_orchestrator.h"
#include <iostream>
#include <thread>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <random>

namespace dds {
namespace pipeline {

PipelineTask::PipelineTask(const std::string& id, const std::string& name, 
                          std::function<bool(TaskContext&)> executor)
    : task_id_(id), name_(name), executor_(executor), status_(TaskStatus::PENDING),
      retry_count_(0), max_retries_(3) {
}

bool PipelineTask::execute(TaskContext& context) {
    context.task_id = task_id_;
    context.start_time = std::chrono::system_clock::now();
    status_ = TaskStatus::RUNNING;
    
    try {
        bool success = executor_(context);
        context.end_time = std::chrono::system_clock::now();
        
        if (success) {
            status_ = TaskStatus::COMPLETED;
            std::cout << "✅ Task completed: " << name_ << std::endl;
            return true;
        } else {
            if (retry_count_ < max_retries_) {
                retry_count_++;
                status_ = TaskStatus::PENDING;
                std::cout << "🔄 Task retry " << retry_count_ << "/" << max_retries_ << ": " << name_ << std::endl;
                return false;
            } else {
                status_ = TaskStatus::FAILED;
                error_message_ = "Task execution failed after " + std::to_string(max_retries_) + " retries";
                std::cout << "❌ Task failed: " << name_ << " - " << error_message_ << std::endl;
                return false;
            }
        }
    } catch (const std::exception& e) {
        context.end_time = std::chrono::system_clock::now();
        status_ = TaskStatus::FAILED;
        error_message_ = e.what();
        std::cout << "❌ Task exception: " << name_ << " - " << error_message_ << std::endl;
        return false;
    }
}

bool PipelineTask::can_execute(const std::map<std::string, TaskStatus>& task_statuses) const {
    if (status_ != TaskStatus::PENDING) return false;
    
    for (const auto& dep : dependencies_) {
        auto it = task_statuses.find(dep);
        if (it == task_statuses.end() || it->second != TaskStatus::COMPLETED) {
            return false;
        }
    }
    
    return true;
}

void DataOrchestrator::create_pipeline(const std::string& pipeline_id) {
    std::lock_guard<std::mutex> lock(orchestrator_mutex_);
    pipelines_[pipeline_id] = std::vector<std::shared_ptr<PipelineTask>>();
    std::cout << "📋 Created pipeline: " << pipeline_id << std::endl;
}

void DataOrchestrator::add_task_to_pipeline(const std::string& pipeline_id, std::shared_ptr<PipelineTask> task) {
    std::lock_guard<std::mutex> lock(orchestrator_mutex_);
    
    auto it = pipelines_.find(pipeline_id);
    if (it != pipelines_.end()) {
        it->second.push_back(task);
        std::cout << "➕ Added task '" << task->get_name() << "' to pipeline: " << pipeline_id << std::endl;
    } else {
        std::cout << "❌ Pipeline not found: " << pipeline_id << std::endl;
    }
}

std::shared_ptr<PipelineTask> DataOrchestrator::create_data_ingestion_task(const std::string& source, const std::string& destination) {
    std::string task_id = generate_task_id("ingest");
    
    return std::make_shared<PipelineTask>(task_id, "Data Ingestion: " + source,
        [source, destination](TaskContext& ctx) -> bool {
            std::cout << "📥 Ingesting data from " << source << " to " << destination << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Simulate work
            ctx.output_data["ingested_records"] = "1000";
            ctx.output_data["destination_path"] = destination;
            return true;
        });
}

std::shared_ptr<PipelineTask> DataOrchestrator::create_data_transformation_task(const std::string& transform_type, const std::map<std::string, std::string>& config) {
    std::string task_id = generate_task_id("transform");
    
    return std::make_shared<PipelineTask>(task_id, "Data Transformation: " + transform_type,
        [transform_type, config](TaskContext& ctx) -> bool {
            std::cout << "🔄 Applying transformation: " << transform_type << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(800)); // Simulate work
            ctx.output_data["transformed_records"] = ctx.input_data["ingested_records"];
            ctx.output_data["transformation_type"] = transform_type;
            return true;
        });
}

std::shared_ptr<PipelineTask> DataOrchestrator::create_data_validation_task(const std::vector<std::string>& validation_rules) {
    std::string task_id = generate_task_id("validate");
    
    return std::make_shared<PipelineTask>(task_id, "Data Validation",
        [validation_rules](TaskContext& ctx) -> bool {
            std::cout << "✅ Validating data with " << validation_rules.size() << " rules" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(300)); // Simulate work
            ctx.output_data["validation_status"] = "passed";
            ctx.output_data["validated_records"] = ctx.input_data["transformed_records"];
            return true;
        });
}

std::shared_ptr<PipelineTask> DataOrchestrator::create_ml_training_task(const std::string& algorithm, const std::map<std::string, std::string>& params) {
    std::string task_id = generate_task_id("ml_train");
    
    return std::make_shared<PipelineTask>(task_id, "ML Training: " + algorithm,
        [algorithm, params](TaskContext& ctx) -> bool {
            std::cout << "🤖 Training ML model with " << algorithm << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // Simulate training
            ctx.output_data["model_id"] = "model_" + std::to_string(std::rand() % 10000);
            ctx.output_data["accuracy"] = "0.94";
            ctx.output_data["algorithm"] = algorithm;
            return true;
        });
}

std::shared_ptr<PipelineTask> DataOrchestrator::create_data_export_task(const std::string& destination, const std::string& format) {
    std::string task_id = generate_task_id("export");
    
    return std::make_shared<PipelineTask>(task_id, "Data Export: " + format,
        [destination, format](TaskContext& ctx) -> bool {
            std::cout << "📤 Exporting data to " << destination << " in " << format << " format" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(400)); // Simulate export
            ctx.output_data["export_path"] = destination;
            ctx.output_data["export_format"] = format;
            ctx.output_data["exported_size"] = "15.2MB";
            return true;
        });
}

PipelineResult DataOrchestrator::execute_pipeline(const std::string& pipeline_id, const std::map<std::string, std::string>& parameters) {
    std::cout << "🚀 Starting pipeline execution: " << pipeline_id << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    PipelineResult result;
    result.pipeline_id = pipeline_id;
    result.success = true;
    
    std::lock_guard<std::mutex> lock(orchestrator_mutex_);
    auto it = pipelines_.find(pipeline_id);
    if (it == pipelines_.end()) {
        result.success = false;
        result.error_summary = "Pipeline not found: " + pipeline_id;
        return result;
    }
    
    auto& tasks = it->second;
    std::map<std::string, TaskStatus> local_task_statuses;
    
    // Initialize task statuses
    for (auto& task : tasks) {
        local_task_statuses[task->get_id()] = TaskStatus::PENDING;
        task->set_status(TaskStatus::PENDING);
    }
    
    // Execute tasks in dependency order
    bool all_completed = false;
    int max_iterations = tasks.size() * 2; // Prevent infinite loops
    int iteration = 0;
    
    while (!all_completed && iteration < max_iterations) {
        bool progress_made = false;
        
        for (auto& task : tasks) {
            if (task->can_execute(local_task_statuses)) {
                TaskContext context;
                context.pipeline_id = pipeline_id;
                context.parameters = parameters;
                
                // Pass output from dependencies as input
                for (const auto& dep_id : task->get_dependencies()) {
                    // In a real implementation, you'd pass actual data between tasks
                }
                
                bool success = task->execute(context);
                local_task_statuses[task->get_id()] = task->get_status();
                
                if (!success && task->get_status() == TaskStatus::FAILED) {
                    result.failed_tasks.push_back(task->get_id());
                    result.success = false;
                }
                
                progress_made = true;
            }
        }
        
        // Check if all tasks are completed or failed
        all_completed = true;
        for (const auto& pair : local_task_statuses) {
            if (pair.second == TaskStatus::PENDING || pair.second == TaskStatus::RUNNING) {
                all_completed = false;
                break;
            }
        }
        
        if (!progress_made && !all_completed) {
            result.success = false;
            result.error_summary = "Pipeline execution stalled - possible circular dependencies";
            break;
        }
        
        iteration++;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    result.task_results = local_task_statuses;
    
    // Update global status
    task_statuses_.insert(local_task_statuses.begin(), local_task_statuses.end());
    execution_history_[pipeline_id] = result;
    
    if (result.success) {
        std::cout << "🎉 Pipeline completed successfully: " << pipeline_id 
                  << " (duration: " << result.total_duration.count() << "ms)" << std::endl;
    } else {
        std::cout << "❌ Pipeline failed: " << pipeline_id << " - " << result.error_summary << std::endl;
    }
    
    return result;
}

std::vector<std::string> DataOrchestrator::get_pipeline_list() const {
    std::lock_guard<std::mutex> lock(orchestrator_mutex_);
    std::vector<std::string> pipeline_ids;
    
    for (const auto& pair : pipelines_) {
        pipeline_ids.push_back(pair.first);
    }
    
    return pipeline_ids;
}

void DataOrchestrator::print_pipeline_summary(const std::string& pipeline_id) const {
    std::lock_guard<std::mutex> lock(orchestrator_mutex_);
    
    auto it = pipelines_.find(pipeline_id);
    if (it == pipelines_.end()) {
        std::cout << "Pipeline not found: " << pipeline_id << std::endl;
        return;
    }
    
    std::cout << "\n📋 Pipeline Summary: " << pipeline_id << std::endl;
    std::cout << "=====================" << std::endl;
    std::cout << "Total tasks: " << it->second.size() << std::endl;
    
    for (const auto& task : it->second) {
        std::cout << "  • " << task->get_name() << " [" << status_to_string(task->get_status()) << "]";
        if (!task->get_dependencies().empty()) {
            std::cout << " (depends on: ";
            for (size_t i = 0; i < task->get_dependencies().size(); ++i) {
                std::cout << task->get_dependencies()[i];
                if (i < task->get_dependencies().size() - 1) std::cout << ", ";
            }
            std::cout << ")";
        }
        std::cout << std::endl;
    }
}

void DataOrchestrator::print_execution_report(const std::string& pipeline_id) const {
    auto it = execution_history_.find(pipeline_id);
    if (it == execution_history_.end()) {
        std::cout << "No execution history for pipeline: " << pipeline_id << std::endl;
        return;
    }
    
    const auto& result = it->second;
    std::cout << "\n📊 Execution Report: " << pipeline_id << std::endl;
    std::cout << "========================" << std::endl;
    std::cout << "Status: " << (result.success ? "SUCCESS" : "FAILED") << std::endl;
    std::cout << "Duration: " << result.total_duration.count() << "ms" << std::endl;
    std::cout << "Total tasks: " << result.task_results.size() << std::endl;
    
    if (!result.failed_tasks.empty()) {
        std::cout << "Failed tasks: ";
        for (size_t i = 0; i < result.failed_tasks.size(); ++i) {
            std::cout << result.failed_tasks[i];
            if (i < result.failed_tasks.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
    }
    
    if (!result.error_summary.empty()) {
        std::cout << "Error: " << result.error_summary << std::endl;
    }
}

std::string DataOrchestrator::generate_task_id(const std::string& prefix) const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1000, 9999);
    
    return prefix + "_" + std::to_string(dis(gen));
}

std::string DataOrchestrator::status_to_string(TaskStatus status) const {
    switch (status) {
        case TaskStatus::PENDING: return "PENDING";
        case TaskStatus::RUNNING: return "RUNNING";
        case TaskStatus::COMPLETED: return "COMPLETED";
        case TaskStatus::FAILED: return "FAILED";
        case TaskStatus::SKIPPED: return "SKIPPED";
        case TaskStatus::CANCELLED: return "CANCELLED";
        default: return "UNKNOWN";
    }
}

} // namespace pipeline
} // namespace dds
