#pragma once

#include "utils/types.h"
#include "communication/mpi_communicator.h"
#include <Eigen/Dense>

namespace dds {

class LinearRegression {
public:
    LinearRegression();
    ~LinearRegression();
    
    // Initialize algorithm with parameters
    void initialize(const LinearRegressionParams& params);
    
    // Training methods
    bool train(const Matrix& X, const Vector& y, int max_iterations = 100);
    bool train_distributed(const std::vector<Matrix>& X_partitions, 
                          const std::vector<Vector>& y_partitions,
                          int max_iterations = 100);
    
    // Prediction
    Vector predict(const Matrix& X) const;
    double predict_single(const Vector& x) const;
    
    // Model evaluation
    double compute_loss(const Matrix& X, const Vector& y) const;
    double compute_accuracy(const Matrix& X, const Vector& y, double threshold = 0.5) const;
    double compute_r_squared(const Matrix& X, const Vector& y) const;
    
    // Model parameters
    Vector get_weights() const { return weights_; }
    double get_bias() const { return bias_; }
    void set_weights(const Vector& weights) { weights_ = weights; }
    void set_bias(double bias) { bias_ = bias; }
    
    // Training history
    struct TrainingHistory {
        std::vector<double> losses;
        std::vector<double> accuracies;
        std::vector<Vector> weight_history;
        std::vector<double> bias_history;
        std::vector<double> gradients_norm;
    };
    
    const TrainingHistory& get_training_history() const { return history_; }
    void clear_training_history() { history_.losses.clear(); history_.accuracies.clear(); 
                                   history_.weight_history.clear(); history_.bias_history.clear();
                                   history_.gradients_norm.clear(); }
    
    // Distributed training helpers
    bool compute_gradients(const Matrix& X, const Vector& y, Vector& weight_grad, double& bias_grad);
    bool update_parameters(const Vector& weight_grad, double bias_grad, double learning_rate);
    bool check_convergence(const Vector& prev_weights, double prev_bias, double tolerance) const;
    
    // MPI communication for distributed training
    bool broadcast_parameters(int root = 0);
    bool gather_gradients(const Vector& local_weight_grad, double local_bias_grad,
                         std::vector<Vector>& all_weight_grads, std::vector<double>& all_bias_grads,
                         int root = 0);
    bool reduce_gradients(const Vector& local_weight_grad, double local_bias_grad,
                         Vector& global_weight_grad, double& global_bias_grad,
                         int root = 0);
    
    // Model persistence
    bool save_model(const std::string& path);
    bool load_model(const std::string& path);
    
    // Performance metrics
    struct TrainingMetrics {
        double total_time;
        double computation_time;
        double communication_time;
        int num_iterations;
        double final_loss;
        double final_accuracy;
        double final_r_squared;
    };
    
    const TrainingMetrics& get_training_metrics() const { return metrics_; }
    void reset_training_metrics() { metrics_ = TrainingMetrics{}; }

private:
    // Model parameters
    Vector weights_;
    double bias_;
    double learning_rate_;
    int max_iterations_;
    double tolerance_;
    bool use_regularization_;
    double regularization_strength_;
    
    // Training state
    TrainingHistory history_;
    TrainingMetrics metrics_;
    
    // MPI communicator reference
    MPICommunicator* mpi_comm_;
    
    // Internal methods
    double compute_loss_single(const Matrix& X, const Vector& y) const;
    Vector compute_predictions(const Matrix& X) const;
    void update_training_history(double loss, double accuracy, double gradient_norm);
    void update_metrics(double computation_time, double communication_time);
    
    // Regularization
    double compute_regularization_loss() const;
    Vector compute_regularization_gradient() const;
    
    // Numerical stability
    bool is_numerically_stable(const Vector& weights, double bias) const;
    void clip_gradients(Vector& weight_grad, double& bias_grad, double max_norm = 1.0);
    
    // Feature scaling
    void normalize_features(Matrix& X, Vector& mean, Vector& std_dev);
    void denormalize_features(Matrix& X, const Vector& mean, const Vector& std_dev);
    void denormalize_parameters(const Vector& mean, const Vector& std_dev);
};

// Utility functions for linear regression
namespace linear_regression_utils {
    
    // Data preprocessing
    Matrix add_bias_column(const Matrix& X);
    Matrix remove_bias_column(const Matrix& X);
    std::pair<Vector, Vector> compute_feature_statistics(const Matrix& X);
    Matrix normalize_features(const Matrix& X, const Vector& mean, const Vector& std_dev);
    
    // Model evaluation
    double mean_squared_error(const Vector& y_true, const Vector& y_pred);
    double mean_absolute_error(const Vector& y_true, const Vector& y_pred);
    double root_mean_squared_error(const Vector& y_true, const Vector& y_pred);
    double r_squared_score(const Vector& y_true, const Vector& y_pred);
    
    // Cross-validation
    std::vector<std::pair<Matrix, Matrix>> split_data_cv(const Matrix& X, const Vector& y, 
                                                        int num_folds);
    double cross_validate(const Matrix& X, const Vector& y, 
                         const LinearRegressionParams& params, int num_folds = 5);
    
    // Hyperparameter tuning
    struct HyperparameterGrid {
        std::vector<double> learning_rates;
        std::vector<double> regularization_strengths;
        std::vector<int> max_iterations;
    };
    
    LinearRegressionParams grid_search(const Matrix& X, const Vector& y,
                                      const HyperparameterGrid& grid, int num_folds = 5);
    
    // Feature selection
    std::vector<int> select_features(const Matrix& X, const Vector& y, 
                                   int num_features, const std::string& method = "correlation");
    Matrix select_features_matrix(const Matrix& X, const std::vector<int>& feature_indices);
    
} // namespace linear_regression_utils

} // namespace dds 