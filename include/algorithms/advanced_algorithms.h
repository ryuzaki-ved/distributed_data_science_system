#pragma once

#include "../utils/types.h"
#include "../utils/eigen_stub.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <random>

namespace dds {
namespace algorithms {

// Forward declarations
class DecisionTree;

// Neural Network Layer Types
enum class LayerType {
    DENSE,
    CONVOLUTIONAL,
    LSTM,
    GRU,
    DROPOUT,
    BATCH_NORMALIZATION,
    ACTIVATION
};

// Activation Functions
enum class ActivationType {
    RELU,
    SIGMOID,
    TANH,
    SOFTMAX,
    LEAKY_RELU,
    ELU,
    SWISH,
    GELU,
    MISH,
    SELU,
    HARD_SIGMOID,
    HARD_SWISH
};

// Neural Network Layer
class NeuralLayer {
protected:
    LayerType type_;
    int input_size_;
    int output_size_;
    Matrix weights_;
    Vector biases_;
    Matrix activations_;
    Matrix gradients_;
    ActivationType activation_;
    
    // Cache for backward pass
    Matrix input_cache_;
    Matrix linear_cache_;

public:
    NeuralLayer(LayerType type, int input_size, int output_size, ActivationType activation = ActivationType::RELU);
    virtual ~NeuralLayer() = default;
    
    // Forward and backward propagation
    virtual Matrix forward(const Matrix& input);
    virtual Matrix backward(const Matrix& gradient);
    
    // Parameter management
    virtual void initialize_weights(double std_dev = 0.01);
    virtual void update_weights(double learning_rate);
    virtual void zero_gradients();
    
    // Getters
    LayerType get_type() const { return type_; }
    int get_input_size() const { return input_size_; }
    int get_output_size() const { return output_size_; }
    const Matrix& get_weights() const { return weights_; }
    const Vector& get_biases() const { return biases_; }
    
    // Activation functions
    static Matrix relu(const Matrix& x);
    static Matrix sigmoid(const Matrix& x);
    static Matrix tanh(const Matrix& x);
    static Matrix softmax(const Matrix& x);
    static Matrix leaky_relu(const Matrix& x, double alpha = 0.01);
    static Matrix elu(const Matrix& x, double alpha = 1.0);
    static Matrix swish(const Matrix& x, double beta = 1.0);
    static Matrix gelu(const Matrix& x);
    static Matrix mish(const Matrix& x);
    static Matrix selu(const Matrix& x);
    static Matrix hard_sigmoid(const Matrix& x);
    static Matrix hard_swish(const Matrix& x);
    
    // Activation derivatives
    static Matrix relu_derivative(const Matrix& x);
    static Matrix sigmoid_derivative(const Matrix& x);
    static Matrix tanh_derivative(const Matrix& x);
    static Matrix softmax_derivative(const Matrix& x);
    static Matrix leaky_relu_derivative(const Matrix& x, double alpha = 0.01);
    static Matrix elu_derivative(const Matrix& x, double alpha = 1.0);
    static Matrix swish_derivative(const Matrix& x, double beta = 1.0);
    static Matrix gelu_derivative(const Matrix& x);
    static Matrix mish_derivative(const Matrix& x);
    static Matrix selu_derivative(const Matrix& x);
    static Matrix hard_sigmoid_derivative(const Matrix& x);
    static Matrix hard_swish_derivative(const Matrix& x);
};

// Dense Layer Implementation
class DenseLayer : public NeuralLayer {
public:
    DenseLayer(int input_size, int output_size, ActivationType activation = ActivationType::RELU);
    
    Matrix forward(const Matrix& input) override;
    Matrix backward(const Matrix& gradient) override;
    void initialize_weights(double std_dev = 0.01) override;
};

// Dropout Layer
class DropoutLayer : public NeuralLayer {
private:
    double dropout_rate_;
    Eigen::MatrixXd mask_;

public:
    DropoutLayer(double dropout_rate = 0.5);
    
    Eigen::MatrixXd forward(const Eigen::MatrixXd& input) override;
    Eigen::MatrixXd backward(const Eigen::MatrixXd& gradient) override;
};

// Neural Network
class NeuralNetwork {
private:
    std::vector<std::unique_ptr<NeuralLayer>> layers_;
    double learning_rate_;
    int batch_size_;
    int epochs_;
    std::function<double(const Matrix&, const Matrix&)> loss_function_;
    std::function<Matrix(const Matrix&, const Matrix&)> loss_derivative_;

public:
    NeuralNetwork(double learning_rate = 0.01, int batch_size = 32);
    
    // Layer management
    void add_layer(std::unique_ptr<NeuralLayer> layer);
    void add_dense_layer(int units, ActivationType activation = ActivationType::RELU);
    void add_dropout_layer(double rate = 0.5);
    
    // Training
    void fit(const Matrix& X, const Matrix& y, int epochs = 100);
    Matrix predict(const Matrix& X);
    double evaluate(const Matrix& X, const Matrix& y);
    
    // Loss functions
    void set_loss_function(const std::string& loss_type);
    static double mse_loss(const Matrix& y_true, const Matrix& y_pred);
    static double cross_entropy_loss(const Matrix& y_true, const Matrix& y_pred);
    static Matrix mse_derivative(const Matrix& y_true, const Matrix& y_pred);
    static Matrix cross_entropy_derivative(const Matrix& y_true, const Matrix& y_pred);
    
    // Model persistence
    bool save_model(const std::string& filepath);
    bool load_model(const std::string& filepath);
    
