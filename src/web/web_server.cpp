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
    average_compression_ratio_(0.0),                     analytics_enabled_(true), total_requests_(0),
                                                    total_responses_(0), total_errors_(0), analytics_start_time_(std::chrono::steady_clock::now()),
                                security_enabled_(true), security_log_file_("security.log"), max_cache_size_(1000),
                                cache_ttl_(std::chrono::seconds(300)), intelligent_caching_enabled_(true),                                 cache_hit_ratio_(0.0),
                                total_cache_requests_(0), cache_stats_start_time_(std::chrono::steady_clock::now()),
                                                                  server_healthy_(true), consecutive_errors_(0), total_errors_(0),
                                  last_health_check_(std::chrono::steady_clock::now()), health_check_interval_(std::chrono::seconds(30)),
                                  error_log_file_("error.log"), auto_recovery_enabled_(true),
                                                                                            content_negotiation_enabled_(true), default_content_type_("application/json"),
                                                          default_encoding_("identity"), default_language_("en"),
                                                          session_management_enabled_(true), authentication_enabled_(true),
                                                          session_timeout_(std::chrono::seconds(3600)), token_expiry_(std::chrono::seconds(7200)),
                                                          jwt_secret_("your-secret-key-change-in-production"), session_cookie_name_("session_id"),
                                                          auth_cookie_name_("auth_token"), max_sessions_per_user_(5), max_failed_attempts_(5),
                                                          lockout_duration_(std::chrono::seconds(900)),
                                                          api_documentation_enabled_(true), swagger_ui_enabled_(true),
                                                          api_version_("1.0.0"), api_title_("Distributed Data Science System API"),
                                                          api_description_("A comprehensive API for distributed data science operations including job management, HDFS operations, and machine learning algorithms"),
                                                          api_contact_email_("admin@dds-system.com"), api_license_name_("MIT"),
                                                          api_license_url_("https://opensource.org/licenses/MIT") {
    
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
                                                        std::cout << "🛡️ Security features: " << (security_enabled_ ? "Enabled" : "Disabled") << std::endl;
                                    std::cout << "🧠 Intelligent caching: " << (intelligent_caching_enabled_ ? "Enabled" : "Disabled") << " (max: " << max_cache_size_ << " entries, TTL: " << cache_ttl_.count() << "s)" << std::endl;
                                            std::cout << "🔄 Auto-recovery: " << (auto_recovery_enabled_ ? "Enabled" : "Disabled") << " (health check: " << health_check_interval_.count() << "s)" << std::endl;
        std::cout << "🤝 Content negotiation: " << (content_negotiation_enabled_ ? "Enabled" : "Disabled") << " (default: " << default_content_type_ << ")" << std::endl;
        std::cout << "🔐 Session management: " << (session_management_enabled_ ? "Enabled" : "Disabled") << " (timeout: " << session_timeout_.count() << "s)" << std::endl;
        std::cout << "🔑 Authentication: " << (authentication_enabled_ ? "Enabled" : "Disabled") << " (max sessions: " << max_sessions_per_user_ << ")" << std::endl;
        std::cout << "📚 API Documentation: " << (api_documentation_enabled_ ? "Enabled" : "Disabled") << " (version: " << api_version_ << ")" << std::endl;
        std::cout << "🔍 Swagger UI: " << (swagger_ui_enabled_ ? "Enabled" : "Disabled") << std::endl;
    std::cout << "🛣️ Routing framework enabled with middleware support" << std::endl;
    std::cout << "📊 Monitoring and health checks enabled (interval: " << health_check_interval_ << "s)" << std::endl;
    
    // Initialize default routes
    initialize_default_routes();
    
    // Initialize API documentation
    initialize_api_documentation();
    
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

                    // Security endpoints
                    add_get_route("/api/security/status", [this](const HttpRequest& req, HttpResponse& res) {
                        return handle_security_status(req, res);
                    });

                                                    add_post_route("/api/security/csrf-token", [this](const HttpRequest& req, HttpResponse& res) {
                                    return handle_csrf_token_generation(req, res);
                                });

                                // Cache management endpoints
                                add_get_route("/api/cache/status", [this](const HttpRequest& req, HttpResponse& res) {
                                    return handle_cache_status(req, res);
                                });

                                add_post_route("/api/cache/clear", [this](const HttpRequest& req, HttpResponse& res) {
                                    return handle_cache_management(req, res);
                                });

                                // Error handling and recovery endpoints
                                add_get_route("/api/health", [this](const HttpRequest& req, HttpResponse& res) {
                                    return handle_health_check(req, res);
                                });

                                add_get_route("/api/error/status", [this](const HttpRequest& req, HttpResponse& res) {
                                    return handle_error_status(req, res);
                                });

                                        // Content negotiation endpoints
        add_get_route("/api/negotiation/test", [this](const HttpRequest& req, HttpResponse& res) {
            return handle_content_negotiation_test(req, res);
        });

        // Authentication and session management endpoints
        add_post_route("/api/auth/login", [this](const HttpRequest& req, HttpResponse& res) {
            return handle_login(req, res);
        });

        add_post_route("/api/auth/logout", [this](const HttpRequest& req, HttpResponse& res) {
            return handle_logout(req, res);
        });

        add_post_route("/api/auth/register", [this](const HttpRequest& req, HttpResponse& res) {
            return handle_register(req, res);
        });

        add_get_route("/api/auth/session", [this](const HttpRequest& req, HttpResponse& res) {
            return handle_session_status(req, res);
        });

        add_get_route("/api/auth/profile", [this](const HttpRequest& req, HttpResponse& res) {
            return require_authentication(req, res, [this](const HttpRequest& req, HttpResponse& res) {
                return handle_user_profile(req, res);
            });
        });

        add_post_route("/api/auth/change-password", [this](const HttpRequest& req, HttpResponse& res) {
            return require_authentication(req, res, [this](const HttpRequest& req, HttpResponse& res) {
                return handle_change_password(req, res);
            });
        });

        // API documentation endpoints
        add_get_route("/api/docs", [this](const HttpRequest& req, HttpResponse& res) {
            return handle_api_docs(req, res);
        });

        add_get_route("/api/docs/openapi.json", [this](const HttpRequest& req, HttpResponse& res) {
            return handle_openapi_spec(req, res);
        });

        add_get_route("/api/docs/swagger", [this](const HttpRequest& req, HttpResponse& res) {
            return handle_swagger_ui(req, res);
        });

        add_get_route("/api/docs/markdown", [this](const HttpRequest& req, HttpResponse& res) {
            return handle_markdown_docs(req, res);
        });

        add_get_route("/api/docs/postman", [this](const HttpRequest& req, HttpResponse& res) {
            return handle_postman_collection(req, res);
        });

        add_get_route("/api/docs/endpoints", [this](const HttpRequest& req, HttpResponse& res) {
            return handle_endpoint_docs(req, res);
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
    std::cout << "   Security Enabled: " << (security_enabled_ ? "Enabled" : "Disabled") << std::endl;
    std::cout << "   Blocked IPs: " << blocked_ips_.size() << std::endl;
                        std::cout << "   Security Events: " << security_event_counts_.size() << std::endl;
                                                        std::cout << "   Cache Size: " << get_cache_size() << "/" << max_cache_size_ << " entries" << std::endl;
                                    std::cout << "   Cache Hit Ratio: " << std::fixed << std::setprecision(2) << get_cache_hit_ratio() << "%" << std::endl;
                                    std::cout << "   Cache Hits: " << cache_hits_ << ", Misses: " << cache_misses_ << std::endl;
                                    std::cout << "   Server Health: " << (server_healthy_ ? "Healthy" : "Unhealthy") << std::endl;
                                    std::cout << "   Total Errors: " << total_errors_ << ", Consecutive: " << consecutive_errors_ << std::endl;
                                            std::cout << "   Auto-recovery: " << (auto_recovery_enabled_ ? "Enabled" : "Disabled") << std::endl;
        std::cout << "   Content Negotiation: " << (content_negotiation_enabled_ ? "Enabled" : "Disabled") << std::endl;
        std::cout << "   Negotiation Requests: " << content_negotiation_stats_["total_requests"] << std::endl;
        std::cout << "   Session Management: " << (session_management_enabled_ ? "Enabled" : "Disabled") << std::endl;
        std::cout << "   Active Sessions: " << session_stats_["active_sessions"] << std::endl;
        std::cout << "   Total Sessions: " << session_stats_["total_sessions"] << std::endl;
        std::cout << "   Authentication: " << (authentication_enabled_ ? "Enabled" : "Disabled") << std::endl;
        std::cout << "   Successful Logins: " << authentication_stats_["successful_logins"] << std::endl;
        std::cout << "   Failed Logins: " << authentication_stats_["failed_logins"] << std::endl;
        std::cout << "   Registered Users: " << authentication_stats_["registered_users"] << std::endl;
        std::cout << "   API Documentation: " << (api_documentation_enabled_ ? "Enabled" : "Disabled") << std::endl;
        std::cout << "   Documentation Requests: " << documentation_stats_["total_requests"] << std::endl;
        std::cout << "   Swagger UI: " << (swagger_ui_enabled_ ? "Enabled" : "Disabled") << std::endl;
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

// Security method implementations
std::string WebServer::sanitize_input(const std::string& input) {
    if (!security_enabled_) {
        return input;
    }
    
    std::string sanitized = input;
    
    // Remove null bytes
    sanitized.erase(std::remove(sanitized.begin(), sanitized.end(), '\0'), sanitized.end());
    
    // Basic XSS protection
    sanitized = encode_html_entities(sanitized);
    
    // Remove potentially dangerous characters
    std::string dangerous_chars = "<>\"'&";
    for (char c : dangerous_chars) {
        sanitized.erase(std::remove(sanitized.begin(), sanitized.end(), c), sanitized.end());
    }
    
    return sanitized;
}

std::string WebServer::encode_html_entities(const std::string& input) {
    std::string encoded = input;
    
    // Replace common HTML entities
    std::map<std::string, std::string> entities = {
        {"<", "&lt;"}, {">", "&gt;"}, {"\"", "&quot;"}, {"'", "&#39;"}, {"&", "&amp;"}
    };
    
    for (const auto& entity : entities) {
        size_t pos = 0;
        while ((pos = encoded.find(entity.first, pos)) != std::string::npos) {
            encoded.replace(pos, entity.first.length(), entity.second);
            pos += entity.second.length();
        }
    }
    
    return encoded;
}

std::string WebServer::validate_json(const std::string& json) {
    if (!security_enabled_) {
        return json;
    }
    
    std::string validated = json;
    
    // Basic JSON validation - check for balanced braces and brackets
    int brace_count = 0, bracket_count = 0;
    bool in_string = false;
    char escape_char = 0;
    
    for (size_t i = 0; i < validated.length(); ++i) {
        char c = validated[i];
        
        if (escape_char) {
            escape_char = 0;
            continue;
        }
        
        if (c == '\\') {
            escape_char = c;
            continue;
        }
        
        if (c == '"' && !escape_char) {
            in_string = !in_string;
            continue;
        }
        
        if (!in_string) {
            if (c == '{') brace_count++;
            else if (c == '}') brace_count--;
            else if (c == '[') bracket_count++;
            else if (c == ']') bracket_count--;
        }
    }
    
    if (brace_count != 0 || bracket_count != 0) {
        log_security_event("JSON_VALIDATION_FAILED", "unknown", "Unbalanced braces/brackets");
        return "";
    }
    
    return validated;
}

bool WebServer::is_sql_injection_attempt(const std::string& input) {
    if (!security_enabled_) {
        return false;
    }
    
    std::string lower_input = input;
    std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);
    
    // Common SQL injection patterns
    std::vector<std::string> sql_patterns = {
        "select", "insert", "update", "delete", "drop", "create", "alter", "union",
        "exec", "execute", "script", "javascript", "vbscript", "onload", "onerror",
        "1=1", "1=0", "or 1", "and 1", "union select", "information_schema"
    };
    
    for (const auto& pattern : sql_patterns) {
        if (lower_input.find(pattern) != std::string::npos) {
            log_security_event("SQL_INJECTION_ATTEMPT", "unknown", "Pattern detected: " + pattern);
            return true;
        }
    }
    
    return false;
}

bool WebServer::is_xss_attempt(const std::string& input) {
    if (!security_enabled_) {
        return false;
    }
    
    std::string lower_input = input;
    std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);
    
    // Common XSS patterns
    std::vector<std::string> xss_patterns = {
        "<script", "javascript:", "vbscript:", "onload=", "onerror=", "onclick=",
        "onmouseover=", "onfocus=", "onblur=", "eval(", "document.cookie",
        "window.location", "alert(", "confirm(", "prompt("
    };
    
    for (const auto& pattern : xss_patterns) {
        if (lower_input.find(pattern) != std::string::npos) {
            log_security_event("XSS_ATTEMPT", "unknown", "Pattern detected: " + pattern);
            return true;
        }
    }
    
    return false;
}

std::string WebServer::generate_csrf_token() {
    std::lock_guard<std::mutex> lock(security_mutex_);
    
    std::string token = generate_secure_random_string(32);
    std::string session_id = generate_secure_random_string(16);
    
    csrf_tokens_[session_id] = token;
    
    return token;
}

bool WebServer::validate_csrf_token(const std::string& token) {
    if (!security_enabled_) {
        return true;
    }
    
    std::lock_guard<std::mutex> lock(security_mutex_);
    
    for (const auto& pair : csrf_tokens_) {
        if (pair.second == token) {
            return true;
        }
    }
    
    log_security_event("CSRF_TOKEN_INVALID", "unknown", "Invalid CSRF token");
    return false;
}

void WebServer::add_security_headers(HttpResponse& res) {
    if (!security_enabled_) {
        return;
    }
    
    res.headers["X-Content-Type-Options"] = "nosniff";
    res.headers["X-Frame-Options"] = "SAMEORIGIN";
    res.headers["X-XSS-Protection"] = "1; mode=block";
    res.headers["Referrer-Policy"] = "strict-origin-when-cross-origin";
    res.headers["Content-Security-Policy"] = "default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'";
    res.headers["Strict-Transport-Security"] = "max-age=31536000; includeSubDomains";
}

std::string WebServer::hash_password(const std::string& password) {
    // Simple hash implementation (in production, use bcrypt or similar)
    std::hash<std::string> hasher;
    return std::to_string(hasher(password));
}

bool WebServer::verify_password(const std::string& password, const std::string& hash) {
    return hash_password(password) == hash;
}

std::string WebServer::generate_secure_random_string(size_t length) {
    static const std::string charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string result;
    result.reserve(length);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, charset.size() - 1);
    
    for (size_t i = 0; i < length; ++i) {
        result += charset[dis(gen)];
    }
    
    return result;
}

