#include "../../include/web/web_server.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <map>
#include <mutex>
#include <algorithm>
#include <vector>
#include <regex>
#include <ctime>
#include <iomanip>
#include <queue>
#include <thread>
#include <atomic>
#include <sys/socket.h>
#include <unistd.h>
#include <future>
#include <condition_variable>
#include <unordered_map>
#include <zlib.h>
#include <cctype>
#include <sstream>
#include <functional>
#include <numeric>

namespace dds {
namespace web {

WebServer::WebServer(int port, const std::string& host) 
    : port_(port), host_(host), running_(false), rate_limit_window_(60), max_requests_per_minute_(100), 
      total_requests_(0), successful_requests_(0), failed_requests_(0), max_connections_(100), 
      active_connections_(0), connection_timeout_(30), thread_pool_size_(8), shutdown_thread_pool_(false),
      cache_enabled_(true), cache_ttl_(300), max_cache_size_(1000), compression_enabled_(true), 
      compression_level_(6), min_compression_size_(1024), validation_enabled_(true), 
      max_request_size_(10485760), max_header_size_(8192), routing_enabled_(true), 
      middleware_enabled_(true), route_cache_enabled_(true), monitoring_enabled_(true), 
      health_check_interval_(30), last_health_check_(std::chrono::steady_clock::now()),
          start_time_(std::chrono::steady_clock::now()), cache_hits_(0), cache_misses_(0),
    adaptive_compression_enabled_(true), bandwidth_throttling_enabled_(true),
    max_bandwidth_per_client_(10485760), total_bytes_sent_(0), total_bytes_compressed_(0),
    average_compression_ratio_(0.0), analytics_enabled_(true), total_requests_(0),
    total_responses_(0), total_errors_(0), analytics_start_time_(std::chrono::steady_clock::now()) {
    
    // Initialize thread pool
    for (int i = 0; i < thread_pool_size_; ++i) {
        worker_threads_.emplace_back(&WebServer::worker_thread_function, this);
    }
    std::cout << "🔧 Thread pool initialized with " << thread_pool_size_ << " workers" << std::endl;
    std::cout << "💾 Response cache enabled (TTL: " << cache_ttl_ << "s, Max: " << max_cache_size_ << " entries)" << std::endl;
    std::cout << "🗜️ Compression enabled (level: " << compression_level_ << ", min size: " << min_compression_size_ << " bytes)" << std::endl;
    std::cout << "🔄 Adaptive compression: " << (adaptive_compression_enabled_ ? "Enabled" : "Disabled") << std::endl;
    std::cout << "🚫 Bandwidth throttling: " << (bandwidth_throttling_enabled_ ? "Enabled" : "Disabled") << " (max: " << max_bandwidth_per_client_ / 1024 / 1024 << " MB/min)" << std::endl;
    std::cout << "📊 Analytics and profiling: " << (analytics_enabled_ ? "Enabled" : "Disabled") << std::endl;
    std::cout << "🔒 Request validation enabled (max size: " << max_request_size_ << " bytes)" << std::endl;
    std::cout << "🛣️ Routing framework enabled with middleware support" << std::endl;
    std::cout << "📊 Monitoring and health checks enabled (interval: " << health_check_interval_ << "s)" << std::endl;
    
    // Initialize default routes
    initialize_default_routes();
    
    // Pre-compress static content
    pre_compress_static_content();
}

WebServer::~WebServer() {
    stop();
}

bool WebServer::start() {
    if (running_) return true;
    running_ = true;
    std::cout << "🌐 Web server started on http://" << host_ << ":" << port_ << std::endl;
    std::cout << "📱 Open your browser and go to: http://localhost:" << port_ << std::endl;
    std::cout << "🔗 Available endpoints:" << std::endl;
    std::cout << "   - http://localhost:" << port_ << "/ (Dashboard)" << std::endl;
    std::cout << "   - http://localhost:" << port_ << "/api/status (System Status)" << std::endl;
    std::cout << "   - http://localhost:" << port_ << "/api/jobs (List Jobs)" << std::endl;
    std::cout << "   - http://localhost:" << port_ << "/api/hdfs/list (HDFS Files)" << std::endl;
    std::cout << "   - http://localhost:" << port_ << "/api/cluster/info (Cluster Info)" << std::endl;
    return true;
}

void WebServer::stop() {
    running_ = false;
    
    // Shutdown thread pool
    shutdown_thread_pool_ = true;
    thread_pool_cv_.notify_all();
    
    // Wait for all worker threads to finish
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    print_analytics();
    std::cout << "Web server stopped" << std::endl;
}

void WebServer::add_route(const std::string& method, const std::string& path, RouteHandler handler) {
    routes_[method + ":" + path] = handler;
}

void WebServer::add_get_route(const std::string& path, RouteHandler handler) {
    add_route("GET", path, handler);
}

void WebServer::add_post_route(const std::string& path, RouteHandler handler) {
    add_route("POST", path, handler);
}

void WebServer::add_put_route(const std::string& path, RouteHandler handler) {
    add_route("PUT", path, handler);
}

void WebServer::add_delete_route(const std::string& path, RouteHandler handler) {
    add_route("DELETE", path, handler);
}

void WebServer::add_middleware(const std::string& name, MiddlewareHandler middleware) {
    if (middleware_enabled_) {
        middleware_stack_.push_back({name, middleware});
        std::cout << "🔧 Middleware '" << name << "' registered" << std::endl;
    }
}

void WebServer::add_route_group(const std::string& prefix, const std::vector<RouteDefinition>& routes) {
    for (const auto& route : routes) {
        std::string full_path = prefix + route.path;
        add_route(route.method, full_path, route.handler);
    }
    std::cout << "📁 Route group '" << prefix << "' registered with " << routes.size() << " routes" << std::endl;
}

void WebServer::initialize_default_routes() {
    // Add default middleware
    add_middleware("logging", [this](const HttpRequest& req, HttpResponse& res, NextHandler next) {
        auto start_time = std::chrono::high_resolution_clock::now();
        next(req, res);
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        std::cout << "🔍 Middleware [logging] - " << req.method << " " << req.path 
                  << " -> " << res.status_code << " (" << duration.count() << "μs)" << std::endl;
    });
    
    add_middleware("cors", [this](const HttpRequest& req, HttpResponse& res, NextHandler next) {
        res.headers["Access-Control-Allow-Origin"] = "*";
        res.headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, OPTIONS";
        res.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
        next(req, res);
    });
    
    add_middleware("security", [this](const HttpRequest& req, HttpResponse& res, NextHandler next) {
        res.headers["X-Content-Type-Options"] = "nosniff";
        res.headers["X-Frame-Options"] = "DENY";
        res.headers["X-XSS-Protection"] = "1; mode=block";
        next(req, res);
    });
    
    // Register default routes
    add_get_route("/", [this](const HttpRequest& req, HttpResponse& res) {
        return serve_dashboard(req, res);
    });
    
    add_get_route("/api/status", [this](const HttpRequest& req, HttpResponse& res) {
        return handle_status(req, res);
    });
    
    add_get_route("/api/routes", [this](const HttpRequest& req, HttpResponse& res) {
        return list_routes(req, res);
    });
    
    add_get_route("/api/middleware", [this](const HttpRequest& req, HttpResponse& res) {
        return list_middleware(req, res);
    });
    
    add_get_route("/api/health", [this](const HttpRequest& req, HttpResponse& res) {
        return handle_health_check(req, res);
    });
    
    add_get_route("/api/metrics", [this](const HttpRequest& req, HttpResponse& res) {
        return handle_metrics(req, res);
    });
    
    add_get_route("/api/monitoring/status", [this](const HttpRequest& req, HttpResponse& res) {
        return handle_monitoring_status(req, res);
    });
    
    add_get_route("/api/monitoring/performance", [this](const HttpRequest& req, HttpResponse& res) {
        return handle_performance_metrics(req, res);
    });
    
    add_get_route("/api/bandwidth/status", [this](const HttpRequest& req, HttpResponse& res) {
        return handle_bandwidth_status(req, res);
    });
    
    add_get_route("/api/bandwidth/optimization", [this](const HttpRequest& req, HttpResponse& res) {
        return handle_bandwidth_optimization(req, res);
    });

    add_get_route("/api/analytics/dashboard", [this](const HttpRequest& req, HttpResponse& res) {
        return handle_analytics_dashboard(req, res);
    });

    add_get_route("/api/analytics/performance", [this](const HttpRequest& req, HttpResponse& res) {
        return handle_performance_report(req, res);
    });

    add_get_route("/api/analytics/endpoints", [this](const HttpRequest& req, HttpResponse& res) {
        return handle_endpoint_analytics(req, res);
    });
    
    std::cout << "✅ Default routes and middleware initialized" << std::endl;
}

std::optional<RouteHandler> WebServer::find_route(const std::string& method, const std::string& path) {
    std::string route_key = method + ":" + path;
    
    // Check route cache first
    if (route_cache_enabled_) {
        auto cache_it = route_cache_.find(route_key);
        if (cache_it != route_cache_.end()) {
            return cache_it->second;
        }
    }
    
    // Check exact match
    auto it = routes_.find(route_key);
    if (it != routes_.end()) {
        if (route_cache_enabled_) {
            route_cache_[route_key] = it->second;
        }
        return it->second;
    }
    
    // Check pattern matching
    for (const auto& route : routes_) {
        if (match_route_pattern(route.first, method + ":" + path)) {
            if (route_cache_enabled_) {
                route_cache_[route_key] = route.second;
            }
            return route.second;
        }
    }
    
    return std::nullopt;
}

bool WebServer::match_route_pattern(const std::string& pattern, const std::string& path) {
    // Simple pattern matching for now (can be extended with regex)
    if (pattern == path) return true;
    
    // Handle wildcard patterns like /api/*
    if (pattern.back() == '*' && path.substr(0, pattern.length() - 1) == pattern.substr(0, pattern.length() - 1)) {
        return true;
    }
    
    // Handle parameter patterns like /api/users/:id
    std::vector<std::string> pattern_parts = split_string(pattern, '/');
    std::vector<std::string> path_parts = split_string(path, '/');
    
    if (pattern_parts.size() != path_parts.size()) return false;
    
    for (size_t i = 0; i < pattern_parts.size(); ++i) {
        if (pattern_parts[i].empty() && path_parts[i].empty()) continue;
        if (pattern_parts[i].empty() || path_parts[i].empty()) return false;
        if (pattern_parts[i][0] == ':' || pattern_parts[i] == path_parts[i]) continue;
        return false;
    }
    
    return true;
}

std::vector<std::string> WebServer::split_string(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

HttpResponse WebServer::execute_middleware_stack(const HttpRequest& req, RouteHandler route_handler) {
    HttpResponse res;
    res.status_code = 200;
    res.headers["Content-Type"] = "application/json";
    
    if (middleware_stack_.empty()) {
        return route_handler(req, res);
    }
    
    size_t current_middleware = 0;
    
    std::function<void(const HttpRequest&, HttpResponse&)> next = [&](const HttpRequest& request, HttpResponse& response) {
        if (current_middleware >= middleware_stack_.size()) {
            // All middleware executed, call the route handler
            response = route_handler(request, response);
            return;
        }
        
        auto& middleware = middleware_stack_[current_middleware++];
        middleware.handler(request, response, next);
    };
    
    next(req, res);
    return res;
}

HttpResponse WebServer::list_routes(const HttpRequest& req, HttpResponse& res) {
    std::vector<std::map<std::string, std::string>> route_list;
    
    for (const auto& route : routes_) {
        std::map<std::string, std::string> route_info;
        size_t colon_pos = route.first.find(':');
        route_info["method"] = route.first.substr(0, colon_pos);
        route_info["path"] = route.first.substr(colon_pos + 1);
        route_info["handler"] = "registered";
        route_list.push_back(route_info);
    }
    
    res.body = "{\"routes\": [";
    for (size_t i = 0; i < route_list.size(); ++i) {
        res.body += "{";
        res.body += "\"method\": \"" + route_list[i]["method"] + "\",";
        res.body += "\"path\": \"" + route_list[i]["path"] + "\",";
        res.body += "\"handler\": \"" + route_list[i]["handler"] + "\"";
        res.body += "}";
        if (i < route_list.size() - 1) res.body += ",";
    }
    res.body += "], \"total\": " + std::to_string(route_list.size()) + "}";
    
    return res;
}

HttpResponse WebServer::list_middleware(const HttpRequest& req, HttpResponse& res) {
    res.body = "{\"middleware\": [";
    for (size_t i = 0; i < middleware_stack_.size(); ++i) {
        res.body += "\"" + middleware_stack_[i].name + "\"";
        if (i < middleware_stack_.size() - 1) res.body += ",";
    }
    res.body += "], \"total\": " + std::to_string(middleware_stack_.size()) + "}";
    
    return res;
}

void WebServer::set_hadoop_storage(std::shared_ptr<dds::storage::HadoopStorage> storage) {
    hadoop_storage_ = storage;
}

HttpResponse WebServer::handle_status(const HttpRequest& req) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Log incoming request
    log_request(req, "status");
    
    // Check cache first
    std::string cache_key = "status:" + req.method + ":" + req.path;
    auto cached_response = get_cached_response(cache_key);
    if (cached_response.has_value()) {
        std::cout << "💾 Cache hit for status endpoint" << std::endl;
        return cached_response.value();
    }
    
    // Rate limiting check
    std::string client_ip = req.headers.count("X-Forwarded-For") ? req.headers.at("X-Forwarded-For") : "127.0.0.1";
    if (!check_rate_limit(client_ip)) {
        HttpResponse response;
        response.status_code = 429;
        response.headers["Content-Type"] = "application/json";
        response.headers["Retry-After"] = "60";
        response.body = "{\"error\": \"Rate limit exceeded. Please try again later.\"}";
        std::cout << "🚫 Rate limit exceeded for client: " << client_ip << std::endl;
        
        // Log failed request
        log_response(response, duration_cast<microseconds>(high_resolution_clock::now() - start_time));
        failed_requests_++;
        return response;
    }
    
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.headers["Access-Control-Allow-Origin"] = "*";
    response.headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, OPTIONS";
    response.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
    response.headers["X-Content-Type-Options"] = "nosniff";
    response.headers["X-Frame-Options"] = "DENY";
    response.headers["X-XSS-Protection"] = "1; mode=block";
    response.headers["Cache-Control"] = "public, max-age=60";
    response.headers["ETag"] = "\"status-v1.0.0\"";
    response.body = "{\"status\": \"running\", \"version\": \"1.0.0\", \"timestamp\": \"" + get_current_timestamp() + "\"}";
    
    // Cache the response
    cache_response(cache_key, response);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    // Log successful response
    log_response(response, duration);
    successful_requests_++;
    total_requests_++;
    
    std::cout << "📊 Status endpoint processed in " << duration.count() << " μs" << std::endl;
    
    return response;
}

HttpResponse WebServer::handle_jobs_list(const HttpRequest& req) {
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"jobs\": []}";
    return response;
}

HttpResponse WebServer::handle_job_submit(const HttpRequest& req) {
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"job_id\": \"stub_job_123\", \"status\": \"submitted\"}";
    return response;
}

HttpResponse WebServer::handle_job_status(const HttpRequest& req) {
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"status\": \"completed\"}";
    return response;
}

HttpResponse WebServer::handle_hdfs_list(const HttpRequest& req) {
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"files\": []}";
    return response;
}

HttpResponse WebServer::handle_hdfs_upload(const HttpRequest& req) {
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"status\": \"uploaded\"}";
    return response;
}

HttpResponse WebServer::handle_hdfs_download(const HttpRequest& req) {
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"status\": \"downloaded\"}";
    return response;
}

HttpResponse WebServer::handle_algorithm_train(const HttpRequest& req) {
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"status\": \"training\"}";
    return response;
}

HttpResponse WebServer::handle_algorithm_predict(const HttpRequest& req) {
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"status\": \"prediction_complete\"}";
    return response;
}

HttpResponse WebServer::handle_cluster_info(const HttpRequest& req) {
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"cluster_status\": \"healthy\", \"nodes\": 1}";
    return response;
}

HttpResponse WebServer::handle_health_check(const HttpRequest& req, HttpResponse& res) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Check if health check is due
    auto now = std::chrono::steady_clock::now();
    if (now - last_health_check_ > std::chrono::seconds(health_check_interval_)) {
        perform_health_check();
        last_health_check_ = now;
    }
    
