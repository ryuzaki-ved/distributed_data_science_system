#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <queue>

namespace dds {
namespace resources {

// Resource types
enum class ResourceType {
    CPU_CORE,
    GPU_DEVICE,
    MEMORY_BLOCK,
    STORAGE_SPACE,
    NETWORK_BANDWIDTH,
    DATABASE_CONNECTION,
    THREAD_WORKER
};

// Resource allocation priority
enum class AllocationPriority {
    LOW,
    NORMAL,
    HIGH,
    CRITICAL
};

// Resource state
enum class ResourceState {
    AVAILABLE,
    ALLOCATED,
    RESERVED,
    MAINTENANCE,
    FAILED
};

// Resource descriptor
struct Resource {
    std::string resource_id;
    ResourceType type;
    ResourceState state;
    std::string owner_id;
    std::chrono::system_clock::time_point allocated_at;
    std::chrono::system_clock::time_point expires_at;
    std::map<std::string, std::string> properties;
    double utilization;
    int64_t capacity;
    int64_t used_capacity;
};

// Resource request
struct ResourceRequest {
    std::string request_id;
    std::string requester_id;
    ResourceType resource_type;
    AllocationPriority priority;
    int64_t required_capacity;
    std::chrono::seconds max_wait_time;
    std::map<std::string, std::string> requirements;
    std::chrono::system_clock::time_point requested_at;
    bool auto_release;
    std::chrono::seconds lease_duration;
};

// Resource allocation result
struct AllocationResult {
    std::string request_id;
    bool success;
    std::vector<std::string> allocated_resources;
    std::string error_message;
    std::chrono::system_clock::time_point allocated_at;
    std::chrono::system_clock::time_point expires_at;
};

// Resource pool manager
class ResourcePool {
private:
    std::map<ResourceType, std::vector<std::shared_ptr<Resource>>> resource_pools_;
    std::queue<ResourceRequest> pending_requests_;
    std::map<std::string, AllocationResult> active_allocations_;
    std::map<std::string, std::vector<std::string>> user_allocations_;
    
    std::mutex pool_mutex_;
    std::condition_variable resource_available_;
    std::atomic<bool> running_;
    
    // Resource monitoring
    std::thread monitor_thread_;
    std::chrono::seconds monitoring_interval_;
    
    // Statistics
    std::map<ResourceType, int> allocation_counts_;
    std::map<ResourceType, std::chrono::milliseconds> total_allocation_time_;
    std::map<std::string, int> user_allocation_counts_;

public:
    ResourcePool() : running_(false), monitoring_interval_(std::chrono::seconds(30)) {}
    ~ResourcePool() { stop(); }
    
    // Pool management
    void start();
    void stop();
    bool is_running() const { return running_; }
    
    // Resource registration
    void register_resource(ResourceType type, const std::string& resource_id, 
                          int64_t capacity, const std::map<std::string, std::string>& properties = {});
    void unregister_resource(const std::string& resource_id);
    void set_resource_state(const std::string& resource_id, ResourceState state);
    
    // Resource allocation
    AllocationResult allocate_resource(const ResourceRequest& request);
    AllocationResult allocate_resources(const std::vector<ResourceRequest>& requests);
    bool release_resource(const std::string& resource_id, const std::string& user_id);
    bool release_all_user_resources(const std::string& user_id);
    
    // Resource queries
    std::vector<std::shared_ptr<Resource>> get_available_resources(ResourceType type) const;
    std::vector<std::shared_ptr<Resource>> get_allocated_resources(const std::string& user_id) const;
    std::shared_ptr<Resource> get_resource(const std::string& resource_id) const;
    
    // Resource reservation
    bool reserve_resource(const std::string& resource_id, const std::string& user_id, 
                         std::chrono::seconds duration);
    bool cancel_reservation(const std::string& resource_id, const std::string& user_id);
    
    // Pool statistics
    int get_total_resources(ResourceType type) const;
    int get_available_count(ResourceType type) const;
    int get_allocated_count(ResourceType type) const;
    double get_pool_utilization(ResourceType type) const;
    std::map<ResourceType, double> get_utilization_summary() const;
    
    // User management
    std::vector<std::string> get_user_resources(const std::string& user_id) const;
    int get_user_allocation_count(const std::string& user_id) const;
    void set_user_quota(const std::string& user_id, ResourceType type, int max_resources);
    
    // Monitoring and maintenance
    void perform_health_check();
    void cleanup_expired_allocations();
    void defragment_pools();
    
    // Reporting
    void print_pool_status() const;
    void print_resource_summary(ResourceType type) const;
    void print_user_allocations(const std::string& user_id) const;
    void export_pool_metrics(const std::string& filename) const;
    
    // Configuration
    void set_monitoring_interval(std::chrono::seconds interval) { monitoring_interval_ = interval; }
    void enable_auto_scaling(ResourceType type, int min_resources, int max_resources);

private:
    void monitoring_loop();
    std::shared_ptr<Resource> find_best_resource(const ResourceRequest& request);
    bool meets_requirements(const Resource& resource, const ResourceRequest& request) const;
    void process_pending_requests();
    void update_resource_utilization();
    std::string generate_request_id() const;
    std::string resource_type_to_string(ResourceType type) const;
    std::string resource_state_to_string(ResourceState state) const;
    bool check_user_quota(const std::string& user_id, ResourceType type) const;
};

// Resource factory for common resource types
class ResourceFactory {
public:
    static std::shared_ptr<Resource> create_cpu_core(const std::string& id, int core_count = 1);
    static std::shared_ptr<Resource> create_gpu_device(const std::string& id, const std::string& model, int memory_gb);
    static std::shared_ptr<Resource> create_memory_block(const std::string& id, int64_t size_mb);
    static std::shared_ptr<Resource> create_storage_space(const std::string& id, int64_t size_gb, const std::string& type);
    static std::shared_ptr<Resource> create_database_connection(const std::string& id, const std::string& db_type);
    static std::shared_ptr<Resource> create_thread_worker(const std::string& id, int thread_count = 1);
};

} // namespace resources
} // namespace dds