bool WebServer::is_rate_limited_by_ip(const std::string& ip) {
    if (!security_enabled_) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(security_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto it = ip_rate_limits_.find(ip);
    
    if (it != ip_rate_limits_.end()) {
        auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(now - it->second);
        if (time_diff.count() < 60) { // 1 minute rate limit
            return true;
        }
    }
    
    ip_rate_limits_[ip] = now;
    return false;
}

void WebServer::log_security_event(const std::string& event, const std::string& ip, const std::string& details) {
    if (!security_enabled_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(security_mutex_);
    
    // Update event count
    security_event_counts_[event]++;
    
    // Log to file
    std::ofstream log_file(security_log_file_, std::ios::app);
    if (log_file.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::string timestamp = std::ctime(&time_t);
        timestamp.pop_back(); // Remove newline
        
        log_file << "[" << timestamp << "] SECURITY: " << event << " from " << ip << " - " << details << std::endl;
        log_file.close();
    }
    
    std::cout << "🛡️ Security event: " << event << " from " << ip << " - " << details << std::endl;
}

HttpResponse WebServer::handle_security_status(const HttpRequest& req, HttpResponse& res) {
    res.status_code = 200;
    res.headers["Content-Type"] = "application/json";
    
    std::lock_guard<std::mutex> lock(security_mutex_);
    
    std::stringstream json;
    json << "{";
    json << "\"security_enabled\": " << (security_enabled_ ? "true" : "false") << ",";
    json << "\"blocked_ips_count\": " << blocked_ips_.size() << ",";
    json << "\"security_events\": {";
    
    bool first = true;
    for (const auto& event : security_event_counts_) {
        if (!first) json << ",";
        json << "\"" << event.first << "\": " << event.second;
        first = false;
    }
    json << "}";
    json << "}";
    
    res.body = json.str();
    return res;
}

HttpResponse WebServer::handle_csrf_token_generation(const HttpRequest& req, HttpResponse& res) {
    res.status_code = 200;
    res.headers["Content-Type"] = "application/json";
    
    std::string token = generate_csrf_token();
    
    std::stringstream json;
    json << "{\"csrf_token\": \"" << token << "\"}";
    
    res.body = json.str();
    return res;
}

// Advanced caching method implementations
std::optional<HttpResponse> WebServer::get_cached_response(const std::string& cache_key) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = response_cache_.find(cache_key);
    if (it != response_cache_.end()) {
        auto now = std::chrono::steady_clock::now();
        auto access_time = cache_access_times_[cache_key];
        
        // Check if cache entry has expired
        if (now - access_time < cache_ttl_) {
            // Update access time and move to front of LRU
            cache_access_times_[cache_key] = now;
            update_cache_access_time(cache_key);
            cache_hit_counts_[cache_key]++;
            cache_hits_++;
            total_cache_requests_++;
            update_cache_hit_ratio();
            return it->second;
        } else {
            // Remove expired entry
            response_cache_.erase(it);
            cache_access_times_.erase(cache_key);
            auto lru_it = std::find(cache_lru_order_.begin(), cache_lru_order_.end(), cache_key);
            if (lru_it != cache_lru_order_.end()) {
                cache_lru_order_.erase(lru_it);
            }
        }
    }
    
    cache_miss_counts_[cache_key]++;
    cache_misses_++;
    total_cache_requests_++;
    update_cache_hit_ratio();
    return std::nullopt;
}

void WebServer::cache_response(const std::string& cache_key, const HttpResponse& response) {
    if (!intelligent_caching_enabled_ || !should_cache_response(response)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // Check if cache is full and evict LRU entry if necessary
    if (response_cache_.size() >= max_cache_size_) {
        evict_lru_cache_entry();
    }
    
    auto now = std::chrono::steady_clock::now();
    response_cache_[cache_key] = response;
    cache_access_times_[cache_key] = now;
    
    // Add to LRU order
    auto it = std::find(cache_lru_order_.begin(), cache_lru_order_.end(), cache_key);
    if (it != cache_lru_order_.end()) {
        cache_lru_order_.erase(it);
    }
    cache_lru_order_.push_back(cache_key);
}

void WebServer::invalidate_cache(const std::string& cache_key) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    response_cache_.erase(cache_key);
    cache_access_times_.erase(cache_key);
    
    auto it = std::find(cache_lru_order_.begin(), cache_lru_order_.end(), cache_key);
    if (it != cache_lru_order_.end()) {
        cache_lru_order_.erase(it);
    }
}

void WebServer::clear_cache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    response_cache_.clear();
    cache_access_times_.clear();
    cache_lru_order_.clear();
    cache_hit_counts_.clear();
    cache_miss_counts_.clear();
    
    std::cout << "🧹 Cache cleared" << std::endl;
}

void WebServer::update_cache_access_time(const std::string& cache_key) {
    auto it = std::find(cache_lru_order_.begin(), cache_lru_order_.end(), cache_key);
    if (it != cache_lru_order_.end()) {
        cache_lru_order_.erase(it);
        cache_lru_order_.push_back(cache_key);
    }
}

void WebServer::evict_lru_cache_entry() {
    if (!cache_lru_order_.empty()) {
        std::string lru_key = cache_lru_order_.front();
        response_cache_.erase(lru_key);
        cache_access_times_.erase(lru_key);
        cache_lru_order_.erase(cache_lru_order_.begin());
    }
}

bool WebServer::should_cache_response(const HttpResponse& response) {
    // Don't cache error responses
    if (response.status_code >= 400) {
        return false;
    }
    
    // Don't cache very large responses
    if (response.body.size() > 1024 * 1024) { // 1MB limit
        return false;
    }
    
    // Check content type for cacheable responses
    auto content_type = response.headers.find("Content-Type");
    if (content_type != response.headers.end()) {
        std::string type = content_type->second;
        return type.find("text/") == 0 || 
               type.find("application/json") == 0 ||
               type.find("application/xml") == 0 ||
               type.find("image/") == 0;
    }
    
    return true;
}

std::string WebServer::generate_cache_key(const HttpRequest& req) {
    std::string key = req.method + ":" + req.path;
    
    // Include query parameters in cache key
    if (!req.query_params.empty()) {
        key += "?";
        for (const auto& param : req.query_params) {
            key += param.first + "=" + param.second + "&";
        }
        key.pop_back(); // Remove trailing &
    }
    
    // Include relevant headers that affect response
    auto accept_header = req.headers.find("Accept");
    if (accept_header != req.headers.end()) {
        key += "|Accept:" + accept_header->second;
    }
    
    return key;
}