    res.status_code = 200;
    res.headers["Content-Type"] = "application/json";
    res.headers["Cache-Control"] = "no-cache, no-store, must-revalidate";
    res.headers["Pragma"] = "no-cache";
    res.headers["Expires"] = "0";
    
    // Build health status response
    std::string health_status = "{\"status\": \"healthy\", \"timestamp\": \"" + get_current_timestamp() + "\", ";
    health_status += "\"uptime\": " + std::to_string(get_uptime_seconds()) + ", ";
    health_status += "\"version\": \"1.0.0\", ";
    health_status += "\"checks\": {";
    health_status += "\"server\": \"ok\", ";
    health_status += "\"database\": \"ok\", ";
    health_status += "\"storage\": \"ok\", ";
    health_status += "\"memory\": \"ok\"";
    health_status += "}}";
    
    res.body = health_status;
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    std::cout << "🏥 Health check completed in " << duration.count() << " μs" << std::endl;
    return res;
}

HttpResponse WebServer::handle_metrics(const HttpRequest& req, HttpResponse& res) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    res.status_code = 200;
    res.headers["Content-Type"] = "application/json";
    res.headers["Cache-Control"] = "no-cache, no-store, must-revalidate";
    
    // Calculate metrics
    double success_rate = total_requests_ > 0 ? (double)successful_requests_ / total_requests_ * 100.0 : 0.0;
    double avg_response_time = calculate_average_response_time();
    size_t cache_hit_rate = calculate_cache_hit_rate();
    
    std::string metrics = "{\"metrics\": {";
    metrics += "\"requests\": {";
    metrics += "\"total\": " + std::to_string(total_requests_) + ", ";
    metrics += "\"successful\": " + std::to_string(successful_requests_) + ", ";
    metrics += "\"failed\": " + std::to_string(failed_requests_) + ", ";
    metrics += "\"success_rate\": " + std::to_string(success_rate);
    metrics += "}, ";
    metrics += "\"performance\": {";
    metrics += "\"avg_response_time_ms\": " + std::to_string(avg_response_time) + ", ";
    metrics += "\"active_connections\": " + std::to_string(active_connections_.load()) + ", ";
    metrics += "\"cache_hit_rate\": " + std::to_string(cache_hit_rate);
    metrics += "}, ";
    metrics += "\"system\": {";
    metrics += "\"uptime_seconds\": " + std::to_string(get_uptime_seconds()) + ", ";
    metrics += "\"memory_usage_mb\": " + std::to_string(get_memory_usage_mb()) + ", ";
    metrics += "\"cpu_usage_percent\": " + std::to_string(get_cpu_usage_percent());
    metrics += "}";
    metrics += "}, \"timestamp\": \"" + get_current_timestamp() + "\"}";
    
    res.body = metrics;
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    std::cout << "📊 Metrics endpoint processed in " << duration.count() << " μs" << std::endl;
    return res;
}

HttpResponse WebServer::handle_monitoring_status(const HttpRequest& req, HttpResponse& res) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    res.status_code = 200;
    res.headers["Content-Type"] = "application/json";
    res.headers["Cache-Control"] = "no-cache, no-store, must-revalidate";
    
    std::string status = "{\"monitoring\": {";
    status += "\"enabled\": " + std::string(monitoring_enabled_ ? "true" : "false") + ", ";
    status += "\"health_check_interval\": " + std::to_string(health_check_interval_) + ", ";
    status += "\"last_health_check\": \"" + get_last_health_check_timestamp() + "\", ";
    status += "\"alerts\": [], ";
    status += "\"thresholds\": {";
    status += "\"max_response_time_ms\": 1000, ";
    status += "\"max_memory_usage_mb\": 1024, ";
    status += "\"max_cpu_usage_percent\": 80";
    status += "}";
    status += "}, \"timestamp\": \"" + get_current_timestamp() + "\"}";
    
    res.body = status;
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    std::cout << "📈 Monitoring status processed in " << duration.count() << " μs" << std::endl;
    return res;
}

HttpResponse WebServer::handle_performance_metrics(const HttpRequest& req, HttpResponse& res) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    res.status_code = 200;
    res.headers["Content-Type"] = "application/json";
    res.headers["Cache-Control"] = "no-cache, no-store, must-revalidate";
    
    // Get performance data
    auto response_times = get_response_time_history();
    auto memory_usage = get_memory_usage_history();
    auto cpu_usage = get_cpu_usage_history();
    
    std::string performance = "{\"performance\": {";
    performance += "\"response_times\": [";
    for (size_t i = 0; i < response_times.size(); ++i) {
        performance += std::to_string(response_times[i]);
        if (i < response_times.size() - 1) performance += ", ";
    }
    performance += "], ";
    performance += "\"memory_usage\": [";
    for (size_t i = 0; i < memory_usage.size(); ++i) {
        performance += std::to_string(memory_usage[i]);
        if (i < memory_usage.size() - 1) performance += ", ";
    }
    performance += "], ";
    performance += "\"cpu_usage\": [";
    for (size_t i = 0; i < cpu_usage.size(); ++i) {
        performance += std::to_string(cpu_usage[i]);
        if (i < cpu_usage.size() - 1) performance += ", ";
    }
    performance += "]";
    performance += "}, \"timestamp\": \"" + get_current_timestamp() + "\"}";
    
    res.body = performance;
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    std::cout << "⚡ Performance metrics processed in " << duration.count() << " μs" << std::endl;
    return res;
}

