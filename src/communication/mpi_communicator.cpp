#include "communication/mpi_communicator.h"
#include "utils/types.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <algorithm>

namespace dds {

// Global MPI communicator instance
std::unique_ptr<MPICommunicator> g_mpi_communicator;

MPICommunicator::MPICommunicator() 
    : rank_(-1), size_(-1), initialized_(false), has_error_(false), running_(false) {
}

MPICommunicator::~MPICommunicator() {
    if (initialized_) {
        finalize();
    }
}

bool MPICommunicator::initialize(int argc, char* argv[]) {
    if (initialized_) {
        return true;
    }
    
    int provided;
    int result = MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (!check_mpi_error(result, "MPI_Init_thread")) {
        return false;
    }
    
    result = MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
    if (!check_mpi_error(result, "MPI_Comm_rank")) {
        return false;
    }
    
    result = MPI_Comm_size(MPI_COMM_WORLD, &size_);
    if (!check_mpi_error(result, "MPI_Comm_size")) {
        return false;
    }
    
    initialized_ = true;
    reset_performance_metrics();
    
    return true;
}

void MPICommunicator::finalize() {
    if (running_) {
        stop_message_loop();
    }
    
    if (initialized_) {
        MPI_Finalize();
        initialized_ = false;
    }
}

bool MPICommunicator::send(const MPIMessage& message, int destination) {
    if (!initialized_) return false;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<char> data = serialize_message(message);
    int data_size = static_cast<int>(data.size());
    
    // Send message size first
    int result = MPI_Send(&data_size, 1, MPI_INT, destination, message.tag, MPI_COMM_WORLD);
    if (!check_mpi_error(result, "MPI_Send (size)")) {
        return false;
    }
    
    // Send message data
    result = MPI_Send(data.data(), data_size, MPI_CHAR, destination, message.tag + 1, MPI_COMM_WORLD);
    if (!check_mpi_error(result, "MPI_Send (data)")) {
        return false;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    update_metrics(duration.count() / 1000000.0);
    
    return true;
}

bool MPICommunicator::receive(MPIMessage& message, int source, int tag) {
    if (!initialized_) return false;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Receive message size first
    int data_size;
    MPI_Status status;
    int result = MPI_Recv(&data_size, 1, MPI_INT, source, tag, MPI_COMM_WORLD, &status);
    if (!check_mpi_error(result, "MPI_Recv (size)")) {
        return false;
    }
    
    // Receive message data
    std::vector<char> data(data_size);
    result = MPI_Recv(data.data(), data_size, MPI_CHAR, source, tag + 1, MPI_COMM_WORLD, &status);
    if (!check_mpi_error(result, "MPI_Recv (data)")) {
        return false;
    }
    
    // Deserialize message
    if (!deserialize_message(data, message)) {
        return false;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    update_metrics(duration.count() / 1000000.0);
    
    return true;
}

bool MPICommunicator::broadcast(MPIMessage& message, int root) {
    if (!initialized_) return false;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<char> data;
    int data_size;
    
    if (rank_ == root) {
        data = serialize_message(message);
        data_size = static_cast<int>(data.size());
    }
    
    // Broadcast data size
    int result = MPI_Bcast(&data_size, 1, MPI_INT, root, MPI_COMM_WORLD);
    if (!check_mpi_error(result, "MPI_Bcast (size)")) {
        return false;
    }
    
    if (rank_ != root) {
        data.resize(data_size);
    }
    
    // Broadcast data
    result = MPI_Bcast(data.data(), data_size, MPI_CHAR, root, MPI_COMM_WORLD);
    if (!check_mpi_error(result, "MPI_Bcast (data)")) {
        return false;
    }
    
    if (rank_ != root) {
        if (!deserialize_message(data, message)) {
            return false;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    update_metrics(duration.count() / 1000000.0);
    
    return true;
}

bool MPICommunicator::broadcast_matrix(Matrix& matrix, int root) {
    if (!initialized_) return false;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    Index rows, cols;
    if (rank_ == root) {
        rows = matrix.rows();
        cols = matrix.cols();
    }
    
    // Broadcast dimensions
    int result = MPI_Bcast(&rows, 1, MPI_LONG_LONG, root, MPI_COMM_WORLD);
    if (!check_mpi_error(result, "MPI_Bcast (rows)")) {
        return false;
    }
    
    result = MPI_Bcast(&cols, 1, MPI_LONG_LONG, root, MPI_COMM_WORLD);
    if (!check_mpi_error(result, "MPI_Bcast (cols)")) {
        return false;
    }
    
    if (rank_ != root) {
        matrix.resize(rows, cols);
    }
    
    // Broadcast matrix data
    result = MPI_Bcast(matrix.data(), rows * cols, MPI_DOUBLE, root, MPI_COMM_WORLD);
    if (!check_mpi_error(result, "MPI_Bcast (matrix)")) {
        return false;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    update_metrics(duration.count() / 1000000.0);
    
    return true;
}

bool MPICommunicator::broadcast_vector(Vector& vector, int root) {
    if (!initialized_) return false;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    Index size;
    if (rank_ == root) {
        size = vector.size();
    }
    
    // Broadcast size
    int result = MPI_Bcast(&size, 1, MPI_LONG_LONG, root, MPI_COMM_WORLD);
    if (!check_mpi_error(result, "MPI_Bcast (size)")) {
        return false;
    }
    
    if (rank_ != root) {
        vector.resize(size);
    }
    
    // Broadcast vector data
    result = MPI_Bcast(vector.data(), size, MPI_DOUBLE, root, MPI_COMM_WORLD);
    if (!check_mpi_error(result, "MPI_Bcast (vector)")) {
        return false;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    update_metrics(duration.count() / 1000000.0);
    
    return true;
}

bool MPICommunicator::all_reduce_matrix(const Matrix& send_matrix, Matrix& recv_matrix, MPI_Op op) {
    if (!initialized_) return false;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    Index rows = send_matrix.rows();
    Index cols = send_matrix.cols();
    
    // Ensure all matrices have the same dimensions
    recv_matrix.resize(rows, cols);
    
    // Perform all-reduce operation
    int result = MPI_Allreduce(send_matrix.data(), recv_matrix.data(), 
                              rows * cols, MPI_DOUBLE, op, MPI_COMM_WORLD);
    if (!check_mpi_error(result, "MPI_Allreduce (matrix)")) {
        return false;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    update_metrics(duration.count() / 1000000.0);
    
    return true;
}

bool MPICommunicator::all_reduce_vector(const Vector& send_vector, Vector& recv_vector, MPI_Op op) {
    if (!initialized_) return false;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    Index size = send_vector.size();
    recv_vector.resize(size);
    
    // Perform all-reduce operation
    int result = MPI_Allreduce(send_vector.data(), recv_vector.data(), 
                              size, MPI_DOUBLE, op, MPI_COMM_WORLD);
    if (!check_mpi_error(result, "MPI_Allreduce (vector)")) {
        return false;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    update_metrics(duration.count() / 1000000.0);
    
    return true;
}

bool MPICommunicator::barrier() {
    if (!initialized_) return false;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    int result = MPI_Barrier(MPI_COMM_WORLD);
    if (!check_mpi_error(result, "MPI_Barrier")) {
        return false;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    update_metrics(duration.count() / 1000000.0);
    
    return true;
}

void MPICommunicator::set_message_handler(MessageType type, 
                                         std::function<void(const MPIMessage&)> handler) {
    std::lock_guard<std::mutex> lock(message_mutex_);
    message_handlers_[type] = handler;
}

void MPICommunicator::start_message_loop() {
    if (running_) return;
    
    running_ = true;
    message_thread_ = std::thread(&MPICommunicator::message_loop, this);
}

void MPICommunicator::stop_message_loop() {
    if (!running_) return;
    
    running_ = false;
    message_cv_.notify_all();
    
    if (message_thread_.joinable()) {
        message_thread_.join();
    }
}

void MPICommunicator::message_loop() {
    while (running_) {
        MPIMessage message;
        if (receive(message, MPI_ANY_SOURCE, MPI_ANY_TAG)) {
            auto it = message_handlers_.find(message.type);
            if (it != message_handlers_.end()) {
                it->second(message);
            }
        }
    }
}

PerformanceMetrics MPICommunicator::get_performance_metrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return metrics_;
}

void MPICommunicator::reset_performance_metrics() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    metrics_ = PerformanceMetrics{};
}

void MPICommunicator::update_metrics(double communication_time) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    metrics_.communication_time += communication_time;
    metrics_.total_time += communication_time;
    metrics_.num_mpi_calls++;
}

bool MPICommunicator::check_mpi_error(int error_code, const std::string& operation) {
    if (error_code != MPI_SUCCESS) {
        has_error_ = true;
        last_error_ = "MPI error in " + operation + ": " + std::to_string(error_code);
        return false;
    }
    return true;
}

std::vector<char> MPICommunicator::serialize_message(const MPIMessage& message) {
    std::vector<char> data;
    
    // Serialize message header
    size_t header_size = sizeof(MessageType) + sizeof(int) * 3 + sizeof(size_t);
    size_t total_size = header_size + message.data.size();
    
    data.resize(total_size);
    char* ptr = data.data();
    
    // Write header
    std::memcpy(ptr, &message.type, sizeof(MessageType));
    ptr += sizeof(MessageType);
    std::memcpy(ptr, &message.source_rank, sizeof(int));
    ptr += sizeof(int);
    std::memcpy(ptr, &message.destination_rank, sizeof(int));
    ptr += sizeof(int);
    std::memcpy(ptr, &message.tag, sizeof(int));
    ptr += sizeof(int);
    std::memcpy(ptr, &message.data_size, sizeof(size_t));
    ptr += sizeof(size_t);
    
    // Write data
    if (!message.data.empty()) {
        std::memcpy(ptr, message.data.data(), message.data.size());
    }
    
    return data;
}

bool MPICommunicator::deserialize_message(const std::vector<char>& data, MPIMessage& message) {
    if (data.size() < sizeof(MessageType) + sizeof(int) * 3 + sizeof(size_t)) {
        return false;
    }
    
    const char* ptr = data.data();
    
    // Read header
    std::memcpy(&message.type, ptr, sizeof(MessageType));
    ptr += sizeof(MessageType);
    std::memcpy(&message.source_rank, ptr, sizeof(int));
    ptr += sizeof(int);
    std::memcpy(&message.destination_rank, ptr, sizeof(int));
    ptr += sizeof(int);
    std::memcpy(&message.tag, ptr, sizeof(int));
    ptr += sizeof(int);
    std::memcpy(&message.data_size, ptr, sizeof(size_t));
    ptr += sizeof(size_t);
    
    // Read data
    size_t data_offset = sizeof(MessageType) + sizeof(int) * 3 + sizeof(size_t);
    if (data.size() > data_offset) {
        message.data.assign(ptr, ptr + (data.size() - data_offset));
    }
    
    return true;
}

void MPICommunicator::clear_error() {
    has_error_ = false;
    last_error_.clear();
}

// MPI utility functions
namespace mpi_utils {

bool synchronize_all_nodes() {
    if (!g_mpi_communicator) return false;
    return g_mpi_communicator->barrier();
}

bool wait_for_all_nodes(int timeout_seconds) {
    if (!g_mpi_communicator) return false;
    
    auto start_time = std::chrono::steady_clock::now();
    auto timeout = std::chrono::seconds(timeout_seconds);
    
    while (std::chrono::steady_clock::now() - start_time < timeout) {
        if (g_mpi_communicator->barrier()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return false;
}

std::vector<PartitionInfo> distribute_data_partitions(const std::string& data_path,
                                                     PartitionStrategy strategy,
                                                     int num_partitions) {
    std::vector<PartitionInfo> partitions;
    
    if (!g_mpi_communicator) return partitions;
    
    // This is a simplified implementation
    // In a real system, you would read the actual data file and partition it
    for (int i = 0; i < num_partitions; ++i) {
        PartitionInfo partition;
        partition.partition_id = i;
        partition.node_rank = i % g_mpi_communicator->get_size();
        partition.data_path = data_path + "_partition_" + std::to_string(i);
        partition.num_rows = 1000;  // Placeholder
        partition.num_cols = 10;    // Placeholder
        partition.data_size_bytes = 80000;  // Placeholder
        partitions.push_back(partition);
    }
    
    return partitions;
}

ComputationResult aggregate_computation_results(const std::vector<ComputationResult>& results,
                                               JobType job_type) {
    ComputationResult aggregated_result;
    
    if (results.empty()) return aggregated_result;
    
    // Use the first result as base
    aggregated_result = results[0];
    
    // Aggregate based on job type
    switch (job_type) {
        case JobType::LINEAR_REGRESSION:
        case JobType::LOGISTIC_REGRESSION:
            // Average the parameters
            aggregated_result.parameters = Matrix::Zero(
                results[0].parameters.rows(), results[0].parameters.cols());
            aggregated_result.gradients = Vector::Zero(results[0].gradients.size());
            
            for (const auto& result : results) {
                aggregated_result.parameters += result.parameters;
                aggregated_result.gradients += result.gradients;
            }
            
            aggregated_result.parameters /= results.size();
            aggregated_result.gradients /= results.size();
            break;
            
        case JobType::KMEANS_CLUSTERING:
            // Average the centroids
            aggregated_result.parameters = Matrix::Zero(
                results[0].parameters.rows(), results[0].parameters.cols());
            
            for (const auto& result : results) {
                aggregated_result.parameters += result.parameters;
            }
            
            aggregated_result.parameters /= results.size();
            break;
            
        default:
            break;
    }
    
    return aggregated_result;
}

} // namespace mpi_utils

} // namespace dds 