#pragma once

#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <mpi.h>
#include <Eigen/Dense>

namespace dds {

// Forward declarations
class Job;
class WorkerNode;
class DataPartition;

// Basic types
using Matrix = Eigen::MatrixXd;
using Vector = Eigen::VectorXd;
using Index = Eigen::Index;
using Scalar = double;

// Job types
enum class JobType {
    LINEAR_REGRESSION,
    LOGISTIC_REGRESSION,
    KMEANS_CLUSTERING,
    DBSCAN_CLUSTERING,
    UNKNOWN
};

// Job status
enum class JobStatus {
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED,
    CANCELLED
};

// Node status
enum class NodeStatus {
    IDLE,
    BUSY,
    OFFLINE,
    FAILED
};

// Data partitioning strategy
enum class PartitionStrategy {
    ROW_BASED,
    COLUMN_BASED,
    BLOCK_BASED,
    ROUND_ROBIN
};

// Communication types
enum class MessageType {
    JOB_SUBMIT,
    JOB_STATUS,
    DATA_PARTITION,
    COMPUTATION_RESULT,
    SYNC_REQUEST,
    SYNC_RESPONSE,
    HEARTBEAT,
    NODE_FAILURE,
    CHECKPOINT,
    RECOVERY
};

// Job configuration
struct JobConfig {
    JobType type = JobType::UNKNOWN;
    std::string data_path;
    std::string output_path;
    PartitionStrategy partition_strategy = PartitionStrategy::ROW_BASED;
    int num_partitions = 1;
    int max_iterations = 100;
    double tolerance = 1e-6;
    double learning_rate = 0.01;
    int k_clusters = 3;  // For clustering algorithms
    double epsilon = 0.5;  // For DBSCAN
    int min_points = 5;   // For DBSCAN
    bool enable_checkpointing = true;
    int checkpoint_interval = 10;
};

// Job metadata
struct JobMetadata {
    std::string job_id;
    std::string user_id;
    JobType type;
    JobStatus status;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point started_at;
    std::chrono::system_clock::time_point completed_at;
    std::string error_message;
    double progress = 0.0;
    int current_iteration = 0;
    int total_iterations = 0;
};

// Node information
struct NodeInfo {
    int rank;
    std::string hostname;
    std::string ip_address;
    NodeStatus status;
    int num_cores;
    size_t available_memory;
    std::chrono::system_clock::time_point last_heartbeat;
    std::vector<std::string> assigned_jobs;
};

// Data partition information
struct PartitionInfo {
    int partition_id;
    int node_rank;
    std::string data_path;
    size_t num_rows;
    size_t num_cols;
    size_t data_size_bytes;
    bool is_loaded = false;
};

// Computation result
struct ComputationResult {
    std::string job_id;
    int partition_id;
    int iteration;
    Matrix parameters;
    Vector gradients;
    double loss;
    double accuracy;
    std::chrono::system_clock::time_point timestamp;
};

// MPI message structure
struct MPIMessage {
    MessageType type;
    int source_rank;
    int destination_rank;
    int tag;
    std::vector<char> data;
    size_t data_size;
};

// Algorithm parameters
struct LinearRegressionParams {
    Vector weights;
    double bias;
    double learning_rate;
    int max_iterations;
    double tolerance;
    bool use_regularization;
    double regularization_strength;
};

struct LogisticRegressionParams {
    Matrix weights;
    Vector bias;
    double learning_rate;
    int max_iterations;
    double tolerance;
    bool use_regularization;
    double regularization_strength;
    int num_classes;
};

struct KMeansParams {
    Matrix centroids;
    Vector cluster_assignments;
    int k;
    int max_iterations;
    double tolerance;
    std::string initialization_method;
};

struct DBSCANParams {
    double epsilon;
    int min_points;
    Vector cluster_labels;
    std::vector<std::vector<int>> clusters;
    Matrix core_points;
};

// Performance metrics
struct PerformanceMetrics {
    double total_time;
    double computation_time;
    double communication_time;
    double io_time;
    size_t memory_usage;
    int num_mpi_calls;
    double throughput;
};

// Checkpoint data
struct CheckpointData {
    std::string job_id;
    int iteration;
    Matrix model_parameters;
    Vector model_state;
    std::chrono::system_clock::time_point timestamp;
    std::string checkpoint_path;
};

// Utility functions
std::string job_type_to_string(JobType type);
JobType string_to_job_type(const std::string& str);
std::string job_status_to_string(JobStatus status);
std::string node_status_to_string(NodeStatus status);
std::string partition_strategy_to_string(PartitionStrategy strategy);

// Serialization helpers
std::vector<char> serialize_matrix(const Matrix& matrix);
Matrix deserialize_matrix(const std::vector<char>& data);
std::vector<char> serialize_vector(const Vector& vector);
Vector deserialize_vector(const std::vector<char>& data);

} // namespace dds 