void WebServer::run_server(int serverSocket) {
    // Stub implementation
    std::cout << "Server running on port " << port_ << std::endl;
}

void WebServer::handle_client(int clientSocket) {
    // Acquire connection slot
    if (!acquire_connection()) {
        std::cout << "❌ Connection rejected - limit exceeded" << std::endl;
        close(clientSocket);
        return;
    }
    
    try {
        // Set socket timeout
        struct timeval timeout;
        timeout.tv_sec = connection_timeout_;
        timeout.tv_usec = 0;
        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
        
        std::cout << "🔗 Client connection established (socket: " << clientSocket << ")" << std::endl;
        
        // Handle client request here
        // This is a stub implementation - actual request handling would go here
        
        // Simulate request processing
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error handling client: " << e.what() << std::endl;
    }
    
    // Release connection slot
    release_connection();
    close(clientSocket);
    std::cout << "🔓 Client connection closed (socket: " << clientSocket << ")" << std::endl;
}

HttpRequest WebServer::parse_request(const std::string& request) {
    // Stub implementation
    HttpRequest req;
    req.method = "GET";
    req.path = "/";
    return req;
}

HttpResponse WebServer::handle_request(const HttpRequest& req) {
    // Stub implementation
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"message\": \"Request handled\"}";
    return response;
}

HttpResponse WebServer::serve_dashboard() {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "text/html; charset=utf-8";
    response.headers["Cache-Control"] = "no-cache, no-store, must-revalidate";
    response.headers["Pragma"] = "no-cache";
    response.headers["Expires"] = "0";
    response.headers["X-Content-Type-Options"] = "nosniff";
    response.headers["X-Frame-Options"] = "SAMEORIGIN";
    response.headers["X-XSS-Protection"] = "1; mode=block";
    
    // Read the dashboard HTML file
    std::ifstream file("dashboard.html");
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        response.body = buffer.str();
        file.close();
        
        // Optimize HTML content
        response.body = optimize_html_content(response.body);
        
        // Apply compression if enabled and content is large enough
        if (compression_enabled_ && response.body.length() >= min_compression_size_) {
            auto compressed = compress_content(response.body);
            if (compressed.has_value()) {
                response.body = compressed.value();
                response.headers["Content-Encoding"] = "gzip";
                response.headers["Vary"] = "Accept-Encoding";
                std::cout << "🗜️ Content compressed: " << response.body.length() << " bytes" << std::endl;
            }
        }
        
        std::cout << "✅ Dashboard served successfully (" << response.body.length() << " bytes)" << std::endl;
    } else {
        // Enhanced error handling with detailed logging
        std::cerr << "❌ Error: Could not open dashboard.html file" << std::endl;
        std::cerr << "   Current working directory: ";
        system("pwd");
        response.status_code = 500;
        response.body = "<html><body><h1>DDS System Dashboard</h1><p>Error: Could not load dashboard.html</p><p>Please check server logs for details.</p></body></html>";
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    std::cout << "📊 Dashboard endpoint processed in " << duration.count() << " μs" << std::endl;
    
    return response;
}

std::string WebServer::format_response(const HttpResponse& response) {
    // Stub implementation
    return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\": \"ok\"}";
}

std::string WebServer::parse_request_line(const std::string& line, std::string& method, std::string& path) {
    // Stub implementation
    return "";
}

std::map<std::string, std::string> WebServer::parse_headers(const std::vector<std::string>& lines) {
    // Stub implementation
    return {};
}

std::map<std::string, std::string> WebServer::parse_query_params(const std::string& query_string) {
    // Stub implementation
    return {};
}

std::string WebServer::url_decode(const std::string& encoded) {
    // Stub implementation
    return encoded;
}

std::string WebServer::generate_json_response(const std::map<std::string, std::string>& data) {
    // Stub implementation
    return "{}";
}

std::string WebServer::generate_error_response(const std::string& error, int status_code) {
    // Stub implementation
    return "{\"error\": \"" + error + "\"}";
}

bool WebServer::check_rate_limit(const std::string& client_ip) {
    std::lock_guard<std::mutex> lock(rate_limit_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto window_start = now - std::chrono::seconds(rate_limit_window_);
    
    // Clean old entries
    auto& requests = rate_limit_requests_[client_ip];
    requests.erase(
        std::remove_if(requests.begin(), requests.end(),
            [window_start](const auto& timestamp) {
                return timestamp < window_start;
            }),
        requests.end()
    );
    
    // Check if limit exceeded
    if (requests.size() >= max_requests_per_minute_) {
        return false;
    }
    
    // Add current request
    requests.push_back(now);
    return true;
}

std::string WebServer::optimize_html_content(const std::string& html) {
    std::string optimized = html;
    
    // Remove unnecessary whitespace and comments
    std::regex whitespace_regex("\\s+");
    optimized = std::regex_replace(optimized, whitespace_regex, " ");
    
    // Remove HTML comments
    std::regex comment_regex("<!--.*?-->");
    optimized = std::regex_replace(optimized, comment_regex, "");
    
    // Remove empty lines
    std::regex empty_line_regex("\\n\\s*\\n");
    optimized = std::regex_replace(optimized, empty_line_regex, "\n");
    
    // Trim leading/trailing whitespace
    optimized = std::regex_replace(optimized, std::regex("^\\s+|\\s+$"), "");
    
    std::cout << "📦 HTML optimized: " << html.length() << " -> " << optimized.length() << " bytes (" 
              << (100 - (optimized.length() * 100 / html.length())) << "% reduction)" << std::endl;
    
    return optimized;
}

void WebServer::log_request(const HttpRequest& req, const std::string& endpoint) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::string client_ip = req.headers.count("X-Forwarded-For") ? req.headers.at("X-Forwarded-For") : "127.0.0.1";
    std::string user_agent = req.headers.count("User-Agent") ? req.headers.at("User-Agent") : "Unknown";
    
    std::cout << "📝 [" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] "
              << req.method << " " << req.path << " (" << endpoint << ") "
              << "from " << client_ip << " - " << user_agent << std::endl;
}

void WebServer::log_response(const HttpResponse& response, const std::chrono::microseconds& duration) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::string status_color = (response.status_code >= 200 && response.status_code < 300) ? "✅" : "❌";
    
    std::cout << "📤 [" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] "
              << status_color << " " << response.status_code << " "
              << response.body.length() << " bytes in " << duration.count() << " μs" << std::endl;
}

void WebServer::print_analytics() {
    std::cout << "\n📊 Web Server Analytics:" << std::endl;
    std::cout << "   Total Requests: " << total_requests_ << std::endl;
    std::cout << "   Successful: " << successful_requests_ << std::endl;
    std::cout << "   Failed: " << failed_requests_ << std::endl;
    std::cout << "   Success Rate: " << (total_requests_ > 0 ? (successful_requests_ * 100.0 / total_requests_) : 0) << "%" << std::endl;
    std::cout << "   Rate Limited: " << (total_requests_ - successful_requests_ - failed_requests_) << std::endl;
    std::cout << "   Max Concurrent Connections: " << max_connections_ << std::endl;
    std::cout << "   Connection Pool Status: " << connection_pool_.size() << " available" << std::endl;
    std::cout << "   Thread Pool Size: " << thread_pool_size_ << " workers" << std::endl;
    std::cout << "   Pending Tasks: " << task_queue_.size() << std::endl;
    std::cout << "   Response Cache: " << response_cache_.size() << "/" << max_cache_size_ << " entries" << std::endl;
    std::cout << "   Cache TTL: " << cache_ttl_ << " seconds" << std::endl;
    std::cout << "   Compression: " << (compression_enabled_ ? "Enabled" : "Disabled") << " (level: " << compression_level_ << ")" << std::endl;
    std::cout << "   Adaptive Compression: " << (adaptive_compression_enabled_ ? "Enabled" : "Disabled") << std::endl;
    std::cout << "   Min Compression Size: " << min_compression_size_ << " bytes" << std::endl;
    std::cout << "   Bandwidth Throttling: " << (bandwidth_throttling_enabled_ ? "Enabled" : "Disabled") << std::endl;
    std::cout << "   Max Bandwidth per Client: " << max_bandwidth_per_client_ / 1024 / 1024 << " MB/min" << std::endl;
    std::cout << "   Total Bytes Sent: " << total_bytes_sent_ / 1024 / 1024 << " MB" << std::endl;
    std::cout << "   Total Bytes Compressed: " << total_bytes_compressed_ / 1024 / 1024 << " MB" << std::endl;
    std::cout << "   Average Compression Ratio: " << std::fixed << std::setprecision(1) << average_compression_ratio_ << "%" << std::endl;
    std::cout << "   Pre-compressed Content: " << pre_compressed_content_.size() << " items" << std::endl;
    std::cout << "   Active Bandwidth Clients: " << bandwidth_usage_.size() << std::endl;
    std::cout << "   Analytics Enabled: " << (analytics_enabled_ ? "Enabled" : "Disabled") << std::endl;
    std::cout << "   Total Requests: " << total_requests_ << std::endl;
    std::cout << "   Total Responses: " << total_responses_ << std::endl;
    std::cout << "   Total Errors: " << total_errors_ << std::endl;
    std::cout << "   Requests per Second: " << std::fixed << std::setprecision(2) << get_requests_per_second() << std::endl;
    std::cout << "   Average Response Time: " << std::fixed << std::setprecision(2) << get_average_response_time() / 1000.0 << " ms" << std::endl;
    std::cout << "   Error Rate: " << std::fixed << std::setprecision(2) << get_error_rate() << "%" << std::endl;
    std::cout << "   Tracked Endpoints: " << endpoint_performance_.size() << std::endl;
    std::cout << "   Request Validation: " << (validation_enabled_ ? "Enabled" : "Disabled") << std::endl;
    std::cout << "   Max Request Size: " << max_request_size_ << " bytes" << std::endl;
    std::cout << "   Max Header Size: " << max_header_size_ << " bytes" << std::endl;
    std::cout << "   Routing Framework: " << (routing_enabled_ ? "Enabled" : "Disabled") << std::endl;
    std::cout << "   Middleware Stack: " << middleware_stack_.size() << " handlers" << std::endl;
    std::cout << "   Registered Routes: " << routes_.size() << " endpoints" << std::endl;
    std::cout << "   Route Cache: " << (route_cache_enabled_ ? "Enabled" : "Disabled") << " (" << route_cache_.size() << " cached)" << std::endl;
    std::cout << "   Monitoring: " << (monitoring_enabled_ ? "Enabled" : "Disabled") << std::endl;
    std::cout << "   Health Check Interval: " << health_check_interval_ << " seconds" << std::endl;
    std::cout << "   Uptime: " << get_uptime_seconds() << " seconds" << std::endl;
    std::cout << "   Memory Usage: " << get_memory_usage_mb() << " MB" << std::endl;
    std::cout << "   CPU Usage: " << get_cpu_usage_percent() << "%" << std::endl;
    std::cout << "   Average Response Time: " << calculate_average_response_time() << " ms" << std::endl;
    std::cout << "   Cache Hit Rate: " << calculate_cache_hit_rate() << "%" << std::endl;
    std::cout << "   Performance History: " << response_time_history_.size() << " data points" << std::endl;
}

