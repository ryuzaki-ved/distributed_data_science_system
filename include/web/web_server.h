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

// Web server for the DDS system
class WebServer {
private:
    int port_;
    std::string host_;
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> server_thread_;
    std::map<std::string, RouteHandler> routes_;
    std::shared_ptr<dds::storage::HadoopStorage> hadoop_storage_;

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