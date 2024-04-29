#include "../../include/resources/resource_pool.h"
#include <iostream>
#include <algorithm>
#include <thread>
#include <random>
#include <fstream>
#include <iomanip>

namespace dds {
namespace resources {

void ResourcePool::start() {
    if (running_) return;
    
    running_ = true;
    monitor_thread_ = std::thread(&ResourcePool::monitoring_loop, this);
    std::cout << "🔧 Resource pool manager started" << std::endl;
}

void ResourcePool::stop() {
    if (!running_) return;
    
    running_ = false;
    resource_available_.notify_all();
    
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
    
    std::cout << "🛑 Resource pool manager stopped" << std::endl;
}

void ResourcePool::register_resource(ResourceType type, const std::string& resource_id, 
                                    int64_t capacity, const std::map<std::string, std::string>& properties) {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    auto resource = std::make_shared<Resource>();
    resource->resource_id = resource_id;
    resource->type = type;
    resource->state = ResourceState::AVAILABLE;
    resource->capacity = capacity;
    resource->used_capacity = 0;
    resource->utilization = 0.0;
    resource->properties = properties;
    
    resource_pools_[type].push_back(resource);
    
    std::cout << "➕ Registered " << resource_type_to_string(type) 
              << " resource: " << resource_id << " (capacity: " << capacity << ")" << std::endl;
}

void ResourcePool::unregister_resource(const std::string& resource_id) {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    for (auto& pool_pair : resource_pools_) {
        auto& resources = pool_pair.second;
        resources.erase(
            std::remove_if(resources.begin(), resources.end(),
                [&resource_id](const std::shared_ptr<Resource>& res) {
                    return res->resource_id == resource_id;
                }),
            resources.end());
    }
    
    std::cout << "➖ Unregistered resource: " << resource_id << std::endl;
}

AllocationResult ResourcePool::allocate_resource(const ResourceRequest& request) {
    std::unique_lock<std::mutex> lock(pool_mutex_);
    
    AllocationResult result;
    result.request_id = request.request_id;
    result.success = false;
    result.allocated_at = std::chrono::system_clock::now();
    
    // Check user quota
    if (!check_user_quota(request.requester_id, request.resource_type)) {
        result.error_message = "User quota exceeded";
        return result;
    }
    
    // Find suitable resource
    auto resource = find_best_resource(request);
    if (!resource) {
        // Wait for resource if allowed
        if (request.max_wait_time.count() > 0) {
            pending_requests_.push(request);
            
            auto timeout = std::chrono::system_clock::now() + request.max_wait_time;
            if (resource_available_.wait_until(lock, timeout) == std::cv_status::timeout) {
                result.error_message = "Allocation timeout";
                return result;
            }
            
            // Try again after wait
            resource = find_best_resource(request);
        }
        
        if (!resource) {
            result.error_message = "No suitable resource available";
            return result;
        }
    }
    
    // Allocate resource
    resource->state = ResourceState::ALLOCATED;
    resource->owner_id = request.requester_id;
    resource->allocated_at = std::chrono::system_clock::now();
    resource->used_capacity = request.required_capacity;
    resource->utilization = static_cast<double>(resource->used_capacity) / resource->capacity;
    
    if (request.auto_release && request.lease_duration.count() > 0) {
        resource->expires_at = resource->allocated_at + request.lease_duration;
    }
    
    result.success = true;
    result.allocated_resources.push_back(resource->resource_id);
    result.expires_at = resource->expires_at;
    
    // Update tracking
    active_allocations_[resource->resource_id] = result;
    user_allocations_[request.requester_id].push_back(resource->resource_id);
    allocation_counts_[request.resource_type]++;
    user_allocation_counts_[request.requester_id]++;
    
    std::cout << "✅ Allocated " << resource_type_to_string(request.resource_type) 
              << " resource: " << resource->resource_id << " to user: " << request.requester_id << std::endl;
    
    return result;
}

bool ResourcePool::release_resource(const std::string& resource_id, const std::string& user_id) {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    auto resource = get_resource(resource_id);
    if (!resource) {
        std::cout << "❌ Resource not found: " << resource_id << std::endl;
        return false;
    }
    
    if (resource->owner_id != user_id) {
        std::cout << "❌ Unauthorized release attempt by user: " << user_id << std::endl;
        return false;
    }
    
    // Release resource
    resource->state = ResourceState::AVAILABLE;
    resource->owner_id.clear();
    resource->used_capacity = 0;
    resource->utilization = 0.0;
    resource->expires_at = std::chrono::system_clock::time_point{};
    
    // Update tracking
    active_allocations_.erase(resource_id);
    auto& user_resources = user_allocations_[user_id];
    user_resources.erase(
        std::remove(user_resources.begin(), user_resources.end(), resource_id),
        user_resources.end());
    
    std::cout << "🔓 Released resource: " << resource_id << " from user: " << user_id << std::endl;
    
    // Notify waiting requests
    resource_available_.notify_all();
    process_pending_requests();
    
    return true;
}

bool ResourcePool::release_all_user_resources(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    auto it = user_allocations_.find(user_id);
    if (it == user_allocations_.end()) {
        return true; // No resources to release
    }
    
    auto resource_ids = it->second; // Copy the list
    bool all_released = true;
    
    for (const auto& resource_id : resource_ids) {
        auto resource = get_resource(resource_id);
        if (resource && resource->owner_id == user_id) {
            resource->state = ResourceState::AVAILABLE;
            resource->owner_id.clear();
            resource->used_capacity = 0;
            resource->utilization = 0.0;
            resource->expires_at = std::chrono::system_clock::time_point{};
            active_allocations_.erase(resource_id);
        } else {
            all_released = false;
        }
    }
    
    user_allocations_[user_id].clear();
    
    std::cout << "🔓 Released all resources for user: " << user_id << std::endl;
    resource_available_.notify_all();
    
    return all_released;
}

std::vector<std::shared_ptr<Resource>> ResourcePool::get_available_resources(ResourceType type) const {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    std::vector<std::shared_ptr<Resource>> available;
    
    auto it = resource_pools_.find(type);
    if (it != resource_pools_.end()) {
        for (const auto& resource : it->second) {
            if (resource->state == ResourceState::AVAILABLE) {
                available.push_back(resource);
            }
        }
    }
    
    return available;
}

std::shared_ptr<Resource> ResourcePool::get_resource(const std::string& resource_id) const {
    for (const auto& pool_pair : resource_pools_) {
        for (const auto& resource : pool_pair.second) {
            if (resource->resource_id == resource_id) {
                return resource;
            }
        }
    }
    return nullptr;
}

int ResourcePool::get_available_count(ResourceType type) const {
    return get_available_resources(type).size();
}

int ResourcePool::get_allocated_count(ResourceType type) const {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    int count = 0;
    
    auto it = resource_pools_.find(type);
    if (it != resource_pools_.end()) {
        for (const auto& resource : it->second) {
            if (resource->state == ResourceState::ALLOCATED) {
                count++;
            }
        }
    }
    
    return count;
}

double ResourcePool::get_pool_utilization(ResourceType type) const {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    auto it = resource_pools_.find(type);
    if (it == resource_pools_.end() || it->second.empty()) {
        return 0.0;
    }
    
    double total_utilization = 0.0;
    for (const auto& resource : it->second) {
        total_utilization += resource->utilization;
    }
    
    return total_utilization / it->second.size();
}

void ResourcePool::print_pool_status() const {
    std::cout << "\n🔧 Resource Pool Status" << std::endl;
    std::cout << "======================" << std::endl;
    
    for (const auto& pool_pair : resource_pools_) {
        ResourceType type = pool_pair.first;
        const auto& resources = pool_pair.second;
        
        int available = 0, allocated = 0, reserved = 0, maintenance = 0;
        
        for (const auto& resource : resources) {
            switch (resource->state) {
                case ResourceState::AVAILABLE: available++; break;
                case ResourceState::ALLOCATED: allocated++; break;
                case ResourceState::RESERVED: reserved++; break;
                case ResourceState::MAINTENANCE: maintenance++; break;
                default: break;
            }
        }
        
        std::cout << resource_type_to_string(type) << ":" << std::endl;
        std::cout << "  Total: " << resources.size() 
                  << ", Available: " << available
                  << ", Allocated: " << allocated
                  << ", Reserved: " << reserved
                  << ", Maintenance: " << maintenance << std::endl;
        std::cout << "  Utilization: " << std::fixed << std::setprecision(1) 
                  << get_pool_utilization(type) * 100 << "%" << std::endl;
    }
    
    std::cout << "\nActive allocations: " << active_allocations_.size() << std::endl;
    std::cout << "Pending requests: " << pending_requests_.size() << std::endl;
}

void ResourcePool::cleanup_expired_allocations() {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    auto now = std::chrono::system_clock::now();
    std::vector<std::string> expired_resources;
    
    for (const auto& pool_pair : resource_pools_) {
        for (const auto& resource : pool_pair.second) {
            if (resource->state == ResourceState::ALLOCATED && 
                resource->expires_at != std::chrono::system_clock::time_point{} &&
                now > resource->expires_at) {
                expired_resources.push_back(resource->resource_id);
            }
        }
    }
    
    for (const auto& resource_id : expired_resources) {
        auto resource = get_resource(resource_id);
        if (resource) {
            std::cout << "⏰ Auto-releasing expired resource: " << resource_id << std::endl;
            release_resource(resource_id, resource->owner_id);
        }
    }
}

void ResourcePool::monitoring_loop() {
    while (running_) {
        std::this_thread::sleep_for(monitoring_interval_);
        
        if (!running_) break;
        
        cleanup_expired_allocations();
        update_resource_utilization();
        process_pending_requests();
    }
}

std::shared_ptr<Resource> ResourcePool::find_best_resource(const ResourceRequest& request) {
    auto it = resource_pools_.find(request.resource_type);
    if (it == resource_pools_.end()) {
        return nullptr;
    }
    
    std::shared_ptr<Resource> best_resource = nullptr;
    double best_score = -1.0;
    
    for (const auto& resource : it->second) {
        if (resource->state != ResourceState::AVAILABLE) continue;
        if (!meets_requirements(*resource, request)) continue;
        
        // Score based on capacity match and current utilization
        double capacity_score = static_cast<double>(request.required_capacity) / resource->capacity;
        double utilization_score = 1.0 - resource->utilization;
        double total_score = (capacity_score * 0.7) + (utilization_score * 0.3);
        
        if (total_score > best_score) {
            best_score = total_score;
            best_resource = resource;
        }
    }
    
    return best_resource;
}

bool ResourcePool::meets_requirements(const Resource& resource, const ResourceRequest& request) const {
    if (resource.capacity < request.required_capacity) {
        return false;
    }
    
    for (const auto& req_pair : request.requirements) {
        auto it = resource.properties.find(req_pair.first);
        if (it == resource.properties.end() || it->second != req_pair.second) {
            return false;
        }
    }
    
    return true;
}

void ResourcePool::process_pending_requests() {
    while (!pending_requests_.empty()) {
        auto request = pending_requests_.front();
        pending_requests_.pop();
        
        auto result = allocate_resource(request);
        if (!result.success) {
            // Put back in queue if still valid
            auto now = std::chrono::system_clock::now();
            if (now < (request.requested_at + request.max_wait_time)) {
                pending_requests_.push(request);
            }
            break; // Try again later
        }
    }
}

void ResourcePool::update_resource_utilization() {
    // Simulate resource utilization updates
    for (auto& pool_pair : resource_pools_) {
        for (auto& resource : pool_pair.second) {
            if (resource->state == ResourceState::ALLOCATED) {
                // Simulate some fluctuation in utilization
                static std::random_device rd;
                static std::mt19937 gen(rd());
                static std::uniform_real_distribution<> dis(0.8, 1.0);
                
                resource->utilization = std::min(1.0, resource->utilization * dis(gen));
            }
        }
    }
}

bool ResourcePool::check_user_quota(const std::string& user_id, ResourceType type) const {
    // Simplified quota check - in real implementation would have configurable quotas
    int current_count = get_user_allocation_count(user_id);
    return current_count < 10; // Default quota of 10 resources per user
}

std::string ResourcePool::resource_type_to_string(ResourceType type) const {
    switch (type) {
        case ResourceType::CPU_CORE: return "CPU_CORE";
        case ResourceType::GPU_DEVICE: return "GPU_DEVICE";
        case ResourceType::MEMORY_BLOCK: return "MEMORY_BLOCK";
        case ResourceType::STORAGE_SPACE: return "STORAGE_SPACE";
        case ResourceType::NETWORK_BANDWIDTH: return "NETWORK_BANDWIDTH";
        case ResourceType::DATABASE_CONNECTION: return "DATABASE_CONNECTION";
        case ResourceType::THREAD_WORKER: return "THREAD_WORKER";
        default: return "UNKNOWN";
    }
}

// ResourceFactory implementations
std::shared_ptr<Resource> ResourceFactory::create_cpu_core(const std::string& id, int core_count) {
    auto resource = std::make_shared<Resource>();
    resource->resource_id = id;
    resource->type = ResourceType::CPU_CORE;
    resource->state = ResourceState::AVAILABLE;
    resource->capacity = core_count;
    resource->properties["core_count"] = std::to_string(core_count);
    return resource;
}

std::shared_ptr<Resource> ResourceFactory::create_gpu_device(const std::string& id, const std::string& model, int memory_gb) {
    auto resource = std::make_shared<Resource>();
    resource->resource_id = id;
    resource->type = ResourceType::GPU_DEVICE;
    resource->state = ResourceState::AVAILABLE;
    resource->capacity = memory_gb * 1024; // MB
    resource->properties["model"] = model;
    resource->properties["memory_gb"] = std::to_string(memory_gb);
    return resource;
}

std::shared_ptr<Resource> ResourceFactory::create_memory_block(const std::string& id, int64_t size_mb) {
    auto resource = std::make_shared<Resource>();
    resource->resource_id = id;
    resource->type = ResourceType::MEMORY_BLOCK;
    resource->state = ResourceState::AVAILABLE;
    resource->capacity = size_mb;
    resource->properties["size_mb"] = std::to_string(size_mb);
    return resource;
}

} // namespace resources
} // namespace dds
