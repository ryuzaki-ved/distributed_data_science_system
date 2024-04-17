#include <iostream>
#include <string>
#include "utils/types.h"

int main() {
    std::cout << "=== Distributed Data Science System Demo ===" << std::endl;
    std::cout << "Version: 1.0.0" << std::endl;
    std::cout << "Building successfully with minimal dependencies!" << std::endl;
    
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
    
    std::cout << "\n=== Demo completed successfully! ===" << std::endl;
    std::cout << "Your Distributed Data Science System is now building!" << std::endl;
    
    return 0;
} 