double WebServer::get_cache_hit_ratio() {
    return cache_hit_ratio_;
}

size_t WebServer::get_cache_size() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return response_cache_.size();
}

std::map<std::string, size_t> WebServer::get_cache_hit_counts() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return cache_hit_counts_;
}

std::map<std::string, size_t> WebServer::get_cache_miss_counts() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return cache_miss_counts_;
}

void WebServer::reset_cache_stats() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    cache_hit_counts_.clear();
    cache_miss_counts_.clear();
    cache_hits_ = 0;
    cache_misses_ = 0;
    total_cache_requests_ = 0;
    cache_hit_ratio_ = 0.0;
    cache_stats_start_time_ = std::chrono::steady_clock::now();
    
    std::cout << "📊 Cache statistics reset" << std::endl;
}

void WebServer::update_cache_hit_ratio() {
    if (total_cache_requests_ > 0) {
        cache_hit_ratio_ = static_cast<double>(cache_hits_) / total_cache_requests_ * 100.0;
    }
}

HttpResponse WebServer::handle_cache_status(const HttpRequest& req, HttpResponse& res) {
    res.headers["Content-Type"] = "application/json";
    
    std::map<std::string, size_t> hit_counts = get_cache_hit_counts();
    std::map<std::string, size_t> miss_counts = get_cache_miss_counts();
    
    std::stringstream json;
    json << "{";
    json << "\"enabled\":" << (intelligent_caching_enabled_ ? "true" : "false") << ",";
    json << "\"cache_size\":" << get_cache_size() << ",";
    json << "\"max_cache_size\":" << max_cache_size_ << ",";
    json << "\"cache_ttl_seconds\":" << cache_ttl_.count() << ",";
    json << "\"total_requests\":" << total_cache_requests_ << ",";
    json << "\"cache_hits\":" << cache_hits_ << ",";
    json << "\"cache_misses\":" << cache_misses_ << ",";
    json << "\"hit_ratio\":" << std::fixed << std::setprecision(2) << get_cache_hit_ratio() << ",";
    json << "\"top_hit_endpoints\":{";
    
    // Add top hit endpoints
    std::vector<std::pair<std::string, size_t>> sorted_hits(hit_counts.begin(), hit_counts.end());
    std::sort(sorted_hits.begin(), sorted_hits.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (size_t i = 0; i < std::min(sorted_hits.size(), size_t(5)); ++i) {
        if (i > 0) json << ",";
        json << "\"" << sorted_hits[i].first << "\":" << sorted_hits[i].second;
    }
    
    json << "},";
    json << "\"top_miss_endpoints\":{";
    
    // Add top miss endpoints
    std::vector<std::pair<std::string, size_t>> sorted_misses(miss_counts.begin(), miss_counts.end());
    std::sort(sorted_misses.begin(), sorted_misses.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (size_t i = 0; i < std::min(sorted_misses.size(), size_t(5)); ++i) {
        if (i > 0) json << ",";
        json << "\"" << sorted_misses[i].first << "\":" << sorted_misses[i].second;
    }
    
    json << "}";
    json << "}";
    
    res.body = json.str();
    return res;
}

HttpResponse WebServer::handle_cache_management(const HttpRequest& req, HttpResponse& res) {
    res.headers["Content-Type"] = "application/json";
    
    if (req.method == "POST" && req.path.find("/api/cache/clear") != std::string::npos) {
        clear_cache();
        res.body = "{\"message\":\"Cache cleared successfully\",\"status\":\"success\"}";
    } else {
        res.status_code = 400;
        res.body = "{\"error\":\"Invalid cache management operation\",\"status\":\"error\"}";
    }
    
    return res;
}

// Error handling and recovery methods
void WebServer::handle_server_error(const std::exception& e, HttpResponse& res) {
    std::lock_guard<std::mutex> lock(error_mutex_);
    
    total_errors_++;
    consecutive_errors_++;
    server_healthy_ = false;
    
    std::string error_msg = "Internal Server Error: " + std::string(e.what());
    log_error("SERVER_ERROR", error_msg);
    
    res.status_code = 500;
    res.status_text = "Internal Server Error";
    res.headers["Content-Type"] = "application/json";
    res.body = "{\"error\":\"Internal Server Error\",\"message\":\"" + sanitize_input(e.what()) + "\",\"timestamp\":\"" + get_current_timestamp() + "\"}";
    
    // Auto-recovery attempt
    if (auto_recovery_enabled_ && consecutive_errors_ > 5) {
        std::cout << "⚠️ Auto-recovery triggered due to consecutive errors" << std::endl;
        recover_from_error();
    }
}

void WebServer::handle_client_error(int status_code, const std::string& message, HttpResponse& res) {
    std::lock_guard<std::mutex> lock(error_mutex_);
    
    total_errors_++;
    log_error("CLIENT_ERROR", message, "Status: " + std::to_string(status_code));
    
    res.status_code = status_code;
    res.status_text = get_status_text(status_code);
    res.headers["Content-Type"] = "application/json";
    res.body = "{\"error\":\"" + get_status_text(status_code) + "\",\"message\":\"" + sanitize_input(message) + "\",\"timestamp\":\"" + get_current_timestamp() + "\"}";
}

void WebServer::log_error(const std::string& error_type, const std::string& message, const std::string& details) {
    std::lock_guard<std::mutex> lock(error_mutex_);
    
    std::string timestamp = get_current_timestamp();
    std::string log_entry = "[" + timestamp + "] " + error_type + ": " + message;
    if (!details.empty()) {
        log_entry += " | " + details;
    }
    
    error_log_.push_back(log_entry);
    error_counts_[error_type]++;
    
    // Keep log size manageable
    if (error_log_.size() > 1000) {
        error_log_.erase(error_log_.begin(), error_log_.begin() + 500);
    }
    
    // Write to file
    std::ofstream log_file(error_log_file_, std::ios::app);
    if (log_file.is_open()) {
        log_file << log_entry << std::endl;
        log_file.close();
    }
    
    std::cout << "❌ " << log_entry << std::endl;
}

void WebServer::recover_from_error() {
    std::cout << "🔄 Attempting server recovery..." << std::endl;
    
    try {
        // Cleanup resources
        cleanup_resources();
        
        // Restart failed components
        restart_failed_components();
        
        // Perform health check
        perform_health_check();
        
        if (server_healthy_) {
            consecutive_errors_ = 0;
            std::cout << "✅ Server recovery successful" << std::endl;
        } else {
            std::cout << "❌ Server recovery failed" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "❌ Recovery attempt failed: " << e.what() << std::endl;
    }
}

bool WebServer::is_server_healthy() {
    return server_healthy_ && consecutive_errors_ < 10;
}

void WebServer::perform_health_check() {
    auto now = std::chrono::steady_clock::now();
    if (now - last_health_check_ < health_check_interval_) {
        return;
    }
    
    last_health_check_ = now;
    
    try {
        // Check basic functionality
        bool healthy = true;
        
        // Check if thread pool is responsive
        if (thread_pool_.size() == 0) {
            healthy = false;
        }
        
        // Check if cache is accessible
        {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            if (response_cache_.size() > max_cache_size_ * 2) {
                healthy = false;
            }
        }
        
        // Check if analytics are working
        if (analytics_enabled_ && total_requests_ < 0) {
            healthy = false;
        }
        
        server_healthy_ = healthy;
        
        if (!healthy) {
            std::cout << "⚠️ Health check failed" << std::endl;
        }
    } catch (const std::exception& e) {
        server_healthy_ = false;
        std::cout << "❌ Health check error: " << e.what() << std::endl;
    }
}

void WebServer::cleanup_resources() {
    std::cout << "🧹 Cleaning up resources..." << std::endl;
    
    // Clear error log if too large
    {
        std::lock_guard<std::mutex> lock(error_mutex_);
        if (error_log_.size() > 2000) {
            error_log_.clear();
        }
    }
    
    // Clear cache if too large
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        if (response_cache_.size() > max_cache_size_ * 1.5) {
            clear_cache();
        }
    }
    
    // Reset analytics if corrupted
    if (analytics_enabled_ && total_requests_ < 0) {
        total_requests_ = 0;
        total_responses_ = 0;
        total_errors_ = 0;
    }
}

void WebServer::restart_failed_components() {
    std::cout << "🔄 Restarting failed components..." << std::endl;
    
    // Reset error counters
    consecutive_errors_ = 0;
    
    // Reinitialize critical components
    try {
        // Reinitialize thread pool if needed
        if (thread_pool_.size() == 0) {
            thread_pool_.resize(4);
        }
        
        // Clear corrupted cache entries
        {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            auto it = response_cache_.begin();
            while (it != response_cache_.end()) {
                if (it->second.status_code < 200 || it->second.status_code >= 600) {
                    it = response_cache_.erase(it);
                } else {
                    ++it;
                }
            }
        }
        
        std::cout << "✅ Component restart completed" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "❌ Component restart failed: " << e.what() << std::endl;
    }
}

HttpResponse WebServer::handle_error_status(const HttpRequest& req, HttpResponse& res) {
    std::lock_guard<std::mutex> lock(error_mutex_);
    
    res.status_code = 200;
    res.status_text = "OK";
    res.headers["Content-Type"] = "application/json";
    
    std::stringstream json;
    json << "{";
    json << "\"server_healthy\":" << (server_healthy_ ? "true" : "false") << ",";
    json << "\"total_errors\":" << total_errors_ << ",";
    json << "\"consecutive_errors\":" << consecutive_errors_ << ",";
    json << "\"auto_recovery_enabled\":" << (auto_recovery_enabled_ ? "true" : "false") << ",";
    json << "\"last_health_check\":\"" << get_current_timestamp() << "\",";
    json << "\"error_counts\":{";
    
    bool first = true;
    for (const auto& pair : error_counts_) {
        if (!first) json << ",";
        json << "\"" << pair.first << "\":" << pair.second;
        first = false;
    }
    json << "},";
    
    json << "\"recent_errors\":[";
    first = true;
    for (size_t i = std::max(0UL, error_log_.size() - 10); i < error_log_.size(); ++i) {
        if (!first) json << ",";
        json << "\"" << sanitize_input(error_log_[i]) << "\"";
        first = false;
    }
    json << "]";
    json << "}";
    
    res.body = json.str();
    return res;
}

HttpResponse WebServer::handle_health_check(const HttpRequest& req, HttpResponse& res) {
    perform_health_check();
    
    res.status_code = server_healthy_ ? 200 : 503;
    res.status_text = server_healthy_ ? "OK" : "Service Unavailable";
    res.headers["Content-Type"] = "application/json";
    
    std::stringstream json;
    json << "{";
    json << "\"status\":\"" << (server_healthy_ ? "healthy" : "unhealthy") << "\",";
    json << "\"timestamp\":\"" << get_current_timestamp() << "\",";
    json << "\"uptime\":\"" << get_uptime() << "\",";
    json << "\"consecutive_errors\":" << consecutive_errors_ << ",";
    json << "\"total_errors\":" << total_errors_ << ",";
    json << "\"auto_recovery_enabled\":" << (auto_recovery_enabled_ ? "true" : "false");
    json << "}";
    
    res.body = json.str();
    return res;
}

// Content negotiation and request processing methods
std::string WebServer::negotiate_content_type(const HttpRequest& req, const std::vector<std::string>& available_types) {
    if (!content_negotiation_enabled_ || available_types.empty()) {
        return default_content_type_;
    }
    
    auto it = req.headers.find("Accept");
    if (it == req.headers.end()) {
        return available_types[0];
    }
    
    return parse_accept_header(it->second, available_types);
}

std::string WebServer::negotiate_encoding(const HttpRequest& req, const std::vector<std::string>& available_encodings) {
    if (!content_negotiation_enabled_ || available_encodings.empty()) {
        return default_encoding_;
    }
    
    auto it = req.headers.find("Accept-Encoding");
    if (it == req.headers.end()) {
        return available_encodings[0];
    }
    
    return parse_accept_header(it->second, available_encodings);
}

std::string WebServer::negotiate_language(const HttpRequest& req, const std::vector<std::string>& available_languages) {
    if (!content_negotiation_enabled_ || available_languages.empty()) {
        return default_language_;
    }
    
    auto it = req.headers.find("Accept-Language");
    if (it == req.headers.end()) {
        return available_languages[0];
    }
    
    return parse_accept_header(it->second, available_languages);
}

std::string WebServer::parse_accept_header(const std::string& accept_header, const std::vector<std::string>& available_options) {
    auto quality_map = parse_quality_values(accept_header);
    return select_best_match(quality_map, available_options);
}

std::map<std::string, double> WebServer::parse_quality_values(const std::string& header_value) {
    std::map<std::string, double> quality_map;
    std::vector<std::string> parts = split_string(header_value, ',');
    
    for (const auto& part : parts) {
        std::vector<std::string> subparts = split_string(part, ';');
        if (subparts.empty()) continue;
        
        std::string type = subparts[0];
        // Remove leading/trailing whitespace
        type.erase(0, type.find_first_not_of(" \t"));
        type.erase(type.find_last_not_of(" \t") + 1);
        
        double quality = 1.0;
        for (size_t i = 1; i < subparts.size(); ++i) {
            if (subparts[i].find("q=") == 0) {
                try {
                    quality = std::stod(subparts[i].substr(2));
                } catch (...) {
                    quality = 0.0;
                }
                break;
            }
        }
        
        quality_map[type] = quality;
    }
    
    return quality_map;
}

std::string WebServer::select_best_match(const std::map<std::string, double>& quality_map, const std::vector<std::string>& available_options) {
    std::string best_match = available_options[0];
    double best_quality = 0.0;
    
    for (const auto& option : available_options) {
        auto it = quality_map.find(option);
        if (it != quality_map.end() && it->second > best_quality) {
            best_quality = it->second;
            best_match = option;
        }
    }
    
    // Check for wildcard matches
    for (const auto& pair : quality_map) {
        if (pair.first == "*/*" || pair.first == "*") {
            if (pair.second > best_quality) {
                best_quality = pair.second;
                best_match = available_options[0];
            }
        }
    }
    
    return best_match;
}

void WebServer::add_vary_header(HttpResponse& res, const std::vector<std::string>& vary_fields) {
    if (vary_fields.empty()) return;
    
    std::string vary_value;
    for (size_t i = 0; i < vary_fields.size(); ++i) {
        if (i > 0) vary_value += ", ";
        vary_value += vary_fields[i];
    }
    
    res.headers["Vary"] = vary_value;
}

void WebServer::add_content_negotiation_headers(HttpResponse& res, const HttpRequest& req) {
    // Add Vary header for content negotiation
    std::vector<std::string> vary_fields;
    if (req.headers.find("Accept") != req.headers.end()) {
        vary_fields.push_back("Accept");
    }
    if (req.headers.find("Accept-Encoding") != req.headers.end()) {
        vary_fields.push_back("Accept-Encoding");
    }
    if (req.headers.find("Accept-Language") != req.headers.end()) {
        vary_fields.push_back("Accept-Language");
    }
    
    add_vary_header(res, vary_fields);
}

std::string WebServer::get_preferred_content_type(const HttpRequest& req) {
    std::vector<std::string> available_types = {"application/json", "application/xml", "text/html", "text/plain"};
    return negotiate_content_type(req, available_types);
}

std::string WebServer::get_preferred_encoding(const HttpRequest& req) {
    std::vector<std::string> available_encodings = {"gzip", "deflate", "identity"};
    return negotiate_encoding(req, available_encodings);
}

std::string WebServer::get_preferred_language(const HttpRequest& req) {
    std::vector<std::string> available_languages = {"en", "es", "fr", "de", "zh"};
    return negotiate_language(req, available_languages);
}

bool WebServer::supports_content_type(const HttpRequest& req, const std::string& content_type) {
    auto it = req.headers.find("Accept");
    if (it == req.headers.end()) return true;
    
    auto quality_map = parse_quality_values(it->second);
    return quality_map.find(content_type) != quality_map.end() || quality_map.find("*/*") != quality_map.end();
}

bool WebServer::supports_encoding(const HttpRequest& req, const std::string& encoding) {
    auto it = req.headers.find("Accept-Encoding");
    if (it == req.headers.end()) return true;
    
    auto quality_map = parse_quality_values(it->second);
    return quality_map.find(encoding) != quality_map.end() || quality_map.find("*") != quality_map.end();
}

bool WebServer::supports_language(const HttpRequest& req, const std::string& language) {
    auto it = req.headers.find("Accept-Language");
    if (it == req.headers.end()) return true;
    
    auto quality_map = parse_quality_values(it->second);
    return quality_map.find(language) != quality_map.end() || quality_map.find("*") != quality_map.end();
}

void WebServer::process_request_headers(const HttpRequest& req) {
    std::lock_guard<std::mutex> lock(content_negotiation_mutex_);
    
    // Extract client preferences
    std::string client_ip = extract_client_info(req);
    std::vector<std::string> preferences;
    
    if (req.headers.find("Accept") != req.headers.end()) {
        preferences.push_back("Accept: " + req.headers.at("Accept"));
    }
    if (req.headers.find("Accept-Encoding") != req.headers.end()) {
        preferences.push_back("Accept-Encoding: " + req.headers.at("Accept-Encoding"));
    }
    if (req.headers.find("Accept-Language") != req.headers.end()) {
        preferences.push_back("Accept-Language: " + req.headers.at("Accept-Language"));
    }
    
    client_preferences_[client_ip] = preferences;
    
    // Update statistics
    content_negotiation_stats_["total_requests"]++;
}

void WebServer::validate_request_headers(const HttpRequest& req) {
    // Validate required headers
    for (const auto& header : req.headers) {
        if (header.first.empty() || header.second.empty()) {
            throw std::runtime_error("Invalid header: " + header.first);
        }
    }
}

void WebServer::normalize_request_headers(HttpRequest& req) {
    // Normalize header names to lowercase
    std::map<std::string, std::string> normalized_headers;
    for (const auto& header : req.headers) {
        std::string key = header.first;
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        normalized_headers[key] = header.second;
    }
    req.headers = normalized_headers;
}

std::string WebServer::extract_client_info(const HttpRequest& req) {
    auto it = req.headers.find("x-forwarded-for");
    if (it != req.headers.end()) {
        return it->second;
    }
    
    it = req.headers.find("x-real-ip");
    if (it != req.headers.end()) {
        return it->second;
    }
    
    return "unknown";
}

void WebServer::log_request_details(const HttpRequest& req) {
    std::lock_guard<std::mutex> lock(content_negotiation_mutex_);
    
    std::string client_info = extract_client_info(req);
    std::cout << "📝 Request from " << client_info << ":" << std::endl;
    std::cout << "   Method: " << req.method << std::endl;
    std::cout << "   Path: " << req.path << std::endl;
    std::cout << "   Content-Type: " << get_preferred_content_type(req) << std::endl;
    std::cout << "   Encoding: " << get_preferred_encoding(req) << std::endl;
    std::cout << "   Language: " << get_preferred_language(req) << std::endl;
}

HttpResponse WebServer::handle_content_negotiation_test(const HttpRequest& req, HttpResponse& res) {
    process_request_headers(req);
    
    res.status_code = 200;
    res.status_text = "OK";
    
    // Determine content type based on negotiation
    std::string content_type = get_preferred_content_type(req);
    res.headers["Content-Type"] = content_type;
    
    // Add content negotiation headers
    add_content_negotiation_headers(res, req);
    
    // Create response based on negotiated content type
    if (content_type == "application/json") {
        res.body = "{\"message\":\"Content negotiation test\",\"content_type\":\"" + content_type + "\"}";
    } else if (content_type == "application/xml") {
        res.body = "<?xml version=\"1.0\"?><response><message>Content negotiation test</message><content_type>" + content_type + "</content_type></response>";
    } else if (content_type == "text/html") {
        res.body = "<html><body><h1>Content Negotiation Test</h1><p>Content-Type: " + content_type + "</p></body></html>";
    } else {
        res.body = "Content negotiation test\nContent-Type: " + content_type;
    }
    
    return res;
}

// Session management and authentication methods
std::string WebServer::create_session(const std::string& username, const std::string& user_agent) {
    std::lock_guard<std::mutex> lock(session_mutex_);
    
    // Generate unique session ID
    std::string session_id = generate_secure_random_string(32);
    
    // Create session
    Session session;
    session.username = username;
    session.user_agent = user_agent;
    session.created_at = std::chrono::steady_clock::now();
    session.last_activity = std::chrono::steady_clock::now();
    session.is_active = true;
    
    active_sessions_[session_id] = session;
    session_stats_["total_sessions"]++;
    session_stats_["active_sessions"]++;
    
    return session_id;
}

bool WebServer::validate_session(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(session_mutex_);
    
    auto it = active_sessions_.find(session_id);
    if (it == active_sessions_.end()) {
        return false;
    }
    
    Session& session = it->second;
    auto now = std::chrono::steady_clock::now();
    
    // Check if session has expired
    if (now - session.last_activity > session_timeout_) {
        active_sessions_.erase(it);
        session_stats_["active_sessions"]--;
        session_stats_["expired_sessions"]++;
        return false;
    }
    
    // Update last activity
    session.last_activity = now;
    return session.is_active;
}

void WebServer::invalidate_session(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(session_mutex_);
    
    auto it = active_sessions_.find(session_id);
    if (it != active_sessions_.end()) {
        active_sessions_.erase(it);
        session_stats_["active_sessions"]--;
        session_stats_["invalidated_sessions"]++;
    }
}

std::string WebServer::get_session_user(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(session_mutex_);
    
    auto it = active_sessions_.find(session_id);
    if (it != active_sessions_.end()) {
        return it->second.username;
    }
    return "";
}

void WebServer::update_session_activity(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(session_mutex_);
    
    auto it = active_sessions_.find(session_id);
    if (it != active_sessions_.end()) {
        it->second.last_activity = std::chrono::steady_clock::now();
    }
}

void WebServer::cleanup_expired_sessions() {
    std::lock_guard<std::mutex> lock(session_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto it = active_sessions_.begin();
    
    while (it != active_sessions_.end()) {
        if (now - it->second.last_activity > session_timeout_) {
            it = active_sessions_.erase(it);
            session_stats_["active_sessions"]--;
            session_stats_["expired_sessions"]++;
        } else {
            ++it;
        }
    }
}

std::string WebServer::generate_jwt_token(const std::string& username, const std::vector<std::string>& roles) {
    // Simple JWT-like token generation (in production, use a proper JWT library)
    std::string payload = username + "|" + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
    
    for (const auto& role : roles) {
        payload += "|" + role;
    }
    
    // Simple hash-based signature
    std::hash<std::string> hasher;
    std::string signature = std::to_string(hasher(payload + jwt_secret_));
    
    return payload + "." + signature;
}

bool WebServer::validate_jwt_token(const std::string& token, std::string& username) {
    size_t dot_pos = token.find('.');
    if (dot_pos == std::string::npos) {
        return false;
    }
    
    std::string payload = token.substr(0, dot_pos);
    std::string signature = token.substr(dot_pos + 1);
    
    // Verify signature
    std::hash<std::string> hasher;
    std::string expected_signature = std::to_string(hasher(payload + jwt_secret_));
    
    if (signature != expected_signature) {
        return false;
    }
    
    // Extract username
    size_t pipe_pos = payload.find('|');
    if (pipe_pos == std::string::npos) {
        return false;
    }
    
    username = payload.substr(0, pipe_pos);
    
    // Check expiration
    size_t timestamp_pos = payload.find('|', pipe_pos + 1);
    if (timestamp_pos == std::string::npos) {
        return false;
    }
    
    std::string timestamp_str = payload.substr(pipe_pos + 1, timestamp_pos - pipe_pos - 1);
    auto token_time = std::chrono::seconds(std::stoll(timestamp_str));
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now().time_since_epoch());
    
    if (now - token_time > token_expiry_) {
        return false;
    }
    
    return true;
}

std::string WebServer::hash_password(const std::string& password) {
    // Simple hash function (in production, use bcrypt or similar)
    std::hash<std::string> hasher;
    return std::to_string(hasher(password + "salt"));
}

bool WebServer::verify_password(const std::string& password, const std::string& hash) {
    return hash_password(password) == hash;
}

bool WebServer::authenticate_user(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(user_mutex_);
    
    // Check if user is locked out
    if (is_user_locked_out(username)) {
        authentication_stats_["locked_out_attempts"]++;
        return false;
    }
    
    auto it = registered_users_.find(username);
    if (it == registered_users_.end()) {
        record_failed_login(username);
        return false;
    }
    
    if (verify_password(password, it->second.password_hash)) {
        reset_failed_attempts(username);
        it->second.last_login = std::chrono::steady_clock::now();
        authentication_stats_["successful_logins"]++;
        return true;
    } else {
        record_failed_login(username);
        return false;
    }
}

bool WebServer::register_user(const std::string& username, const std::string& password, const std::vector<std::string>& roles) {
    std::lock_guard<std::mutex> lock(user_mutex_);
    
    if (registered_users_.find(username) != registered_users_.end()) {
        return false;
    }
    
    User user;
    user.username = username;
    user.password_hash = hash_password(password);
    user.roles = roles;
    user.created_at = std::chrono::steady_clock::now();
    user.failed_attempts = 0;
    user.is_active = true;
    
    registered_users_[username] = user;
    user_roles_[username] = roles;
    
    for (const auto& role : roles) {
        if (role_permissions_.find(role) == role_permissions_.end()) {
            role_permissions_[role] = {};
        }
    }
    
    authentication_stats_["registered_users"]++;
    return true;
}

bool WebServer::has_permission(const std::string& username, const std::string& permission) {
    std::lock_guard<std::mutex> lock(user_mutex_);
    
    auto roles_it = user_roles_.find(username);
    if (roles_it == user_roles_.end()) {
        return false;
    }
    
    for (const auto& role : roles_it->second) {
        auto perms_it = role_permissions_.find(role);
        if (perms_it != role_permissions_.end()) {
            for (const auto& perm : perms_it->second) {
                if (perm == permission) {
                    return true;
                }
            }
        }
    }
    
    return false;
}

std::vector<std::string> WebServer::get_user_roles(const std::string& username) {
    std::lock_guard<std::mutex> lock(user_mutex_);
    
    auto it = user_roles_.find(username);
    if (it != user_roles_.end()) {
        return it->second;
    }
    return {};
}

std::vector<std::string> WebServer::get_role_permissions(const std::string& role) {
    std::lock_guard<std::mutex> lock(user_mutex_);
    
    auto it = role_permissions_.find(role);
    if (it != role_permissions_.end()) {
        return it->second;
    }
    return {};
}

void WebServer::add_role_permission(const std::string& role, const std::string& permission) {
    std::lock_guard<std::mutex> lock(user_mutex_);
    role_permissions_[role].push_back(permission);
}

void WebServer::remove_role_permission(const std::string& role, const std::string& permission) {
    std::lock_guard<std::mutex> lock(user_mutex_);
    
    auto it = role_permissions_.find(role);
    if (it != role_permissions_.end()) {
        auto& permissions = it->second;
        permissions.erase(std::remove(permissions.begin(), permissions.end(), permission), permissions.end());
    }
}

bool WebServer::is_user_locked_out(const std::string& username) {
    auto it = registered_users_.find(username);
    if (it == registered_users_.end()) {
        return false;
    }
    
    auto now = std::chrono::steady_clock::now();
    return it->second.failed_attempts >= max_failed_attempts_ && 
           now < it->second.lockout_until;
}

void WebServer::record_failed_login(const std::string& username) {
    auto it = registered_users_.find(username);
    if (it != registered_users_.end()) {
        it->second.failed_attempts++;
        if (it->second.failed_attempts >= max_failed_attempts_) {
            it->second.lockout_until = std::chrono::steady_clock::now() + lockout_duration_;
        }
        authentication_stats_["failed_logins"]++;
    }
}

void WebServer::reset_failed_attempts(const std::string& username) {
    auto it = registered_users_.find(username);
    if (it != registered_users_.end()) {
        it->second.failed_attempts = 0;
    }
}

std::string WebServer::extract_session_id(const HttpRequest& req) {
    auto it = req.headers.find("cookie");
    if (it == req.headers.end()) {
        return "";
    }
    
    std::string cookies = it->second;
    std::string session_cookie = session_cookie_name_ + "=";
    size_t pos = cookies.find(session_cookie);
    
    if (pos != std::string::npos) {
        pos += session_cookie.length();
        size_t end_pos = cookies.find(';', pos);
        if (end_pos == std::string::npos) {
            end_pos = cookies.length();
        }
        return cookies.substr(pos, end_pos - pos);
    }
    
    return "";
}

void WebServer::add_session_cookie(HttpResponse& res, const std::string& session_id) {
    res.headers["Set-Cookie"] = session_cookie_name_ + "=" + session_id + 
                                "; HttpOnly; Path=/; Max-Age=" + std::to_string(session_timeout_.count());
}

void WebServer::add_auth_cookie(HttpResponse& res, const std::string& token) {
    res.headers["Set-Cookie"] = auth_cookie_name_ + "=" + token + 
                                "; HttpOnly; Path=/; Max-Age=" + std::to_string(token_expiry_.count());
}

HttpResponse WebServer::handle_login(const HttpRequest& req, HttpResponse& res) {
    if (req.method != "POST") {
        return handle_client_error(405, "Method not allowed", res);
    }
    
    // Parse JSON body
    std::string username, password;
    try {
        // Simple JSON parsing (in production, use a proper JSON library)
        size_t user_pos = req.body.find("\"username\":\"");
        size_t pass_pos = req.body.find("\"password\":\"");
        
        if (user_pos != std::string::npos && pass_pos != std::string::npos) {
            user_pos += 12;
            size_t user_end = req.body.find("\"", user_pos);
            username = req.body.substr(user_pos, user_end - user_pos);
            
            pass_pos += 12;
            size_t pass_end = req.body.find("\"", pass_pos);
            password = req.body.substr(pass_pos, pass_end - pass_pos);
        }
    } catch (...) {
        return handle_client_error(400, "Invalid JSON format", res);
    }
    
    if (username.empty() || password.empty()) {
        return handle_client_error(400, "Username and password required", res);
    }
    
    if (authenticate_user(username, password)) {
        std::string session_id = create_session(username, req.headers["user-agent"]);
        std::vector<std::string> roles = get_user_roles(username);
        std::string token = generate_jwt_token(username, roles);
        
        res.status_code = 200;
        res.status_text = "OK";
        res.headers["Content-Type"] = "application/json";
        add_session_cookie(res, session_id);
        add_auth_cookie(res, token);
        
        res.body = "{\"success\":true,\"message\":\"Login successful\",\"username\":\"" + username + "\"}";
    } else {
        res.status_code = 401;
        res.status_text = "Unauthorized";
        res.headers["Content-Type"] = "application/json";
        res.body = "{\"success\":false,\"message\":\"Invalid credentials\"}";
    }
    
    return res;
}

HttpResponse WebServer::handle_logout(const HttpRequest& req, HttpResponse& res) {
    std::string session_id = extract_session_id(req);
    if (!session_id.empty()) {
        invalidate_session(session_id);
    }
    
    res.status_code = 200;
    res.status_text = "OK";
    res.headers["Content-Type"] = "application/json";
    res.headers["Set-Cookie"] = session_cookie_name_ + "=; HttpOnly; Path=/; Max-Age=0";
    res.headers["Set-Cookie"] = auth_cookie_name_ + "=; HttpOnly; Path=/; Max-Age=0";
    res.body = "{\"success\":true,\"message\":\"Logout successful\"}";
    
    return res;
}

HttpResponse WebServer::handle_register(const HttpRequest& req, HttpResponse& res) {
    if (req.method != "POST") {
        return handle_client_error(405, "Method not allowed", res);
    }
    
    // Parse JSON body
    std::string username, password;
    try {
        size_t user_pos = req.body.find("\"username\":\"");
        size_t pass_pos = req.body.find("\"password\":\"");
        
        if (user_pos != std::string::npos && pass_pos != std::string::npos) {
            user_pos += 12;
            size_t user_end = req.body.find("\"", user_pos);
            username = req.body.substr(user_pos, user_end - user_pos);
            
            pass_pos += 12;
            size_t pass_end = req.body.find("\"", pass_pos);
            password = req.body.substr(pass_pos, pass_end - pass_pos);
        }
    } catch (...) {
        return handle_client_error(400, "Invalid JSON format", res);
    }
    
    if (username.empty() || password.empty()) {
        return handle_client_error(400, "Username and password required", res);
    }
    
    if (register_user(username, password, {"user"})) {
        res.status_code = 201;
        res.status_text = "Created";
        res.headers["Content-Type"] = "application/json";
        res.body = "{\"success\":true,\"message\":\"User registered successfully\"}";
    } else {
        res.status_code = 409;
        res.status_text = "Conflict";
        res.headers["Content-Type"] = "application/json";
        res.body = "{\"success\":false,\"message\":\"Username already exists\"}";
    }
    
    return res;
}

HttpResponse WebServer::handle_session_status(const HttpRequest& req, HttpResponse& res) {
    std::string session_id = extract_session_id(req);
    
    res.status_code = 200;
    res.status_text = "OK";
    res.headers["Content-Type"] = "application/json";
    
    if (!session_id.empty() && validate_session(session_id)) {
        std::string username = get_session_user(session_id);
        std::vector<std::string> roles = get_user_roles(username);
        
        res.body = "{\"authenticated\":true,\"username\":\"" + username + "\",\"roles\":" + 
                   "[\"" + (roles.empty() ? "" : roles[0]) + "\"]}";
    } else {
        res.body = "{\"authenticated\":false}";
    }
    
    return res;
}

HttpResponse WebServer::handle_user_profile(const HttpRequest& req, HttpResponse& res) {
    std::string session_id = extract_session_id(req);
    if (session_id.empty() || !validate_session(session_id)) {
        return handle_client_error(401, "Authentication required", res);
    }
    
    std::string username = get_session_user(session_id);
    std::vector<std::string> roles = get_user_roles(username);
    
    res.status_code = 200;
    res.status_text = "OK";
    res.headers["Content-Type"] = "application/json";
    
    std::string roles_json = "[";
    for (size_t i = 0; i < roles.size(); ++i) {
        if (i > 0) roles_json += ",";
        roles_json += "\"" + roles[i] + "\"";
    }
    roles_json += "]";
    
    res.body = "{\"username\":\"" + username + "\",\"roles\":" + roles_json + "}";
    
    return res;
}

HttpResponse WebServer::handle_change_password(const HttpRequest& req, HttpResponse& res) {
    if (req.method != "POST") {
        return handle_client_error(405, "Method not allowed", res);
    }
    
    std::string session_id = extract_session_id(req);
    if (session_id.empty() || !validate_session(session_id)) {
        return handle_client_error(401, "Authentication required", res);
    }
    
    std::string username = get_session_user(session_id);
    
    // Parse JSON body
    std::string old_password, new_password;
    try {
        size_t old_pos = req.body.find("\"old_password\":\"");
        size_t new_pos = req.body.find("\"new_password\":\"");
        
        if (old_pos != std::string::npos && new_pos != std::string::npos) {
            old_pos += 16;
            size_t old_end = req.body.find("\"", old_pos);
            old_password = req.body.substr(old_pos, old_end - old_pos);
            
            new_pos += 16;
            size_t new_end = req.body.find("\"", new_pos);
            new_password = req.body.substr(new_pos, new_end - new_pos);
        }
    } catch (...) {
        return handle_client_error(400, "Invalid JSON format", res);
    }
    
    if (old_password.empty() || new_password.empty()) {
        return handle_client_error(400, "Old and new passwords required", res);
    }
    
    std::lock_guard<std::mutex> lock(user_mutex_);
    auto it = registered_users_.find(username);
    if (it == registered_users_.end()) {
        return handle_client_error(404, "User not found", res);
    }
    
    if (!verify_password(old_password, it->second.password_hash)) {
        return handle_client_error(400, "Invalid old password", res);
    }
    
    it->second.password_hash = hash_password(new_password);
    
    res.status_code = 200;
    res.status_text = "OK";
    res.headers["Content-Type"] = "application/json";
    res.body = "{\"success\":true,\"message\":\"Password changed successfully\"}";
    
    return res;
}

HttpResponse WebServer::require_authentication(const HttpRequest& req, HttpResponse& res, 
                                              const std::function<HttpResponse(const HttpRequest&, HttpResponse&)>& handler) {
    std::string session_id = extract_session_id(req);
    
    if (session_id.empty() || !validate_session(session_id)) {
        return handle_client_error(401, "Authentication required", res);
    }
    
    return handler(req, res);
}

HttpResponse WebServer::require_permission(const HttpRequest& req, HttpResponse& res, 
                                          const std::string& permission,
                                          const std::function<HttpResponse(const HttpRequest&, HttpResponse&)>& handler) {
    std::string session_id = extract_session_id(req);
    
    if (session_id.empty() || !validate_session(session_id)) {
        return handle_client_error(401, "Authentication required", res);
    }
    
    std::string username = get_session_user(session_id);
    if (!has_permission(username, permission)) {
        return handle_client_error(403, "Insufficient permissions", res);
    }
    
    return handler(req, res);
}

// API Documentation and OpenAPI/Swagger Methods

void WebServer::enable_api_documentation(bool enabled) {
    std::lock_guard<std::mutex> lock(documentation_mutex_);
    api_documentation_enabled_ = enabled;
    std::cout << "📚 API Documentation " << (enabled ? "enabled" : "disabled") << std::endl;
}

void WebServer::enable_swagger_ui(bool enabled) {
    std::lock_guard<std::mutex> lock(documentation_mutex_);
    swagger_ui_enabled_ = enabled;
    std::cout << "🔍 Swagger UI " << (enabled ? "enabled" : "disabled") << std::endl;
}

void WebServer::set_api_info(const std::string& title, const std::string& description, const std::string& version) {
    std::lock_guard<std::mutex> lock(documentation_mutex_);
    api_title_ = title;
    api_description_ = description;
    api_version_ = version;
    std::cout << "📝 API Info updated: " << title << " v" << version << std::endl;
}

void WebServer::set_api_contact(const std::string& email) {
    std::lock_guard<std::mutex> lock(documentation_mutex_);
    api_contact_email_ = email;
}

void WebServer::set_api_license(const std::string& name, const std::string& url) {
    std::lock_guard<std::mutex> lock(documentation_mutex_);
    api_license_name_ = name;
    api_license_url_ = url;
}

void WebServer::add_endpoint_documentation(const std::string& endpoint, const std::string& method, 
                                         const std::string& summary, const std::string& description, 
                                         const std::string& tag) {
    std::lock_guard<std::mutex> lock(documentation_mutex_);
    std::string key = method + ":" + endpoint;
    endpoint_summaries_[key] = summary;
    endpoint_descriptions_[key] = description;
    endpoint_tags_[key] = tag;
}

void WebServer::add_endpoint_parameter(const std::string& endpoint, const std::string& method,
                                     const std::string& name, const std::string& type, 
                                     const std::string& description, bool required) {
    std::lock_guard<std::mutex> lock(documentation_mutex_);
    std::string key = method + ":" + endpoint;
    endpoint_parameters_[key][name] = type + "|" + description + "|" + (required ? "required" : "optional");
}

void WebServer::add_endpoint_response(const std::string& endpoint, const std::string& method,
                                    int status_code, const std::string& description) {
    std::lock_guard<std::mutex> lock(documentation_mutex_);
    std::string key = method + ":" + endpoint;
    endpoint_responses_[key][status_code] = description;
}

void WebServer::add_endpoint_example(const std::string& endpoint, const std::string& method,
                                   const std::string& example) {
    std::lock_guard<std::mutex> lock(documentation_mutex_);
    std::string key = method + ":" + endpoint;
    endpoint_examples_[key] = example;
}

void WebServer::add_endpoint_schema(const std::string& endpoint, const std::string& method,
                                  const std::string& field, const std::string& type, 
                                  const std::string& description) {
    std::lock_guard<std::mutex> lock(documentation_mutex_);
    std::string key = method + ":" + endpoint;
    endpoint_schemas_[key][field] = type + "|" + description;
}

void WebServer::add_required_field(const std::string& endpoint, const std::string& method,
                                 const std::string& field) {
    std::lock_guard<std::mutex> lock(documentation_mutex_);
    std::string key = method + ":" + endpoint;
    endpoint_required_fields_[key].push_back(field);
}

std::string WebServer::generate_openapi_spec() {
    std::lock_guard<std::mutex> lock(documentation_mutex_);
    
    std::string spec = "{\n";
    spec += "  \"openapi\": \"3.0.0\",\n";
    spec += "  \"info\": {\n";
    spec += "    \"title\": \"" + api_title_ + "\",\n";
    spec += "    \"description\": \"" + api_description_ + "\",\n";
    spec += "    \"version\": \"" + api_version_ + "\",\n";
    spec += "    \"contact\": {\n";
    spec += "      \"email\": \"" + api_contact_email_ + "\"\n";
    spec += "    },\n";
    spec += "    \"license\": {\n";
    spec += "      \"name\": \"" + api_license_name_ + "\",\n";
    spec += "      \"url\": \"" + api_license_url_ + "\"\n";
    spec += "    }\n";
    spec += "  },\n";
    spec += "  \"servers\": [\n";
    spec += "    {\n";
    spec += "      \"url\": \"http://" + host_ + ":" + std::to_string(port_) + "\",\n";
    spec += "      \"description\": \"Development server\"\n";
    spec += "    }\n";
    spec += "  ],\n";
    spec += "  \"paths\": {\n";
    
    // Add paths from registered routes
    for (const auto& route : routes_) {
        std::string method_path = route.first;
        size_t colon_pos = method_path.find(':');
        if (colon_pos != std::string::npos) {
            std::string method = method_path.substr(0, colon_pos);
            std::string path = method_path.substr(colon_pos + 1);
            
            spec += "    \"" + path + "\": {\n";
            spec += "      \"" + method + "\": {\n";
            
            std::string key = method + ":" + path;
            if (endpoint_summaries_.find(key) != endpoint_summaries_.end()) {
                spec += "        \"summary\": \"" + endpoint_summaries_[key] + "\",\n";
            }
            if (endpoint_descriptions_.find(key) != endpoint_descriptions_.end()) {
                spec += "        \"description\": \"" + endpoint_descriptions_[key] + "\",\n";
            }
            if (endpoint_tags_.find(key) != endpoint_tags_.end()) {
                spec += "        \"tags\": [\"" + endpoint_tags_[key] + "\"],\n";
            }
            
            // Add parameters
            if (endpoint_parameters_.find(key) != endpoint_parameters_.end()) {
                spec += "        \"parameters\": [\n";
                for (const auto& param : endpoint_parameters_[key]) {
                    std::vector<std::string> parts = split_string(param.second, '|');
                    if (parts.size() >= 2) {
                        spec += "          {\n";
                        spec += "            \"name\": \"" + param.first + "\",\n";
                        spec += "            \"in\": \"query\",\n";
                        spec += "            \"schema\": {\n";
                        spec += "              \"type\": \"" + parts[0] + "\"\n";
                        spec += "            },\n";
                        spec += "            \"description\": \"" + parts[1] + "\",\n";
                        spec += "            \"required\": " + (parts.size() > 2 && parts[2] == "required" ? "true" : "false") + "\n";
                        spec += "          }";
                        if (&param != &endpoint_parameters_[key].rbegin().operator*()) {
                            spec += ",";
                        }
                        spec += "\n";
                    }
                }
                spec += "        ],\n";
            }
            
            // Add responses
            spec += "        \"responses\": {\n";
            if (endpoint_responses_.find(key) != endpoint_responses_.end()) {
                for (const auto& response : endpoint_responses_[key]) {
                    spec += "          \"" + std::to_string(response.first) + "\": {\n";
                    spec += "            \"description\": \"" + response.second + "\"\n";
                    spec += "          }";
                    if (&response != &endpoint_responses_[key].rbegin().operator*()) {
                        spec += ",";
                    }
                    spec += "\n";
                }
            } else {
                spec += "          \"200\": {\n";
                spec += "            \"description\": \"Successful operation\"\n";
                spec += "          }\n";
            }
            spec += "        }\n";
            spec += "      }\n";
            spec += "    }";
            if (&route != &routes_.rbegin().operator*()) {
                spec += ",";
            }
            spec += "\n";
        }
    }
    
    spec += "  }\n";
    spec += "}\n";
    
    return spec;
}

std::string WebServer::generate_swagger_ui_html() {
    std::string html = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>)" + api_title_ + R"( - API Documentation</title>
    <link rel="stylesheet" type="text/css" href="https://unpkg.com/swagger-ui-dist@4.15.5/swagger-ui.css" />
    <style>
        html { box-sizing: border-box; overflow: -moz-scrollbars-vertical; overflow-y: scroll; }
        *, *:before, *:after { box-sizing: inherit; }
        body { margin:0; background: #fafafa; }
    </style>
</head>
<body>
    <div id="swagger-ui"></div>
    <script src="https://unpkg.com/swagger-ui-dist@4.15.5/swagger-ui-bundle.js"></script>
    <script src="https://unpkg.com/swagger-ui-dist@4.15.5/swagger-ui-standalone-preset.js"></script>
    <script>
        window.onload = function() {
            const ui = SwaggerUIBundle({
                url: '/api/docs/openapi.json',
                dom_id: '#swagger-ui',
                deepLinking: true,
                presets: [
                    SwaggerUIBundle.presets.apis,
                    SwaggerUIStandalonePreset
                ],
                plugins: [
                    SwaggerUIBundle.plugins.DownloadUrl
                ],
                layout: "StandaloneLayout"
            });
        };
    </script>
</body>
</html>
)";
    return html;
}

std::string WebServer::generate_api_documentation_html() {
    std::string html = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>)" + api_title_ + R"( - API Documentation</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background: #f5f5f5; }
        .container { max-width: 1200px; margin: 0 auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #333; border-bottom: 3px solid #007bff; padding-bottom: 10px; }
        h2 { color: #555; margin-top: 30px; }
        .endpoint { background: #f8f9fa; border: 1px solid #dee2e6; border-radius: 5px; padding: 20px; margin: 15px 0; }
        .method { display: inline-block; padding: 5px 10px; border-radius: 3px; color: white; font-weight: bold; margin-right: 10px; }
        .get { background: #28a745; }
        .post { background: #007bff; }
        .put { background: #ffc107; color: #212529; }
        .delete { background: #dc3545; }
        .path { font-family: monospace; font-size: 16px; color: #333; }
        .description { margin: 10px 0; color: #666; }
        .parameters { margin: 15px 0; }
        .parameter { background: #e9ecef; padding: 10px; border-radius: 3px; margin: 5px 0; }
        .responses { margin: 15px 0; }
        .response { background: #d4edda; padding: 8px; border-radius: 3px; margin: 3px 0; }
        .example { background: #f8f9fa; border: 1px solid #dee2e6; border-radius: 3px; padding: 15px; margin: 10px 0; font-family: monospace; }
        .nav { background: #007bff; padding: 15px; margin-bottom: 20px; border-radius: 5px; }
        .nav a { color: white; text-decoration: none; margin-right: 20px; }
        .nav a:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <div class="container">
        <div class="nav">
            <a href="/api/docs/swagger">Swagger UI</a>
            <a href="/api/docs/openapi.json">OpenAPI JSON</a>
            <a href="/api/docs/markdown">Markdown</a>
            <a href="/api/docs/postman">Postman Collection</a>
        </div>
        
        <h1>)" + api_title_ + R"(</h1>
        <p>)" + api_description_ + R"(</p>
        <p><strong>Version:</strong> )" + api_version_ + R"(</p>
        <p><strong>Contact:</strong> <a href="mailto:)" + api_contact_email_ + R"(">)" + api_contact_email_ + R"(</a></p>
        
        <h2>API Endpoints</h2>
)";
    
    // Group endpoints by tag
    std::map<std::string, std::vector<std::string>> tagged_endpoints;
    for (const auto& route : routes_) {
        std::string method_path = route.first;
        size_t colon_pos = method_path.find(':');
        if (colon_pos != std::string::npos) {
            std::string method = method_path.substr(0, colon_pos);
            std::string path = method_path.substr(colon_pos + 1);
            std::string key = method + ":" + path;
            
            std::string tag = "General";
            if (endpoint_tags_.find(key) != endpoint_tags_.end()) {
                tag = endpoint_tags_[key];
            }
            tagged_endpoints[tag].push_back(key);
        }
    }
    
    for (const auto& tag_group : tagged_endpoints) {
        html += "        <h3>" + tag_group.first + "</h3>\n";
        
        for (const auto& endpoint_key : tag_group.second) {
            size_t colon_pos = endpoint_key.find(':');
            std::string method = endpoint_key.substr(0, colon_pos);
            std::string path = endpoint_key.substr(colon_pos + 1);
            
            html += "        <div class=\"endpoint\">\n";
            html += "            <span class=\"method " + method + "\">" + method + "</span>\n";
            html += "            <span class=\"path\">" + path + "</span>\n";
            
            if (endpoint_summaries_.find(endpoint_key) != endpoint_summaries_.end()) {
                html += "            <div class=\"description\"><strong>" + endpoint_summaries_[endpoint_key] + "</strong></div>\n";
            }
            if (endpoint_descriptions_.find(endpoint_key) != endpoint_descriptions_.end()) {
                html += "            <div class=\"description\">" + endpoint_descriptions_[endpoint_key] + "</div>\n";
            }
            
            // Add parameters
            if (endpoint_parameters_.find(endpoint_key) != endpoint_parameters_.end()) {
                html += "            <div class=\"parameters\">\n";
                html += "                <h4>Parameters:</h4>\n";
                for (const auto& param : endpoint_parameters_[endpoint_key]) {
                    std::vector<std::string> parts = split_string(param.second, '|');
                    if (parts.size() >= 2) {
                        html += "                <div class=\"parameter\">\n";
                        html += "                    <strong>" + param.first + "</strong> (" + parts[0] + ") - " + parts[1] + "\n";
                        html += "                </div>\n";
                    }
                }
                html += "            </div>\n";
            }
            
            // Add responses
            if (endpoint_responses_.find(endpoint_key) != endpoint_responses_.end()) {
                html += "            <div class=\"responses\">\n";
                html += "                <h4>Responses:</h4>\n";
                for (const auto& response : endpoint_responses_[endpoint_key]) {
                    html += "                <div class=\"response\">\n";
                    html += "                    <strong>" + std::to_string(response.first) + "</strong> - " + response.second + "\n";
                    html += "                </div>\n";
                }
                html += "            </div>\n";
            }
            
            // Add examples
            if (endpoint_examples_.find(endpoint_key) != endpoint_examples_.end()) {
                html += "            <div class=\"example\">\n";
                html += "                <h4>Example:</h4>\n";
                html += "                <pre>" + endpoint_examples_[endpoint_key] + "</pre>\n";
                html += "            </div>\n";
            }
            
            html += "        </div>\n";
        }
    }
    
    html += R"(
    </div>
</body>
</html>
)";
    
    return html;
}

HttpResponse WebServer::handle_openapi_spec(const HttpRequest& req, HttpResponse& res) {
    std::lock_guard<std::mutex> lock(documentation_mutex_);
    documentation_stats_["total_requests"]++;
    
    res.status_code = 200;
    res.headers["Content-Type"] = "application/json";
    res.headers["Cache-Control"] = "public, max-age=3600";
    res.body = generate_openapi_spec();
    
    return res;
}

HttpResponse WebServer::handle_swagger_ui(const HttpRequest& req, HttpResponse& res) {
    std::lock_guard<std::mutex> lock(documentation_mutex_);
    documentation_stats_["total_requests"]++;
    
    res.status_code = 200;
    res.headers["Content-Type"] = "text/html";
    res.headers["Cache-Control"] = "public, max-age=3600";
    res.body = generate_swagger_ui_html();
    
    return res;
}

HttpResponse WebServer::handle_api_docs(const HttpRequest& req, HttpResponse& res) {
    std::lock_guard<std::mutex> lock(documentation_mutex_);
    documentation_stats_["total_requests"]++;
    
    res.status_code = 200;
    res.headers["Content-Type"] = "text/html";
    res.headers["Cache-Control"] = "public, max-age=3600";
    res.body = generate_api_documentation_html();
    
    return res;
}

HttpResponse WebServer::handle_endpoint_docs(const HttpRequest& req, HttpResponse& res) {
    std::lock_guard<std::mutex> lock(documentation_mutex_);
    documentation_stats_["total_requests"]++;
    
    res.status_code = 200;
    res.headers["Content-Type"] = "application/json";
    
    std::string json = "{\n";
    json += "  \"endpoints\": [\n";
    
    for (const auto& route : routes_) {
        std::string method_path = route.first;
        size_t colon_pos = method_path.find(':');
        if (colon_pos != std::string::npos) {
            std::string method = method_path.substr(0, colon_pos);
            std::string path = method_path.substr(colon_pos + 1);
            
            json += "    {\n";
            json += "      \"method\": \"" + method + "\",\n";
            json += "      \"path\": \"" + path + "\",\n";
            
            std::string key = method + ":" + path;
            if (endpoint_summaries_.find(key) != endpoint_summaries_.end()) {
                json += "      \"summary\": \"" + endpoint_summaries_[key] + "\",\n";
            }
            if (endpoint_descriptions_.find(key) != endpoint_descriptions_.end()) {
                json += "      \"description\": \"" + endpoint_descriptions_[key] + "\",\n";
            }
            if (endpoint_tags_.find(key) != endpoint_tags_.end()) {
                json += "      \"tag\": \"" + endpoint_tags_[key] + "\"\n";
            }
            
            json += "    }";
            if (&route != &routes_.rbegin().operator*()) {
                json += ",";
            }
            json += "\n";
        }
    }
    
    json += "  ]\n";
    json += "}\n";
    
    res.body = json;
    return res;
}

void WebServer::initialize_api_documentation() {
    std::lock_guard<std::mutex> lock(documentation_mutex_);
    
    // Initialize documentation statistics
    documentation_stats_["total_requests"] = 0;
    documentation_stats_["openapi_requests"] = 0;
    documentation_stats_["swagger_requests"] = 0;
    documentation_stats_["html_requests"] = 0;
    
    std::cout << "📚 API Documentation system initialized" << std::endl;
}

void WebServer::register_endpoint_documentation() {
    // Register documentation for existing endpoints
    add_endpoint_documentation("/", "GET", "Dashboard", "Main dashboard page", "UI");
    add_endpoint_documentation("/api/status", "GET", "System Status", "Get system status and health information", "System");
    add_endpoint_documentation("/api/jobs", "GET", "List Jobs", "Get list of all jobs", "Jobs");
    add_endpoint_documentation("/api/jobs", "POST", "Submit Job", "Submit a new job", "Jobs");
    add_endpoint_documentation("/api/jobs/{id}", "GET", "Get Job Status", "Get status of a specific job", "Jobs");
    add_endpoint_documentation("/api/hdfs/list", "GET", "List HDFS Files", "List files in HDFS", "Storage");
    add_endpoint_documentation("/api/hdfs/upload", "POST", "Upload File", "Upload file to HDFS", "Storage");
    add_endpoint_documentation("/api/hdfs/download", "GET", "Download File", "Download file from HDFS", "Storage");
    add_endpoint_documentation("/api/cluster/info", "GET", "Cluster Info", "Get cluster information", "System");
    
    // Add parameters
    add_endpoint_parameter("/api/jobs/{id}", "GET", "id", "string", "Job ID", true);
    add_endpoint_parameter("/api/hdfs/download", "GET", "path", "string", "File path in HDFS", true);
    
    // Add responses
    add_endpoint_response("/api/status", "GET", 200, "System status retrieved successfully");
    add_endpoint_response("/api/status", "GET", 500, "Internal server error");
    add_endpoint_response("/api/jobs", "GET", 200, "Jobs list retrieved successfully");
    add_endpoint_response("/api/jobs", "POST", 201, "Job submitted successfully");
    add_endpoint_response("/api/jobs", "POST", 400, "Invalid job data");
    
    std::cout << "📝 Endpoint documentation registered" << std::endl;
}

std::string WebServer::format_json_schema(const std::map<std::string, std::string>& schema) {
    std::string json = "{\n";
    json += "  \"type\": \"object\",\n";
    json += "  \"properties\": {\n";
    
    for (const auto& field : schema) {
        json += "    \"" + field.first + "\": {\n";
        json += "      \"type\": \"" + field.second + "\"\n";
        json += "    }";
        if (&field != &schema.rbegin().operator*()) {
            json += ",";
        }
        json += "\n";
    }
    
    json += "  }\n";
    json += "}\n";
    
    return json;
}

std::string WebServer::generate_markdown_documentation() {
    std::string markdown = "# " + api_title_ + "\n\n";
    markdown += api_description_ + "\n\n";
    markdown += "**Version:** " + api_version_ + "\n\n";
    markdown += "**Contact:** " + api_contact_email_ + "\n\n";
    markdown += "## Endpoints\n\n";
    
    for (const auto& route : routes_) {
        std::string method_path = route.first;
        size_t colon_pos = method_path.find(':');
        if (colon_pos != std::string::npos) {
            std::string method = method_path.substr(0, colon_pos);
            std::string path = method_path.substr(colon_pos + 1);
            
            markdown += "### " + method + " " + path + "\n\n";
            
            std::string key = method + ":" + path;
            if (endpoint_summaries_.find(key) != endpoint_summaries_.end()) {
                markdown += "**Summary:** " + endpoint_summaries_[key] + "\n\n";
            }
            if (endpoint_descriptions_.find(key) != endpoint_descriptions_.end()) {
                markdown += endpoint_descriptions_[key] + "\n\n";
            }
            
            if (endpoint_parameters_.find(key) != endpoint_parameters_.end()) {
                markdown += "**Parameters:**\n\n";
                for (const auto& param : endpoint_parameters_[key]) {
                    std::vector<std::string> parts = split_string(param.second, '|');
                    if (parts.size() >= 2) {
                        markdown += "- `" + param.first + "` (" + parts[0] + ") - " + parts[1] + "\n";
                    }
                }
                markdown += "\n";
            }
            
            if (endpoint_responses_.find(key) != endpoint_responses_.end()) {
                markdown += "**Responses:**\n\n";
                for (const auto& response : endpoint_responses_[key]) {
                    markdown += "- `" + std::to_string(response.first) + "` - " + response.second + "\n";
                }
                markdown += "\n";
            }
        }
    }
    
    return markdown;
}

std::string WebServer::generate_postman_collection() {
    std::string collection = "{\n";
    collection += "  \"info\": {\n";
    collection += "    \"name\": \"" + api_title_ + "\",\n";
    collection += "    \"description\": \"" + api_description_ + "\",\n";
    collection += "    \"version\": \"" + api_version_ + "\"\n";
    collection += "  },\n";
    collection += "  \"item\": [\n";
    
    for (const auto& route : routes_) {
        std::string method_path = route.first;
        size_t colon_pos = method_path.find(':');
        if (colon_pos != std::string::npos) {
            std::string method = method_path.substr(0, colon_pos);
            std::string path = method_path.substr(colon_pos + 1);
            
            collection += "    {\n";
            collection += "      \"name\": \"" + method + " " + path + "\",\n";
            collection += "      \"request\": {\n";
            collection += "        \"method\": \"" + method + "\",\n";
            collection += "        \"url\": {\n";
            collection += "          \"raw\": \"{{base_url}}" + path + "\",\n";
            collection += "          \"host\": [\"{{base_url}}\"],\n";
            collection += "          \"path\": [";
            
            std::vector<std::string> path_parts = split_string(path, '/');
            for (size_t i = 0; i < path_parts.size(); ++i) {
                if (!path_parts[i].empty()) {
                    collection += "\"" + path_parts[i] + "\"";
                    if (i < path_parts.size() - 1) {
                        collection += ", ";
                    }
                }
            }
            
            collection += "]\n";
            collection += "        }\n";
            collection += "      }\n";
            collection += "    }";
            if (&route != &routes_.rbegin().operator*()) {
                collection += ",";
            }
            collection += "\n";
        }
    }
    
    collection += "  ]\n";
    collection += "}\n";
    
    return collection;
}

HttpResponse WebServer::handle_markdown_docs(const HttpRequest& req, HttpResponse& res) {
    std::lock_guard<std::mutex> lock(documentation_mutex_);
    documentation_stats_["total_requests"]++;
    
    res.status_code = 200;
    res.headers["Content-Type"] = "text/markdown";
    res.headers["Content-Disposition"] = "attachment; filename=\"api-documentation.md\"";
    res.body = generate_markdown_documentation();
    
    return res;
}

HttpResponse WebServer::handle_postman_collection(const HttpRequest& req, HttpResponse& res) {
    std::lock_guard<std::mutex> lock(documentation_mutex_);
    documentation_stats_["total_requests"]++;
    
    res.status_code = 200;
    res.headers["Content-Type"] = "application/json";
    res.headers["Content-Disposition"] = "attachment; filename=\"postman-collection.json\"";
    res.body = generate_postman_collection();
    
    return res;
}

void WebServer::export_documentation(const std::string& format, const std::string& file_path) {
    std::lock_guard<std::mutex> lock(documentation_mutex_);
    
    std::ofstream file(file_path);
    if (file.is_open()) {
        if (format == "openapi") {
            file << generate_openapi_spec();
        } else if (format == "markdown") {
            file << generate_markdown_documentation();
        } else if (format == "postman") {
            file << generate_postman_collection();
        }
        file.close();
        std::cout << "📄 Documentation exported to " << file_path << std::endl;
    } else {
        std::cerr << "❌ Failed to export documentation to " << file_path << std::endl;
    }
}

size_t WebServer::get_documentation_requests() {
    std::lock_guard<std::mutex> lock(documentation_mutex_);
    return documentation_stats_["total_requests"];
}

void WebServer::reset_documentation_stats() {
    std::lock_guard<std::mutex> lock(documentation_mutex_);
    documentation_stats_.clear();
    documentation_stats_["total_requests"] = 0;
    documentation_stats_["openapi_requests"] = 0;
    documentation_stats_["swagger_requests"] = 0;
    documentation_stats_["html_requests"] = 0;
}

} // namespace web
} // namespace dds 