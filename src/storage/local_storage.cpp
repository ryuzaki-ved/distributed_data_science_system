#include "storage/hdfs_storage.h"
#include "utils/types.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cstring>

namespace dds {

LocalStorage::LocalStorage() 
    : base_path_("./data"), initialized_(false), has_error_(false) {
}

LocalStorage::~LocalStorage() {
    if (initialized_) {
        shutdown();
    }
}

bool LocalStorage::initialize(const std::string& base_path) {
    base_path_ = base_path;
    
    try {
        if (!std::filesystem::exists(base_path_)) {
            std::filesystem::create_directories(base_path_);
        }
        initialized_ = true;
        return true;
    } catch (const std::exception& e) {
        has_error_ = true;
        last_error_ = "Failed to initialize local storage: " + std::string(e.what());
        return false;
    }
}

void LocalStorage::shutdown() {
    initialized_ = false;
}

bool LocalStorage::file_exists(const std::string& path) {
    if (!initialized_) return false;
    
    std::string full_path = get_full_path(path);
    return std::filesystem::exists(full_path) && std::filesystem::is_regular_file(full_path);
}

bool LocalStorage::create_file(const std::string& path) {
    if (!initialized_) return false;
    
    std::string full_path = get_full_path(path);
    
    try {
        // Create parent directories if they don't exist
        std::filesystem::path file_path(full_path);
        std::filesystem::create_directories(file_path.parent_path());
        
        // Create empty file
        std::ofstream file(full_path);
        return file.good();
    } catch (const std::exception& e) {
        has_error_ = true;
        last_error_ = "Failed to create file: " + std::string(e.what());
        return false;
    }
}

bool LocalStorage::delete_file(const std::string& path) {
    if (!initialized_) return false;
    
    std::string full_path = get_full_path(path);
    
    try {
        return std::filesystem::remove(full_path);
    } catch (const std::exception& e) {
        has_error_ = true;
        last_error_ = "Failed to delete file: " + std::string(e.what());
        return false;
    }
}

bool LocalStorage::copy_file(const std::string& src_path, const std::string& dst_path) {
    if (!initialized_) return false;
    
    std::string full_src_path = get_full_path(src_path);
    std::string full_dst_path = get_full_path(dst_path);
    
    try {
        std::filesystem::copy_file(full_src_path, full_dst_path);
        return true;
    } catch (const std::exception& e) {
        has_error_ = true;
        last_error_ = "Failed to copy file: " + std::string(e.what());
        return false;
    }
}

bool LocalStorage::move_file(const std::string& src_path, const std::string& dst_path) {
    if (!initialized_) return false;
    
    std::string full_src_path = get_full_path(src_path);
    std::string full_dst_path = get_full_path(dst_path);
    
    try {
        std::filesystem::rename(full_src_path, full_dst_path);
        return true;
    } catch (const std::exception& e) {
        has_error_ = true;
        last_error_ = "Failed to move file: " + std::string(e.what());
        return false;
    }
}

bool LocalStorage::directory_exists(const std::string& path) {
    if (!initialized_) return false;
    
    std::string full_path = get_full_path(path);
    return std::filesystem::exists(full_path) && std::filesystem::is_directory(full_path);
}

bool LocalStorage::create_directory(const std::string& path) {
    if (!initialized_) return false;
    
    std::string full_path = get_full_path(path);
    
    try {
        std::filesystem::create_directories(full_path);
        return true;
    } catch (const std::exception& e) {
        has_error_ = true;
        last_error_ = "Failed to create directory: " + std::string(e.what());
        return false;
    }
}

bool LocalStorage::delete_directory(const std::string& path, bool recursive) {
    if (!initialized_) return false;
    
    std::string full_path = get_full_path(path);
    
    try {
        if (recursive) {
            std::filesystem::remove_all(full_path);
        } else {
            std::filesystem::remove(full_path);
        }
        return true;
    } catch (const std::exception& e) {
        has_error_ = true;
        last_error_ = "Failed to delete directory: " + std::string(e.what());
        return false;
    }
}

std::vector<std::string> LocalStorage::list_directory(const std::string& path) {
    std::vector<std::string> files;
    
    if (!initialized_) return files;
    
    std::string full_path = get_full_path(path);
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(full_path)) {
            files.push_back(entry.path().filename().string());
        }
    } catch (const std::exception& e) {
        has_error_ = true;
        last_error_ = "Failed to list directory: " + std::string(e.what());
    }
    
    return files;
}

