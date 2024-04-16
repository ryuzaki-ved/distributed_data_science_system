#pragma once

#include "utils/types.h"
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <mutex>

namespace dds {

class HDFSStorage {
public:
    HDFSStorage();
    ~HDFSStorage();
    
    // Initialize HDFS connection
    bool initialize(const std::string& hdfs_uri = "hdfs://localhost:9000");
    void shutdown();
    
    // File operations
    bool file_exists(const std::string& path);
    bool create_file(const std::string& path);
    bool delete_file(const std::string& path);
    bool copy_file(const std::string& src_path, const std::string& dst_path);
    bool move_file(const std::string& src_path, const std::string& dst_path);
    
    // Directory operations
    bool directory_exists(const std::string& path);
    bool create_directory(const std::string& path);
    bool delete_directory(const std::string& path, bool recursive = false);
    std::vector<std::string> list_directory(const std::string& path);
    
    // Data reading and writing
    bool write_matrix(const std::string& path, const Matrix& matrix);
    bool read_matrix(const std::string& path, Matrix& matrix);
    bool write_vector(const std::string& path, const Vector& vector);
    bool read_vector(const std::string& path, Vector& vector);
    
    // CSV file operations
    bool write_csv(const std::string& path, const Matrix& data, 
                  const std::vector<std::string>& headers = {});
    bool read_csv(const std::string& path, Matrix& data, 
                 std::vector<std::string>& headers);
    
    // Binary file operations
    bool write_binary(const std::string& path, const std::vector<char>& data);
    bool read_binary(const std::string& path, std::vector<char>& data);
    
    // Data partitioning
    std::vector<PartitionInfo> partition_data(const std::string& data_path,
                                             PartitionStrategy strategy,
                                             int num_partitions);
    bool write_partition(const std::string& path, const Matrix& partition_data,
                        int partition_id);
    bool read_partition(const std::string& path, Matrix& partition_data);
    
    // Checkpointing
    bool save_checkpoint(const std::string& path, const CheckpointData& checkpoint);
    bool load_checkpoint(const std::string& path, CheckpointData& checkpoint);
    
    // Performance monitoring
    struct FileInfo {
        std::string path;
        size_t size_bytes;
        std::string owner;
        std::string permissions;
        std::chrono::system_clock::time_point last_modified;
        bool is_directory;
    };
    
    FileInfo get_file_info(const std::string& path);
    size_t get_file_size(const std::string& path);
    
    // Error handling
    bool has_error() const { return has_error_; }
    std::string get_last_error() const { return last_error_; }
    void clear_error();

private:
    // HDFS connection
    void* hdfs_fs_;  // HDFS filesystem handle
    std::string hdfs_uri_;
    bool initialized_;
    bool has_error_;
    std::string last_error_;
    mutable std::mutex hdfs_mutex_;
    
    // Internal methods
    bool check_hdfs_error(int result, const std::string& operation);
    std::string get_full_path(const std::string& path);
    
    // CSV parsing helpers
    std::vector<std::string> split_csv_line(const std::string& line, char delimiter = ',');
    std::string escape_csv_field(const std::string& field);
    
    // Binary serialization helpers
    std::vector<char> serialize_checkpoint(const CheckpointData& checkpoint);
    bool deserialize_checkpoint(const std::vector<char>& data, CheckpointData& checkpoint);
};

// Local file system wrapper for development/testing
class LocalStorage {
public:
    LocalStorage();
    ~LocalStorage();
    
    // Initialize local storage
    bool initialize(const std::string& base_path = "./data");
    void shutdown();
    
    // File operations (same interface as HDFSStorage)
    bool file_exists(const std::string& path);
    bool create_file(const std::string& path);
    bool delete_file(const std::string& path);
    bool copy_file(const std::string& src_path, const std::string& dst_path);
    bool move_file(const std::string& src_path, const std::string& dst_path);
    
    // Directory operations
    bool directory_exists(const std::string& path);
    bool create_directory(const std::string& path);
    bool delete_directory(const std::string& path, bool recursive = false);
    std::vector<std::string> list_directory(const std::string& path);
    
    // Data reading and writing
    bool write_matrix(const std::string& path, const Matrix& matrix);
    bool read_matrix(const std::string& path, Matrix& matrix);
    bool write_vector(const std::string& path, const Vector& vector);
    bool read_vector(const std::string& path, Vector& vector);
    
    // CSV file operations
    bool write_csv(const std::string& path, const Matrix& data, 
                  const std::vector<std::string>& headers = {});
    bool read_csv(const std::string& path, Matrix& data, 
                 std::vector<std::string>& headers);
    
    // Binary file operations
    bool write_binary(const std::string& path, const std::vector<char>& data);
    bool read_binary(const std::string& path, std::vector<char>& data);
    
    // Data partitioning
    std::vector<PartitionInfo> partition_data(const std::string& data_path,
                                             PartitionStrategy strategy,
                                             int num_partitions);
    bool write_partition(const std::string& path, const Matrix& partition_data,
                        int partition_id);
    bool read_partition(const std::string& path, Matrix& partition_data);
    
    // Checkpointing
    bool save_checkpoint(const std::string& path, const CheckpointData& checkpoint);
    bool load_checkpoint(const std::string& path, CheckpointData& checkpoint);
    
    // File information
    HDFSStorage::FileInfo get_file_info(const std::string& path);
    size_t get_file_size(const std::string& path);
    
    // Error handling
    bool has_error() const { return has_error_; }
    std::string get_last_error() const { return last_error_; }
    void clear_error();

private:
    std::string base_path_;
    bool initialized_;
    bool has_error_;
    std::string last_error_;
    mutable std::mutex storage_mutex_;
    
    // Internal methods
    std::string get_full_path(const std::string& path);
    bool check_error(bool condition, const std::string& error_msg);
    
    // CSV parsing helpers (same as HDFSStorage)
    std::vector<std::string> split_csv_line(const std::string& line, char delimiter = ',');
    std::string escape_csv_field(const std::string& field);
    
    // Binary serialization helpers (same as HDFSStorage)
    std::vector<char> serialize_checkpoint(const CheckpointData& checkpoint);
    bool deserialize_checkpoint(const std::vector<char>& data, CheckpointData& checkpoint);
};

// Storage factory
enum class StorageType {
    HDFS,
    LOCAL
};

class StorageFactory {
public:
    static std::unique_ptr<HDFSStorage> create_hdfs_storage(const std::string& hdfs_uri = "hdfs://localhost:9000");
    static std::unique_ptr<LocalStorage> create_local_storage(const std::string& base_path = "./data");
    
    // Generic storage interface
    class IStorage {
    public:
        virtual ~IStorage() = default;
        
        // Common interface methods
        virtual bool initialize() = 0;
        virtual void shutdown() = 0;
        virtual bool file_exists(const std::string& path) = 0;
        virtual bool write_matrix(const std::string& path, const Matrix& matrix) = 0;
        virtual bool read_matrix(const std::string& path, Matrix& matrix) = 0;
        virtual bool write_vector(const std::string& path, const Vector& vector) = 0;
        virtual bool read_vector(const std::string& path, Vector& vector) = 0;
        virtual bool has_error() const = 0;
        virtual std::string get_last_error() const = 0;
    };
    
    static std::unique_ptr<IStorage> create_storage(StorageType type, 
                                                   const std::string& config = "");
};

} // namespace dds 