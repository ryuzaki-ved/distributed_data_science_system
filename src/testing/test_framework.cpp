#include "../../include/testing/test_framework.h"
#include <stdexcept>
#include <cmath>

namespace dds {
namespace testing {

void TestSuite::add_test(const std::string& name, std::function<void()> test_func) {
    tests_.push_back([this, name, test_func]() {
        TestResult result;
        result.test_name = name;
        result.passed = true;
        result.error_message = "";
        
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            test_func();
            std::cout << "✅ PASS: " << name << std::endl;
        } catch (const std::exception& e) {
            result.passed = false;
            result.error_message = e.what();
            std::cout << "❌ FAIL: " << name << " - " << e.what() << std::endl;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        results_.push_back(result);
        
        if (result.passed) {
            passed_count_++;
        } else {
            failed_count_++;
        }
    });
}

void TestSuite::run_all_tests() {
    std::cout << "🧪 Running test suite: " << suite_name_ << std::endl;
    std::cout << "===========================================" << std::endl;
    
    passed_count_ = 0;
    failed_count_ = 0;
    results_.clear();
    
    for (auto& test : tests_) {
        test();
    }
    
    print_summary();
}

void TestSuite::assert_true(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message.empty() ? "Assertion failed" : message);
    }
}

void TestSuite::assert_equals(double expected, double actual, double tolerance) {
    if (std::abs(expected - actual) > tolerance) {
        throw std::runtime_error("Expected " + std::to_string(expected) + 
                                " but got " + std::to_string(actual));
    }
}

void TestSuite::assert_not_null(void* ptr, const std::string& message) {
    if (ptr == nullptr) {
        throw std::runtime_error(message.empty() ? "Pointer is null" : message);
    }
}

void TestSuite::print_summary() const {
    std::cout << "===========================================" << std::endl;
    std::cout << "📊 Test Summary for " << suite_name_ << std::endl;
    std::cout << "Total Tests: " << (passed_count_ + failed_count_) << std::endl;
    std::cout << "✅ Passed: " << passed_count_ << std::endl;
    std::cout << "❌ Failed: " << failed_count_ << std::endl;
    
    if (failed_count_ == 0) {
        std::cout << "🎉 All tests passed!" << std::endl;
    } else {
        std::cout << "⚠️  Some tests failed!" << std::endl;
    }
    std::cout << "===========================================" << std::endl;
}

} // namespace testing
} // namespace dds
