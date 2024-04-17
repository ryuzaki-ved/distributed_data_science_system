#include "client/dds_client.h"
#include "utils/types.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace dds;

int main(int argc, char* argv[]) {
    std::cout << "==========================================" << std::endl;
    std::cout << "  Distributed Data Science System Demo" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << std::endl;
    
    try {
        // Initialize client configuration
        ClientConfig config;
        config.server_host = "localhost";
        config.server_port = 8080;
        config.connection_timeout = 30;
        config.request_timeout = 300;
        config.enable_retry = true;
        config.max_retries = 3;
        config.log_level = LogLevel::INFO;
        
        // Create and initialize client
        DDSClient client;
        if (!client.initialize(config)) {
            std::cerr << "Failed to initialize client" << std::endl;
            return 1;
        }
        
        // Connect to the system
        if (!client.connect()) {
            std::cerr << "Failed to connect to DDS system" << std::endl;
            return 1;
        }
        
        std::cout << "Connected to DDS system successfully!" << std::endl;
        
        // Example 1: Linear Regression
        std::cout << "\n=== Example 1: Linear Regression ===" << std::endl;
        
        LinearRegressionParams lr_params;
        lr_params.learning_rate = 0.01;
        lr_params.tolerance = 1e-6;
        lr_params.max_iterations = 100;
        lr_params.loss_type = LossType::MSE;
        
        JobID lr_job_id = client.submit_linear_regression(
            "data/linear_regression_data.csv",
            "results/linear_regression_results.json",
            lr_params
        );
        
        if (lr_job_id != -1) {
            std::cout << "Linear regression job submitted with ID: " << lr_job_id << std::endl;
            
            // Wait for completion
            if (client.wait_for_job_completion(lr_job_id, 300)) {
                JobResult lr_result = client.get_job_result(lr_job_id);
                std::cout << "Linear regression completed successfully!" << std::endl;
                std::cout << "Execution time: " << lr_result.execution_time << " seconds" << std::endl;
                std::cout << "Final loss: " << lr_result.metrics["final_loss"] << std::endl;
            } else {
                std::cout << "Linear regression job timed out or failed" << std::endl;
            }
        }
        
        // Example 2: Logistic Regression
        std::cout << "\n=== Example 2: Logistic Regression ===" << std::endl;
        
        LogisticRegressionParams logreg_params;
        logreg_params.learning_rate = 0.01;
        logreg_params.tolerance = 1e-6;
        logreg_params.max_iterations = 100;
        logreg_params.use_regularization = true;
        logreg_params.lambda = 0.1;
        logreg_params.optimizer = OptimizerType::SGD;
        
        JobID logreg_job_id = client.submit_logistic_regression(
            "data/logistic_regression_data.csv",
            "results/logistic_regression_results.json",
            logreg_params
        );
        
        if (logreg_job_id != -1) {
            std::cout << "Logistic regression job submitted with ID: " << logreg_job_id << std::endl;
            
            // Monitor job progress
            while (true) {
                ClientJobStatus status = client.get_job_status(logreg_job_id);
                std::cout << "Progress: " << (status.progress * 100) << "% - " << status.message << std::endl;
                
                if (status.state == JobState::COMPLETED || status.state == JobState::FAILED) {
                    break;
                }
                
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
            
            if (client.get_job_status(logreg_job_id).state == JobState::COMPLETED) {
                JobResult logreg_result = client.get_job_result(logreg_job_id);
                std::cout << "Logistic regression completed successfully!" << std::endl;
                std::cout << "Accuracy: " << logreg_result.metrics["accuracy"] << std::endl;
                std::cout << "F1 Score: " << logreg_result.metrics["f1_score"] << std::endl;
            }
        }
        
        // Example 3: K-means Clustering
        std::cout << "\n=== Example 3: K-means Clustering ===" << std::endl;
        
        KMeansParams kmeans_params;
        kmeans_params.k = 3;
        kmeans_params.max_iterations = 100;
        kmeans_params.tolerance = 1e-6;
        kmeans_params.init_method = InitializationType::KMEANS_PLUS_PLUS;
        kmeans_params.n_init = 10;
        
        JobID kmeans_job_id = client.submit_kmeans(
            "data/kmeans_data.csv",
            "results/kmeans_results.json",
            kmeans_params
        );
        
        if (kmeans_job_id != -1) {
            std::cout << "K-means clustering job submitted with ID: " << kmeans_job_id << std::endl;
            
            // Wait for completion
            if (client.wait_for_job_completion(kmeans_job_id, 300)) {
                JobResult kmeans_result = client.get_job_result(kmeans_job_id);
                std::cout << "K-means clustering completed successfully!" << std::endl;
                std::cout << "Number of clusters: " << kmeans_result.metrics["num_clusters"] << std::endl;
                std::cout << "Inertia: " << kmeans_result.metrics["inertia"] << std::endl;
                std::cout << "Silhouette Score: " << kmeans_result.metrics["silhouette_score"] << std::endl;
            }
        }
        
        // Example 4: DBSCAN Clustering
        std::cout << "\n=== Example 4: DBSCAN Clustering ===" << std::endl;
        
        DBSCANParams dbscan_params;
        dbscan_params.epsilon = 0.5;
        dbscan_params.min_points = 5;
        dbscan_params.distance_metric = DistanceMetric::EUCLIDEAN;
        dbscan_params.verbose = true;
        
        JobID dbscan_job_id = client.submit_dbscan(
            "data/dbscan_data.csv",
            "results/dbscan_results.json",
            dbscan_params
        );
        
        if (dbscan_job_id != -1) {
            std::cout << "DBSCAN clustering job submitted with ID: " << dbscan_job_id << std::endl;
            
            // Wait for completion
            if (client.wait_for_job_completion(dbscan_job_id, 300)) {
                JobResult dbscan_result = client.get_job_result(dbscan_job_id);
                std::cout << "DBSCAN clustering completed successfully!" << std::endl;
                std::cout << "Number of clusters: " << dbscan_result.metrics["num_clusters"] << std::endl;
                std::cout << "Number of noise points: " << dbscan_result.metrics["num_noise_points"] << std::endl;
            }
        }
        
        // Example 5: Batch Job Submission
        std::cout << "\n=== Example 5: Batch Job Submission ===" << std::endl;
        
        std::vector<JobConfig> batch_configs;
        
        // Create multiple linear regression jobs with different parameters
        for (double lr : {0.001, 0.01, 0.1}) {
            LinearRegressionParams params;
            params.learning_rate = lr;
            params.max_iterations = 50;
            
            JobConfig config;
            config.type = JobType::LINEAR_REGRESSION;
            config.data_path = "data/linear_regression_data.csv";
            config.output_path = "results/lr_batch_lr_" + std::to_string(lr) + ".json";
            config.algorithm_params = params;
            
            batch_configs.push_back(config);
        }
        
        std::vector<JobID> batch_job_ids = client.submit_batch_jobs(batch_configs);
        std::cout << "Submitted " << batch_job_ids.size() << " batch jobs" << std::endl;
        
        // Wait for all batch jobs to complete
        std::vector<JobResult> batch_results = client.wait_for_batch_completion(batch_job_ids);
        std::cout << "All batch jobs completed!" << std::endl;
        
        // Example 6: System Monitoring
        std::cout << "\n=== Example 6: System Monitoring ===" << std::endl;
        
        // Get worker status
        std::vector<WorkerInfo> workers = client.get_worker_status();
        std::cout << "Active workers: " << workers.size() << std::endl;
        
        for (const auto& worker : workers) {
            std::cout << "Worker " << worker.worker_id << " (Rank " << worker.rank << "): "
                      << "CPU: " << (worker.cpu_usage * 100) << "%, "
                      << "Memory: " << (worker.memory_usage * 100) << "%, "
                      << "Active jobs: " << worker.assigned_jobs.size() << std::endl;
        }
        
        // Get system metrics
        std::vector<double> system_metrics = client.get_system_metrics();
        std::cout << "System load: " << system_metrics[0] << std::endl;
        std::cout << "Average job completion time: " << system_metrics[1] << " seconds" << std::endl;
        
        // Example 7: Data Management
        std::cout << "\n=== Example 7: Data Management ===" << std::endl;
        
        // Upload data file
        if (client.upload_data("local_data.csv", "remote_data.csv")) {
            std::cout << "Data file uploaded successfully" << std::endl;
        }
        
        // List data files
        std::vector<std::string> files;
        if (client.list_data_files("data", files)) {
            std::cout << "Available data files:" << std::endl;
            for (const auto& file : files) {
                std::cout << "  - " << file << std::endl;
            }
        }
        
        // Example 8: Client Metrics
        std::cout << "\n=== Example 8: Client Metrics ===" << std::endl;
        
        ClientMetrics metrics = client.get_client_metrics();
        std::cout << "Total jobs submitted: " << metrics.total_jobs_submitted << std::endl;
        std::cout << "Completed jobs: " << metrics.completed_jobs << std::endl;
        std::cout << "Failed jobs: " << metrics.failed_jobs << std::endl;
        std::cout << "Average job time: " << metrics.average_job_time << " seconds" << std::endl;
        
        // Generate report
        client.generate_report("session_report.json");
        std::cout << "Session report generated: session_report.json" << std::endl;
        
        // Disconnect from the system
        client.disconnect();
        std::cout << "\nDisconnected from DDS system" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n==========================================" << std::endl;
    std::cout << "  Demo completed successfully!" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    return 0;
} 