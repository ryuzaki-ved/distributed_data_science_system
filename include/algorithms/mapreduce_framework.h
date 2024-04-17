#pragma once

#include "../utils/types.h"
#include "../utils/eigen_stub.h"
#include "../storage/hadoop_storage.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace dds {
namespace algorithms {

// MapReduce job types
enum class MapReduceJobType {
    LINEAR_REGRESSION,
    KMEANS_CLUSTERING,
    DBSCAN_CLUSTERING,
    MATRIX_MULTIPLICATION,
    FEATURE_EXTRACTION,
    DATA_PREPROCESSING
};

// MapReduce job status
enum class MapReduceJobStatus {
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED,
    CANCELLED
};

// MapReduce job configuration
struct MapReduceJobConfig {
    std::string job_name;
    MapReduceJobType job_type;
    std::string input_path;
    std::string output_path;
    int num_mappers = 4;
    int num_reducers = 2;
    std::string mapper_class;
    std::string reducer_class;
    std::vector<std::string> additional_args;
    
    // Algorithm-specific parameters
    double learning_rate = 0.01;
    int max_iterations = 1000;
    double tolerance = 1e-6;
    int k_clusters = 3;
    double epsilon = 0.5;
    int min_points = 5;
};

// MapReduce job result
struct MapReduceJobResult {
    std::string job_id;
    MapReduceJobStatus status;
    std::string output_path;
    double execution_time;
    std::string error_message;
    
    // Algorithm-specific results
    Eigen::VectorXd coefficients;  // For linear regression
    Eigen::MatrixXd centroids;     // For clustering
    std::vector<int> cluster_labels;
    double accuracy;
    double loss;
};

// Abstract base class for MapReduce algorithms
class MapReduceAlgorithm {
protected:
    MapReduceJobConfig config_;
    std::shared_ptr<dds::storage::HadoopStorage> storage_;
    std::shared_ptr<dds::storage::HadoopJobManager> job_manager_;

public:
    MapReduceAlgorithm(const MapReduceJobConfig& config,
                      std::shared_ptr<dds::storage::HadoopStorage> storage,
                      std::shared_ptr<dds::storage::HadoopJobManager> job_manager);
    virtual ~MapReduceAlgorithm() = default;
    
    // Core MapReduce operations
    virtual bool prepare_data() = 0;
    virtual bool submit_job() = 0;
    virtual bool monitor_job() = 0;
    virtual bool collect_results() = 0;
    
    // Complete execution pipeline
    virtual MapReduceJobResult execute();
    
    // Configuration
    void set_config(const MapReduceJobConfig& config);
    const MapReduceJobConfig& get_config() const;
    
    // Utility methods
    virtual std::string generate_mapper_code() = 0;
    virtual std::string generate_reducer_code() = 0;
    virtual bool validate_config() = 0;
    
protected:
    // Helper methods
    bool create_input_data();
    bool parse_output_data();
    std::string get_job_id() const;
    void set_job_id(const std::string& job_id);
    
private:
    std::string job_id_;
};

// Distributed Linear Regression using MapReduce
class DistributedLinearRegression : public MapReduceAlgorithm {
private:
    Eigen::MatrixXd training_data_;
    Eigen::VectorXd labels_;
    Eigen::VectorXd coefficients_;
    double intercept_;
    double final_loss_;

public:
    DistributedLinearRegression(const MapReduceJobConfig& config,
                               std::shared_ptr<dds::storage::HadoopStorage> storage,
                               std::shared_ptr<dds::storage::HadoopJobManager> job_manager);
    
    // Override base class methods
    bool prepare_data() override;
    bool submit_job() override;
    bool monitor_job() override;
    bool collect_results() override;
    
    // Linear regression specific methods
    bool fit(const Eigen::MatrixXd& X, const Eigen::VectorXd& y);
    Eigen::VectorXd predict(const Eigen::MatrixXd& X) const;
    double get_loss() const { return final_loss_; }
    Eigen::VectorXd get_coefficients() const { return coefficients_; }
    double get_intercept() const { return intercept_; }
    
