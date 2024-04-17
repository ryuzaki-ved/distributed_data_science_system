#pragma once

#include "../utils/types.h"
#include "../utils/eigen_stub.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace dds {
namespace storage {

// Forward declarations for Hadoop types
struct HadoopConfig;
struct HDFSFile;
struct HDFSConnection;

// Hadoop configuration
struct HadoopConfig {
    std::string namenode_host = "localhost";
    int namenode_port = 9000;
    std::string hdfs_url = "hdfs://localhost:9000";
    std::string hadoop_home = "/usr/local/hadoop";
    int replication_factor = 3;
    std::string block_size = "128MB";
    std::string username = "hdfs";
    
    // YARN configuration
    std::string yarn_resourcemanager_host = "localhost";
    int yarn_resourcemanager_port = 8032;
    std::string yarn_memory = "4GB";
    int yarn_cores = 4;
    
    // Security
    bool enable_kerberos = false;
    std::string keytab_file;
    std::string principal;
};

// HDFS file information
struct HDFSFileInfo {
    std::string path;
    size_t size;
    bool is_directory;
    std::string owner;
    std::string group;
    std::string permissions;
    time_t modification_time;
    time_t access_time;
};

// Hadoop storage manager
class HadoopStorage {
private:
    std::unique_ptr<HadoopConfig> config_;
    std::unique_ptr<HDFSConnection> connection_;
    bool initialized_;

public:
    HadoopStorage();
    explicit HadoopStorage(const HadoopConfig& config);
    ~HadoopStorage();
    
    // Initialization and connection
    bool initialize();
    bool connect();
    void disconnect();
    bool is_connected() const;
    
    // File operations
    bool file_exists(const std::string& path);
    bool create_file(const std::string& path, const std::string& content);
    bool create_file(const std::string& path, const std::vector<char>& data);
    bool read_file(const std::string& path, std::string& content);
    bool read_file(const std::string& path, std::vector<char>& data);
    bool delete_file(const std::string& path);
    bool copy_file(const std::string& src_path, const std::string& dst_path);
    bool move_file(const std::string& src_path, const std::string& dst_path);
    
    // Directory operations
    bool create_directory(const std::string& path);
    bool delete_directory(const std::string& path, bool recursive = false);
    std::vector<HDFSFileInfo> list_directory(const std::string& path);
    
    // Matrix/Vector operations
    bool save_matrix(const std::string& path, const Eigen::MatrixXd& matrix);
    bool load_matrix(const std::string& path, Eigen::MatrixXd& matrix);
    bool save_vector(const std::string& path, const Eigen::VectorXd& vector);
    bool load_vector(const std::string& path, Eigen::VectorXd& vector);
    
    // Data processing operations
    bool save_dataset(const std::string& path, const Eigen::MatrixXd& features, 
                     const Eigen::VectorXd& labels);
    bool load_dataset(const std::string& path, Eigen::MatrixXd& features, 
                     Eigen::VectorXd& labels);
    
    // Batch operations
    bool save_batch_data(const std::string& base_path, 
                        const std::vector<Eigen::MatrixXd>& matrices,
                        const std::vector<std::string>& names);
    bool load_batch_data(const std::string& base_path,
                        std::vector<Eigen::MatrixXd>& matrices,
                        const std::vector<std::string>& names);
    
    // Configuration
    void set_config(const HadoopConfig& config);
    const HadoopConfig& get_config() const;
    
    // Utility functions
    std::string get_full_path(const std::string& relative_path) const;
    size_t get_file_size(const std::string& path);
    std::string get_file_checksum(const std::string& path);
    
    // Error handling
    std::string get_last_error() const;
    void clear_error();
    
private:
    // Internal helper methods
    bool ensure_connected();
    std::string serialize_matrix(const Eigen::MatrixXd& matrix);
    bool deserialize_matrix(const std::string& data, Eigen::MatrixXd& matrix);
    std::string serialize_vector(const Eigen::VectorXd& vector);
    bool deserialize_vector(const std::string& data, Eigen::VectorXd& vector);
};

// Hadoop job manager for MapReduce operations
class HadoopJobManager {
private:
    std::unique_ptr<HadoopConfig> config_;
    bool initialized_;

public:
    HadoopJobManager();
    explicit HadoopJobManager(const HadoopConfig& config);
    ~HadoopJobManager();
    
    // Job management
    bool submit_job(const std::string& job_name, 
                   const std::string& input_path,
                   const std::string& output_path,
                   const std::string& mapper_class,
                   const std::string& reducer_class);
    
    bool submit_job(const std::string& job_name,
                   const std::vector<std::string>& input_paths,
                   const std::string& output_path,
                   const std::string& mapper_class,
                   const std::string& reducer_class);
    
    // Job monitoring
    bool is_job_running(const std::string& job_id);
    bool is_job_completed(const std::string& job_id);
    bool is_job_failed(const std::string& job_id);
    double get_job_progress(const std::string& job_id);
    
    // Job control
    bool kill_job(const std::string& job_id);
    bool pause_job(const std::string& job_id);
    bool resume_job(const std::string& job_id);
    
    // Job information
    struct JobInfo {
        std::string job_id;
        std::string job_name;
        std::string status;
        std::string user;
        time_t start_time;
        time_t finish_time;
        int maps_total;
        int maps_completed;
        int reduces_total;
        int reduces_completed;
        double progress;
    };
    
    JobInfo get_job_info(const std::string& job_id);
    std::vector<JobInfo> list_jobs();
    
    // Configuration
    void set_config(const HadoopConfig& config);
    const HadoopConfig& get_config() const;
    
private:
    bool ensure_initialized();
    std::string get_last_error() const;
};

} // namespace storage
} // namespace dds 