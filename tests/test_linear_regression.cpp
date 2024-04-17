#include "algorithms/linear_regression.h"
#include "utils/types.h"
#include "communication/mpi_communicator.h"
#include <gtest/gtest.h>
#include <Eigen/Dense>
#include <random>
#include <iostream>

using namespace dds;

class LinearRegressionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize MPI communicator for testing
        g_mpi_communicator = std::make_unique<MPICommunicator>();
        int argc = 1;
        char* argv[] = {const_cast<char*>("test")};
        g_mpi_communicator->initialize(argc, argv);
        
        // Generate test data
        generate_test_data();
    }
    
    void TearDown() override {
        if (g_mpi_communicator) {
            g_mpi_communicator->finalize();
        }
    }
    
    void generate_test_data() {
        // Generate synthetic data for testing
        std::mt19937 gen(42);  // Fixed seed for reproducibility
        std::normal_distribution<double> noise(0.0, 0.1);
        
        // True parameters
        true_weights_ = Vector::Random(3);
        true_bias_ = 2.0;
        
        // Generate features
        X_ = Matrix::Random(1000, 3);
        
        // Generate target values with noise
        y_ = X_ * true_weights_ + Vector::Constant(X_.rows(), true_bias_);
        for (int i = 0; i < y_.size(); ++i) {
            y_(i) += noise(gen);
        }
        
        // Split data for testing
        int train_size = static_cast<int>(X_.rows() * 0.8);
        X_train_ = X_.topRows(train_size);
        y_train_ = y_.head(train_size);
        X_test_ = X_.bottomRows(X_.rows() - train_size);
        y_test_ = y_.tail(y_.size() - train_size);
    }
    
    // Test data
    Matrix X_, X_train_, X_test_;
    Vector y_, y_train_, y_test_;
    Vector true_weights_;
    double true_bias_;
};

// Test basic initialization
TEST_F(LinearRegressionTest, Initialization) {
    LinearRegression lr;
    LinearRegressionParams params;
    params.learning_rate = 0.01;
    params.tolerance = 1e-6;
    params.max_iterations = 100;
    
    lr.initialize(params);
    
    EXPECT_TRUE(lr.is_initialized());
    EXPECT_EQ(lr.get_params().learning_rate, 0.01);
    EXPECT_EQ(lr.get_params().tolerance, 1e-6);
    EXPECT_EQ(lr.get_params().max_iterations, 100);
}

// Test local training
TEST_F(LinearRegressionTest, LocalTraining) {
    LinearRegression lr;
    LinearRegressionParams params;
    params.learning_rate = 0.01;
    params.tolerance = 1e-6;
    params.max_iterations = 100;
    params.loss_type = LossType::MSE;
    
    lr.initialize(params);
    
    // Train the model
    bool success = lr.train(X_train_, y_train_);
    EXPECT_TRUE(success);
    
    // Check that training history is recorded
    const auto& history = lr.get_training_history();
    EXPECT_FALSE(history.empty());
    EXPECT_GT(history.size(), 0);
    
    // Check that loss decreases over time
    for (size_t i = 1; i < history.size(); ++i) {
        EXPECT_LE(history[i].loss, history[i-1].loss);
    }
}

// Test prediction
TEST_F(LinearRegressionTest, Prediction) {
    LinearRegression lr;
    LinearRegressionParams params;
    params.learning_rate = 0.01;
    params.tolerance = 1e-6;
    params.max_iterations = 100;
    
    lr.initialize(params);
    lr.train(X_train_, y_train_);
    
    // Make predictions
    Vector predictions = lr.predict(X_test_);
    
    EXPECT_EQ(predictions.size(), X_test_.rows());
    EXPECT_EQ(predictions.size(), y_test_.size());
    
    // Check that predictions are reasonable
    for (int i = 0; i < predictions.size(); ++i) {
        EXPECT_FALSE(std::isnan(predictions(i)));
        EXPECT_FALSE(std::isinf(predictions(i)));
    }
}

// Test loss computation
TEST_F(LinearRegressionTest, LossComputation) {
    LinearRegression lr;
    LinearRegressionParams params;
    params.learning_rate = 0.01;
    params.tolerance = 1e-6;
    params.max_iterations = 100;
    params.loss_type = LossType::MSE;
    
    lr.initialize(params);
    lr.train(X_train_, y_train_);
    
    // Compute loss on test set
    double test_loss = lr.compute_loss(X_test_, y_test_);
    
    EXPECT_GT(test_loss, 0.0);
    EXPECT_FALSE(std::isnan(test_loss));
    EXPECT_FALSE(std::isinf(test_loss));
    
    // Test MAE loss
    params.loss_type = LossType::MAE;
    lr.initialize(params);
    lr.train(X_train_, y_train_);
    
    double mae_loss = lr.compute_loss(X_test_, y_test_);
    EXPECT_GT(mae_loss, 0.0);
    EXPECT_FALSE(std::isnan(mae_loss));
}