bool LocalStorage::write_matrix(const std::string& path, const Matrix& matrix) {
    if (!initialized_) return false;
    
    std::string full_path = get_full_path(path);
    
    try {
        // Create parent directories if they don't exist
        std::filesystem::path file_path(full_path);
        std::filesystem::create_directories(file_path.parent_path());
        
        std::ofstream file(full_path, std::ios::binary);
        if (!file.good()) {
            has_error_ = true;
            last_error_ = "Failed to open file for writing: " + full_path;
            return false;
        }
        
        // Write matrix dimensions
        Index rows = matrix.rows();
        Index cols = matrix.cols();
        file.write(reinterpret_cast<const char*>(&rows), sizeof(Index));
        file.write(reinterpret_cast<const char*>(&cols), sizeof(Index));
        
        // Write matrix data
        file.write(reinterpret_cast<const char*>(matrix.data()), 
                  rows * cols * sizeof(Scalar));
        
        return file.good();
    } catch (const std::exception& e) {
        has_error_ = true;
        last_error_ = "Failed to write matrix: " + std::string(e.what());
        return false;
    }
}

bool LocalStorage::read_matrix(const std::string& path, Matrix& matrix) {
    if (!initialized_) return false;
    
    std::string full_path = get_full_path(path);
    
    try {
        std::ifstream file(full_path, std::ios::binary);
        if (!file.good()) {
            has_error_ = true;
            last_error_ = "Failed to open file for reading: " + full_path;
            return false;
        }
        
        // Read matrix dimensions
        Index rows, cols;
        file.read(reinterpret_cast<char*>(&rows), sizeof(Index));
        file.read(reinterpret_cast<char*>(&cols), sizeof(Index));
        
        // Resize matrix and read data
        matrix.resize(rows, cols);
        file.read(reinterpret_cast<char*>(matrix.data()), 
                 rows * cols * sizeof(Scalar));
        
        return file.good();
    } catch (const std::exception& e) {
        has_error_ = true;
        last_error_ = "Failed to read matrix: " + std::string(e.what());
        return false;
    }
}

bool LocalStorage::write_vector(const std::string& path, const Vector& vector) {
    if (!initialized_) return false;
    
    std::string full_path = get_full_path(path);
    
    try {
        // Create parent directories if they don't exist
        std::filesystem::path file_path(full_path);
        std::filesystem::create_directories(file_path.parent_path());
        
        std::ofstream file(full_path, std::ios::binary);
        if (!file.good()) {
            has_error_ = true;
            last_error_ = "Failed to open file for writing: " + full_path;
            return false;
        }
        
        // Write vector size
        Index size = vector.size();
        file.write(reinterpret_cast<const char*>(&size), sizeof(Index));
        
        // Write vector data
        file.write(reinterpret_cast<const char*>(vector.data()), 
                  size * sizeof(Scalar));
        
        return file.good();
    } catch (const std::exception& e) {
        has_error_ = true;
        last_error_ = "Failed to write vector: " + std::string(e.what());
        return false;
    }
}

bool LocalStorage::read_vector(const std::string& path, Vector& vector) {
    if (!initialized_) return false;
    
    std::string full_path = get_full_path(path);
    
    try {
        std::ifstream file(full_path, std::ios::binary);
        if (!file.good()) {
            has_error_ = true;
            last_error_ = "Failed to open file for reading: " + full_path;
            return false;
        }
        
        // Read vector size
        Index size;
        file.read(reinterpret_cast<char*>(&size), sizeof(Index));
        
        // Resize vector and read data
        vector.resize(size);
        file.read(reinterpret_cast<char*>(vector.data()), 
                 size * sizeof(Scalar));
        
        return file.good();
    } catch (const std::exception& e) {
        has_error_ = true;
        last_error_ = "Failed to read vector: " + std::string(e.what());
        return false;
    }
}

