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
    ELU
};

// Neural Network Layer
class NeuralLayer {
protected:
    LayerType type_;
    int input_size_;
    int output_size_;
    Eigen::MatrixXd weights_;
    Eigen::VectorXd biases_;
    Eigen::MatrixXd activations_;
    Eigen::MatrixXd gradients_;
    ActivationType activation_;

public:
    NeuralLayer(LayerType type, int input_size, int output_size, ActivationType activation = ActivationType::RELU);
    virtual ~NeuralLayer() = default;
    
    // Forward and backward propagation
    virtual Eigen::MatrixXd forward(const Eigen::MatrixXd& input);
    virtual Eigen::MatrixXd backward(const Eigen::MatrixXd& gradient);
    
    // Parameter management
    virtual void initialize_weights(double std_dev = 0.01);
    virtual void update_weights(double learning_rate);
    virtual void zero_gradients();
    
    // Getters
    LayerType get_type() const { return type_; }
    int get_input_size() const { return input_size_; }
    int get_output_size() const { return output_size_; }
    const Eigen::MatrixXd& get_weights() const { return weights_; }
    const Eigen::VectorXd& get_biases() const { return biases_; }
    
    // Activation functions
    static Eigen::MatrixXd relu(const Eigen::MatrixXd& x);
    static Eigen::MatrixXd sigmoid(const Eigen::MatrixXd& x);
    static Eigen::MatrixXd tanh(const Eigen::MatrixXd& x);
    static Eigen::MatrixXd softmax(const Eigen::MatrixXd& x);
    static Eigen::MatrixXd leaky_relu(const Eigen::MatrixXd& x, double alpha = 0.01);
    static Eigen::MatrixXd elu(const Eigen::MatrixXd& x, double alpha = 1.0);
    
    // Activation derivatives
    static Eigen::MatrixXd relu_derivative(const Eigen::MatrixXd& x);
    static Eigen::MatrixXd sigmoid_derivative(const Eigen::MatrixXd& x);
    static Eigen::MatrixXd tanh_derivative(const Eigen::MatrixXd& x);
    static Eigen::MatrixXd softmax_derivative(const Eigen::MatrixXd& x);
    static Eigen::MatrixXd leaky_relu_derivative(const Eigen::MatrixXd& x, double alpha = 0.01);
    static Eigen::MatrixXd elu_derivative(const Eigen::MatrixXd& x, double alpha = 1.0);
};

// Dense Layer Implementation
class DenseLayer : public NeuralLayer {
public:
    DenseLayer(int input_size, int output_size, ActivationType activation = ActivationType::RELU);
    
    Eigen::MatrixXd forward(const Eigen::MatrixXd& input) override;
    Eigen::MatrixXd backward(const Eigen::MatrixXd& gradient) override;
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
    std::function<double(const Eigen::MatrixXd&, const Eigen::MatrixXd&)> loss_function_;
    std::function<Eigen::MatrixXd(const Eigen::MatrixXd&, const Eigen::MatrixXd&)> loss_derivative_;

public:
    NeuralNetwork(double learning_rate = 0.01, int batch_size = 32);
    
    // Layer management
    void add_layer(std::unique_ptr<NeuralLayer> layer);
    void add_dense_layer(int units, ActivationType activation = ActivationType::RELU);
    void add_dropout_layer(double rate = 0.5);
    
    // Training
    void fit(const Eigen::MatrixXd& X, const Eigen::MatrixXd& y, int epochs = 100);
    Eigen::MatrixXd predict(const Eigen::MatrixXd& X);
    double evaluate(const Eigen::MatrixXd& X, const Eigen::MatrixXd& y);
    
    // Loss functions
    void set_loss_function(const std::string& loss_type);
    static double mse_loss(const Eigen::MatrixXd& y_true, const Eigen::MatrixXd& y_pred);
    static double cross_entropy_loss(const Eigen::MatrixXd& y_true, const Eigen::MatrixXd& y_pred);
    static Eigen::MatrixXd mse_derivative(const Eigen::MatrixXd& y_true, const Eigen::MatrixXd& y_pred);
    static Eigen::MatrixXd cross_entropy_derivative(const Eigen::MatrixXd& y_true, const Eigen::MatrixXd& y_pred);
    
    // Model persistence
    bool save_model(const std::string& filepath);
    bool load_model(const std::string& filepath);
    