// Test accuracy computation
TEST_F(LinearRegressionTest, AccuracyComputation) {
    LinearRegression lr;
    LinearRegressionParams params;
    params.learning_rate = 0.01;
    params.tolerance = 1e-6;
    params.max_iterations = 100;
    
    lr.initialize(params);
    lr.train(X_train_, y_train_);
    
    // Compute accuracy with different thresholds
    double accuracy_0_1 = lr.compute_accuracy(X_test_, y_test_, 0.1);
    double accuracy_0_5 = lr.compute_accuracy(X_test_, y_test_, 0.5);
    double accuracy_1_0 = lr.compute_accuracy(X_test_, y_test_, 1.0);
    
    EXPECT_GE(accuracy_0_1, 0.0);
    EXPECT_LE(accuracy_0_1, 1.0);
    EXPECT_GE(accuracy_0_5, 0.0);
    EXPECT_LE(accuracy_0_5, 1.0);
    EXPECT_GE(accuracy_1_0, 0.0);
    EXPECT_LE(accuracy_1_0, 1.0);
    
    // Accuracy should increase with larger threshold
    EXPECT_GE(accuracy_0_5, accuracy_0_1);
    EXPECT_GE(accuracy_1_0, accuracy_0_5);
}

// Test R-squared computation
TEST_F(LinearRegressionTest, RSquaredComputation) {
    LinearRegression lr;
    LinearRegressionParams params;
    params.learning_rate = 0.01;
    params.tolerance = 1e-6;
    params.max_iterations = 100;
    
    lr.initialize(params);
    lr.train(X_train_, y_train_);
    
    double r_squared = lr.compute_r_squared(X_test_, y_test_);
    
    EXPECT_GE(r_squared, 0.0);
    EXPECT_LE(r_squared, 1.0);
    EXPECT_FALSE(std::isnan(r_squared));
    EXPECT_FALSE(std::isinf(r_squared));
}

// Test distributed training
TEST_F(LinearRegressionTest, DistributedTraining) {
    if (!g_mpi_communicator || g_mpi_communicator->get_size() < 2) {
        GTEST_SKIP() << "Skipping distributed test - need at least 2 MPI processes";
    }
    
    LinearRegression lr;
    LinearRegressionParams params;
    params.learning_rate = 0.01;
    params.tolerance = 1e-6;
    params.max_iterations = 50;
    
    lr.initialize(params);
    
    // Partition data for distributed training
    int num_partitions = g_mpi_communicator->get_size();
    std::vector<Matrix> X_partitions;
    std::vector<Vector> y_partitions;
    
    int partition_size = X_train_.rows() / num_partitions;
    for (int i = 0; i < num_partitions; ++i) {
        int start_idx = i * partition_size;
        int end_idx = (i == num_partitions - 1) ? X_train_.rows() : (i + 1) * partition_size;
        
        X_partitions.push_back(X_train_.middleRows(start_idx, end_idx - start_idx));
        y_partitions.push_back(y_train_.segment(start_idx, end_idx - start_idx));
    }
    
    // Train distributed
    bool success = lr.train_distributed(X_partitions, y_partitions);
    EXPECT_TRUE(success);
    
    // Check that model was trained
    const auto& history = lr.get_training_history();
    EXPECT_FALSE(history.empty());
}

// Test model persistence
TEST_F(LinearRegressionTest, ModelPersistence) {
    LinearRegression lr1, lr2;
    LinearRegressionParams params;
    params.learning_rate = 0.01;
    params.tolerance = 1e-6;
    params.max_iterations = 100;
    
    lr1.initialize(params);
    lr1.train(X_train_, y_train_);
    
    // Save model
    bool save_success = lr1.save_model("test_model.json");
    EXPECT_TRUE(save_success);
    
    // Load model
    bool load_success = lr2.load_model("test_model.json");
    EXPECT_TRUE(load_success);
    
    // Compare predictions
    Vector pred1 = lr1.predict(X_test_);
    Vector pred2 = lr2.predict(X_test_);
    
    EXPECT_EQ(pred1.size(), pred2.size());
    for (int i = 0; i < pred1.size(); ++i) {
        EXPECT_NEAR(pred1(i), pred2(i), 1e-10);
    }
    
    // Clean up
    std::remove("test_model.json");
}

// Test convergence
TEST_F(LinearRegressionTest, Convergence) {
    LinearRegression lr;
    LinearRegressionParams params;
    params.learning_rate = 0.01;
    params.tolerance = 1e-6;
    params.max_iterations = 1000;
    
    lr.initialize(params);
    lr.train(X_train_, y_train_);
    
    const auto& history = lr.get_training_history();
    
    // Check that training converged (loss should be small)
    if (!history.empty()) {
        double final_loss = history.back().loss;
        EXPECT_LT(final_loss, 1.0);  // Loss should be reasonable
    }
}