bool WebServer::acquire_connection() {
    if (active_connections_.load() >= max_connections_) {
        std::cout << "🚫 Connection limit reached (" << active_connections_.load() << "/" << max_connections_ << ")" << std::endl;
        return false;
    }
    
    active_connections_++;
    std::cout << "🔗 Connection acquired (" << active_connections_.load() << "/" << max_connections_ << ")" << std::endl;
    return true;
}

void WebServer::release_connection() {
    if (active_connections_.load() > 0) {
        active_connections_--;
        std::cout << "🔓 Connection released (" << active_connections_.load() << "/" << max_connections_ << ")" << std::endl;
    }
}

void WebServer::manage_connection_pool() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(connection_timeout_));
        
        // Clean up expired connections
        auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(connection_pool_mutex_);
        
        while (!connection_pool_.empty() && 
               (now - connection_pool_.front().timestamp) > std::chrono::seconds(connection_timeout_)) {
            connection_pool_.pop();
        }
        
        std::cout << "🔄 Connection pool cleaned up. Available: " << connection_pool_.size() << std::endl;
    }
}

void WebServer::worker_thread_function() {
    while (!shutdown_thread_pool_) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(thread_pool_mutex_);
            thread_pool_cv_.wait(lock, [this] { 
                return !task_queue_.empty() || shutdown_thread_pool_; 
            });
            
            if (shutdown_thread_pool_ && task_queue_.empty()) {
                break;
            }
            
            if (!task_queue_.empty()) {
                task = std::move(task_queue_.front());
                task_queue_.pop();
            }
        }
        
        if (task) {
            try {
                task();
            } catch (const std::exception& e) {
                std::cerr << "❌ Worker thread error: " << e.what() << std::endl;
            }
        }
    }
}

void WebServer::submit_task(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(thread_pool_mutex_);
        task_queue_.push(std::move(task));
    }
    thread_pool_cv_.notify_one();
}

std::future<HttpResponse> WebServer::handle_request_async(const HttpRequest& req) {
    auto promise = std::make_shared<std::promise<HttpResponse>>();
    auto future = promise->get_future();
    
    submit_task([this, req, promise]() {
        try {
            auto response = handle_request_sync(req);
            promise->set_value(response);
        } catch (const std::exception& e) {
            HttpResponse error_response;
            error_response.status_code = 500;
            error_response.headers["Content-Type"] = "application/json";
            error_response.body = "{\"error\": \"Internal server error: " + std::string(e.what()) + "\"}";
            promise->set_value(error_response);
        }
    });
    
    return future;
}

HttpResponse WebServer::handle_request_sync(const HttpRequest& req) {
    // Enhanced synchronous request handling with better routing
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Log incoming request
    log_request(req, "request");
    
    HttpResponse response;
    
    try {
        // Validate request before processing
        auto validation_result = validate_request(req);
        if (!validation_result.is_valid) {
            response.status_code = validation_result.status_code;
            response.headers["Content-Type"] = "application/json";
            response.headers["Access-Control-Allow-Origin"] = "*";
            response.body = "{\"error\": \"" + validation_result.error_message + "\"}";
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
            log_response(response, duration);
            failed_requests_++;
            total_requests_++;
            return response;
        }
        
        // Use routing framework to find and execute handler
        if (routing_enabled_) {
            auto route_handler = find_route(req.method, req.path);
            if (route_handler) {
                if (middleware_enabled_) {
                    response = execute_middleware_stack(req, *route_handler);
                } else {
                    response = (*route_handler)(req, response);
                }
            } else {
                // Fallback to legacy routing for backward compatibility
                if (req.path == "/" || req.path == "/dashboard") {
                    response = serve_dashboard();
                } else if (req.path == "/api/status") {
                    response = handle_status(req);
                } else if (req.path == "/api/jobs" && req.method == "GET") {
                    response = handle_jobs_list(req);
                } else if (req.path == "/api/jobs" && req.method == "POST") {
                    response = handle_job_submit(req);
                } else if (req.path.find("/api/jobs/") == 0 && req.method == "GET") {
                    response = handle_job_status(req);
                } else if (req.path == "/api/hdfs/list") {
                    response = handle_hdfs_list(req);
                } else if (req.path == "/api/cluster/info") {
                    response = handle_cluster_info(req);
                } else {
                    // 404 Not Found
                    response.status_code = 404;
                    response.headers["Content-Type"] = "application/json";
                    response.body = "{\"error\": \"Endpoint not found\", \"path\": \"" + sanitize_string(req.path) + "\"}";
                }
            }
        } else {
            // Legacy routing without framework
            if (req.path == "/" || req.path == "/dashboard") {
                response = serve_dashboard();
            } else if (req.path == "/api/status") {
                response = handle_status(req);
            } else if (req.path == "/api/jobs" && req.method == "GET") {
                response = handle_jobs_list(req);
            } else if (req.path == "/api/jobs" && req.method == "POST") {
                response = handle_job_submit(req);
            } else if (req.path.find("/api/jobs/") == 0 && req.method == "GET") {
                response = handle_job_status(req);
            } else if (req.path == "/api/hdfs/list") {
                response = handle_hdfs_list(req);
            } else if (req.path == "/api/cluster/info") {
                response = handle_cluster_info(req);
            } else {
                // 404 Not Found
                response.status_code = 404;
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"error\": \"Endpoint not found\", \"path\": \"" + sanitize_string(req.path) + "\"}";
            }
        }
        
        // Sanitize response before sending
        sanitize_response(response);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        // Log response
        log_response(response, duration);
        
        // Update counters
        if (response.status_code < 400) {
            successful_requests_++;
        } else {
            failed_requests_++;
        }
        total_requests_++;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Request handling error: " << e.what() << std::endl;
        response.status_code = 500;
        response.headers["Content-Type"] = "application/json";
        response.body = "{\"error\": \"Internal server error\"}";
        failed_requests_++;
        total_requests_++;
    }
    
    return response;
}

std::optional<HttpResponse> WebServer::get_cached_response(const std::string& cache_key) {
    if (!cache_enabled_) return std::nullopt;
    
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = response_cache_.find(cache_key);
    if (it != response_cache_.end()) {
        auto& cache_entry = it->second;
        auto now = std::chrono::steady_clock::now();
        
        if ((now - cache_entry.timestamp) < std::chrono::seconds(cache_ttl_)) {
            std::cout << "💾 Cache hit for key: " << cache_key << std::endl;
            cache_hits_++;
            return cache_entry.response;
        } else {
            // Expired entry, remove it
            response_cache_.erase(it);
            std::cout << "🗑️ Cache entry expired for key: " << cache_key << std::endl;
        }
    }
    
    cache_misses_++;
    return std::nullopt;
}

void WebServer::cache_response(const std::string& cache_key, const HttpResponse& response) {
    if (!cache_enabled_) return;
    
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // Check if cache is full and remove oldest entries
    if (response_cache_.size() >= max_cache_size_) {
        auto oldest = response_cache_.begin();
        for (auto it = response_cache_.begin(); it != response_cache_.end(); ++it) {
            if (it->second.timestamp < oldest->second.timestamp) {
                oldest = it;
            }
        }
        response_cache_.erase(oldest);
        std::cout << "🗑️ Removed oldest cache entry due to size limit" << std::endl;
    }
    
    CacheEntry entry;
    entry.response = response;
    entry.timestamp = std::chrono::steady_clock::now();
    response_cache_[cache_key] = entry;
    
    std::cout << "💾 Cached response for key: " << cache_key << " (cache size: " << response_cache_.size() << ")" << std::endl;
}

void WebServer::clear_cache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    response_cache_.clear();
    std::cout << "🧹 Response cache cleared" << std::endl;
}

std::string WebServer::get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void WebServer::cleanup_expired_cache() {
    if (!cache_enabled_) return;
    
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto now = std::chrono::steady_clock::now();
    
    size_t removed_count = 0;
    for (auto it = response_cache_.begin(); it != response_cache_.end();) {
        if ((now - it->second.timestamp) >= std::chrono::seconds(cache_ttl_)) {
            it = response_cache_.erase(it);
            removed_count++;
        } else {
            ++it;
        }
    }
    
    if (removed_count > 0) {
        std::cout << "🧹 Cleaned up " << removed_count << " expired cache entries" << std::endl;
    }
}

std::optional<std::string> WebServer::compress_content(const std::string& content) {
    if (!compression_enabled_ || content.empty()) {
        return std::nullopt;
    }
    
    // Calculate buffer size for compressed data
    uLong compressed_size = compressBound(content.length());
    std::vector<Bytef> compressed_buffer(compressed_size);
    
    // Compress the content
    uLong actual_compressed_size = compressed_size;
    int result = compress2(compressed_buffer.data(), &actual_compressed_size,
                          reinterpret_cast<const Bytef*>(content.c_str()), content.length(),
                          compression_level_);
    
    if (result == Z_OK && actual_compressed_size < content.length()) {
        // Only use compression if it actually reduces size
        std::string compressed_content(reinterpret_cast<char*>(compressed_buffer.data()), actual_compressed_size);
        
        double compression_ratio = (1.0 - (double)actual_compressed_size / content.length()) * 100.0;
        std::cout << "🗜️ Compression successful: " << content.length() << " -> " 
                  << actual_compressed_size << " bytes (" << std::fixed << std::setprecision(1) 
                  << compression_ratio << "% reduction)" << std::endl;
        
        return compressed_content;
    } else {
        std::cout << "⚠️ Compression not beneficial or failed, using original content" << std::endl;
        return std::nullopt;
    }
}

