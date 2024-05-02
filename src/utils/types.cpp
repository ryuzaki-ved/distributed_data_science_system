
#include <string>
#include <variant>
#include <iostream>

// Variant class: holds int, double, or std::string
class Variant {
public:
    using ValueType = std::variant<int, double, std::string>;

    Variant(int v) : value(v) {}
    Variant(double v) : value(v) {}
    Variant(const std::string& v) : value(v) {}
    Variant(const char* v) : value(std::string(v)) {}

    // Print the value held by the variant
    void print() const {
        std::visit([](const auto& arg) {
            std::cout << arg << std::endl;
        }, value);
    }

    const ValueType& get() const { return value; }

private:
    ValueType value;
};
#include "utils/types.h"
#include <sstream>
#include <cstring>
#include <algorithm>

namespace dds {

std::string job_type_to_string(JobType type) {
    switch (type) {
        case JobType::LINEAR_REGRESSION:      return "linear_regression";
        case JobType::LOGISTIC_REGRESSION:    return "logistic_regression";
        case JobType::KMEANS_CLUSTERING:      return "kmeans_clustering";
        case JobType::DBSCAN_CLUSTERING:      return "dbscan_clustering";
        case JobType::UNKNOWN:
        default:                             return "unknown";
    }
}

JobType string_to_job_type(const std::string& str) {
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);
    if (lower_str == "linear_regression" || lower_str == "linear") return JobType::LINEAR_REGRESSION;
    if (lower_str == "logistic_regression" || lower_str == "logistic") return JobType::LOGISTIC_REGRESSION;
    if (lower_str == "kmeans_clustering" || lower_str == "kmeans") return JobType::KMEANS_CLUSTERING;
    if (lower_str == "dbscan_clustering" || lower_str == "dbscan") return JobType::DBSCAN_CLUSTERING;
    return JobType::UNKNOWN;
}

std::string job_status_to_string(JobStatus status) {
    switch (status) {
        case JobStatus::PENDING:
            return "pending";
        case JobStatus::RUNNING:
            return "running";
        case JobStatus::COMPLETED:
            return "completed";
        case JobStatus::FAILED:
            return "failed";
        case JobStatus::CANCELLED:
            return "cancelled";
        default:
            return "unknown";
    }
}

std::string node_status_to_string(NodeStatus status) {
    switch (status) {
        case NodeStatus::IDLE:
            return "idle";
        case NodeStatus::BUSY:
            return "busy";
        case NodeStatus::OFFLINE:
            return "offline";
        case NodeStatus::FAILED:
            return "failed";
        default:
            return "unknown";
    }
}

std::string partition_strategy_to_string(PartitionStrategy strategy) {
    switch (strategy) {
        case PartitionStrategy::ROW_BASED:
            return "row_based";
        case PartitionStrategy::COLUMN_BASED:
            return "column_based";
        case PartitionStrategy::BLOCK_BASED:
            return "block_based";
        case PartitionStrategy::ROUND_ROBIN:
            return "round_robin";
        default:
            return "unknown";
    }
}

std::vector<char> serialize_matrix(const Matrix& matrix) {
    std::vector<char> data;
    
    // Serialize matrix dimensions
    Index rows = matrix.rows();
    Index cols = matrix.cols();
    
    size_t header_size = sizeof(Index) * 2;
    size_t data_size = sizeof(Scalar) * rows * cols;
    size_t total_size = header_size + data_size;
    
    data.resize(total_size);
    char* ptr = data.data();
    
    // Write dimensions
    std::memcpy(ptr, &rows, sizeof(Index));
    ptr += sizeof(Index);
    std::memcpy(ptr, &cols, sizeof(Index));
    ptr += sizeof(Index);
    
    // Write matrix data
    std::memcpy(ptr, matrix.data(), data_size);
    
    return data;
}

Matrix deserialize_matrix(const std::vector<char>& data) {
    if (data.size() < sizeof(Index) * 2) {
        throw std::runtime_error("Invalid matrix data: insufficient header size");
    }
    
    const char* ptr = data.data();
    
    // Read dimensions
    Index rows, cols;
    std::memcpy(&rows, ptr, sizeof(Index));
    ptr += sizeof(Index);
    std::memcpy(&cols, ptr, sizeof(Index));
    ptr += sizeof(Index);
    
    // Create matrix
    Matrix matrix(rows, cols);
    
    // Read matrix data
    size_t data_size = sizeof(Scalar) * rows * cols;
    if (data.size() < sizeof(Index) * 2 + data_size) {
        throw std::runtime_error("Invalid matrix data: insufficient data size");
    }
    
    std::memcpy(matrix.data(), ptr, data_size);
    
    return matrix;
}

std::vector<char> serialize_vector(const Vector& vector) {
    std::vector<char> data;
    
    // Serialize vector size
    Index size = vector.size();
    
    size_t header_size = sizeof(Index);
    size_t data_size = sizeof(Scalar) * size;
    size_t total_size = header_size + data_size;
    
    data.resize(total_size);
    char* ptr = data.data();
    
    // Write size
    std::memcpy(ptr, &size, sizeof(Index));
    ptr += sizeof(Index);
    
    // Write vector data
    std::memcpy(ptr, vector.data(), data_size);
    
    return data;
}

Vector deserialize_vector(const std::vector<char>& data) {
    if (data.size() < sizeof(Index)) {
        throw std::runtime_error("Invalid vector data: insufficient header size");
    }
    
    const char* ptr = data.data();
    
    // Read size
    Index size;
    std::memcpy(&size, ptr, sizeof(Index));
    ptr += sizeof(Index);
    
    // Create vector
    Vector vector(size);
    
    // Read vector data
    size_t data_size = sizeof(Scalar) * size;
    if (data.size() < sizeof(Index) + data_size) {
        throw std::runtime_error("Invalid vector data: insufficient data size");
    }
    
    std::memcpy(vector.data(), ptr, data_size);
    
    return vector;
}

} // namespace dds 