// Test parameter validation
TEST_F(LinearRegressionTest, ParameterValidation) {
    LinearRegression lr;
    
    // Test with invalid learning rate
    LinearRegressionParams params;
    params.learning_rate = -0.01;
    params.tolerance = 1e-6;
    params.max_iterations = 100;
    
    lr.initialize(params);
    bool success = lr.train(X_train_, y_train_);
    // Should still work but might not converge well
    EXPECT_TRUE(success);
    
    // Test with very small tolerance
    params.learning_rate = 0.01;
    params.tolerance = 1e-12;
    lr.initialize(params);
    success = lr.train(X_train_, y_train_);
    EXPECT_TRUE(success);
}

// Test with different data sizes
TEST_F(LinearRegressionTest, DifferentDataSizes) {
    LinearRegression lr;
    LinearRegressionParams params;
    params.learning_rate = 0.01;
    params.tolerance = 1e-6;
    params.max_iterations = 100;
    
    lr.initialize(params);
    
    // Test with small dataset
    Matrix X_small = X_train_.topRows(10);
    Vector y_small = y_train_.head(10);
    
    bool success = lr.train(X_small, y_small);
    EXPECT_TRUE(success);
    
    // Test with single sample
    Matrix X_single = X_train_.topRows(1);
    Vector y_single = y_train_.head(1);
    
    success = lr.train(X_single, y_single);
    EXPECT_TRUE(success);
}

// Test utility functions
TEST_F(LinearRegressionTest, UtilityFunctions) {
    // Test feature normalization
    Matrix X_normalized = linear_regression_utils::normalize_features(X_train_);
    EXPECT_EQ(X_normalized.rows(), X_train_.rows());
    EXPECT_EQ(X_normalized.cols(), X_train_.cols());
    
    // Check that normalized features have reasonable statistics
    Vector mean = X_normalized.colwise().mean();
    Vector std_dev = ((X_normalized.rowwise() - mean.transpose()).array().square().colwise().mean()).sqrt();
    
    for (int i = 0; i < mean.size(); ++i) {
        EXPECT_NEAR(mean(i), 0.0, 1e-10);
        EXPECT_NEAR(std_dev(i), 1.0, 1e-10);
    }
    
    // Test train-test split
    auto [X_train_split, X_test_split] = linear_regression_utils::train_test_split(X_, y_, 0.2, 42);
    
    EXPECT_EQ(X_train_split.rows() + X_test_split.rows(), X_.rows());
    EXPECT_EQ(X_train_split.cols(), X_.cols());
    EXPECT_EQ(X_test_split.cols(), X_.cols());
    
    // Test evaluation metrics
    Vector y_pred = Vector::Random(y_test_.size());
    double mse = linear_regression_utils::mean_squared_error(y_test_, y_pred);
    double mae = linear_regression_utils::mean_absolute_error(y_test_, y_pred);
    double r2 = linear_regression_utils::r_squared_score(y_test_, y_pred);
    
    EXPECT_GT(mse, 0.0);
    EXPECT_GT(mae, 0.0);
    EXPECT_LE(r2, 1.0);
    EXPECT_FALSE(std::isnan(mse));
    EXPECT_FALSE(std::isnan(mae));
    EXPECT_FALSE(std::isnan(r2));
}

// Performance test
TEST_F(LinearRegressionTest, PerformanceTest) {
    LinearRegression lr;
    LinearRegressionParams params;
    params.learning_rate = 0.01;
    params.tolerance = 1e-6;
    params.max_iterations = 100;
    
    lr.initialize(params);
    
    // Measure training time
    auto start_time = std::chrono::high_resolution_clock::now();
    bool success = lr.train(X_train_, y_train_);
    auto end_time = std::chrono::high_resolution_clock::now();
    
    EXPECT_TRUE(success);
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Training time: " << duration.count() << " ms" << std::endl;
    
    // Training should complete in reasonable time (less than 10 seconds)
    EXPECT_LT(duration.count(), 10000);
    
    // Measure prediction time
    start_time = std::chrono::high_resolution_clock::now();
    Vector predictions = lr.predict(X_test_);
    end_time = std::chrono::high_resolution_clock::now();
    
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Prediction time: " << duration.count() << " ms" << std::endl;
    
    // Prediction should be fast (less than 1 second)
    EXPECT_LT(duration.count(), 1000);
}

// Test error handling
TEST_F(LinearRegressionTest, ErrorHandling) {
    LinearRegression lr;
    
    // Test training without initialization
    bool success = lr.train(X_train_, y_train_);
    EXPECT_FALSE(success);
    
    // Test prediction without training
    LinearRegressionParams params;
    params.learning_rate = 0.01;
    params.tolerance = 1e-6;
    params.max_iterations = 100;
    
    lr.initialize(params);
    Vector predictions = lr.predict(X_test_);
    EXPECT_TRUE(predictions.empty());
    
    // Test with mismatched dimensions
    Matrix X_wrong = Matrix::Random(100, 5);  // Different number of features
    success = lr.train(X_wrong, y_train_);
    EXPECT_FALSE(success);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Initialize MPI for testing
    MPI_Init(&argc, &argv);
    
    int result = RUN_ALL_TESTS();
    
    // Finalize MPI
    MPI_Finalize();
    
    return result;
} 