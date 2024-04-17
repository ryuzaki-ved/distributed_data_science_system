#include "algorithms/linear_regression.h"
#include "communication/mpi_communicator.h"
#include "utils/types.h"
#include "../../include/utils/eigen_stub.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <random>

namespace dds {

LinearRegression::LinearRegression() 
    : initialized_(false), 
      params_(),
      weights_(),
      bias_(0.0),
      training_history_() {
}

LinearRegression::~LinearRegression() {
}

void LinearRegression::initialize(const LinearRegressionParams& params) {
    params_ = params;
    initialized_ = true;
    training_history_.clear();
}

bool LinearRegression::train(const Matrix& X, const Vector& y, int max_iterations) {
    if (!initialized_) {
        std::cerr << "LinearRegression not initialized" << std::endl;
        return false;
    }
    
    if (X.rows() != y.size()) {
        std::cerr << "X and y dimensions don't match" << std::endl;
        return false;
    }
    
    // Initialize parameters
    int num_features = X.cols();
    weights_ = Vector::Zero(num_features);
    bias_ = 0.0;
    
    training_history_.clear();
    training_history_.reserve(max_iterations);
    
    // Training loop
    for (int iteration = 0; iteration < max_iterations; ++iteration) {
        Vector weight_grad;
        double bias_grad;
        
        // Compute gradients
        if (!compute_gradients(X, y, weight_grad, bias_grad)) {
            return false;
        }
        
        // Update parameters
        if (!update_parameters(weight_grad, bias_grad, params_.learning_rate)) {
            return false;
        }
        
        // Record training history
        double loss = compute_loss(X, y);
        training_history_.push_back({iteration, loss, weights_.norm()});
        
        // Check convergence
        if (iteration > 0 && check_convergence(prev_weights_, prev_bias_, params_.tolerance)) {
            std::cout << "Converged at iteration " << iteration << std::endl;
            break;
        }
        
        prev_weights_ = weights_;
        prev_bias_ = bias_;
    }
    
    return true;
}

bool LinearRegression::train_distributed(const std::vector<Matrix>& X_partitions,
                                         const std::vector<Vector>& y_partitions,
                                         int max_iterations) {
    if (!initialized_ || !g_mpi_communicator) {
        std::cerr << "LinearRegression not initialized or MPI not available" << std::endl;
        return false;
    }
    
    int rank = g_mpi_communicator->get_rank();
    int size = g_mpi_communicator->get_size();
    
    // Initialize parameters on master
    if (rank == 0) {
        if (X_partitions.empty() || y_partitions.empty()) {
            std::cerr << "Empty partitions provided" << std::endl;
            return false;
        }
        int num_features = X_partitions[0].cols();
        weights_ = Vector::Zero(num_features);
        bias_ = 0.0;
    }
    
    // Broadcast initial parameters
    if (!broadcast_parameters(0)) {
        return false;
    }
    
    training_history_.clear();
    training_history_.reserve(max_iterations);
    
    // Distributed training loop
    for (int iteration = 0; iteration < max_iterations; ++iteration) {
        Vector local_weight_grad = Vector::Zero(weights_.size());
        double local_bias_grad = 0.0;
        
        // Compute local gradients on each node
        for (size_t i = 0; i < X_partitions.size(); ++i) {
            Vector partition_weight_grad;
            double partition_bias_grad;
            
            if (!compute_gradients(X_partitions[i], y_partitions[i], 
                                  partition_weight_grad, partition_bias_grad)) {
                return false;
            }
            
            local_weight_grad += partition_weight_grad;
            local_bias_grad += partition_bias_grad;
        }
        
        // Reduce gradients across all nodes
        Vector global_weight_grad;
        double global_bias_grad;
        if (!reduce_gradients(local_weight_grad, local_bias_grad,
                             global_weight_grad, global_bias_grad, 0)) {
            return false;
        }
        
        // Update parameters on master
        if (rank == 0) {
            if (!update_parameters(global_weight_grad, global_bias_grad, params_.learning_rate)) {
                return false;
            }
            
            // Record training history
            double total_loss = 0.0;
            for (size_t i = 0; i < X_partitions.size(); ++i) {
                total_loss += compute_loss(X_partitions[i], y_partitions[i]);
            }
            total_loss /= X_partitions.size();
            
            training_history_.push_back({iteration, total_loss, weights_.norm()});
            
            // Check convergence
            if (iteration > 0 && check_convergence(prev_weights_, prev_bias_, params_.tolerance)) {
                std::cout << "Converged at iteration " << iteration << std::endl;
                break;
            }
            
            prev_weights_ = weights_;
            prev_bias_ = bias_;
        }
        
        // Broadcast updated parameters
        if (!broadcast_parameters(0)) {
            return false;
        }
    }
    
    return true;
}

Vector LinearRegression::predict(const Matrix& X) const {
    if (!initialized_) {
        std::cerr << "LinearRegression not initialized" << std::endl;
        return Vector();
    }
    
    return X * weights_ + Vector::Constant(X.rows(), bias_);
}

double LinearRegression::compute_loss(const Matrix& X, const Vector& y) const {
    if (!initialized_) return std::numeric_limits<double>::infinity();
    
    Vector predictions = predict(X);
    Vector residuals = predictions - y;
    
    if (params_.loss_type == LossType::MSE) {
        return residuals.squaredNorm() / y.size();
    } else if (params_.loss_type == LossType::MAE) {
        return residuals.cwiseAbs().sum() / y.size();
    }
    
    return std::numeric_limits<double>::infinity();
}

double LinearRegression::compute_accuracy(const Matrix& X, const Vector& y, double threshold) const {
    if (!initialized_) return 0.0;
    
    Vector predictions = predict(X);
    int correct = 0;
    
    for (int i = 0; i < y.size(); ++i) {
        if (std::abs(predictions(i) - y(i)) <= threshold) {
            correct++;
        }
    }
    
    return static_cast<double>(correct) / y.size();
}

double LinearRegression::compute_r_squared(const Matrix& X, const Vector& y) const {
    if (!initialized_) return 0.0;
    
    Vector predictions = predict(X);
    double mean_y = y.mean();
    
    double ss_res = (y - predictions).squaredNorm();
    double ss_tot = (y - Vector::Constant(y.size(), mean_y)).squaredNorm();
    
    if (ss_tot == 0.0) return 1.0;
    return 1.0 - (ss_res / ss_tot);
}

bool LinearRegression::compute_gradients(const Matrix& X, const Vector& y, 
                                         Vector& weight_grad, double& bias_grad) {
    if (!initialized_) return false;
    
    Vector predictions = predict(X);
    Vector residuals = predictions - y;
    int n = y.size();
    
    // Compute gradients
    weight_grad = (2.0 / n) * X.transpose() * residuals;
    bias_grad = (2.0 / n) * residuals.sum();
    
    return true;
}

bool LinearRegression::update_parameters(const Vector& weight_grad, double bias_grad, 
                                         double learning_rate) {
    if (!initialized_) return false;
    
    weights_ -= learning_rate * weight_grad;
    bias_ -= learning_rate * bias_grad;
    
    return true;
}

bool LinearRegression::check_convergence(const Vector& prev_weights, double prev_bias, 
                                         double tolerance) const {
    if (!initialized_) return false;
    
    double weight_diff = (weights_ - prev_weights).norm();
    double bias_diff = std::abs(bias_ - prev_bias);
    
    return weight_diff < tolerance && bias_diff < tolerance;
}

bool LinearRegression::broadcast_parameters(int root) {
    if (!g_mpi_communicator) return false;
    
    int rank = g_mpi_communicator->get_rank();
    
    if (rank == root) {
        // Broadcast weights
        if (!g_mpi_communicator->broadcast_matrix(weights_, root)) {
            return false;
        }
        
        // Broadcast bias
        if (!g_mpi_communicator->broadcast(&bias_, 1, MPI_DOUBLE, root)) {
            return false;
        }
    } else {
        // Receive weights
        if (!g_mpi_communicator->broadcast_matrix(weights_, root)) {
            return false;
        }
        
        // Receive bias
        if (!g_mpi_communicator->broadcast(&bias_, 1, MPI_DOUBLE, root)) {
            return false;
        }
    }
    
    return true;
}

bool LinearRegression::reduce_gradients(const Vector& local_weight_grad, double local_bias_grad,
                                        Vector& global_weight_grad, double& global_bias_grad,
                                        int root) {
    if (!g_mpi_communicator) return false;
    
    // Reduce weight gradients
    if (!g_mpi_communicator->all_reduce_matrix(local_weight_grad, global_weight_grad, MPI_SUM)) {
        return false;
    }
    
    // Reduce bias gradient
    if (!g_mpi_communicator->all_reduce(&local_bias_grad, &global_bias_grad, 1, MPI_DOUBLE, MPI_SUM, root)) {
        return false;
    }
    
    return true;
}

// Utility functions
namespace linear_regression_utils {

Matrix normalize_features(const Matrix& X) {
    Matrix normalized = X;
    Vector mean = X.colwise().mean();
    Vector std_dev = ((X.rowwise() - mean.transpose()).array().square().colwise().mean()).sqrt();
    
    for (int i = 0; i < X.cols(); ++i) {
        if (std_dev(i) > 1e-8) {
            normalized.col(i) = (X.col(i) - Vector::Constant(X.rows(), mean(i))) / std_dev(i);
        }
    }
    
    return normalized;
}

Vector add_bias_term(const Matrix& X) {
    Matrix X_with_bias(X.rows(), X.cols() + 1);
    X_with_bias << X, Vector::Ones(X.rows());
    return X_with_bias;
}

double mean_squared_error(const Vector& y_true, const Vector& y_pred) {
    return (y_true - y_pred).squaredNorm() / y_true.size();
}

double mean_absolute_error(const Vector& y_true, const Vector& y_pred) {
    return (y_true - y_pred).cwiseAbs().sum() / y_true.size();
}

double r_squared_score(const Vector& y_true, const Vector& y_pred) {
    double mean_y = y_true.mean();
    double ss_res = (y_true - y_pred).squaredNorm();
    double ss_tot = (y_true - Vector::Constant(y_true.size(), mean_y)).squaredNorm();
    
    if (ss_tot == 0.0) return 1.0;
    return 1.0 - (ss_res / ss_tot);
}

std::pair<Matrix, Vector> train_test_split(const Matrix& X, const Vector& y, 
                                           double test_size, int random_state) {
    int n_samples = X.rows();
    int n_test = static_cast<int>(n_samples * test_size);
    int n_train = n_samples - n_test;
    
    // Create indices
    std::vector<int> indices(n_samples);
    std::iota(indices.begin(), indices.end(), 0);
    
    // Shuffle indices
    if (random_state >= 0) {
        std::mt19937 gen(random_state);
        std::shuffle(indices.begin(), indices.end(), gen);
    } else {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(indices.begin(), indices.end(), gen);
    }
    
    // Split data
    Matrix X_train(n_train, X.cols());
    Matrix X_test(n_test, X.cols());
    Vector y_train(n_train);
    Vector y_test(n_test);
    
    for (int i = 0; i < n_train; ++i) {
        X_train.row(i) = X.row(indices[i]);
        y_train(i) = y(indices[i]);
    }
    
    for (int i = 0; i < n_test; ++i) {
        X_test.row(i) = X.row(indices[n_train + i]);
        y_test(i) = y(indices[n_train + i]);
    }
    
    return {X_train, X_test};
}

} // namespace linear_regression_utils

} // namespace dds 