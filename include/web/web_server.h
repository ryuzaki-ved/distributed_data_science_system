#pragma once

#include "../utils/types.h"
#include "../storage/hadoop_storage.h"
#include <string>
#include <map>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>



namespace dds {
namespace web {

// HTTP request structure
struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> query_params;
};

// HTTP response structure
struct HttpResponse {
    int status_code;
    std::string body;
    std::map<std::string, std::string> headers;
    
    HttpResponse() : status_code(200) {
        headers["Content-Type"] = "application/json";
    }
};

// Route handler function type
using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

// Middleware handler function type
using NextHandler = std::function<void(const HttpRequest&, HttpResponse&)>;
using MiddlewareHandler = std::function<void(const HttpRequest&, HttpResponse&, NextHandler)>;

// Route definition structure
struct RouteDefinition {
    std::string method;
    std::string path;
    RouteHandler handler;
};

// Middleware definition structure
struct MiddlewareDefinition {
    std::string name;
    MiddlewareHandler handler;
};

// Web server for the DDS system
class WebServer {
private:
    int port_;
    std::string host_;
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> server_thread_;
    std::map<std::string, RouteHandler> routes_;
    std::shared_ptr<dds::storage::HadoopStorage> hadoop_storage_;
    
    // Routing framework members
    std::vector<MiddlewareDefinition> middleware_stack_;
    std::map<std::string, RouteHandler> route_cache_;
    bool routing_enabled_;
    bool middleware_enabled_;
    bool route_cache_enabled_;
    
    // Monitoring members
    bool monitoring_enabled_;
    int health_check_interval_;
    std::chrono::steady_clock::time_point last_health_check_;
    std::chrono::steady_clock::time_point start_time_;
    std::mutex monitoring_mutex_;
    std::vector<double> response_time_history_;
    std::vector<size_t> memory_usage_history_;
    std::vector<double> cpu_usage_history_;
    size_t cache_hits_;
    size_t cache_misses_;
    
    // Bandwidth optimization members
    bool compression_enabled_;
    int compression_level_;
    size_t min_compression_size_;
    bool adaptive_compression_enabled_;
    bool bandwidth_throttling_enabled_;
    size_t max_bandwidth_per_client_;
    std::map<std::string, std::pair<size_t, std::chrono::steady_clock::time_point>> bandwidth_usage_;
    std::mutex bandwidth_mutex_;
    std::map<std::string, std::string> pre_compressed_content_;
    std::mutex pre_compressed_mutex_;
    size_t total_bytes_sent_;
    size_t total_bytes_compressed_;
    double average_compression_ratio_;

public:
    WebServer(int port = 8080, const std::string& host = "localhost");
    ~WebServer();
    
    // Server control
    bool start();
    void stop();
    bool is_running() const { return running_; }
    
    // Route registration
    void add_route(const std::string& method, const std::string& path, RouteHandler handler);
    void add_get_route(const std::string& path, RouteHandler handler);
    void add_post_route(const std::string& path, RouteHandler handler);
    void add_put_route(const std::string& path, RouteHandler handler);
    void add_delete_route(const std::string& path, RouteHandler handler);
    
    // Routing framework
    void add_middleware(const std::string& name, MiddlewareHandler middleware);
    void add_route_group(const std::string& prefix, const std::vector<RouteDefinition>& routes);
    std::optional<RouteHandler> find_route(const std::string& method, const std::string& path);
    HttpResponse execute_middleware_stack(const HttpRequest& req, RouteHandler route_handler);
    HttpResponse list_routes(const HttpRequest& req, HttpResponse& res);
    HttpResponse list_middleware(const HttpRequest& req, HttpResponse& res);
    
    // Monitoring endpoints
    HttpResponse handle_health_check(const HttpRequest& req, HttpResponse& res);
    HttpResponse handle_metrics(const HttpRequest& req, HttpResponse& res);
    HttpResponse handle_monitoring_status(const HttpRequest& req, HttpResponse& res);
    HttpResponse handle_performance_metrics(const HttpRequest& req, HttpResponse& res);
    HttpResponse handle_bandwidth_status(const HttpRequest& req, HttpResponse& res);
    HttpResponse handle_bandwidth_optimization(const HttpRequest& req, HttpResponse& res);
    
    // Storage integration
    void set_hadoop_storage(std::shared_ptr<dds::storage::HadoopStorage> storage);
    