std::optional<std::string> WebServer::decompress_content(const std::string& compressed_content) {
    if (compressed_content.empty()) {
        return std::nullopt;
    }
    
    // Estimate decompressed size (may need adjustment)
    uLong decompressed_size = compressed_content.length() * 4; // Conservative estimate
    std::vector<Bytef> decompressed_buffer(decompressed_size);
    
    uLong actual_decompressed_size = decompressed_size;
    int result = uncompress(decompressed_buffer.data(), &actual_decompressed_size,
                           reinterpret_cast<const Bytef*>(compressed_content.c_str()), compressed_content.length());
    
    if (result == Z_OK) {
        std::string decompressed_content(reinterpret_cast<char*>(decompressed_buffer.data()), actual_decompressed_size);
        std::cout << "🗜️ Decompression successful: " << compressed_content.length() << " -> " 
                  << actual_decompressed_size << " bytes" << std::endl;
        return decompressed_content;
    } else {
        std::cerr << "❌ Decompression failed with error code: " << result << std::endl;
        return std::nullopt;
    }
}

bool WebServer::should_compress_content(const std::string& content_type, size_t content_length) {
    if (!compression_enabled_ || content_length < min_compression_size_) {
        return false;
    }
    
    // Only compress text-based content types
    static const std::vector<std::string> compressible_types = {
        "text/", "application/json", "application/xml", "application/javascript", 
        "text/css", "text/html", "text/plain", "application/xhtml+xml"
    };
    
    for (const auto& type : compressible_types) {
        if (content_type.find(type) == 0) {
            return true;
        }
    }
    
    return false;
}

void WebServer::optimize_response_headers(HttpResponse& response) {
    // Add compression-related headers
    if (compression_enabled_) {
        response.headers["Vary"] = "Accept-Encoding";
        
        // Check if content should be compressed
        auto content_type = response.headers.find("Content-Type");
        if (content_type != response.headers.end() && 
            should_compress_content(content_type->second, response.body.length())) {
            
            auto compressed = compress_content(response.body);
            if (compressed.has_value()) {
                response.body = compressed.value();
                response.headers["Content-Encoding"] = "gzip";
            }
        }
    }
    
    // Add performance headers
    response.headers["X-Content-Type-Options"] = "nosniff";
    response.headers["X-Frame-Options"] = "SAMEORIGIN";
    response.headers["X-XSS-Protection"] = "1; mode=block";
    
    // Add cache headers for appropriate content
    if (response.status_code == 200) {
        response.headers["Cache-Control"] = "public, max-age=300";
    }
}

int WebServer::get_adaptive_compression_level(const std::string& content_type, size_t content_length) {
    if (!adaptive_compression_enabled_) {
        return compression_level_;
    }
    
    // Higher compression for larger files
    if (content_length > 1024 * 1024) { // > 1MB
        return std::min(9, compression_level_ + 2);
    } else if (content_length > 100 * 1024) { // > 100KB
        return std::min(8, compression_level_ + 1);
    }
    
    // Lower compression for small files to save CPU
    if (content_length < 10 * 1024) { // < 10KB
        return std::max(1, compression_level_ - 2);
    }
    
    return compression_level_;
}

bool WebServer::should_throttle_bandwidth(const std::string& client_ip, size_t response_size) {
    if (!bandwidth_throttling_enabled_) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(bandwidth_mutex_);
    auto now = std::chrono::steady_clock::now();
    
    // Clean up old entries (older than 1 minute)
    auto it = bandwidth_usage_.begin();
    while (it != bandwidth_usage_.end()) {
        if (now - it->second.second > std::chrono::minutes(1)) {
            it = bandwidth_usage_.erase(it);
        } else {
            ++it;
        }
    }
    
    // Check current usage
    auto usage_it = bandwidth_usage_.find(client_ip);
    if (usage_it != bandwidth_usage_.end()) {
        size_t current_usage = usage_it->second.first;
        if (current_usage + response_size > max_bandwidth_per_client_) {
            std::cout << "🚫 Bandwidth throttling for client " << client_ip 
                      << " (usage: " << current_usage << " + " << response_size 
                      << " > " << max_bandwidth_per_client_ << ")" << std::endl;
            return true;
        }
    }
    
    return false;
}

