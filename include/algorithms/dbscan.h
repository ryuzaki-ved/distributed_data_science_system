#pragma once

#include "utils/types.h"
#include <Eigen/Dense>
#include <vector>
#include <unordered_set>
#include <queue>
#include <string>

namespace dds {

// Forward declarations
class MPICommunicator;

struct DBSCANParams {
    double epsilon = 0.5;         // Neighborhood radius
    int min_points = 5;           // Minimum points for core point
    DistanceMetric distance_metric = DistanceMetric::EUCLIDEAN;
    bool use_approximate_neighbors = false;  // Use approximate nearest neighbors
    int approximate_neighbors_k = 10;        // k for approximate neighbors
    bool verbose = false;         // Verbose output
    int random_state = 42;        // Random seed for approximate methods
};

struct DBSCANPoint {
    int point_id;
    Vector coordinates;
    int cluster_id;
    bool is_core_point;
    bool is_noise;
    std::vector<int> neighbors;
};

struct DBSCANResult {
    std::vector<int> labels;
    std::vector<DBSCANPoint> points;
    int num_clusters;
    int num_noise_points;
    std::vector<std::vector<int>> cluster_points;
    std::vector<Vector> cluster_centroids;
    double epsilon;
    int min_points;
    bool converged;
    std::vector<double> cluster_densities;
    std::vector<double> cluster_radii;
};

class DBSCAN {
public:
    DBSCAN();
    ~DBSCAN();
    
    // Initialization
    void initialize(const DBSCANParams& params);
    bool is_initialized() const { return initialized_; }
    
    // Training
    bool fit(const Matrix& X);
    bool fit_distributed(const std::vector<Matrix>& X_partitions);
    
    // Prediction
    std::vector<int> predict(const Matrix& X) const;
    std::vector<int> predict_distributed(const std::vector<Matrix>& X_partitions) const;
    
    // Results
    const DBSCANResult& get_result() const { return result_; }
    const std::vector<int>& get_labels() const { return result_.labels; }
    int get_num_clusters() const { return result_.num_clusters; }
    int get_num_noise_points() const { return result_.num_noise_points; }
    const std::vector<DBSCANPoint>& get_points() const { return result_.points; }
    
    // Model management
    const DBSCANParams& get_params() const { return params_; }
    bool save_model(const std::string& filepath) const;
    bool load_model(const std::string& filepath);
    
    // Core algorithm methods
    std::vector<int> find_neighbors(const Vector& point, const Matrix& X, double epsilon);
    bool is_core_point(const std::vector<int>& neighbors, int min_points);
    std::vector<int> expand_cluster(const Matrix& X, int point_id, int cluster_id, 
                                   std::vector<bool>& visited, std::vector<bool>& clustered);
    
    // Distributed processing helpers
    bool merge_clusters_across_partitions(const std::vector<DBSCANResult>& partition_results);
    bool find_boundary_points(const Matrix& X, std::vector<int>& boundary_points);
    bool exchange_boundary_points(const std::vector<int>& local_boundary_points,
                                  std::vector<std::vector<int>>& all_boundary_points);
    bool resolve_cluster_conflicts(const std::vector<DBSCANResult>& partition_results);
    
    // Performance metrics
    double get_training_time() const { return training_time_; }
    double get_prediction_time() const { return prediction_time_; }
    double get_silhouette_score(const Matrix& X) const;
    double get_calinski_harabasz_score(const Matrix& X) const;
    double get_davies_bouldin_score(const Matrix& X) const;

private:
    bool initialized_;
    DBSCANParams params_;
    DBSCANResult result_;
    double training_time_;
    double prediction_time_;
    
    // Helper methods
    double compute_distance(const Vector& p1, const Vector& p2) const;
    std::vector<std::vector<int>> build_neighborhood_graph(const Matrix& X);
    std::vector<int> find_connected_components(const std::vector<std::vector<int>>& graph);
    bool validate_data(const Matrix& X) const;
    void reset_state();
    std::vector<int> approximate_nearest_neighbors(const Vector& point, const Matrix& X, int k);
};

// Utility functions for DBSCAN clustering
namespace dbscan_utils {
    
