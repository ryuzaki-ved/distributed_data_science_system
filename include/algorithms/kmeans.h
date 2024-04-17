#pragma once

#include "utils/types.h"
#include <Eigen/Dense>
#include <vector>
#include <random>
#include <string>

namespace dds {

// Forward declarations
class MPICommunicator;

struct KMeansParams {
    int k = 3;                    // Number of clusters
    int max_iterations = 100;     // Maximum iterations
    double tolerance = 1e-6;      // Convergence tolerance
    InitializationType init_method = InitializationType::KMEANS_PLUS_PLUS;
    int n_init = 10;              // Number of initializations
    int random_state = 42;        // Random seed
    bool compute_inertia = true;  // Whether to compute inertia
    bool verbose = false;         // Verbose output
};

struct ClusterInfo {
    int cluster_id;
    Vector centroid;
    int num_points;
    double inertia;
    std::vector<int> point_indices;
};

struct KMeansResult {
    std::vector<int> labels;
    Matrix centroids;
    double inertia;
    int n_iterations;
    bool converged;
    std::vector<ClusterInfo> clusters;
    std::vector<double> iteration_inertias;
};

class KMeans {
public:
    KMeans();
    ~KMeans();
    
    // Initialization
    void initialize(const KMeansParams& params);
    bool is_initialized() const { return initialized_; }
    
    // Training
    bool fit(const Matrix& X);
    bool fit_distributed(const std::vector<Matrix>& X_partitions);
    
    // Prediction
    std::vector<int> predict(const Matrix& X) const;
    std::vector<int> predict_distributed(const std::vector<Matrix>& X_partitions) const;
    
    // Results
    const KMeansResult& get_result() const { return result_; }
    const Matrix& get_centroids() const { return result_.centroids; }
    const std::vector<int>& get_labels() const { return result_.labels; }
    double get_inertia() const { return result_.inertia; }
    int get_n_iterations() const { return result_.n_iterations; }
    bool has_converged() const { return result_.converged; }
    
    // Model management
    const KMeansParams& get_params() const { return params_; }
    bool save_model(const std::string& filepath) const;
    bool load_model(const std::string& filepath);
    
    // Distributed training helpers
    bool initialize_centroids(const Matrix& X);
    bool assign_clusters(const Matrix& X, std::vector<int>& labels, double& inertia);
    bool update_centroids(const Matrix& X, const std::vector<int>& labels);
    bool check_convergence(const Matrix& prev_centroids, double tolerance) const;
    bool broadcast_centroids(int root = 0);
    bool reduce_centroids(const std::vector<Matrix>& local_centroids,
                          const std::vector<int>& local_counts,
                          Matrix& global_centroids, int root = 0);
    
    // Performance metrics
    double get_training_time() const { return training_time_; }
    double get_prediction_time() const { return prediction_time_; }
    double get_silhouette_score(const Matrix& X) const;
    double get_calinski_harabasz_score(const Matrix& X) const;
    double get_davies_bouldin_score(const Matrix& X) const;

private:
    bool initialized_;
    KMeansParams params_;
    KMeansResult result_;
    double training_time_;
    double prediction_time_;
    std::mt19937 rng_;
    
    // Helper methods
    Matrix kmeans_plus_plus_init(const Matrix& X, int k);
    Matrix random_init(const Matrix& X, int k);
    double compute_inertia(const Matrix& X, const std::vector<int>& labels, const Matrix& centroids);
    std::vector<int> find_nearest_centroids(const Matrix& X, const Matrix& centroids);
    Matrix compute_centroids(const Matrix& X, const std::vector<int>& labels, int k);
    bool validate_data(const Matrix& X) const;
    void reset_state();
};

// Utility functions for K-means clustering
namespace kmeans_utils {
    
    // Initialization methods
    Matrix kmeans_plus_plus_initialization(const Matrix& X, int k, int random_state = 42);
    Matrix random_initialization(const Matrix& X, int k, int random_state = 42);
    Matrix farthest_point_initialization(const Matrix& X, int k, int random_state = 42);
    
    // Evaluation metrics
    double silhouette_score(const Matrix& X, const std::vector<int>& labels);
    double calinski_harabasz_score(const Matrix& X, const std::vector<int>& labels);
    double davies_bouldin_score(const Matrix& X, const std::vector<int>& labels);
    double adjusted_rand_score(const std::vector<int>& labels_true, const std::vector<int>& labels_pred);
    double normalized_mutual_info_score(const std::vector<int>& labels_true, const std::vector<int>& labels_pred);
    
    // Data preprocessing
    Matrix normalize_features(const Matrix& X);
    Matrix standardize_features(const Matrix& X);
    Matrix remove_outliers(const Matrix& X, double threshold = 3.0);
    
    // Optimal k selection
    struct ElbowResult {
        std::vector<int> k_values;
        std::vector<double> inertias;
        std::vector<double> silhouette_scores;
        int optimal_k;
    };
    
    ElbowResult elbow_method(const Matrix& X, int k_min = 2, int k_max = 10, int random_state = 42);
    int optimal_k_silhouette(const Matrix& X, int k_min = 2, int k_max = 10, int random_state = 42);
    int optimal_k_gap_statistic(const Matrix& X, int k_min = 2, int k_max = 10, int n_bootstrap = 100, int random_state = 42);
    
    // Cluster analysis
    struct ClusterStatistics {
        int cluster_id;
        int num_points;
        Vector centroid;
        Vector mean;
        Vector std_dev;
        double radius;
        double density;
        std::vector<int> point_indices;
    };
    
    std::vector<ClusterStatistics> analyze_clusters(const Matrix& X, const std::vector<int>& labels, const Matrix& centroids);
    
    // Visualization helpers
    struct ClusterVisualization {
        Matrix reduced_data;  // 2D or 3D reduced data
        std::vector<int> labels;
        Matrix centroids_2d;
        std::vector<std::string> colors;
    };
    
    ClusterVisualization prepare_visualization(const Matrix& X, const std::vector<int>& labels, 
                                              const Matrix& centroids, int n_components = 2);
    
    // Data splitting for distributed processing
    std::vector<Matrix> partition_data(const Matrix& X, int num_partitions, PartitionStrategy strategy = PartitionStrategy::ROW_BASED);
    
    // Convergence analysis
    struct ConvergenceAnalysis {
        std::vector<double> inertias;
        std::vector<double> centroid_shifts;
        std::vector<int> label_changes;
        bool converged;
        int iterations_to_converge;
        double final_tolerance;
    };
    
    ConvergenceAnalysis analyze_convergence(const std::vector<double>& inertias, 
                                           const std::vector<Matrix>& centroid_history,
                                           double tolerance);
    
} // namespace kmeans_utils

} // namespace dds 