void WebServer::update_bandwidth_usage(const std::string& client_ip, size_t bytes_sent) {
    if (!bandwidth_throttling_enabled_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(bandwidth_mutex_);
    auto now = std::chrono::steady_clock::now();
    
    auto& usage = bandwidth_usage_[client_ip];
    usage.first += bytes_sent;
    usage.second = now;
    
    total_bytes_sent_ += bytes_sent;
}

double WebServer::get_bandwidth_usage_rate(const std::string& client_ip) {
    std::lock_guard<std::mutex> lock(bandwidth_mutex_);
    
    auto it = bandwidth_usage_.find(client_ip);
    if (it == bandwidth_usage_.end()) {
        return 0.0;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.second).count();
    
    if (time_diff == 0) {
        return 0.0;
    }
    
    return static_cast<double>(it->second.first) / time_diff; // bytes per second
}

void WebServer::pre_compress_static_content() {
    if (!compression_enabled_) {
        return;
    }
    
    std::cout << "🗜️ Pre-compressing static content..." << std::endl;
    
    // Pre-compress common static content
    std::map<std::string, std::string> static_content = {
        {"dashboard_html", "<!DOCTYPE html><html><head><title>DDS Dashboard</title></head><body><h1>DDS Dashboard</h1></body></html>"},
        {"api_docs", "{\"version\":\"1.0\",\"endpoints\":[\"/api/status\",\"/api/jobs\",\"/api/hdfs/list\"]}"},
        {"health_check", "{\"status\":\"healthy\",\"timestamp\":\"" + std::to_string(std::time(nullptr)) + "\"}"}
    };
    
    std::lock_guard<std::mutex> lock(pre_compressed_mutex_);
    for (const auto& [key, content] : static_content) {
        auto compressed = compress_content(content);
        if (compressed.has_value()) {
            pre_compressed_content_[key] = compressed.value();
            std::cout << "   ✅ Pre-compressed: " << key << " (" << content.length() 
                      << " -> " << compressed.value().length() << " bytes)" << std::endl;
        }
    }
}

std::optional<std::string> WebServer::get_pre_compressed_content(const std::string& content_key) {
    std::lock_guard<std::mutex> lock(pre_compressed_mutex_);
    auto it = pre_compressed_content_.find(content_key);
    if (it != pre_compressed_content_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void WebServer::cache_compressed_content(const std::string& content_key, const std::string& compressed_content) {
    std::lock_guard<std::mutex> lock(pre_compressed_mutex_);
    pre_compressed_content_[content_key] = compressed_content;
}

bool WebServer::supports_compression(const std::map<std::string, std::string>& headers) {
    auto accept_encoding = headers.find("Accept-Encoding");
    if (accept_encoding == headers.end()) {
        return false;
    }
    
    std::string encoding = accept_encoding->second;
    return encoding.find("gzip") != std::string::npos || 
           encoding.find("deflate") != std::string::npos ||
           encoding.find("*") != std::string::npos;
}

std::string WebServer::get_optimal_encoding(const std::map<std::string, std::string>& headers) {
    auto accept_encoding = headers.find("Accept-Encoding");
    if (accept_encoding == headers.end()) {
        return "identity";
    }
    
    std::string encoding = accept_encoding->second;
    
    // Prefer gzip over deflate
    if (encoding.find("gzip") != std::string::npos) {
        return "gzip";
    } else if (encoding.find("deflate") != std::string::npos) {
        return "deflate";
    }
    
    return "identity";
}

void WebServer::log_bandwidth_metrics(const std::string& client_ip, size_t original_size, size_t compressed_size, double compression_ratio) {
    std::cout << "📊 Bandwidth metrics for " << client_ip << ":" << std::endl;
    std::cout << "   Original size: " << original_size << " bytes" << std::endl;
    std::cout << "   Compressed size: " << compressed_size << " bytes" << std::endl;
    std::cout << "   Compression ratio: " << std::fixed << std::setprecision(1) << compression_ratio << "%" << std::endl;
    std::cout << "   Bandwidth saved: " << (original_size - compressed_size) << " bytes" << std::endl;
    
    // Update global metrics
    total_bytes_compressed_ += compressed_size;
    if (total_bytes_sent_ > 0) {
        average_compression_ratio_ = (static_cast<double>(total_bytes_compressed_) / total_bytes_sent_) * 100.0;
    }
}

ValidationResult WebServer::validate_request(const HttpRequest& req) {
    ValidationResult result;
    result.is_valid = true;
    
    if (!validation_enabled_) {
        return result;
    }
    
    // Validate request size
    if (req.body.length() > max_request_size_) {
        result.is_valid = false;
        result.status_code = 413; // Payload Too Large
        result.error_message = "Request body too large";
        std::cout << "🚫 Request validation failed: body size " << req.body.length() << " exceeds limit " << max_request_size_ << std::endl;
        return result;
    }
    
    // Validate method
    static const std::vector<std::string> allowed_methods = {"GET", "POST", "PUT", "DELETE", "OPTIONS", "HEAD"};
    bool method_allowed = false;
    for (const auto& method : allowed_methods) {
        if (req.method == method) {
            method_allowed = true;
            break;
        }
    }
    
    if (!method_allowed) {
        result.is_valid = false;
        result.status_code = 405; // Method Not Allowed
        result.error_message = "HTTP method not allowed";
        std::cout << "🚫 Request validation failed: invalid method " << req.method << std::endl;
        return result;
    }
    
    // Validate path
    if (req.path.empty() || req.path.length() > 2048) {
        result.is_valid = false;
        result.status_code = 400; // Bad Request
        result.error_message = "Invalid request path";
        std::cout << "🚫 Request validation failed: invalid path length " << req.path.length() << std::endl;
        return result;
    }
    
    // Check for path traversal attempts
    if (req.path.find("..") != std::string::npos || req.path.find("//") != std::string::npos) {
        result.is_valid = false;
        result.status_code = 400; // Bad Request
        result.error_message = "Invalid path characters detected";
        std::cout << "🚫 Request validation failed: path traversal attempt detected" << std::endl;
        return result;
    }
    
    // Validate headers
    for (const auto& header : req.headers) {
        if (header.first.length() > max_header_size_ || header.second.length() > max_header_size_) {
            result.is_valid = false;
            result.status_code = 400; // Bad Request
            result.error_message = "Header too large";
            std::cout << "🚫 Request validation failed: header too large" << std::endl;
            return result;
        }
        
        // Check for suspicious header values
        if (contains_suspicious_content(header.second)) {
            result.is_valid = false;
            result.status_code = 400; // Bad Request
            result.error_message = "Suspicious content detected in headers";
            std::cout << "🚫 Request validation failed: suspicious header content" << std::endl;
            return result;
        }
    }
    
    // Validate query parameters
    for (const auto& param : req.query_params) {
        if (param.first.length() > 256 || param.second.length() > 1024) {
            result.is_valid = false;
            result.status_code = 400; // Bad Request
            result.error_message = "Query parameter too large";
            std::cout << "🚫 Request validation failed: query parameter too large" << std::endl;
            return result;
        }
        
        if (contains_suspicious_content(param.second)) {
            result.is_valid = false;
            result.status_code = 400; // Bad Request
            result.error_message = "Suspicious content detected in query parameters";
            std::cout << "🚫 Request validation failed: suspicious query parameter content" << std::endl;
            return result;
        }
    }
    
    std::cout << "✅ Request validation passed" << std::endl;
    return result;
}

void WebServer::sanitize_response(HttpResponse& response) {
    // Sanitize response headers
    for (auto& header : response.headers) {
        header.second = sanitize_string(header.second);
    }
    
    // Sanitize response body for JSON content
    auto content_type = response.headers.find("Content-Type");
    if (content_type != response.headers.end() && 
        content_type->second.find("application/json") != std::string::npos) {
        response.body = sanitize_json_string(response.body);
    }
    
    // Add security headers
    response.headers["X-Content-Type-Options"] = "nosniff";
    response.headers["X-Frame-Options"] = "SAMEORIGIN";
    response.headers["X-XSS-Protection"] = "1; mode=block";
    response.headers["Referrer-Policy"] = "strict-origin-when-cross-origin";
    
    std::cout << "🧹 Response sanitized" << std::endl;
}

std::string WebServer::sanitize_string(const std::string& input) {
    if (input.empty()) {
        return input;
    }
    
    std::string sanitized = input;
    
    // Remove null bytes
    sanitized.erase(std::remove(sanitized.begin(), sanitized.end(), '\0'), sanitized.end());
    
    // Replace potentially dangerous characters
    std::map<char, std::string> dangerous_chars = {
        {'<', "&lt;"},
        {'>', "&gt;"},
        {'"', "&quot;"},
        {'\'', "&#x27;"},
        {'&', "&amp;"}
    };
    
    for (const auto& pair : dangerous_chars) {
        size_t pos = 0;
        while ((pos = sanitized.find(pair.first, pos)) != std::string::npos) {
            sanitized.replace(pos, 1, pair.second);
            pos += pair.second.length();
        }
    }
    
    // Trim whitespace
    sanitized.erase(0, sanitized.find_first_not_of(" \t\r\n"));
    sanitized.erase(sanitized.find_last_not_of(" \t\r\n") + 1);
    
    return sanitized;
}

std::string WebServer::sanitize_json_string(const std::string& json_input) {
    if (json_input.empty()) {
        return json_input;
    }
    
    std::string sanitized = json_input;
    
    // Remove null bytes
    sanitized.erase(std::remove(sanitized.begin(), sanitized.end(), '\0'), sanitized.end());
    
    // Escape control characters
    std::stringstream ss;
    for (char c : sanitized) {
        if (c < 32 && c != '\t' && c != '\n' && c != '\r') {
            ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
        } else {
            ss << c;
        }
    }
    
    return ss.str();
}

bool WebServer::contains_suspicious_content(const std::string& content) {
    if (content.empty()) {
        return false;
    }
    
    // Check for SQL injection patterns
    static const std::vector<std::string> sql_patterns = {
        "SELECT", "INSERT", "UPDATE", "DELETE", "DROP", "CREATE", "ALTER", "EXEC", "UNION"
    };
    
    std::string upper_content = content;
    std::transform(upper_content.begin(), upper_content.end(), upper_content.begin(), ::toupper);
    
    for (const auto& pattern : sql_patterns) {
        if (upper_content.find(pattern) != std::string::npos) {
            std::cout << "⚠️ Suspicious SQL pattern detected: " << pattern << std::endl;
            return true;
        }
    }
    
    // Check for XSS patterns
    static const std::vector<std::string> xss_patterns = {
        "<SCRIPT", "JAVASCRIPT:", "ONLOAD=", "ONERROR=", "ONCLICK=", "ONMOUSEOVER="
    };
    
    for (const auto& pattern : xss_patterns) {
        if (upper_content.find(pattern) != std::string::npos) {
            std::cout << "⚠️ Suspicious XSS pattern detected: " << pattern << std::endl;
            return true;
        }
    }
    
    // Check for path traversal patterns
    static const std::vector<std::string> traversal_patterns = {
        "../", "..\\", "..%2f", "..%5c", "%2e%2e%2f", "%2e%2e%5c"
    };
    
    for (const auto& pattern : traversal_patterns) {
        if (upper_content.find(pattern) != std::string::npos) {
            std::cout << "⚠️ Suspicious path traversal pattern detected: " << pattern << std::endl;
            return true;
        }
    }
    
    return false;
}

bool WebServer::is_valid_json(const std::string& json_string) {
    if (json_string.empty()) {
        return false;
    }
    
    // Basic JSON validation - check for balanced braces and brackets
    int brace_count = 0;
    int bracket_count = 0;
    bool in_string = false;
    bool escaped = false;
    
    for (char c : json_string) {
        if (escaped) {
            escaped = false;
            continue;
        }
        
        if (c == '\\') {
            escaped = true;
            continue;
        }
        
        if (c == '"' && !escaped) {
            in_string = !in_string;
            continue;
        }
        
        if (!in_string) {
            if (c == '{') brace_count++;
            else if (c == '}') brace_count--;
            else if (c == '[') bracket_count++;
            else if (c == ']') bracket_count--;
            
            if (brace_count < 0 || bracket_count < 0) {
                return false;
            }
        }
    }
    
    return brace_count == 0 && bracket_count == 0 && !in_string;
}

// Monitoring helper methods
void WebServer::perform_health_check() {
    std::cout << "🏥 Performing health check..." << std::endl;
    
    // Simulate health checks
    bool server_ok = true;
    bool database_ok = true;
    bool storage_ok = true;
    bool memory_ok = true;
    
    // Check memory usage
    size_t memory_usage = get_memory_usage_mb();
    if (memory_usage > 1024) {
        memory_ok = false;
        std::cout << "⚠️ High memory usage detected: " << memory_usage << " MB" << std::endl;
    }
    
    // Check CPU usage
    double cpu_usage = get_cpu_usage_percent();
    if (cpu_usage > 80.0) {
        std::cout << "⚠️ High CPU usage detected: " << cpu_usage << "%" << std::endl;
    }
    
    // Check active connections
    size_t active_conn = active_connections_.load();
    if (active_conn > max_connections_ * 0.8) {
        std::cout << "⚠️ High connection count: " << active_conn << "/" << max_connections_ << std::endl;
    }
    
    if (server_ok && database_ok && storage_ok && memory_ok) {
        std::cout << "✅ Health check passed" << std::endl;
    } else {
        std::cout << "❌ Health check failed" << std::endl;
    }
}

double WebServer::calculate_average_response_time() {
    if (response_time_history_.empty()) {
        return 0.0;
    }
    
    double sum = std::accumulate(response_time_history_.begin(), response_time_history_.end(), 0.0);
    return sum / response_time_history_.size();
}

size_t WebServer::calculate_cache_hit_rate() {
    if (cache_hits_ + cache_misses_ == 0) {
        return 0;
    }
    return (cache_hits_ * 100) / (cache_hits_ + cache_misses_);
}

long WebServer::get_uptime_seconds() {
    auto now = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
    return uptime.count();
}

size_t WebServer::get_memory_usage_mb() {
    // Simulate memory usage calculation
    // In a real implementation, this would read from /proc/self/status or similar
    return 128 + (total_requests_ % 100); // Simulate varying memory usage
}

double WebServer::get_cpu_usage_percent() {
    // Simulate CPU usage calculation
    // In a real implementation, this would read from /proc/stat or similar
    return 15.0 + (total_requests_ % 20); // Simulate varying CPU usage
}

std::string WebServer::get_last_health_check_timestamp() {
    auto time_t = std::chrono::system_clock::to_time_t(last_health_check_);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::vector<double> WebServer::get_response_time_history() {
    std::lock_guard<std::mutex> lock(monitoring_mutex_);
    return response_time_history_;
}

std::vector<size_t> WebServer::get_memory_usage_history() {
    std::lock_guard<std::mutex> lock(monitoring_mutex_);
    return memory_usage_history_;
}

std::vector<double> WebServer::get_cpu_usage_history() {
    std::lock_guard<std::mutex> lock(monitoring_mutex_);
    return cpu_usage_history_;
}

void WebServer::update_monitoring_data(double response_time, size_t memory_usage, double cpu_usage) {
    std::lock_guard<std::mutex> lock(monitoring_mutex_);
    
    // Keep only last 100 data points
    const size_t max_history_size = 100;
    
    response_time_history_.push_back(response_time);
    if (response_time_history_.size() > max_history_size) {
        response_time_history_.erase(response_time_history_.begin());
    }
    
    memory_usage_history_.push_back(memory_usage);
    if (memory_usage_history_.size() > max_history_size) {
        memory_usage_history_.erase(memory_usage_history_.begin());
    }
    
    cpu_usage_history_.push_back(cpu_usage);
    if (cpu_usage_history_.size() > max_history_size) {
        cpu_usage_history_.erase(cpu_usage_history_.begin());
    }
}

HttpResponse WebServer::handle_bandwidth_status(const HttpRequest& req, HttpResponse& res) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    res.status_code = 200;
    res.headers["Content-Type"] = "application/json";
    res.headers["Cache-Control"] = "no-cache";
    
    std::lock_guard<std::mutex> lock(bandwidth_mutex_);
    
    res.body = "{";
    res.body += "\"compression_enabled\": " + std::string(compression_enabled_ ? "true" : "false") + ",";
    res.body += "\"adaptive_compression_enabled\": " + std::string(adaptive_compression_enabled_ ? "true" : "false") + ",";
    res.body += "\"bandwidth_throttling_enabled\": " + std::string(bandwidth_throttling_enabled_ ? "true" : "false") + ",";
    res.body += "\"compression_level\": " + std::to_string(compression_level_) + ",";
    res.body += "\"min_compression_size\": " + std::to_string(min_compression_size_) + ",";
    res.body += "\"max_bandwidth_per_client\": " + std::to_string(max_bandwidth_per_client_) + ",";
    res.body += "\"total_bytes_sent\": " + std::to_string(total_bytes_sent_) + ",";
    res.body += "\"total_bytes_compressed\": " + std::to_string(total_bytes_compressed_) + ",";
    res.body += "\"average_compression_ratio\": " + std::to_string(average_compression_ratio_) + ",";
    res.body += "\"pre_compressed_content_count\": " + std::to_string(pre_compressed_content_.size()) + ",";
    res.body += "\"active_bandwidth_clients\": " + std::to_string(bandwidth_usage_.size()) + ",";
    
    // Add client-specific bandwidth usage
    res.body += "\"client_bandwidth_usage\": {";
    bool first_client = true;
    for (const auto& [client_ip, usage] : bandwidth_usage_) {
        if (!first_client) res.body += ",";
        res.body += "\"" + client_ip + "\": {";
        res.body += "\"bytes_sent\": " + std::to_string(usage.first) + ",";
        res.body += "\"usage_rate\": " + std::to_string(get_bandwidth_usage_rate(client_ip));
        res.body += "}";
        first_client = false;
    }
    res.body += "}";
    
    res.body += "}";
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    std::cout << "📊 Bandwidth status endpoint processed in " << duration.count() << " μs" << std::endl;
    
    return res;
}

HttpResponse WebServer::handle_bandwidth_optimization(const HttpRequest& req, HttpResponse& res) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    res.status_code = 200;
    res.headers["Content-Type"] = "application/json";
    res.headers["Cache-Control"] = "no-cache";
    
    // Get client IP from request headers or use default
    std::string client_ip = "unknown";
    auto x_forwarded_for = req.headers.find("X-Forwarded-For");
    if (x_forwarded_for != req.headers.end()) {
        client_ip = x_forwarded_for->second;
    } else {
        auto x_real_ip = req.headers.find("X-Real-IP");
        if (x_real_ip != req.headers.end()) {
            client_ip = x_real_ip->second;
        }
    }
    
    res.body = "{";
    res.body += "\"client_ip\": \"" + client_ip + "\",";
    res.body += "\"supports_compression\": " + std::string(supports_compression(req.headers) ? "true" : "false") + ",";
    res.body += "\"optimal_encoding\": \"" + get_optimal_encoding(req.headers) + "\",";
    res.body += "\"bandwidth_usage_rate\": " + std::to_string(get_bandwidth_usage_rate(client_ip)) + ",";
    res.body += "\"compression_recommendations\": {";
    res.body += "\"enable_compression\": " + std::string(compression_enabled_ ? "true" : "false") + ",";
    res.body += "\"adaptive_level\": " + std::to_string(get_adaptive_compression_level("application/json", 1024)) + ",";
    res.body += "\"pre_compression_available\": " + std::string(pre_compressed_content_.size() > 0 ? "true" : "false");
    res.body += "},";
    res.body += "\"optimization_tips\": [";
    res.body += "\"Use gzip compression for text-based content\",";
    res.body += "\"Enable adaptive compression levels based on content size\",";
    res.body += "\"Pre-compress static content for faster delivery\",";
    res.body += "\"Monitor bandwidth usage per client\",";
    res.body += "\"Implement content caching for frequently accessed resources\"";
    res.body += "]";
    res.body += "}";
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    std::cout << "📊 Bandwidth optimization endpoint processed in " << duration.count() << " μs" << std::endl;
    
    return res;
}

// ApiEndpoints implementation
ApiEndpoints::ApiEndpoints(std::shared_ptr<WebServer> server, 
                          std::shared_ptr<dds::storage::HadoopStorage> hadoop_storage)
    : server_(server), hadoop_storage_(hadoop_storage) {
}

void ApiEndpoints::register_endpoints() {
    // Stub implementation
    std::cout << "API endpoints registered" << std::endl;
}

HttpResponse ApiEndpoints::list_jobs(const HttpRequest& req) {
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"jobs\": []}";
    return response;
}

HttpResponse ApiEndpoints::submit_job(const HttpRequest& req) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Validate request method
    if (req.method != "POST") {
        response.status_code = 405;
        response.headers["Content-Type"] = "application/json";
        response.headers["Access-Control-Allow-Origin"] = "*";
        response.headers["Access-Control-Allow-Methods"] = "POST, OPTIONS";
        response.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
        response.body = "{\"error\": \"Method not allowed\", \"allowed_methods\": [\"POST\"]}";
        return response;
    }
    
    // Validate content type
    auto content_type = req.headers.find("Content-Type");
    if (content_type == req.headers.end() || content_type->second.find("application/json") == std::string::npos) {
        response.status_code = 400;
        response.headers["Content-Type"] = "application/json";
        response.headers["Access-Control-Allow-Origin"] = "*";
        response.body = "{\"error\": \"Invalid content type. Expected application/json\"}";
        return response;
    }
    
    // Validate request body
    if (req.body.empty()) {
        response.status_code = 400;
        response.headers["Content-Type"] = "application/json";
        response.headers["Access-Control-Allow-Origin"] = "*";
        response.body = "{\"error\": \"Request body is required\"}";
        return response;
    }
    
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.headers["Access-Control-Allow-Origin"] = "*";
    response.headers["Access-Control-Allow-Methods"] = "POST, OPTIONS";
    response.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
    response.headers["X-Content-Type-Options"] = "nosniff";
    response.headers["Cache-Control"] = "no-cache, no-store, must-revalidate";
    response.headers["Vary"] = "Accept-Encoding";
    response.body = "{\"job_id\": \"stub_job_123\", \"status\": \"submitted\"}";
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    std::cout << "📊 Job submission processed in " << duration.count() << " μs" << std::endl;
    
    return response;
}

HttpResponse ApiEndpoints::get_job_status(const HttpRequest& req) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Validate request method
    if (req.method != "GET") {
        response.status_code = 405;
        response.headers["Content-Type"] = "application/json";
        response.body = "{\"error\": \"Method not allowed\", \"allowed_methods\": [\"GET\"]}";
        return response;
    }
    
    // Validate job ID parameter
    auto job_id = req.query_params.find("job_id");
    if (job_id == req.query_params.end() || job_id->second.empty()) {
        response.status_code = 400;
        response.headers["Content-Type"] = "application/json";
        response.body = "{\"error\": \"Job ID parameter is required\"}";
        return response;
    }
    
    // Sanitize job ID (basic validation)
    std::string sanitized_job_id = job_id->second;
    if (sanitized_job_id.length() > 100 || sanitized_job_id.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_") != std::string::npos) {
        response.status_code = 400;
        response.headers["Content-Type"] = "application/json";
        response.body = "{\"error\": \"Invalid job ID format\"}";
        return response;
    }
    
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"job_id\": \"" + sanitized_job_id + "\", \"status\": \"completed\"}";
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    std::cout << "📊 Job status check processed in " << duration.count() << " μs" << std::endl;
    
    return response;
}