    // Configuration
    void set_learning_rate(double lr) { learning_rate_ = lr; }
    void set_batch_size(int batch_size) { batch_size_ = batch_size; }
    
private:
    Eigen::MatrixXd forward_pass(const Eigen::MatrixXd& input);
    void backward_pass(const Eigen::MatrixXd& input, const Eigen::MatrixXd& target);
    void update_parameters();
    std::vector<std::pair<Eigen::MatrixXd, Eigen::MatrixXd>> create_batches(const Eigen::MatrixXd& X, const Eigen::MatrixXd& y);
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
    
    void fit(const Eigen::MatrixXd& X, const Eigen::VectorXd& y);
    Eigen::VectorXd predict(const Eigen::MatrixXd& X);
    double evaluate(const Eigen::MatrixXd& X, const Eigen::VectorXd& y);
    
    // Feature importance
    Eigen::VectorXd get_feature_importance() const;
    
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
    
    void fit(const Eigen::MatrixXd& X, const Eigen::VectorXd& y);
    Eigen::VectorXd predict(const Eigen::MatrixXd& X);
    
private:
    std::unique_ptr<Node> build_tree(const Eigen::MatrixXd& X, const Eigen::VectorXd& y, int depth);
    double find_best_split(const Eigen::MatrixXd& X, const Eigen::VectorXd& y, int& best_feature, double& best_threshold);
    double calculate_gini(const Eigen::VectorXd& y);
    double predict_single(const Eigen::VectorXd& x, const Node* node);
};

// Gradient Boosting
class GradientBoosting {
private:
    int n_estimators_;
    double learning_rate_;
    int max_depth_;
    std::vector<std::unique_ptr<DecisionTree>> trees_;
    Eigen::VectorXd initial_prediction_;

public:
    GradientBoosting(int n_estimators = 100, double learning_rate = 0.1, int max_depth = 3);
    
    void fit(const Eigen::MatrixXd& X, const Eigen::VectorXd& y);
    Eigen::VectorXd predict(const Eigen::MatrixXd& X);
    double evaluate(const Eigen::MatrixXd& X, const Eigen::VectorXd& y);
    
private:
    Eigen::VectorXd calculate_gradients(const Eigen::VectorXd& y_true, const Eigen::VectorXd& y_pred);
};

// Support Vector Machine
class SVM {
private:
    double C_;
    double epsilon_;
    std::string kernel_;
    Eigen::VectorXd alphas_;
    Eigen::VectorXd support_vectors_;
    double b_;
    bool trained_;

public:
    SVM(double C = 1.0, double epsilon = 0.001, const std::string& kernel = "rbf");
    
    void fit(const Eigen::MatrixXd& X, const Eigen::VectorXd& y);
    Eigen::VectorXd predict(const Eigen::MatrixXd& X);
    double evaluate(const Eigen::MatrixXd& X, const Eigen::VectorXd& y);
    
private:
    double kernel_function(const Eigen::VectorXd& x1, const Eigen::VectorXd& x2);
    double rbf_kernel(const Eigen::VectorXd& x1, const Eigen::VectorXd& x2, double gamma = 1.0);
    double linear_kernel(const Eigen::VectorXd& x1, const Eigen::VectorXd& x2);
    double polynomial_kernel(const Eigen::VectorXd& x1, const Eigen::VectorXd& x2, int degree = 3);
};

// Principal Component Analysis
class PCA {
private:
    int n_components_;
    Eigen::MatrixXd components_;
    Eigen::VectorXd explained_variance_;
    Eigen::VectorXd mean_;
    bool fitted_;

public:
    PCA(int n_components = 2);
    
    void fit(const Eigen::MatrixXd& X);
    Eigen::MatrixXd transform(const Eigen::MatrixXd& X);
    Eigen::MatrixXd inverse_transform(const Eigen::MatrixXd& X_transformed);
    double explained_variance_ratio(int component) const;
    Eigen::VectorXd get_explained_variance_ratio() const;
    
private:
    void compute_eigenvalues_eigenvectors(const Eigen::MatrixXd& covariance_matrix);
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
    
    void fit(const Eigen::MatrixXd& X, int epochs = 100);
    Eigen::MatrixXd encode(const Eigen::MatrixXd& X);
    Eigen::MatrixXd decode(const Eigen::MatrixXd& encoded);
    Eigen::MatrixXd reconstruct(const Eigen::MatrixXd& X);
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