    // Distance metrics
    double euclidean_distance(const Vector& p1, const Vector& p2);
    double manhattan_distance(const Vector& p1, const Vector& p2);
    double cosine_distance(const Vector& p1, const Vector& p2);
    double chebyshev_distance(const Vector& p1, const Vector& p2);
    double minkowski_distance(const Vector& p1, const Vector& p2, double p);
    
    // Parameter selection
    struct ParameterSuggestion {
        double suggested_epsilon;
        int suggested_min_points;
        double confidence_score;
        std::vector<double> k_distances;
        std::vector<double> reachability_distances;
    };
    
    ParameterSuggestion suggest_parameters(const Matrix& X, int k_neighbors = 4);
    double estimate_epsilon_knee(const Matrix& X, int k_neighbors = 4);
    int estimate_min_points(const Matrix& X, double percentile = 0.95);
    
    // Evaluation metrics
    double silhouette_score(const Matrix& X, const std::vector<int>& labels);
    double calinski_harabasz_score(const Matrix& X, const std::vector<int>& labels);
    double davies_bouldin_score(const Matrix& X, const std::vector<int>& labels);
    double cluster_cohesion(const Matrix& X, const std::vector<int>& labels);
    double cluster_separation(const Matrix& X, const std::vector<int>& labels);
    
    // Data preprocessing
    Matrix normalize_features(const Matrix& X);
    Matrix standardize_features(const Matrix& X);
    Matrix remove_outliers(const Matrix& X, double threshold = 3.0);
    Matrix reduce_dimensionality(const Matrix& X, int n_components = 2);
    
    // Cluster analysis
    struct ClusterAnalysis {
        int cluster_id;
        int num_points;
        Vector centroid;
        double density;
        double radius;
        double diameter;
        std::vector<int> point_indices;
        std::vector<int> boundary_points;
        bool is_stable;
    };
    
    std::vector<ClusterAnalysis> analyze_clusters(const Matrix& X, const std::vector<int>& labels);
    
    // Noise analysis
    struct NoiseAnalysis {
        int num_noise_points;
        double noise_percentage;
        std::vector<int> noise_point_indices;
        std::vector<double> noise_point_distances;
        double average_noise_distance;
    };
    
    NoiseAnalysis analyze_noise(const Matrix& X, const std::vector<int>& labels, double epsilon);
    
    // Visualization helpers
    struct DBSCANVisualization {
        Matrix reduced_data;
        std::vector<int> labels;
        std::vector<Vector> cluster_centroids;
        std::vector<double> cluster_radii;
        std::vector<std::string> colors;
        std::vector<bool> is_noise;
    };
    
    DBSCANVisualization prepare_visualization(const Matrix& X, const std::vector<int>& labels,
                                             const std::vector<Vector>& centroids,
                                             const std::vector<double>& radii,
                                             int n_components = 2);
    
    // Data partitioning for distributed processing
    std::vector<Matrix> partition_data_with_overlap(const Matrix& X, int num_partitions, 
                                                   double overlap_ratio = 0.1);
    
    // Cluster merging
    bool should_merge_clusters(const Vector& centroid1, const Vector& centroid2,
                              double radius1, double radius2, double epsilon);
    std::vector<std::vector<int>> merge_overlapping_clusters(const std::vector<Vector>& centroids,
                                                             const std::vector<double>& radii,
                                                             double epsilon);
    
    // Performance optimization
    struct SpatialIndex {
        std::vector<std::vector<int>> grid;
        Vector grid_min;
        Vector grid_max;
        Vector grid_size;
        int grid_dimensions;
    };
    
    SpatialIndex build_spatial_index(const Matrix& X, double epsilon);
    std::vector<int> query_spatial_index(const SpatialIndex& index, const Vector& point, double epsilon);
    
    // Stability analysis
    struct StabilityAnalysis {
        std::vector<double> stability_scores;
        std::vector<bool> stable_clusters;
        double overall_stability;
        std::vector<int> recommended_parameters;
    };
    
    StabilityAnalysis analyze_stability(const Matrix& X, 
                                       const std::vector<double>& epsilons,
                                       const std::vector<int>& min_points_values);
    
} // namespace dbscan_utils

} // namespace dds 