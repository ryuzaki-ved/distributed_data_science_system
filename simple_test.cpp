#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector> // Added for SimpleMatrix
#include <cstdlib> // Added for rand()

// Simple matrix class for testing
class SimpleMatrix {
private:
    std::vector<std::vector<double>> data;
    int rows_, cols_;

public:
    SimpleMatrix(int rows, int cols) : rows_(rows), cols_(cols) {
        data.resize(rows, std::vector<double>(cols, 0.0));
    }
    
    double& operator()(int i, int j) { return data[i][j]; }
    const double& operator()(int i, int j) const { return data[i][j]; }
    
    int rows() const { return rows_; }
    int cols() const { return cols_; }
    
    void setRandom() {
        for (int i = 0; i < rows_; ++i) {
            for (int j = 0; j < cols_; ++j) {
                data[i][j] = (double)rand() / RAND_MAX * 4.0 - 2.0; // -2 to 2
            }
        }
    }
    
    void print() const {
        for (int i = 0; i < rows_; ++i) {
            for (int j = 0; j < cols_; ++j) {
                std::cout << std::fixed << std::setprecision(4) << data[i][j] << " ";
            }
            std::cout << std::endl;
        }
    }
};

// Activation functions implementation
class ActivationFunctions {
public:
    static SimpleMatrix relu(const SimpleMatrix& x) {
        SimpleMatrix result(x.rows(), x.cols());
        for (int i = 0; i < x.rows(); ++i) {
            for (int j = 0; j < x.cols(); ++j) {
                result(i, j) = std::max(0.0, x(i, j));
            }
        }
        return result;
    }
    
    static SimpleMatrix sigmoid(const SimpleMatrix& x) {
        SimpleMatrix result(x.rows(), x.cols());
        for (int i = 0; i < x.rows(); ++i) {
            for (int j = 0; j < x.cols(); ++j) {
                result(i, j) = 1.0 / (1.0 + std::exp(-x(i, j)));
            }
        }
        return result;
    }
    
    static SimpleMatrix tanh(const SimpleMatrix& x) {
        SimpleMatrix result(x.rows(), x.cols());
        for (int i = 0; i < x.rows(); ++i) {
            for (int j = 0; j < x.cols(); ++j) {
                result(i, j) = std::tanh(x(i, j));
            }
        }
        return result;
    }
    
    static SimpleMatrix softmax(const SimpleMatrix& x) {
        SimpleMatrix result(x.rows(), x.cols());
        for (int j = 0; j < x.cols(); ++j) {
            double max_val = x(0, j);
            for (int i = 1; i < x.rows(); ++i) {
                if (x(i, j) > max_val) max_val = x(i, j);
            }
            
            double sum = 0.0;
            for (int i = 0; i < x.rows(); ++i) {
                result(i, j) = std::exp(x(i, j) - max_val);
                sum += result(i, j);
            }
            
            for (int i = 0; i < x.rows(); ++i) {
                result(i, j) /= sum;
            }
        }
        return result;
    }
    
    static SimpleMatrix leaky_relu(const SimpleMatrix& x, double alpha = 0.01) {
        SimpleMatrix result(x.rows(), x.cols());
        for (int i = 0; i < x.rows(); ++i) {
            for (int j = 0; j < x.cols(); ++j) {
                result(i, j) = (x(i, j) > 0) ? x(i, j) : alpha * x(i, j);
            }
        }
        return result;
    }
    
    static SimpleMatrix elu(const SimpleMatrix& x, double alpha = 1.0) {
        SimpleMatrix result(x.rows(), x.cols());
        for (int i = 0; i < x.rows(); ++i) {
            for (int j = 0; j < x.cols(); ++j) {
                result(i, j) = (x(i, j) > 0) ? x(i, j) : alpha * (std::exp(x(i, j)) - 1);
            }
        }
        return result;
    }
    