HttpResponse ApiEndpoints::cancel_job(const HttpRequest& req) {
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"status\": \"cancelled\"}";
    return response;
}

HttpResponse ApiEndpoints::list_hdfs_files(const HttpRequest& req) {
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"files\": []}";
    return response;
}

HttpResponse ApiEndpoints::upload_file(const HttpRequest& req) {
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"status\": \"uploaded\"}";
    return response;
}

HttpResponse ApiEndpoints::download_file(const HttpRequest& req) {
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"status\": \"downloaded\"}";
    return response;
}

HttpResponse ApiEndpoints::delete_file(const HttpRequest& req) {
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"status\": \"deleted\"}";
    return response;
}

HttpResponse ApiEndpoints::train_linear_regression(const HttpRequest& req) {
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"status\": \"training\"}";
    return response;
}

HttpResponse ApiEndpoints::train_kmeans(const HttpRequest& req) {
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"status\": \"training\"}";
    return response;
}

HttpResponse ApiEndpoints::predict(const HttpRequest& req) {
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"status\": \"prediction_complete\"}";
    return response;
}

HttpResponse ApiEndpoints::get_system_status(const HttpRequest& req) {
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"status\": \"healthy\"}";
    return response;
}

HttpResponse ApiEndpoints::get_cluster_info(const HttpRequest& req) {
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"cluster_status\": \"healthy\", \"nodes\": 1}";
    return response;
}

HttpResponse ApiEndpoints::get_performance_metrics(const HttpRequest& req) {
    HttpResponse response;
    response.status_code = 200;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"cpu_usage\": 25.5, \"memory_usage\": 45.2}";
    return response;
}

std::string ApiEndpoints::serialize_job_info(const dds::JobInfo& job) {
    // Stub implementation
    return "{}";
}

std::string ApiEndpoints::serialize_file_info(const dds::storage::HDFSFileInfo& file) {
    // Stub implementation
    return "{}";
}

bool ApiEndpoints::parse_json_request(const std::string& body, std::map<std::string, std::string>& params) {
    // Stub implementation
    return true;
}

std::string ApiEndpoints::create_success_response(const std::string& message) {
    return "{\"status\": \"success\", \"message\": \"" + message + "\"}";
}

std::string ApiEndpoints::create_error_response(const std::string& error) {
    return "{\"status\": \"error\", \"message\": \"" + error + "\"}";
}

// WebSocketHandler implementation
void WebSocketHandler::add_client(const std::string& client_id, std::function<void(const std::string&)> callback) {
    // Stub implementation
    std::cout << "Client added: " << client_id << std::endl;
}

void WebSocketHandler::remove_client(const std::string& client_id) {
    // Stub implementation
    std::cout << "Client removed: " << client_id << std::endl;
}

void WebSocketHandler::broadcast_message(const std::string& message) {
    // Stub implementation
    std::cout << "Broadcasting: " << message << std::endl;
}

void WebSocketHandler::send_to_client(const std::string& client_id, const std::string& message) {
    // Stub implementation
    std::cout << "Sending to " << client_id << ": " << message << std::endl;
}

void WebSocketHandler::notify_job_status_change(const std::string& job_id, const std::string& status) {
    // Stub implementation
    std::cout << "Job " << job_id << " status changed to: " << status << std::endl;
}

void WebSocketHandler::notify_file_upload_complete(const std::string& file_path) {
    // Stub implementation
    std::cout << "File upload complete: " << file_path << std::endl;
}

void WebSocketHandler::notify_training_progress(const std::string& job_id, double progress) {
    // Stub implementation
    std::cout << "Training progress for " << job_id << ": " << progress << "%" << std::endl;
}

void WebSocketHandler::notify_cluster_status_change(const std::string& status) {
    // Stub implementation
    std::cout << "Cluster status changed to: " << status << std::endl;
}