bool LocalStorage::write_csv(const std::string& path, const Matrix& data, 
                           const std::vector<std::string>& headers) {
    if (!initialized_) return false;
    
    std::string full_path = get_full_path(path);
    
    try {
        // Create parent directories if they don't exist
        std::filesystem::path file_path(full_path);
        std::filesystem::create_directories(file_path.parent_path());
        
        std::ofstream file(full_path);
        if (!file.good()) {
            has_error_ = true;
            last_error_ = "Failed to open file for writing: " + full_path;
            return false;
        }
        
        // Write headers if provided
        if (!headers.empty()) {
            for (size_t i = 0; i < headers.size(); ++i) {
                if (i > 0) file << ",";
                file << escape_csv_field(headers[i]);
            }
            file << "\n";
        }
        
        // Write data
        for (Index i = 0; i < data.rows(); ++i) {
            for (Index j = 0; j < data.cols(); ++j) {
                if (j > 0) file << ",";
                file << data(i, j);
            }
            file << "\n";
        }
        
        return file.good();
    } catch (const std::exception& e) {
        has_error_ = true;
        last_error_ = "Failed to write CSV: " + std::string(e.what());
        return false;
    }
}

bool LocalStorage::read_csv(const std::string& path, Matrix& data, 
                          std::vector<std::string>& headers) {
    if (!initialized_) return false;
    
    std::string full_path = get_full_path(path);
    
    try {
        std::ifstream file(full_path);
        if (!file.good()) {
            has_error_ = true;
            last_error_ = "Failed to open file for reading: " + full_path;
            return false;
        }
        
        std::vector<std::vector<double>> rows;
        std::string line;
        
        // Read headers
        if (std::getline(file, line)) {
            headers = split_csv_line(line);
        }
        
        // Read data rows
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            std::vector<std::string> fields = split_csv_line(line);
            std::vector<double> row;
            
            for (const auto& field : fields) {
                try {
                    row.push_back(std::stod(field));
                } catch (const std::exception&) {
                    row.push_back(0.0);  // Default value for invalid numbers
                }
            }
            
            rows.push_back(row);
        }
        
        // Convert to matrix
        if (rows.empty()) {
            data = Matrix::Zero(0, 0);
            return true;
        }
        
        Index num_rows = rows.size();
        Index num_cols = rows[0].size();
        
        data.resize(num_rows, num_cols);
        
        for (Index i = 0; i < num_rows; ++i) {
            for (Index j = 0; j < num_cols && j < static_cast<Index>(rows[i].size()); ++j) {
                data(i, j) = rows[i][j];
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        has_error_ = true;
        last_error_ = "Failed to read CSV: " + std::string(e.what());
        return false;
    }
}

bool LocalStorage::write_binary(const std::string& path, const std::vector<char>& data) {
    if (!initialized_) return false;
    
    std::string full_path = get_full_path(path);
    
    try {
        // Create parent directories if they don't exist
        std::filesystem::path file_path(full_path);
        std::filesystem::create_directories(file_path.parent_path());
        
        std::ofstream file(full_path, std::ios::binary);
        if (!file.good()) {
            has_error_ = true;
            last_error_ = "Failed to open file for writing: " + full_path;
            return false;
        }
        
        file.write(data.data(), data.size());
        return file.good();
    } catch (const std::exception& e) {
        has_error_ = true;
        last_error_ = "Failed to write binary data: " + std::string(e.what());
        return false;
    }
}

bool LocalStorage::read_binary(const std::string& path, std::vector<char>& data) {
    if (!initialized_) return false;
    
    std::string full_path = get_full_path(path);
    
    try {
        std::ifstream file(full_path, std::ios::binary);
        if (!file.good()) {
            has_error_ = true;
            last_error_ = "Failed to open file for reading: " + full_path;
            return false;
        }
        
        // Get file size
        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        // Read data
        data.resize(size);
        file.read(data.data(), size);
        
        return file.good();
    } catch (const std::exception& e) {
        has_error_ = true;
        last_error_ = "Failed to read binary data: " + std::string(e.what());
        return false;
    }
}

std::vector<PartitionInfo> LocalStorage::partition_data(const std::string& data_path,
                                                      PartitionStrategy strategy,
                                                      int num_partitions) {
    std::vector<PartitionInfo> partitions;
    
    if (!initialized_) return partitions;
    
    // Read the original data
    Matrix data;
    std::vector<std::string> headers;
    if (!read_csv(data_path, data, headers)) {
        return partitions;
    }
    
    Index total_rows = data.rows();
    Index total_cols = data.cols();
    Index rows_per_partition = total_rows / num_partitions;
    
    for (int i = 0; i < num_partitions; ++i) {
        PartitionInfo partition;
        partition.partition_id = i;
        partition.node_rank = i % 4;  // Assuming 4 nodes for now
        partition.data_path = data_path + "_partition_" + std::to_string(i) + ".csv";
        
        // Calculate partition boundaries
        Index start_row = i * rows_per_partition;
        Index end_row = (i == num_partitions - 1) ? total_rows : (i + 1) * rows_per_partition;
        Index partition_rows = end_row - start_row;
        
        partition.num_rows = partition_rows;
        partition.num_cols = total_cols;
        partition.data_size_bytes = partition_rows * total_cols * sizeof(Scalar);
        
        // Extract partition data
        Matrix partition_data = data.block(start_row, 0, partition_rows, total_cols);
        
        // Write partition to file
        if (write_csv(partition.data_path, partition_data, headers)) {
            partition.is_loaded = true;
        }
        
        partitions.push_back(partition);
    }
    
    return partitions;
}

bool LocalStorage::write_partition(const std::string& path, const Matrix& partition_data,
                                 int partition_id) {
    std::string partition_path = path + "_partition_" + std::to_string(partition_id) + ".csv";
    return write_csv(partition_path, partition_data);
}

bool LocalStorage::read_partition(const std::string& path, Matrix& partition_data) {
    std::vector<std::string> headers;
    return read_csv(path, partition_data, headers);
}

bool LocalStorage::save_checkpoint(const std::string& path, const CheckpointData& checkpoint) {
    if (!initialized_) return false;
    
    std::vector<char> data = serialize_checkpoint(checkpoint);
    return write_binary(path, data);
}

bool LocalStorage::load_checkpoint(const std::string& path, CheckpointData& checkpoint) {
    if (!initialized_) return false;
    
    std::vector<char> data;
    if (!read_binary(path, data)) {
        return false;
    }
    
    return deserialize_checkpoint(data, checkpoint);
}

HDFSStorage::FileInfo LocalStorage::get_file_info(const std::string& path) {
    HDFSStorage::FileInfo info;
    
    if (!initialized_) return info;
    
    std::string full_path = get_full_path(path);
    
    try {
        std::filesystem::path file_path(full_path);
        std::filesystem::file_status status = std::filesystem::status(file_path);
        
        info.path = path;
        info.size_bytes = std::filesystem::file_size(file_path);
        info.is_directory = std::filesystem::is_directory(status);
        info.last_modified = std::chrono::system_clock::from_time_t(
            std::filesystem::last_write_time(file_path).time_since_epoch().count());
        
        // These are placeholders for local storage
        info.owner = "local";
        info.permissions = "rw-r--r--";
    } catch (const std::exception& e) {
        has_error_ = true;
        last_error_ = "Failed to get file info: " + std::string(e.what());
    }
    
    return info;
}

size_t LocalStorage::get_file_size(const std::string& path) {
    if (!initialized_) return 0;
    
    std::string full_path = get_full_path(path);
    
    try {
        return std::filesystem::file_size(full_path);
    } catch (const std::exception& e) {
        has_error_ = true;
        last_error_ = "Failed to get file size: " + std::string(e.what());
        return 0;
    }
}

void LocalStorage::clear_error() {
    has_error_ = false;
    last_error_.clear();
}

std::string LocalStorage::get_full_path(const std::string& path) {
    if (std::filesystem::path(path).is_absolute()) {
        return path;
    }
    return (std::filesystem::path(base_path_) / path).string();
}

bool LocalStorage::check_error(bool condition, const std::string& error_msg) {
    if (!condition) {
        has_error_ = true;
        last_error_ = error_msg;
    }
    return condition;
}

std::vector<std::string> LocalStorage::split_csv_line(const std::string& line, char delimiter) {
    std::vector<std::string> fields;
    std::stringstream ss(line);
    std::string field;
    
    while (std::getline(ss, field, delimiter)) {
        fields.push_back(field);
    }
    
    return fields;
}

std::string LocalStorage::escape_csv_field(const std::string& field) {
    if (field.find(',') != std::string::npos || field.find('"') != std::string::npos) {
        std::string escaped = "\"";
        for (char c : field) {
            if (c == '"') {
                escaped += "\"\"";
            } else {
                escaped += c;
            }
        }
        escaped += "\"";
        return escaped;
    }
    return field;
}

std::vector<char> LocalStorage::serialize_checkpoint(const CheckpointData& checkpoint) {
    std::vector<char> data;
    
    // Serialize job_id
    size_t job_id_size = checkpoint.job_id.size();
    data.resize(sizeof(size_t) + job_id_size);
    char* ptr = data.data();
    
    std::memcpy(ptr, &job_id_size, sizeof(size_t));
    ptr += sizeof(size_t);
    std::memcpy(ptr, checkpoint.job_id.data(), job_id_size);
    ptr += job_id_size;
    
    // Serialize iteration
    data.resize(data.size() + sizeof(int));
    std::memcpy(ptr, &checkpoint.iteration, sizeof(int));
    ptr += sizeof(int);
    
    // Serialize model parameters
    std::vector<char> matrix_data = serialize_matrix(checkpoint.model_parameters);
    size_t matrix_size = matrix_data.size();
    data.resize(data.size() + sizeof(size_t) + matrix_size);
    std::memcpy(ptr, &matrix_size, sizeof(size_t));
    ptr += sizeof(size_t);
    std::memcpy(ptr, matrix_data.data(), matrix_size);
    ptr += matrix_size;
    
    // Serialize model state
    std::vector<char> vector_data = serialize_vector(checkpoint.model_state);
    size_t vector_size = vector_data.size();
    data.resize(data.size() + sizeof(size_t) + vector_size);
    std::memcpy(ptr, &vector_size, sizeof(size_t));
    ptr += sizeof(size_t);
    std::memcpy(ptr, vector_data.data(), vector_size);
    
    return data;
}

bool LocalStorage::deserialize_checkpoint(const std::vector<char>& data, CheckpointData& checkpoint) {
    if (data.size() < sizeof(size_t)) return false;
    
    const char* ptr = data.data();
    
    // Deserialize job_id
    size_t job_id_size;
    std::memcpy(&job_id_size, ptr, sizeof(size_t));
    ptr += sizeof(size_t);
    
    if (data.size() < sizeof(size_t) + job_id_size) return false;
    
    checkpoint.job_id.assign(ptr, job_id_size);
    ptr += job_id_size;
    
    // Deserialize iteration
    if (data.size() < sizeof(size_t) + job_id_size + sizeof(int)) return false;
    std::memcpy(&checkpoint.iteration, ptr, sizeof(int));
    ptr += sizeof(int);
    
    // Deserialize model parameters
    if (data.size() < sizeof(size_t) + job_id_size + sizeof(int) + sizeof(size_t)) return false;
    size_t matrix_size;
    std::memcpy(&matrix_size, ptr, sizeof(size_t));
    ptr += sizeof(size_t);
    
    if (data.size() < sizeof(size_t) + job_id_size + sizeof(int) + sizeof(size_t) + matrix_size) return false;
    std::vector<char> matrix_data(ptr, ptr + matrix_size);
    checkpoint.model_parameters = deserialize_matrix(matrix_data);
    ptr += matrix_size;
    
    // Deserialize model state
    if (data.size() < sizeof(size_t) + job_id_size + sizeof(int) + sizeof(size_t) + matrix_size + sizeof(size_t)) return false;
    size_t vector_size;
    std::memcpy(&vector_size, ptr, sizeof(size_t));
    ptr += sizeof(size_t);
    
    if (data.size() < sizeof(size_t) + job_id_size + sizeof(int) + sizeof(size_t) + matrix_size + sizeof(size_t) + vector_size) return false;
    std::vector<char> vector_data(ptr, ptr + vector_size);
    checkpoint.model_state = deserialize_vector(vector_data);
    
    return true;
}

} // namespace dds 