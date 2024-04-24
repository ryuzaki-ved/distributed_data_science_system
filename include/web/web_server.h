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
    
    // Advanced caching members
    std::unordered_map<std::string, HttpResponse> response_cache_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> cache_access_times_;
    std::vector<std::string> cache_lru_order_;
    std::mutex cache_mutex_;
    size_t max_cache_size_;
    std::chrono::seconds cache_ttl_;
    bool intelligent_caching_enabled_;
    std::map<std::string, size_t> cache_hit_counts_;
    std::map<std::string, size_t> cache_miss_counts_;
    double cache_hit_ratio_;
    size_t total_cache_requests_;
    std::chrono::steady_clock::time_point cache_stats_start_time_;

    // Error handling and recovery members
    std::atomic<bool> server_healthy_;
    std::atomic<size_t> consecutive_errors_;
    std::map<std::string, size_t> error_counts_;
    std::vector<std::string> error_log_;
    std::mutex error_mutex_;
    std::string error_log_file_;
    bool auto_recovery_enabled_;
    
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

    // Analytics and profiling members
    bool analytics_enabled_;
    std::map<std::string, std::vector<std::chrono::microseconds>> endpoint_performance_;
    std::map<std::string, size_t> endpoint_request_counts_;
    std::map<std::string, size_t> endpoint_error_counts_;
    std::map<int, size_t> status_code_counts_;
    std::map<std::string, size_t> user_agent_counts_;
    std::map<std::string, size_t> ip_address_counts_;
    std::vector<std::chrono::steady_clock::time_point> request_timestamps_;
    std::mutex analytics_mutex_;
    size_t total_requests_;
    size_t total_responses_;
    size_t total_errors_;
    std::chrono::steady_clock::time_point analytics_start_time_;

    // Security members
    bool security_enabled_;
    std::map<std::string, std::string> csrf_tokens_;
    std::map<std::string, size_t> security_event_counts_;
    std::vector<std::string> blocked_ips_;
    std::map<std::string, std::chrono::steady_clock::time_point> ip_rate_limits_;
    std::mutex security_mutex_;
    std::string security_log_file_;

    // Content negotiation and request processing members
    std::vector<std::string> supported_content_types_;
    std::vector<std::string> supported_encodings_;
    std::vector<std::string> supported_languages_;
    std::map<std::string, std::string> content_type_aliases_;
    std::map<std::string, std::string> encoding_aliases_;
    std::map<std::string, std::string> language_aliases_;
    std::map<std::string, size_t> content_negotiation_stats_;
    std::map<std::string, std::vector<std::string>> client_preferences_;
    std::mutex content_negotiation_mutex_;
    bool content_negotiation_enabled_;
    std::string default_content_type_;
    std::string default_encoding_;
    std::string default_language_;

    // Session and User data structures
    struct Session {
        std::string username;
        std::string user_agent;
        std::chrono::steady_clock::time_point created_at;
        std::chrono::steady_clock::time_point last_activity;
        std::map<std::string, std::string> data;
        bool is_active;
    };

    struct User {
        std::string username;
        std::string password_hash;
        std::vector<std::string> roles;
        std::chrono::steady_clock::time_point created_at;
        std::chrono::steady_clock::time_point last_login;
        size_t failed_attempts;
        std::chrono::steady_clock::time_point lockout_until;
        bool is_active;
    };

    // Session management and authentication members
    std::map<std::string, Session> active_sessions_;
    std::map<std::string, User> registered_users_;
    std::map<std::string, std::vector<std::string>> user_roles_;
    std::map<std::string, std::vector<std::string>> role_permissions_;
    std::map<std::string, size_t> session_stats_;
    std::map<std::string, size_t> authentication_stats_;
    std::mutex session_mutex_;
    std::mutex user_mutex_;
    bool session_management_enabled_;
    bool authentication_enabled_;
    std::chrono::seconds session_timeout_;
    std::chrono::seconds token_expiry_;
    std::string jwt_secret_;
    std::string session_cookie_name_;
    std::string auth_cookie_name_;
    size_t max_sessions_per_user_;
    size_t max_failed_attempts_;
    std::chrono::seconds lockout_duration_;
    
    // API documentation and OpenAPI/Swagger members
    bool api_documentation_enabled_;
    bool swagger_ui_enabled_;
    std::string api_version_;
    std::string api_title_;
    std::string api_description_;
    std::string api_contact_email_;
    std::string api_license_name_;
    std::string api_license_url_;
    std::map<std::string, std::string> endpoint_descriptions_;
    std::map<std::string, std::map<std::string, std::string>> endpoint_parameters_;
    std::map<std::string, std::map<int, std::string>> endpoint_responses_;
    std::map<std::string, std::string> endpoint_tags_;
    std::map<std::string, std::string> endpoint_summaries_;
    std::map<std::string, std::string> endpoint_examples_;
    std::map<std::string, std::vector<std::string>> endpoint_required_fields_;
    std::map<std::string, std::map<std::string, std::string>> endpoint_schemas_;
    std::mutex documentation_mutex_;
    std::map<std::string, size_t> documentation_stats_;
    
    // WebSocket and real-time communication members
    bool websocket_enabled_;
    bool realtime_enabled_;
    std::map<std::string, int> websocket_connections_;
    std::map<std::string, std::function<void(const std::string&)>> websocket_handlers_;
    std::map<std::string, std::vector<std::string>> websocket_rooms_;
    std::map<std::string, std::string> websocket_user_map_;
    std::map<std::string, size_t> websocket_stats_;
    std::mutex websocket_mutex_;
    std::thread websocket_cleanup_thread_;
    std::atomic<bool> websocket_cleanup_running_;
    std::chrono::seconds websocket_heartbeat_interval_;
    std::chrono::seconds websocket_timeout_;
    std::string websocket_upgrade_header_;
    std::string websocket_accept_key_;
    std::map<std::string, std::chrono::steady_clock::time_point> websocket_last_activity_;
    std::queue<std::string> websocket_message_queue_;
    std::mutex websocket_queue_mutex_;
    std::condition_variable websocket_queue_cv_;
    std::thread websocket_message_thread_;
    std::atomic<bool> websocket_message_running_;

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
    RouteHandler find_route(const std::string& method, const std::string& path);
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

        // Analytics and profiling methods
    void record_request_analytics(const HttpRequest& req, const HttpResponse& res,
                                  std::chrono::microseconds response_time);
    void record_endpoint_performance(const std::string& endpoint, std::chrono::microseconds response_time);
    void record_status_code(int status_code);
    void record_user_agent(const std::string& user_agent);
    void record_ip_address(const std::string& ip_address);
    double calculate_endpoint_average_response_time(const std::string& endpoint);
    double calculate_endpoint_error_rate(const std::string& endpoint);
    size_t get_endpoint_request_count(const std::string& endpoint);
    std::map<std::string, double> get_top_performing_endpoints(size_t limit = 10);
    std::map<std::string, double> get_top_error_endpoints(size_t limit = 10);
    std::map<int, size_t> get_status_code_distribution();
    std::map<std::string, size_t> get_user_agent_distribution(size_t limit = 10);
    std::map<std::string, size_t> get_ip_address_distribution(size_t limit = 10);
    double get_requests_per_second();
    double get_average_response_time();
    double get_error_rate();
    void reset_analytics();
    HttpResponse handle_analytics_dashboard(const HttpRequest& req, HttpResponse& res);
    HttpResponse handle_performance_report(const HttpRequest& req, HttpResponse& res);
    HttpResponse handle_endpoint_analytics(const HttpRequest& req, HttpResponse& res);

    // Advanced caching methods
    void cache_response(const std::string& cache_key, const HttpResponse& response);
    void invalidate_cache(const std::string& cache_key);
    void clear_cache();
    void update_cache_access_time(const std::string& cache_key);
    void evict_lru_cache_entry();
    bool should_cache_response(const HttpResponse& response);
    std::string generate_cache_key(const HttpRequest& req);
    double get_cache_hit_ratio();
    size_t get_cache_size();
    std::map<std::string, size_t> get_cache_hit_counts();
    std::map<std::string, size_t> get_cache_miss_counts();
    void reset_cache_stats();
    HttpResponse handle_cache_status(const HttpRequest& req, HttpResponse& res);
    HttpResponse handle_cache_management(const HttpRequest& req, HttpResponse& res);

    // Error handling and recovery methods
    void handle_server_error(const std::exception& e, HttpResponse& res);
    void handle_client_error(int status_code, const std::string& message, HttpResponse& res);
    void log_error(const std::string& error_type, const std::string& message, const std::string& details = "");
    void recover_from_error();
    bool is_server_healthy();
    void perform_health_check();
    void cleanup_resources();
    void restart_failed_components();
    HttpResponse handle_error_status(const HttpRequest& req, HttpResponse& res);
    HttpResponse handle_health_check(const HttpRequest& req, HttpResponse& res);

    // Advanced request processing and content negotiation methods
    std::string negotiate_content_type(const HttpRequest& req, const std::vector<std::string>& available_types);
    std::string negotiate_encoding(const HttpRequest& req, const std::vector<std::string>& available_encodings);
    std::string negotiate_language(const HttpRequest& req, const std::vector<std::string>& available_languages);
    std::string parse_accept_header(const std::string& accept_header, const std::vector<std::string>& available_options);
    std::map<std::string, double> parse_quality_values(const std::string& header_value);
    std::string select_best_match(const std::map<std::string, double>& quality_map, const std::vector<std::string>& available_options);
    void add_vary_header(HttpResponse& res, const std::vector<std::string>& vary_fields);
    void add_content_negotiation_headers(HttpResponse& res, const HttpRequest& req);
    std::string get_preferred_content_type(const HttpRequest& req);
    std::string get_preferred_encoding(const HttpRequest& req);
    std::string get_preferred_language(const HttpRequest& req);
    bool supports_content_type(const HttpRequest& req, const std::string& content_type);
    bool supports_encoding(const HttpRequest& req, const std::string& encoding);
    bool supports_language(const HttpRequest& req, const std::string& language);
    void process_request_headers(const HttpRequest& req);
    void validate_request_headers(const HttpRequest& req);
    void normalize_request_headers(HttpRequest& req);
    std::string extract_client_info(const HttpRequest& req);
    void log_request_details(const HttpRequest& req);
    HttpResponse handle_content_negotiation_test(const HttpRequest& req, HttpResponse& res);

    // Session management and authentication methods
    std::string create_session(const std::string& username, const std::string& user_agent);
    bool validate_session(const std::string& session_id);
    void invalidate_session(const std::string& session_id);
    std::string get_session_user(const std::string& session_id);
    void update_session_activity(const std::string& session_id);
    void cleanup_expired_sessions();
    std::string generate_jwt_token(const std::string& username, const std::vector<std::string>& roles);
    bool validate_jwt_token(const std::string& token, std::string& username);
    std::string hash_password(const std::string& password);
    bool verify_password(const std::string& password, const std::string& hash);
    bool authenticate_user(const std::string& username, const std::string& password);
    bool register_user(const std::string& username, const std::string& password, const std::vector<std::string>& roles);
    bool has_permission(const std::string& username, const std::string& permission);
    std::vector<std::string> get_user_roles(const std::string& username);
    std::vector<std::string> get_role_permissions(const std::string& role);
    void add_role_permission(const std::string& role, const std::string& permission);
    void remove_role_permission(const std::string& role, const std::string& permission);
    bool is_user_locked_out(const std::string& username);
    void record_failed_login(const std::string& username);
    void reset_failed_attempts(const std::string& username);
    std::string extract_session_id(const HttpRequest& req);
    void add_session_cookie(HttpResponse& res, const std::string& session_id);
    void add_auth_cookie(HttpResponse& res, const std::string& token);
    HttpResponse handle_login(const HttpRequest& req, HttpResponse& res);
    HttpResponse handle_logout(const HttpRequest& req, HttpResponse& res);
    HttpResponse handle_register(const HttpRequest& req, HttpResponse& res);
    HttpResponse handle_session_status(const HttpRequest& req, HttpResponse& res);
    HttpResponse handle_user_profile(const HttpRequest& req, HttpResponse& res);
    HttpResponse handle_change_password(const HttpRequest& req, HttpResponse& res);
    HttpResponse require_authentication(const HttpRequest& req, HttpResponse& res, const std::function<HttpResponse(const HttpRequest&, HttpResponse&)>& handler);
        HttpResponse require_permission(const HttpRequest& req, HttpResponse& res, const std::string& permission, const std::function<HttpResponse(const HttpRequest&, HttpResponse&)>& handler);
    
    // API documentation and OpenAPI/Swagger methods
    void enable_api_documentation(bool enabled = true);
    void enable_swagger_ui(bool enabled = true);
    void set_api_info(const std::string& title, const std::string& description, const std::string& version);
    void set_api_contact(const std::string& email);
    void set_api_license(const std::string& name, const std::string& url);
    void add_endpoint_documentation(const std::string& endpoint, const std::string& method, 
                                   const std::string& summary, const std::string& description, 
                                   const std::string& tag = "default");
    void add_endpoint_parameter(const std::string& endpoint, const std::string& method,
                               const std::string& name, const std::string& type, 
                               const std::string& description, bool required = false);
    void add_endpoint_response(const std::string& endpoint, const std::string& method,
                              int status_code, const std::string& description);
    void add_endpoint_example(const std::string& endpoint, const std::string& method,
                             const std::string& example);
    void add_endpoint_schema(const std::string& endpoint, const std::string& method,
                            const std::string& field, const std::string& type, 
                            const std::string& description = "");
    void add_required_field(const std::string& endpoint, const std::string& method,
                           const std::string& field);
    std::string generate_openapi_spec();
    std::string generate_swagger_ui_html();
    std::string generate_api_documentation_html();
    HttpResponse handle_openapi_spec(const HttpRequest& req, HttpResponse& res);
    HttpResponse handle_swagger_ui(const HttpRequest& req, HttpResponse& res);
    HttpResponse handle_api_docs(const HttpRequest& req, HttpResponse& res);
    HttpResponse handle_endpoint_docs(const HttpRequest& req, HttpResponse& res);
    void initialize_api_documentation();
    void register_endpoint_documentation();
    std::string format_json_schema(const std::map<std::string, std::string>& schema);
    std::string generate_markdown_documentation();
    std::string generate_postman_collection();
    HttpResponse handle_markdown_docs(const HttpRequest& req, HttpResponse& res);
    HttpResponse handle_postman_collection(const HttpRequest& req, HttpResponse& res);
    void export_documentation(const std::string& format, const std::string& file_path);
    size_t get_documentation_requests();
    void reset_documentation_stats();
    
    // WebSocket and real-time communication methods
    void enable_websocket(bool enabled = true);
    void enable_realtime(bool enabled = true);
    bool is_websocket_request(const HttpRequest& req);
    std::string generate_websocket_accept_key(const std::string& client_key);
    HttpResponse handle_websocket_upgrade(const HttpRequest& req, HttpResponse& res);
    void handle_websocket_connection(int client_socket, const std::string& connection_id);
    void process_websocket_message(const std::string& connection_id, const std::string& message);
    void broadcast_websocket_message(const std::string& message, const std::string& room = "");
    void send_websocket_message(const std::string& connection_id, const std::string& message);
    void join_websocket_room(const std::string& connection_id, const std::string& room);
    void leave_websocket_room(const std::string& connection_id, const std::string& room);
    void close_websocket_connection(const std::string& connection_id);
    void cleanup_inactive_websocket_connections();
    void start_websocket_cleanup_thread();
    void stop_websocket_cleanup_thread();
    void start_websocket_message_thread();
    void stop_websocket_message_thread();
    void process_websocket_message_queue();
    std::string encode_websocket_frame(const std::string& payload, uint8_t opcode = 0x01);
    std::string decode_websocket_frame(const std::string& frame);
    bool is_websocket_frame_complete(const std::string& frame);
    uint8_t get_websocket_opcode(const std::string& frame);
    std::string get_websocket_payload(const std::string& frame);
    void send_websocket_heartbeat(const std::string& connection_id);
    void update_websocket_activity(const std::string& connection_id);
    bool is_websocket_connection_active(const std::string& connection_id);
    size_t get_websocket_connection_count();
    size_t get_websocket_room_count(const std::string& room);
    std::vector<std::string> get_websocket_connections_in_room(const std::string& room);
    void add_websocket_message_handler(const std::string& event_type, std::function<void(const std::string&, const std::string&)> handler);
    void remove_websocket_message_handler(const std::string& event_type);
    HttpResponse handle_websocket_status(const HttpRequest& req, HttpResponse& res);
    HttpResponse handle_websocket_test(const HttpRequest& req, HttpResponse& res);
    void initialize_websocket_system();
    void cleanup_websocket_resources();
    
    // Security methods
    std::string sanitize_input(const std::string& input);
    std::string encode_html_entities(const std::string& input);
    std::string validate_json(const std::string& json);
    bool is_sql_injection_attempt(const std::string& input);
    bool is_xss_attempt(const std::string& input);
    std::string generate_csrf_token();
    bool validate_csrf_token(const std::string& token);
    void add_security_headers(HttpResponse& res);
    std::string hash_password(const std::string& password);
    std::string generate_secure_random_string(size_t length);
    bool is_rate_limited_by_ip(const std::string& ip);
    void log_security_event(const std::string& event, const std::string& ip, const std::string& details);
    
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
    void update_monitoring_data(double response_time, size_t memory_usage, double cpu_usage);
    
    // Bandwidth optimization methods
    bool should_compress_content(const std::string& content_type, size_t content_length);
    void optimize_response_headers(HttpResponse& response);
    int get_adaptive_compression_level(const std::string& content_type, size_t content_length);
    bool should_throttle_bandwidth(const std::string& client_ip, size_t response_size);
    void update_bandwidth_usage(const std::string& client_ip, size_t bytes_sent);
    double get_bandwidth_usage_rate(const std::string& client_ip);
    void pre_compress_static_content();
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