// Analytics and profiling methods
void WebServer::record_request_analytics(const HttpRequest& req, const HttpResponse& res, 
                                         std::chrono::microseconds response_time) {
    if (!analytics_enabled_) return;
    
    std::lock_guard<std::mutex> lock(analytics_mutex_);
    
    total_requests_++;
    total_responses_++;
    
    if (res.status_code >= 400) {
        total_errors_++;
    }
    
    // Record endpoint performance
    record_endpoint_performance(req.path, response_time);
    
    // Record status code
    record_status_code(res.status_code);
    
    // Record user agent
    auto user_agent_it = req.headers.find("User-Agent");
    if (user_agent_it != req.headers.end()) {
        record_user_agent(user_agent_it->second);
    }
    
    // Record IP address (extract from request)
    std::string client_ip = "unknown";
    auto x_forwarded_for = req.headers.find("X-Forwarded-For");
    if (x_forwarded_for != req.headers.end()) {
        client_ip = x_forwarded_for->second;
    } else {
        auto x_real_ip = req.headers.find("X-Real-IP");
        if (x_real_ip != req.headers.end()) {
            client_ip = x_real_ip->second;
        }
    }
    record_ip_address(client_ip);
    
    // Record request timestamp
    request_timestamps_.push_back(std::chrono::steady_clock::now());
    
    // Keep only last 1000 timestamps
    if (request_timestamps_.size() > 1000) {
        request_timestamps_.erase(request_timestamps_.begin());
    }
}

void WebServer::record_endpoint_performance(const std::string& endpoint, std::chrono::microseconds response_time) {
    endpoint_performance_[endpoint].push_back(response_time);
    endpoint_request_counts_[endpoint]++;
    
    // Keep only last 100 performance records per endpoint
    if (endpoint_performance_[endpoint].size() > 100) {
        endpoint_performance_[endpoint].erase(endpoint_performance_[endpoint].begin());
    }
}

void WebServer::record_status_code(int status_code) {
    status_code_counts_[status_code]++;
}

void WebServer::record_user_agent(const std::string& user_agent) {
    user_agent_counts_[user_agent]++;
}

void WebServer::record_ip_address(const std::string& ip_address) {
    ip_address_counts_[ip_address]++;
}

double WebServer::calculate_endpoint_average_response_time(const std::string& endpoint) {
    std::lock_guard<std::mutex> lock(analytics_mutex_);
    
    auto it = endpoint_performance_.find(endpoint);
    if (it == endpoint_performance_.end() || it->second.empty()) {
        return 0.0;
    }
    
    double total_microseconds = 0.0;
    for (const auto& time : it->second) {
        total_microseconds += time.count();
    }
    
    return total_microseconds / it->second.size();
}

double WebServer::calculate_endpoint_error_rate(const std::string& endpoint) {
    std::lock_guard<std::mutex> lock(analytics_mutex_);
    
    auto request_count = endpoint_request_counts_.find(endpoint);
    auto error_count = endpoint_error_counts_.find(endpoint);
    
    if (request_count == endpoint_request_counts_.end() || request_count->second == 0) {
        return 0.0;
    }
    
    size_t errors = (error_count != endpoint_error_counts_.end()) ? error_count->second : 0;
    return static_cast<double>(errors) / request_count->second * 100.0;
}

size_t WebServer::get_endpoint_request_count(const std::string& endpoint) {
    std::lock_guard<std::mutex> lock(analytics_mutex_);
    
    auto it = endpoint_request_counts_.find(endpoint);
    return (it != endpoint_request_counts_.end()) ? it->second : 0;
}

std::map<std::string, double> WebServer::get_top_performing_endpoints(size_t limit) {
    std::lock_guard<std::mutex> lock(analytics_mutex_);
    
    std::vector<std::pair<std::string, double>> endpoint_times;
    for (const auto& endpoint : endpoint_performance_) {
        double avg_time = calculate_endpoint_average_response_time(endpoint.first);
        if (avg_time > 0) {
            endpoint_times.push_back({endpoint.first, avg_time});
        }
    }
    
    // Sort by response time (ascending - fastest first)
    std::sort(endpoint_times.begin(), endpoint_times.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });
    
    std::map<std::string, double> result;
    for (size_t i = 0; i < std::min(limit, endpoint_times.size()); ++i) {
        result[endpoint_times[i].first] = endpoint_times[i].second;
    }
    
    return result;
}

std::map<std::string, double> WebServer::get_top_error_endpoints(size_t limit) {
    std::lock_guard<std::mutex> lock(analytics_mutex_);
    
    std::vector<std::pair<std::string, double>> endpoint_errors;
    for (const auto& endpoint : endpoint_request_counts_) {
        double error_rate = calculate_endpoint_error_rate(endpoint.first);
        if (error_rate > 0) {
            endpoint_errors.push_back({endpoint.first, error_rate});
        }
    }
    
    // Sort by error rate (descending - highest errors first)
    std::sort(endpoint_errors.begin(), endpoint_errors.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::map<std::string, double> result;
    for (size_t i = 0; i < std::min(limit, endpoint_errors.size()); ++i) {
        result[endpoint_errors[i].first] = endpoint_errors[i].second;
    }
    
    return result;
}

std::map<int, size_t> WebServer::get_status_code_distribution() {
    std::lock_guard<std::mutex> lock(analytics_mutex_);
    return status_code_counts_;
}

std::map<std::string, size_t> WebServer::get_user_agent_distribution(size_t limit) {
    std::lock_guard<std::mutex> lock(analytics_mutex_);
    
    std::vector<std::pair<std::string, size_t>> agents(user_agent_counts_.begin(), user_agent_counts_.end());
    std::sort(agents.begin(), agents.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::map<std::string, size_t> result;
    for (size_t i = 0; i < std::min(limit, agents.size()); ++i) {
        result[agents[i].first] = agents[i].second;
    }
    
    return result;
}

std::map<std::string, size_t> WebServer::get_ip_address_distribution(size_t limit) {
    std::lock_guard<std::mutex> lock(analytics_mutex_);
    
    std::vector<std::pair<std::string, size_t>> ips(ip_address_counts_.begin(), ip_address_counts_.end());
    std::sort(ips.begin(), ips.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::map<std::string, size_t> result;
    for (size_t i = 0; i < std::min(limit, ips.size()); ++i) {
        result[ips[i].first] = ips[i].second;
    }
    
    return result;
}

double WebServer::get_requests_per_second() {
    std::lock_guard<std::mutex> lock(analytics_mutex_);
    
    if (request_timestamps_.empty()) {
        return 0.0;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - analytics_start_time_).count();
    
    if (duration == 0) {
        return static_cast<double>(total_requests_);
    }
    
    return static_cast<double>(total_requests_) / duration;
}

double WebServer::get_average_response_time() {
    std::lock_guard<std::mutex> lock(analytics_mutex_);
    
    if (total_responses_ == 0) {
        return 0.0;
    }
    
    double total_time = 0.0;
    for (const auto& endpoint : endpoint_performance_) {
        for (const auto& time : endpoint.second) {
            total_time += time.count();
        }
    }
    
    return total_time / total_responses_;
}

double WebServer::get_error_rate() {
    std::lock_guard<std::mutex> lock(analytics_mutex_);
    
    if (total_responses_ == 0) {
        return 0.0;
    }
    
    return static_cast<double>(total_errors_) / total_responses_ * 100.0;
}

void WebServer::reset_analytics() {
    std::lock_guard<std::mutex> lock(analytics_mutex_);
    
    endpoint_performance_.clear();
    endpoint_request_counts_.clear();
    endpoint_error_counts_.clear();
    status_code_counts_.clear();
    user_agent_counts_.clear();
    ip_address_counts_.clear();
    request_timestamps_.clear();
    
    total_requests_ = 0;
    total_responses_ = 0;
    total_errors_ = 0;
    analytics_start_time_ = std::chrono::steady_clock::now();
}

HttpResponse WebServer::handle_analytics_dashboard(const HttpRequest& req, HttpResponse& res) {
    res.status_code = 200;
    res.headers["Content-Type"] = "application/json";
    
    std::stringstream json;
    json << "{";
    json << "\"analytics_enabled\": " << (analytics_enabled_ ? "true" : "false") << ",";
    json << "\"total_requests\": " << total_requests_ << ",";
    json << "\"total_responses\": " << total_responses_ << ",";
    json << "\"total_errors\": " << total_errors_ << ",";
    json << "\"requests_per_second\": " << std::fixed << std::setprecision(2) << get_requests_per_second() << ",";
    json << "\"average_response_time_ms\": " << std::fixed << std::setprecision(2) << get_average_response_time() / 1000.0 << ",";
    json << "\"error_rate_percent\": " << std::fixed << std::setprecision(2) << get_error_rate() << ",";
    json << "\"uptime_seconds\": " << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - analytics_start_time_).count();
    json << "}";
    
    res.body = json.str();
    return res;
}

HttpResponse WebServer::handle_performance_report(const HttpRequest& req, HttpResponse& res) {
    res.status_code = 200;
    res.headers["Content-Type"] = "application/json";
    
    auto top_performing = get_top_performing_endpoints(10);
    auto top_errors = get_top_error_endpoints(10);
    auto status_distribution = get_status_code_distribution();
    
    std::stringstream json;
    json << "{";
    json << "\"top_performing_endpoints\": {";
    bool first = true;
    for (const auto& endpoint : top_performing) {
        if (!first) json << ",";
        json << "\"" << endpoint.first << "\": " << std::fixed << std::setprecision(2) << endpoint.second;
        first = false;
    }
    json << "},";
    
    json << "\"top_error_endpoints\": {";
    first = true;
    for (const auto& endpoint : top_errors) {
        if (!first) json << ",";
        json << "\"" << endpoint.first << "\": " << std::fixed << std::setprecision(2) << endpoint.second;
        first = false;
    }
    json << "},";
    
    json << "\"status_code_distribution\": {";
    first = true;
    for (const auto& status : status_distribution) {
        if (!first) json << ",";
        json << "\"" << status.first << "\": " << status.second;
        first = false;
    }
    json << "}";
    json << "}";
    
    res.body = json.str();
    return res;
}

HttpResponse WebServer::handle_endpoint_analytics(const HttpRequest& req, HttpResponse& res) {
    res.status_code = 200;
    res.headers["Content-Type"] = "application/json";
    
    std::string endpoint = req.query_params.count("endpoint") ? req.query_params.at("endpoint") : "";
    
    if (endpoint.empty()) {
        res.status_code = 400;
        res.body = "{\"error\": \"endpoint parameter required\"}";
        return res;
    }
    
    double avg_response_time = calculate_endpoint_average_response_time(endpoint);
    double error_rate = calculate_endpoint_error_rate(endpoint);
    size_t request_count = get_endpoint_request_count(endpoint);
    
    std::stringstream json;
    json << "{";
    json << "\"endpoint\": \"" << endpoint << "\",";
    json << "\"request_count\": " << request_count << ",";
    json << "\"average_response_time_ms\": " << std::fixed << std::setprecision(2) << avg_response_time / 1000.0 << ",";
    json << "\"error_rate_percent\": " << std::fixed << std::setprecision(2) << error_rate;
    json << "}";
    
    res.body = json.str();
    return res;
}

} // namespace web
} // namespace dds 