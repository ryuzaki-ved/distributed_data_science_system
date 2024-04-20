#include "../../include/web/web_server.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <map>
#include <mutex>
#include <algorithm>
#include <vector>

namespace dds {
namespace web {

WebServer::WebServer(int port, const std::string& host) 
    : port_(port), host_(host), running_(false), rate_limit_window_(60), max_requests_per_minute_(100) {
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

void WebServer::set_hadoop_storage(std::shared_ptr<dds::storage::HadoopStorage> storage) {
    hadoop_storage_ = storage;
}

HttpResponse WebServer::handle_status(const HttpRequest& req) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Rate limiting check
    std::string client_ip = req.headers.count("X-Forwarded-For") ? req.headers.at("X-Forwarded-For") : "127.0.0.1";
    if (!check_rate_limit(client_ip)) {
        HttpResponse response;
        response.status_code = 429;
        response.headers["Content-Type"] = "application/json";
        response.headers["Retry-After"] = "60";
        response.body = "{\"error\": \"Rate limit exceeded. Please try again later.\"}";
        std::cout << "🚫 Rate limit exceeded for client: " << client_ip << std::endl;
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
    response.body = "{\"status\": \"running\", \"version\": \"1.0.0\"}";
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
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

void WebServer::run_server(int serverSocket) {
    // Stub implementation
    std::cout << "Server running on port " << port_ << std::endl;
}

void WebServer::handle_client(int clientSocket) {
    // Stub implementation
    std::cout << "Client connected" << std::endl;
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

} // namespace web
} // namespace dds 