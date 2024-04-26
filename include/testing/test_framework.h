#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <iostream>

namespace dds {
namespace testing {

// Test result structure
struct TestResult {
    std::string test_name;
    bool passed;
    std::string error_message;
    std::chrono::milliseconds execution_time;
};

// Test suite for DDS components
class TestSuite {
private:
    std::string suite_name_;
    std::vector<std::function<void()>> tests_;
    std::vector<TestResult> results_;
    int passed_count_;
    int failed_count_;

public:
    TestSuite(const std::string& name) : suite_name_(name), passed_count_(0), failed_count_(0) {}
    
    // Add test case
    void add_test(const std::string& name, std::function<void()> test_func);
    
    // Run all tests
    void run_all_tests();
    
    // Get results
    std::vector<TestResult> get_results() const { return results_; }
    int get_passed_count() const { return passed_count_; }
    int get_failed_count() const { return failed_count_; }
    
    // Assertion helpers
    static void assert_true(bool condition, const std::string& message = "");
    static void assert_equals(double expected, double actual, double tolerance = 1e-6);
    static void assert_not_null(void* ptr, const std::string& message = "");
    
    // Print summary
    void print_summary() const;
};

} // namespace testing
} // namespace dds
