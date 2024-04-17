#pragma once

#include "utils/types.h"
#include "../utils/eigen_stub.h"
#include <vector>
#include <functional>
#include <string>

namespace dds {

// Forward declarations
class MPICommunicator;

struct LogisticRegressionParams {
    double learning_rate = 0.01;
    double tolerance = 1e-6;
    int max_iterations = 100;
    bool use_regularization = false;
    double lambda = 0.1;  // Regularization strength
    RegularizationType reg_type = RegularizationType::L2;
    OptimizerType optimizer = OptimizerType::SGD;
    double momentum = 0.9;  // For momentum-based optimizers
    double beta1 = 0.9;     // For Adam optimizer
    double beta2 = 0.999;   // For Adam optimizer
    double epsilon = 1e-8;  // For Adam optimizer
};

struct TrainingRecord {
    int iteration;
    double loss;
    double accuracy;
    double weight_norm;
    double gradient_norm;
};

class LogisticRegression {
public:
    LogisticRegression();
    ~LogisticRegression();
    
    // Initialization
    void initialize(const LogisticRegressionParams& params);
    bool is_initialized() const { return initialized_; }
    
    // Training
    bool train(const Matrix& X, const Vector& y, int max_iterations = 100);
    bool train_distributed(const std::vector<Matrix>& X_partitions,
                           const std::vector<Vector>& y_partitions,
                           int max_iterations = 100);
    
    // Prediction
    Vector predict(const Matrix& X) const;
    Vector predict_proba(const Matrix& X) const;
    std::vector<int> predict_classes(const Matrix& X, double threshold = 0.5) const;
    
    // Evaluation
    double compute_loss(const Matrix& X, const Vector& y) const;
    double compute_accuracy(const Matrix& X, const Vector& y) const;
    double compute_precision(const Matrix& X, const Vector& y) const;
    double compute_recall(const Matrix& X, const Vector& y) const;
    double compute_f1_score(const Matrix& X, const Vector& y) const;
    double compute_auc(const Matrix& X, const Vector& y) const;
    
    // Model management
    const Vector& get_weights() const { return weights_; }
    double get_bias() const { return bias_; }
    const LogisticRegressionParams& get_params() const { return params_; }
    const std::vector<TrainingRecord>& get_training_history() const { return training_history_; }
    
    // Model persistence
    bool save_model(const std::string& filepath) const;
    bool load_model(const std::string& filepath);
    
    // Distributed training helpers
    bool compute_gradients(const Matrix& X, const Vector& y, Vector& weight_grad, double& bias_grad);
    bool update_parameters(const Vector& weight_grad, double bias_grad, double learning_rate);
    bool check_convergence(const Vector& prev_weights, double prev_bias, double tolerance) const;
    bool broadcast_parameters(int root = 0);
    bool reduce_gradients(const Vector& local_weight_grad, double local_bias_grad,
                          Vector& global_weight_grad, double& global_bias_grad,
                          int root = 0);
    
    // Performance metrics
    double get_training_time() const { return training_time_; }
    double get_prediction_time() const { return prediction_time_; }
    int get_num_iterations() const { return training_history_.size(); }

private:
    bool initialized_;
    LogisticRegressionParams params_;
    Vector weights_;
    double bias_;
    std::vector<TrainingRecord> training_history_;
    double training_time_;
    double prediction_time_;
    
    // For momentum-based optimizers
    Vector weight_velocity_;
    double bias_velocity_;
    
    // For Adam optimizer
    Vector weight_m_;
    Vector weight_v_;
    double bias_m_;
    double bias_v_;
    int timestep_;
    
    // Helper methods
    double sigmoid(double z) const;
    Vector sigmoid(const Vector& z) const;
    double log_likelihood(const Matrix& X, const Vector& y) const;
    double compute_regularization_loss() const;
    void update_optimizer_state(const Vector& weight_grad, double bias_grad, double learning_rate);
    void reset_optimizer_state();
};

// Utility functions for logistic regression
namespace logistic_regression_utils {
    
    // Data preprocessing
    Matrix normalize_features(const Matrix& X);
    Vector add_bias_term(const Matrix& X);
    std::pair<Matrix, Vector> balance_dataset(const Matrix& X, const Vector& y);
    
    // Evaluation metrics
    double binary_cross_entropy(const Vector& y_true, const Vector& y_pred);
    double accuracy_score(const Vector& y_true, const Vector& y_pred);
    double precision_score(const Vector& y_true, const Vector& y_pred);
    double recall_score(const Vector& y_true, const Vector& y_pred);
    double f1_score(const Vector& y_true, const Vector& y_pred);
    double roc_auc_score(const Vector& y_true, const Vector& y_pred);
    
    // Cross-validation
    std::vector<double> cross_validate(const Matrix& X, const Vector& y, 
                                       const LogisticRegressionParams& params,
                                       int n_folds = 5, int random_state = 42);
    
    // Hyperparameter tuning
    LogisticRegressionParams grid_search(const Matrix& X, const Vector& y,
                                         const std::vector<double>& learning_rates,
                                         const std::vector<double>& lambdas,
                                         int n_folds = 5);
    
    // Feature selection
    std::vector<int> select_features(const Matrix& X, const Vector& y,
                                     const LogisticRegressionParams& params,
                                     double threshold = 0.01);
    
    // Data splitting
    std::tuple<Matrix, Matrix, Vector, Vector> train_test_split(
        const Matrix& X, const Vector& y, double test_size = 0.2, int random_state = 42);
    
    // Confusion matrix
    struct ConfusionMatrix {
        int true_positives;
        int true_negatives;
        int false_positives;
        int false_negatives;
        
        double accuracy() const;
        double precision() const;
        double recall() const;
        double f1_score() const;
    };
    
    ConfusionMatrix compute_confusion_matrix(const Vector& y_true, const Vector& y_pred);
    
    // ROC curve
    struct ROCPoint {
        double false_positive_rate;
        double true_positive_rate;
        double threshold;
    };
    
    std::vector<ROCPoint> compute_roc_curve(const Vector& y_true, const Vector& y_pred_proba);
    
} // namespace logistic_regression_utils

} // namespace dds 