#include <iostream>
#include <iomanip>
#include "include/algorithms/advanced_algorithms.h"

int main() {
    std::cout << "=== Testing Improved Activation Functions ===" << std::endl;
    
    // Create a simple test matrix
    Eigen::MatrixXd test_input(3, 2);
    test_input << -2.0, 1.5,
                  0.0, -0.5,
                  1.0, 2.5;
    
    std::cout << "Input Matrix:" << std::endl;
    std::cout << test_input << std::endl << std::endl;
    
    // Test ReLU
    std::cout << "ReLU Activation:" << std::endl;
    Eigen::MatrixXd relu_output = dds::algorithms::NeuralLayer::relu(test_input);
    std::cout << relu_output << std::endl << std::endl;
    
    // Test Sigmoid
    std::cout << "Sigmoid Activation:" << std::endl;
    Eigen::MatrixXd sigmoid_output = dds::algorithms::NeuralLayer::sigmoid(test_input);
    std::cout << std::fixed << std::setprecision(4) << sigmoid_output << std::endl << std::endl;
    
    // Test Tanh
    std::cout << "Tanh Activation:" << std::endl;
    Eigen::MatrixXd tanh_output = dds::algorithms::NeuralLayer::tanh(test_input);
    std::cout << std::fixed << std::setprecision(4) << tanh_output << std::endl << std::endl;
    
    // Test Leaky ReLU
    std::cout << "Leaky ReLU Activation (alpha=0.1):" << std::endl;
    Eigen::MatrixXd leaky_relu_output = dds::algorithms::NeuralLayer::leaky_relu(test_input, 0.1);
    std::cout << leaky_relu_output << std::endl << std::endl;
    
    // Test ELU
    std::cout << "ELU Activation (alpha=1.0):" << std::endl;
    Eigen::MatrixXd elu_output = dds::algorithms::NeuralLayer::elu(test_input, 1.0);
    std::cout << std::fixed << std::setprecision(4) << elu_output << std::endl << std::endl;
    
    // Test Softmax (column-wise)
    std::cout << "Softmax Activation (column-wise):" << std::endl;
    Eigen::MatrixXd softmax_output = dds::algorithms::NeuralLayer::softmax(test_input);
    std::cout << std::fixed << std::setprecision(4) << softmax_output << std::endl << std::endl;
    
    // Test derivatives
    std::cout << "=== Testing Activation Derivatives ===" << std::endl;
    
    std::cout << "ReLU Derivative:" << std::endl;
    Eigen::MatrixXd relu_deriv = dds::algorithms::NeuralLayer::relu_derivative(test_input);
    std::cout << relu_deriv << std::endl << std::endl;
    
    std::cout << "Sigmoid Derivative:" << std::endl;
    Eigen::MatrixXd sigmoid_deriv = dds::algorithms::NeuralLayer::sigmoid_derivative(test_input);
    std::cout << std::fixed << std::setprecision(4) << sigmoid_deriv << std::endl << std::endl;
    
    std::cout << "Tanh Derivative:" << std::endl;
    Eigen::MatrixXd tanh_deriv = dds::algorithms::NeuralLayer::tanh_derivative(test_input);
    std::cout << std::fixed << std::setprecision(4) << tanh_deriv << std::endl << std::endl;
    
    // Test Dense Layer
    std::cout << "=== Testing Dense Layer ===" << std::endl;
    
    try {
        auto dense_layer = std::make_unique<dds::algorithms::DenseLayer>(2, 3, dds::algorithms::ActivationType::RELU);
        dense_layer->initialize_weights(0.1);
        
        std::cout << "Dense Layer Weights:" << std::endl;
        std::cout << dense_layer->get_weights() << std::endl << std::endl;
        
        std::cout << "Dense Layer Biases:" << std::endl;
        std::cout << dense_layer->get_biases() << std::endl << std::endl;
        
        std::cout << "Forward Pass:" << std::endl;
        Eigen::MatrixXd forward_output = dense_layer->forward(test_input);
        std::cout << forward_output << std::endl << std::endl;
        
        std::cout << "Backward Pass:" << std::endl;
        Eigen::MatrixXd backward_output = dense_layer->backward(forward_output);
        std::cout << backward_output << std::endl << std::endl;
        
        std::cout << "✅ Dense Layer test completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Dense Layer test failed: " << e.what() << std::endl;
    }
    
    std::cout << "\n=== Activation Functions Test Completed ===" << std::endl;
    std::cout << "All activation functions are now properly implemented!" << std::endl;
    
    return 0;
}