    // MapReduce code generation
    std::string generate_mapper_code() override;
    std::string generate_reducer_code() override;
    bool validate_config() override;
    
private:
    bool generate_training_data();
    bool parse_coefficients();
};

// Distributed K-Means Clustering using MapReduce
class DistributedKMeans : public MapReduceAlgorithm {
private:
    Eigen::MatrixXd data_;
    Eigen::MatrixXd centroids_;
    std::vector<int> cluster_labels_;
    double final_inertia_;

public:
    DistributedKMeans(const MapReduceJobConfig& config,
                     std::shared_ptr<dds::storage::HadoopStorage> storage,
                     std::shared_ptr<dds::storage::HadoopJobManager> job_manager);
    
    // Override base class methods
    bool prepare_data() override;
    bool submit_job() override;
    bool monitor_job() override;
    bool collect_results() override;
    
    // K-means specific methods
    bool fit(const Eigen::MatrixXd& data);
    std::vector<int> predict(const Eigen::MatrixXd& data) const;
    Eigen::MatrixXd get_centroids() const { return centroids_; }
    double get_inertia() const { return final_inertia_; }
    
    // MapReduce code generation
    std::string generate_mapper_code() override;
    std::string generate_reducer_code() override;
    bool validate_config() override;
    
private:
    bool generate_clustering_data();
    bool parse_centroids();
    Eigen::MatrixXd initialize_centroids(const Eigen::MatrixXd& data, int k);
};

// MapReduce job scheduler
class MapReduceScheduler {
private:
    std::shared_ptr<dds::storage::HadoopStorage> storage_;
    std::shared_ptr<dds::storage::HadoopJobManager> job_manager_;
    std::vector<std::shared_ptr<MapReduceAlgorithm>> pending_jobs_;
    std::vector<std::shared_ptr<MapReduceAlgorithm>> running_jobs_;
    std::vector<std::shared_ptr<MapReduceAlgorithm>> completed_jobs_;

public:
    MapReduceScheduler(std::shared_ptr<dds::storage::HadoopStorage> storage,
                      std::shared_ptr<dds::storage::HadoopJobManager> job_manager);
    
    // Job management
    bool submit_job(std::shared_ptr<MapReduceAlgorithm> algorithm);
    bool cancel_job(const std::string& job_id);
    bool pause_job(const std::string& job_id);
    bool resume_job(const std::string& job_id);
    
    // Job monitoring
    void update_job_status();
    std::vector<MapReduceJobResult> get_completed_jobs();
    std::vector<MapReduceJobResult> get_failed_jobs();
    
    // Batch operations
    bool submit_batch_jobs(const std::vector<std::shared_ptr<MapReduceAlgorithm>>& algorithms);
    bool wait_for_completion(const std::vector<std::string>& job_ids, double timeout = 3600.0);
    
    // Resource management
    bool check_cluster_status();
    double get_cluster_utilization();
    int get_available_slots();
    
private:
    void move_job_to_running(std::shared_ptr<MapReduceAlgorithm> algorithm);
    void move_job_to_completed(std::shared_ptr<MapReduceAlgorithm> algorithm);
    std::shared_ptr<MapReduceAlgorithm> find_job_by_id(const std::string& job_id);
};

// Factory for creating MapReduce algorithms
class MapReduceAlgorithmFactory {
public:
    static std::shared_ptr<MapReduceAlgorithm> create_algorithm(
        MapReduceJobType type,
        const MapReduceJobConfig& config,
        std::shared_ptr<dds::storage::HadoopStorage> storage,
        std::shared_ptr<dds::storage::HadoopJobManager> job_manager);
    
    static MapReduceJobConfig create_linear_regression_config(
        const std::string& input_path,
        const std::string& output_path,
        double learning_rate = 0.01,
        int max_iterations = 1000);
    
    static MapReduceJobConfig create_kmeans_config(
        const std::string& input_path,
        const std::string& output_path,
        int k = 3,
        int max_iterations = 100);
};

} // namespace algorithms
} // namespace dds 