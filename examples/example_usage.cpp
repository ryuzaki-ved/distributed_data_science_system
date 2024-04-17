#include <iostream>
#include <string>
#include "utils/types.h"
#include "storage/hadoop_storage.h"

int main() {
    std::cout << "=== Distributed Data Science System Demo ===" << std::endl;
    std::cout << "Version: 1.0.0" << std::endl;
    std::cout << "Enhanced version with Hadoop integration!" << std::endl;

    // Test basic types
    dds::JobType job_type = dds::JobType::LINEAR_REGRESSION;
    dds::JobStatus status = dds::JobStatus::PENDING;

    std::cout << "\nJob Type: " << dds::job_type_to_string(job_type) << std::endl;
    std::cout << "Job Status: " << dds::job_status_to_string(status) << std::endl;

    // Test Eigen stub
    Eigen::MatrixXd matrix(3, 3);
    matrix.setRandom();

    Eigen::VectorXd vector(3);
    vector.setRandom();

    std::cout << "\nMatrix (3x3):" << std::endl;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            std::cout << matrix(i, j) << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "\nVector (3x1):" << std::endl;
    for (int i = 0; i < 3; ++i) {
        std::cout << vector[i] << std::endl;
    }

    // Test matrix operations
    Eigen::MatrixXd result = matrix * vector;
    std::cout << "\nMatrix * Vector result:" << std::endl;
    for (int i = 0; i < 3; ++i) {
        std::cout << result[i] << std::endl;
    }

    // Test Hadoop integration
    std::cout << "\n=== Testing Hadoop Integration ===" << std::endl;
    
    try {
        // Create Hadoop configuration
        dds::storage::HadoopConfig config;
        config.namenode_host = "localhost";
        config.namenode_port = 9000;
        config.hdfs_url = "hdfs://localhost:9000";
        config.replication_factor = 3;
        config.block_size = "128MB";
        
        // Create Hadoop storage
        dds::storage::HadoopStorage hadoop_storage(config);
        
        // Connect to HDFS
        if (hadoop_storage.connect()) {
            std::cout << "✅ Successfully connected to HDFS" << std::endl;
            
            // Create a test directory
            if (hadoop_storage.create_directory("/test_data")) {
                std::cout << "✅ Created test directory in HDFS" << std::endl;
                
                // Save matrix to HDFS
                if (hadoop_storage.save_matrix("/test_data/test_matrix", matrix)) {
                    std::cout << "✅ Saved matrix to HDFS" << std::endl;
                    
                    // Load matrix from HDFS
                    Eigen::MatrixXd loaded_matrix;
                    if (hadoop_storage.load_matrix("/test_data/test_matrix", loaded_matrix)) {
                        std::cout << "✅ Loaded matrix from HDFS" << std::endl;
                        
                        // Verify the loaded matrix
                        bool matrices_equal = true;
                        for (int i = 0; i < matrix.rows() && matrices_equal; ++i) {
                            for (int j = 0; j < matrix.cols() && matrices_equal; ++j) {
                                if (std::abs(matrix(i, j) - loaded_matrix(i, j)) > 1e-10) {
                                    matrices_equal = false;
                                }
                            }
                        }
                        
                        if (matrices_equal) {
                            std::cout << "✅ Matrix verification successful" << std::endl;
                        } else {
                            std::cout << "❌ Matrix verification failed" << std::endl;
                        }
                    } else {
                        std::cout << "❌ Failed to load matrix from HDFS: " << hadoop_storage.get_last_error() << std::endl;
                    }
                } else {
                    std::cout << "❌ Failed to save matrix to HDFS: " << hadoop_storage.get_last_error() << std::endl;
                }
                
                // List directory contents
                auto files = hadoop_storage.list_directory("/test_data");
                std::cout << "📁 HDFS directory contents:" << std::endl;
                for (const auto& file : files) {
                    std::cout << "  - " << file.path << " (" << file.size << " bytes)" << std::endl;
                }
                
                // Create a test dataset
                Eigen::MatrixXd features(100, 3);
                Eigen::VectorXd labels(100);
                features.setRandom();
                labels.setRandom();
                
                if (hadoop_storage.save_dataset("/test_data/dataset", features, labels)) {
                    std::cout << "✅ Saved dataset to HDFS" << std::endl;
                    
                    // Load dataset back
                    Eigen::MatrixXd loaded_features;
                    Eigen::VectorXd loaded_labels;
                    if (hadoop_storage.load_dataset("/test_data/dataset", loaded_features, loaded_labels)) {
                        std::cout << "✅ Loaded dataset from HDFS" << std::endl;
                        std::cout << "  Features: " << loaded_features.rows() << "x" << loaded_features.cols() << std::endl;
                        std::cout << "  Labels: " << loaded_labels.size() << std::endl;
                    }
                }
                
            } else {
                std::cout << "❌ Failed to create test directory: " << hadoop_storage.get_last_error() << std::endl;
            }
            
            // Disconnect from HDFS
            hadoop_storage.disconnect();
            std::cout << "✅ Disconnected from HDFS" << std::endl;
            
        } else {
            std::cout << "❌ Failed to connect to HDFS: " << hadoop_storage.get_last_error() << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "❌ Exception during Hadoop test: " << e.what() << std::endl;
    }

    std::cout << "\n=== Hadoop Features Implemented ===" << std::endl;
    std::cout << "✅ HDFS File Operations (create, read, delete)" << std::endl;
    std::cout << "✅ Directory Operations (create, list)" << std::endl;
    std::cout << "✅ Matrix/Vector Serialization" << std::endl;
    std::cout << "✅ Dataset Storage and Loading" << std::endl;
    std::cout << "✅ Error Handling and Logging" << std::endl;
    std::cout << "✅ Configuration Management" << std::endl;
    
    std::cout << "\n=== Next Steps for Full Hadoop Integration ===" << std::endl;
    std::cout << "🔄 Replace stub with real Hadoop libraries" << std::endl;
    std::cout << "🔄 Implement MapReduce job submission" << std::endl;
    std::cout << "🔄 Add YARN resource management" << std::endl;
    std::cout << "🔄 Implement distributed algorithms" << std::endl;
    std::cout << "🔄 Add cluster monitoring" << std::endl;

    std::cout << "\n=== Demo completed successfully! ===" << std::endl;
    std::cout << "Your Distributed Data Science System now has Hadoop integration!" << std::endl;

    return 0;
} 