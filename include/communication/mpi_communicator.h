#pragma once

#include "utils/types.h"
#include <mpi.h>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace dds {

class MPICommunicator {
public:
    MPICommunicator();
    ~MPICommunicator();
    
    // Initialize MPI
    bool initialize(int argc, char* argv[]);
    void finalize();
    
    // Basic MPI operations
    int get_rank() const { return rank_; }
    int get_size() const { return size_; }
    bool is_master() const { return rank_ == 0; }
    
    // Point-to-point communication
    bool send(const MPIMessage& message, int destination);
    bool receive(MPIMessage& message, int source, int tag = MPI_ANY_TAG);
    bool send_receive(const MPIMessage& send_msg, MPIMessage& recv_msg, 
                     int destination, int source, int tag = 0);
    
    // Collective communication
    bool broadcast(MPIMessage& message, int root = 0);
    bool gather(const MPIMessage& send_msg, std::vector<MPIMessage>& recv_msgs, int root = 0);
    bool scatter(const std::vector<MPIMessage>& send_msgs, MPIMessage& recv_msg, int root = 0);
    bool reduce(const MPIMessage& send_msg, MPIMessage& recv_msg, MPI_Op op, int root = 0);
    bool all_reduce(const MPIMessage& send_msg, MPIMessage& recv_msg, MPI_Op op);
    bool barrier();
    
    // Matrix and vector operations
    bool broadcast_matrix(Matrix& matrix, int root = 0);
    bool gather_matrix(const Matrix& send_matrix, std::vector<Matrix>& recv_matrices, int root = 0);
    bool reduce_matrix(const Matrix& send_matrix, Matrix& recv_matrix, MPI_Op op, int root = 0);
    bool all_reduce_matrix(const Matrix& send_matrix, Matrix& recv_matrix, MPI_Op op);
    
    bool broadcast_vector(Vector& vector, int root = 0);
    bool gather_vector(const Vector& send_vector, std::vector<Vector>& recv_vectors, int root = 0);
    bool reduce_vector(const Vector& send_vector, Vector& recv_vector, MPI_Op op, int root = 0);
    bool all_reduce_vector(const Vector& send_vector, Vector& recv_vector, MPI_Op op);
    
    // Asynchronous communication
    MPI_Request isend(const MPIMessage& message, int destination, int tag = 0);
    MPI_Request ireceive(MPIMessage& message, int source, int tag = MPI_ANY_TAG);
    bool wait(MPI_Request& request, MPI_Status* status = nullptr);
    bool test(MPI_Request& request, int& flag, MPI_Status* status = nullptr);
    
    // Message handling
    void set_message_handler(MessageType type, 
                           std::function<void(const MPIMessage&)> handler);
    void start_message_loop();
    void stop_message_loop();
    
    // Performance monitoring
    PerformanceMetrics get_performance_metrics() const;
    void reset_performance_metrics();
    
    // Error handling
    bool has_error() const { return has_error_; }
    std::string get_last_error() const { return last_error_; }
    void clear_error();

private:
    // MPI state
    int rank_;
    int size_;
    bool initialized_;
    bool has_error_;
    std::string last_error_;
    
    // Message handling
    std::atomic<bool> running_;
    std::thread message_thread_;
    std::mutex message_mutex_;
    std::condition_variable message_cv_;
    std::unordered_map<MessageType, std::function<void(const MPIMessage&)>> message_handlers_;
    
    // Performance tracking
    mutable std::mutex metrics_mutex_;
    PerformanceMetrics metrics_;
    
    // Internal methods
    void message_loop();
    void update_metrics(double communication_time);
    bool check_mpi_error(int error_code, const std::string& operation);
    
    // Serialization helpers
    std::vector<char> serialize_message(const MPIMessage& message);
    bool deserialize_message(const std::vector<char>& data, MPIMessage& message);
    
    // Custom MPI operations
    static void matrix_sum(void* invec, void* inoutvec, int* len, MPI_Datatype* datatype);
    static void matrix_max(void* invec, void* inoutvec, int* len, MPI_Datatype* datatype);
    static void matrix_min(void* invec, void* inoutvec, int* len, MPI_Datatype* datatype);
    
    static void vector_sum(void* invec, void* inoutvec, int* len, MPI_Datatype* datatype);
    static void vector_max(void* invec, void* inoutvec, int* len, MPI_Datatype* datatype);
    static void vector_min(void* invec, void* inoutvec, int* len, MPI_Datatype* datatype);
};

// Global MPI communicator instance
extern std::unique_ptr<MPICommunicator> g_mpi_communicator;

// Utility functions for common MPI operations
namespace mpi_utils {
    
    // Synchronization
    bool synchronize_all_nodes();
    bool wait_for_all_nodes(int timeout_seconds = 30);
    
    // Data distribution
    std::vector<PartitionInfo> distribute_data_partitions(const std::string& data_path, 
                                                         PartitionStrategy strategy,
                                                         int num_partitions);
    
    // Result aggregation
    ComputationResult aggregate_computation_results(const std::vector<ComputationResult>& results,
                                                   JobType job_type);
    
    // Checkpointing
    bool broadcast_checkpoint(const CheckpointData& checkpoint, int root = 0);
    bool gather_checkpoints(const std::vector<CheckpointData>& checkpoints, int root = 0);
    
    // Fault tolerance
    bool detect_node_failures(std::vector<int>& failed_nodes);
    bool redistribute_failed_node_work(const std::vector<int>& failed_nodes,
                                      const std::vector<PartitionInfo>& partitions);
    
    // Performance optimization
    void optimize_communication_pattern(int num_nodes, int data_size);
    void set_optimal_buffer_size(size_t buffer_size);
    
} // namespace mpi_utils

} // namespace dds 