    // Derivatives
    static SimpleMatrix relu_derivative(const SimpleMatrix& x) {
        SimpleMatrix result(x.rows(), x.cols());
        for (int i = 0; i < x.rows(); ++i) {
            for (int j = 0; j < x.cols(); ++j) {
                result(i, j) = (x(i, j) > 0) ? 1.0 : 0.0;
            }
        }
        return result;
    }
    
    static SimpleMatrix sigmoid_derivative(const SimpleMatrix& x) {
        SimpleMatrix result(x.rows(), x.cols());
        for (int i = 0; i < x.rows(); ++i) {
            for (int j = 0; j < x.cols(); ++j) {
                double sigmoid_val = 1.0 / (1.0 + std::exp(-x(i, j)));
                result(i, j) = sigmoid_val * (1.0 - sigmoid_val);
            }
        }
        return result;
    }
    
    static SimpleMatrix tanh_derivative(const SimpleMatrix& x) {
        SimpleMatrix result(x.rows(), x.cols());
        for (int i = 0; i < x.rows(); ++i) {
            for (int j = 0; j < x.cols(); ++j) {
                double tanh_val = std::tanh(x(i, j));
                result(i, j) = 1.0 - tanh_val * tanh_val;
            }
        }
        return result;
    }
};

int main() {
    std::cout << "=== Testing Improved Activation Functions ===" << std::endl;
    
    // Create a test matrix
    SimpleMatrix test_input(3, 2);
    test_input(0, 0) = -2.0; test_input(0, 1) = 1.5;
    test_input(1, 0) = 0.0;  test_input(1, 1) = -0.5;
    test_input(2, 0) = 1.0;  test_input(2, 1) = 2.5;
    
    std::cout << "Input Matrix:" << std::endl;
    test_input.print();
    std::cout << std::endl;
    
    // Test ReLU
    std::cout << "ReLU Activation:" << std::endl;
    SimpleMatrix relu_output = ActivationFunctions::relu(test_input);
    relu_output.print();
    std::cout << std::endl;
    
    // Test Sigmoid
    std::cout << "Sigmoid Activation:" << std::endl;
    SimpleMatrix sigmoid_output = ActivationFunctions::sigmoid(test_input);
    sigmoid_output.print();
    std::cout << std::endl;
    
    // Test Tanh
    std::cout << "Tanh Activation:" << std::endl;
    SimpleMatrix tanh_output = ActivationFunctions::tanh(test_input);
    tanh_output.print();
    std::cout << std::endl;
    
    // Test Leaky ReLU
    std::cout << "Leaky ReLU Activation (alpha=0.1):" << std::endl;
    SimpleMatrix leaky_relu_output = ActivationFunctions::leaky_relu(test_input, 0.1);
    leaky_relu_output.print();
    std::cout << std::endl;
    
    // Test ELU
    std::cout << "ELU Activation (alpha=1.0):" << std::endl;
    SimpleMatrix elu_output = ActivationFunctions::elu(test_input, 1.0);
    elu_output.print();
    std::cout << std::endl;
    
    // Test Softmax
    std::cout << "Softmax Activation (column-wise):" << std::endl;
    SimpleMatrix softmax_output = ActivationFunctions::softmax(test_input);
    softmax_output.print();
    std::cout << std::endl;
    
    // Test derivatives
    std::cout << "=== Testing Activation Derivatives ===" << std::endl;
    
    std::cout << "ReLU Derivative:" << std::endl;
    SimpleMatrix relu_deriv = ActivationFunctions::relu_derivative(test_input);
    relu_deriv.print();
    std::cout << std::endl;
    
    std::cout << "Sigmoid Derivative:" << std::endl;
    SimpleMatrix sigmoid_deriv = ActivationFunctions::sigmoid_derivative(test_input);
    sigmoid_deriv.print();
    std::cout << std::endl;
    
    std::cout << "Tanh Derivative:" << std::endl;
    SimpleMatrix tanh_deriv = ActivationFunctions::tanh_derivative(test_input);
    tanh_deriv.print();
    std::cout << std::endl;
    
    std::cout << "\n=== Activation Functions Test Completed ===" << std::endl;
    std::cout << "All activation functions are now properly implemented!" << std::endl;
    
    return 0;
}