    // Configuration
    void set_learning_rate(double lr) { learning_rate_ = lr; }
    void set_batch_size(int batch_size) { batch_size_ = batch_size; }
    
private:
    Matrix forward_pass(const Matrix& input);
    void backward_pass(const Matrix& input, const Matrix& target);
    void update_parameters();
    std::vector<std::pair<Matrix, Matrix>> create_batches(const Matrix& X, const Matrix& y);
};

// Ensemble Methods
class RandomForest {
private:
    int n_estimators_;
    int max_depth_;
    int min_samples_split_;
    int min_samples_leaf_;
    std::vector<std::unique_ptr<DecisionTree>> trees_;
    std::mt19937 rng_;

public:
    RandomForest(int n_estimators = 100, int max_depth = 10, 
                int min_samples_split = 2, int min_samples_leaf = 1);
    
    void fit(const Matrix& X, const Vector& y);
    Vector predict(const Matrix& X);
    double evaluate(const Matrix& X, const Vector& y);
    
    // Feature importance
    Vector get_feature_importance() const;
    
private:
    std::vector<int> bootstrap_sample_indices(int n_samples);
};

// Decision Tree for Random Forest
class DecisionTree {
private:
    struct Node {
        bool is_leaf;
        int feature_index;
        double threshold;
        double prediction;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
    };
    
    std::unique_ptr<Node> root_;
    int max_depth_;
    int min_samples_split_;
    int min_samples_leaf_;

public:
    DecisionTree(int max_depth = 10, int min_samples_split = 2, int min_samples_leaf = 1);
    
    void fit(const Matrix& X, const Vector& y);
    Vector predict(const Matrix& X);
    
private:
    std::unique_ptr<Node> build_tree(const Matrix& X, const Vector& y, int depth);
    double find_best_split(const Matrix& X, const Vector& y, int& best_feature, double& best_threshold);
    double calculate_gini(const Vector& y);
    double predict_single(const Vector& x, const Node* node);
};

// Gradient Boosting
class GradientBoosting {
private:
    int n_estimators_;
    double learning_rate_;
    int max_depth_;
    std::vector<std::unique_ptr<DecisionTree>> trees_;
    Vector initial_prediction_;

public:
    GradientBoosting(int n_estimators = 100, double learning_rate = 0.1, int max_depth = 3);
    
    void fit(const Matrix& X, const Vector& y);
    Vector predict(const Matrix& X);
    double evaluate(const Matrix& X, const Vector& y);
    
private:
    Vector calculate_gradients(const Vector& y_true, const Vector& y_pred);
};

// Support Vector Machine
class SVM {
private:
    double C_;
    double epsilon_;
    std::string kernel_;
    Vector alphas_;
    Vector support_vectors_;
    double b_;
    bool trained_;

public:
    SVM(double C = 1.0, double epsilon = 0.001, const std::string& kernel = "rbf");
    
    void fit(const Matrix& X, const Vector& y);
    Vector predict(const Matrix& X);
    double evaluate(const Matrix& X, const Vector& y);
    
private:
    double kernel_function(const Vector& x1, const Vector& x2);
    double rbf_kernel(const Vector& x1, const Vector& x2, double gamma = 1.0);
    double linear_kernel(const Vector& x1, const Vector& x2);
    double polynomial_kernel(const Vector& x1, const Vector& x2, int degree = 3);
};

// Principal Component Analysis
class PCA {
private:
    int n_components_;
    Matrix components_;
    Vector explained_variance_;
    Vector mean_;
    bool fitted_;

public:
    PCA(int n_components = 2);
    
    void fit(const Matrix& X);
    Matrix transform(const Matrix& X);
    Matrix inverse_transform(const Matrix& X_transformed);
    double explained_variance_ratio(int component) const;
    Vector get_explained_variance_ratio() const;
    
private:
    void compute_eigenvalues_eigenvectors(const Matrix& covariance_matrix);
};

// Autoencoder for Dimensionality Reduction
class Autoencoder {
private:
    NeuralNetwork encoder_;
    NeuralNetwork decoder_;
    int encoding_dim_;
    double reconstruction_loss_;

public:
    Autoencoder(int input_dim, int encoding_dim, const std::vector<int>& hidden_layers = {64, 32});
    
    void fit(const Matrix& X, int epochs = 100);
    Matrix encode(const Matrix& X);
    Matrix decode(const Matrix& encoded);
    Matrix reconstruct(const Matrix& X);
    double get_reconstruction_loss() const { return reconstruction_loss_; }
    
private:
    void build_networks(int input_dim, int encoding_dim, const std::vector<int>& hidden_layers);
};

// Model Factory
class ModelFactory {
public:
    static std::unique_ptr<NeuralNetwork> create_neural_network(const std::vector<int>& layers, 
                                                               double learning_rate = 0.01);
    static std::unique_ptr<RandomForest> create_random_forest(int n_estimators = 100, 
                                                             int max_depth = 10);
    static std::unique_ptr<GradientBoosting> create_gradient_boosting(int n_estimators = 100, 
                                                                     double learning_rate = 0.1);
    static std::unique_ptr<SVM> create_svm(double C = 1.0, const std::string& kernel = "rbf");
    static std::unique_ptr<PCA> create_pca(int n_components = 2);
    static std::unique_ptr<Autoencoder> create_autoencoder(int input_dim, int encoding_dim);
};

} // namespace algorithms
} // namespace dds 