    // Default route handlers
    HttpResponse handle_status(const HttpRequest& req);
    HttpResponse handle_jobs_list(const HttpRequest& req);
    HttpResponse handle_job_submit(const HttpRequest& req);
    HttpResponse handle_job_status(const HttpRequest& req);
    HttpResponse handle_hdfs_list(const HttpRequest& req);
    HttpResponse handle_hdfs_upload(const HttpRequest& req);
    HttpResponse handle_hdfs_download(const HttpRequest& req);
    HttpResponse handle_algorithm_train(const HttpRequest& req);
    HttpResponse handle_algorithm_predict(const HttpRequest& req);
    HttpResponse handle_cluster_info(const HttpRequest& req);
    
private:
    void run_server(int serverSocket);
    void handle_client(int clientSocket);
    HttpRequest parse_request(const std::string& request);
    HttpResponse handle_request(const HttpRequest& req);
    HttpResponse serve_dashboard();
    std::string format_response(const HttpResponse& response);
    std::string parse_request_line(const std::string& line, std::string& method, std::string& path);
    std::map<std::string, std::string> parse_headers(const std::vector<std::string>& lines);
    std::map<std::string, std::string> parse_query_params(const std::string& query_string);
    std::string url_decode(const std::string& encoded);
    std::string generate_json_response(const std::map<std::string, std::string>& data);
    std::string generate_error_response(const std::string& error, int status_code = 400);
    
    // Routing framework private methods
    void initialize_default_routes();
    bool match_route_pattern(const std::string& pattern, const std::string& path);
    std::vector<std::string> split_string(const std::string& str, char delimiter);
    
    // Monitoring helper methods
    void perform_health_check();
    double calculate_average_response_time();
    size_t calculate_cache_hit_rate();
    long get_uptime_seconds();
    size_t get_memory_usage_mb();
    double get_cpu_usage_percent();
    std::string get_last_health_check_timestamp();
    std::vector<double> get_response_time_history();
    std::vector<size_t> get_memory_usage_history();
    std::vector<double> get_cpu_usage_history();
    void update_monitoring_data(double response_time, size_t memory_usage, double cpu_usage);
    
    // Bandwidth optimization methods
    std::optional<std::string> compress_content(const std::string& content);
    std::optional<std::string> decompress_content(const std::string& compressed_content);
    bool should_compress_content(const std::string& content_type, size_t content_length);
    void optimize_response_headers(HttpResponse& response);
    int get_adaptive_compression_level(const std::string& content_type, size_t content_length);
    bool should_throttle_bandwidth(const std::string& client_ip, size_t response_size);
    void update_bandwidth_usage(const std::string& client_ip, size_t bytes_sent);
    double get_bandwidth_usage_rate(const std::string& client_ip);
    void pre_compress_static_content();
    std::optional<std::string> get_pre_compressed_content(const std::string& content_key);
    void cache_compressed_content(const std::string& content_key, const std::string& compressed_content);
    bool supports_compression(const std::map<std::string, std::string>& headers);
    std::string get_optimal_encoding(const std::map<std::string, std::string>& headers);
    void log_bandwidth_metrics(const std::string& client_ip, size_t original_size, size_t compressed_size, double compression_ratio);
};

// API endpoints manager
class ApiEndpoints {
private:
    std::shared_ptr<WebServer> server_;
    std::shared_ptr<dds::storage::HadoopStorage> hadoop_storage_;

public:
    ApiEndpoints(std::shared_ptr<WebServer> server, 
                std::shared_ptr<dds::storage::HadoopStorage> hadoop_storage);
    
    // Register all API endpoints
    void register_endpoints();
    
    // Job management endpoints
    HttpResponse list_jobs(const HttpRequest& req);
    HttpResponse submit_job(const HttpRequest& req);
    HttpResponse get_job_status(const HttpRequest& req);
    HttpResponse cancel_job(const HttpRequest& req);
    
    // HDFS management endpoints
    HttpResponse list_hdfs_files(const HttpRequest& req);
    HttpResponse upload_file(const HttpRequest& req);
    HttpResponse download_file(const HttpRequest& req);
    HttpResponse delete_file(const HttpRequest& req);
    
    // Algorithm endpoints
    HttpResponse train_linear_regression(const HttpRequest& req);
    HttpResponse train_kmeans(const HttpRequest& req);
    HttpResponse predict(const HttpRequest& req);
    
    // System monitoring endpoints
    HttpResponse get_system_status(const HttpRequest& req);
    HttpResponse get_cluster_info(const HttpRequest& req);
    HttpResponse get_performance_metrics(const HttpRequest& req);
    
private:
    // Helper methods
    std::string serialize_job_info(const dds::JobInfo& job);
    std::string serialize_file_info(const dds::storage::HDFSFileInfo& file);
    bool parse_json_request(const std::string& body, std::map<std::string, std::string>& params);
    std::string create_success_response(const std::string& message = "Success");
    std::string create_error_response(const std::string& error);
};

// WebSocket support for real-time updates
class WebSocketHandler {
private:
    std::map<std::string, std::function<void(const std::string&)>> clients_;
    std::mutex clients_mutex_;

public:
    void add_client(const std::string& client_id, std::function<void(const std::string&)> callback);
    void remove_client(const std::string& client_id);
    void broadcast_message(const std::string& message);
    void send_to_client(const std::string& client_id, const std::string& message);
    
    // Real-time notifications
    void notify_job_status_change(const std::string& job_id, const std::string& status);
    void notify_file_upload_complete(const std::string& file_path);
    void notify_training_progress(const std::string& job_id, double progress);
    void notify_cluster_status_change(const std::string& status);
};

} // namespace web
} // namespace dds 