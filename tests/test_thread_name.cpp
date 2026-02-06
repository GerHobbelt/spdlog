#include "includes.h"
#include <thread>
#include <chrono>
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <pthread.h>
#endif

TEST_CASE("thread_name_basic", "[thread_name]") {
    std::cout << "\n=== Testing basic thread name functionality ===" << std::endl;
    
    // Test that thread_name function exists and returns non-empty string
    std::cout << "Calling thread_name()..." << std::endl;
    auto name = spdlog::details::os::thread_name();
    std::cout << "  Result: '" << name << "'" << std::endl;
    
    std::cout << "Verifying name is non-empty..." << std::endl;
    REQUIRE_FALSE(name.empty());
    std::cout << "  ✓ Thread name is non-empty" << std::endl;
    
    // Main thread should return "main" as the name
    std::cout << "Verifying main thread returns 'main'..." << std::endl;
    REQUIRE(name == "main");
    std::cout << "  ✓ Main thread correctly returns 'main' as thread name" << std::endl;
    
    // Verify it's not a numeric string (thread ID)
    std::cout << "Verifying main thread name is not numeric..." << std::endl;
    bool is_numeric = std::all_of(name.begin(), name.end(), ::isdigit);
    REQUIRE_FALSE(is_numeric);
    std::cout << "  ✓ Main thread name is descriptive, not a numeric thread ID" << std::endl;
}

TEST_CASE("thread_name_different_threads", "[thread_name]") {
    std::cout << "\n=== Testing thread names across different threads ===" << std::endl;
    
    std::cout << "Getting main thread name..." << std::endl;
    std::string main_thread_name = spdlog::details::os::thread_name();
    std::cout << "  Main thread name: '" << main_thread_name << "'" << std::endl;
    
    std::string worker_thread_name;
    
    std::cout << "Creating worker thread with custom name..." << std::endl;
    std::thread worker([&worker_thread_name]() {
        // Set a meaningful thread name first
#ifndef _WIN32
        pthread_setname_np(
#ifdef __APPLE__
            "worker"
#else
            pthread_self(), "worker"
#endif
        );
#endif
        std::cout << "  [Worker] Getting thread name..." << std::endl;
        worker_thread_name = spdlog::details::os::thread_name();
        std::cout << "  [Worker] Thread name: '" << worker_thread_name << "'" << std::endl;
    });
    
    worker.join();
    std::cout << "Worker thread joined" << std::endl;
    
    // Verify worker thread has a meaningful name
    std::cout << "Verifying worker thread name is non-empty..." << std::endl;
    REQUIRE_FALSE(worker_thread_name.empty());
    std::cout << "  ✓ Worker thread name is non-empty" << std::endl;
    
    std::cout << "Comparing thread names:" << std::endl;
    std::cout << "  Main: '" << main_thread_name << "'" << std::endl;
    std::cout << "  Worker: '" << worker_thread_name << "'" << std::endl;
    
    // Main should be "main", worker should be "worker" (or thread ID on Windows)
    REQUIRE(main_thread_name == "main");
    std::cout << "  ✓ Main thread correctly named 'main'" << std::endl;
    
#ifndef _WIN32
    REQUIRE(worker_thread_name == "worker");
    std::cout << "  ✓ Worker thread correctly named 'worker'" << std::endl;
#else
    std::cout << "  ℹ Windows platform - worker thread uses thread ID fallback" << std::endl;
#endif
    
    REQUIRE(main_thread_name != worker_thread_name);
    std::cout << "  ✓ Thread names are different (as expected)" << std::endl;
}

TEST_CASE("thread_name_with_set_name", "[thread_name]") {
    std::cout << "\n=== Testing thread name with custom name setting ===" << std::endl;
    
    std::string initial_name;
    std::string updated_name;
    
    std::cout << "Creating worker thread..." << std::endl;
    std::thread worker([&]() {
        std::cout << "  [Worker] Getting initial name..." << std::endl;
        // Get initial name (should be thread ID as string)
        initial_name = spdlog::details::os::thread_name();
        std::cout << "  [Worker] Initial name: '" << initial_name << "'" << std::endl;
        
#ifdef _WIN32
        std::cout << "  [Worker] Windows platform detected - limited thread naming support" << std::endl;
        // Windows: Try to set thread name (may not be supported on all versions)
        // For testing purposes, we'll just verify the function works
        updated_name = spdlog::details::os::thread_name();
        std::cout << "  [Worker] After 'change' name: '" << updated_name << "'" << std::endl;
#else
        std::cout << "  [Worker] Unix/Linux platform - setting thread name to 'helper'" << std::endl;
        // Unix/Linux: Set thread name
        int result = pthread_setname_np(
#ifdef __APPLE__
            "helper"
#else
            pthread_self(), "helper"
#endif
        );
        std::cout << "  [Worker] pthread_setname_np result: " << result << std::endl;
        
        std::cout << "  [Worker] Getting name after setting custom name..." << std::endl;
        updated_name = spdlog::details::os::thread_name();
        std::cout << "  [Worker] After change name: '" << updated_name << "'" << std::endl;
#endif
    });
    
    worker.join();
    std::cout << "Worker thread completed" << std::endl;
    
    // Verify all names are non-empty
    std::cout << "Verifying all names are non-empty..." << std::endl;
    REQUIRE_FALSE(initial_name.empty());
    REQUIRE_FALSE(updated_name.empty());
    std::cout << "  ✓ All names are non-empty" << std::endl;
    
#ifndef _WIN32
    std::cout << "Unix platform - verifying thread name change behavior..." << std::endl;
    // On Unix systems, after setting name, thread_name should return the updated name
    std::cout << "  Expected name after change: 'helper'" << std::endl;
    std::cout << "  Actual name after change: '" << updated_name << "'" << std::endl;
    REQUIRE(updated_name == "helper");
    std::cout << "  ✓ Thread name correctly updated to 'helper'" << std::endl;
#else
    std::cout << "Windows platform - thread name change behavior not tested" << std::endl;
#endif
}

TEST_CASE("thread_name_consistency", "[thread_name]") {
    std::cout << "\n=== Testing thread name consistency across multiple calls ===" << std::endl;
    
    std::cout << "Creating worker thread for consistency test..." << std::endl;
    std::thread worker([]() {
        // Set a meaningful thread name
#ifndef _WIN32
        pthread_setname_np(
#ifdef __APPLE__
            "checker"
#else
            pthread_self(), "checker"
#endif
        );
#endif
        std::cout << "  [Worker] Testing function consistency..." << std::endl;
        
        // Call the function multiple times
        std::string call1 = spdlog::details::os::thread_name();
        std::string call2 = spdlog::details::os::thread_name();
        std::string call3 = spdlog::details::os::thread_name();
        
        std::cout << "  [Worker] Call 1: '" << call1 << "'" << std::endl;
        std::cout << "  [Worker] Call 2: '" << call2 << "'" << std::endl;
        std::cout << "  [Worker] Call 3: '" << call3 << "'" << std::endl;
        
        std::cout << "  [Worker] Verifying function consistency..." << std::endl;
        REQUIRE(call1 == call2);
        REQUIRE(call2 == call3);
        std::cout << "  [Worker] ✓ All calls return identical values" << std::endl;
    });
    
    worker.join();
    std::cout << "Consistency test completed" << std::endl;
}

TEST_CASE("thread_name_performance", "[thread_name][performance]") {
    std::cout << "\n=== Testing thread name performance ===" << std::endl;
    
    // This test measures the performance of thread_name function
    const int iterations = 1000;
    std::cout << "Performance test with " << iterations << " iterations" << std::endl;
    
    std::thread worker([iterations]() {
        std::cout << "  [Worker] Setting up performance test..." << std::endl;
        // Set a thread name for more realistic testing
#ifndef _WIN32
        std::cout << "  [Worker] Setting thread name to 'benchmark'..." << std::endl;
        pthread_setname_np(
#ifdef __APPLE__
            "benchmark"
#else
            pthread_self(), "benchmark"
#endif
        );
        std::cout << "  [Worker] Thread name set successfully" << std::endl;
#else
        std::cout << "  [Worker] Windows platform - skipping thread name setting" << std::endl;
#endif
        
        std::cout << "  [Worker] Starting performance test..." << std::endl;
        // Time thread_name function
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i) {
            volatile auto name = spdlog::details::os::thread_name();
            (void)name; // Prevent optimization
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "  [Worker] thread_name() completed: " << duration.count() << " μs" << std::endl;
        
        // Verify performance is reasonable (should complete within reasonable time)
        std::cout << "  [Worker] Verifying performance is reasonable..." << std::endl;
        REQUIRE(duration.count() > 0);
        REQUIRE(duration.count() < 100000); // Should complete within 100ms
        std::cout << "  [Worker] ✓ Performance is within acceptable range" << std::endl;
    });
    
    worker.join();
    std::cout << "Performance test completed" << std::endl;
}

TEST_CASE("thread_name_fallback", "[thread_name]") {
    std::cout << "\n=== Testing thread name fallback behavior ===" << std::endl;
    std::cout << "NOTE: Main thread returns 'main', worker threads return thread ID when no custom name is set" << std::endl;
    
    std::cout << "Testing main thread fallback..." << std::endl;
    std::string main_thread_name = spdlog::details::os::thread_name();
    std::cout << "  Main thread name: '" << main_thread_name << "'" << std::endl;
    REQUIRE(main_thread_name == "main");
    std::cout << "  ✓ Main thread correctly returns 'main'" << std::endl;
    
    std::cout << "Creating worker thread to test fallback behavior..." << std::endl;
    std::string worker_name;
    std::string worker_id_str;
    
    std::thread worker([&]() {
        std::cout << "  [Worker] This thread has NO custom name set" << std::endl;
        std::cout << "  [Worker] Getting thread name and thread ID..." << std::endl;
        worker_name = spdlog::details::os::thread_name();
        worker_id_str = std::to_string(spdlog::details::os::_thread_id());
        
        std::cout << "  [Worker] Thread name from spdlog: '" << worker_name << "'" << std::endl;
        std::cout << "  [Worker] Thread ID as string: '" << worker_id_str << "'" << std::endl;
        
        std::cout << "  [Worker] Verifying expected fallback behavior..." << std::endl;
        std::cout << "  [Worker] Expected: worker thread_name == thread_id (fallback mechanism)" << std::endl;
        REQUIRE(worker_name == worker_id_str);
        std::cout << "  [Worker] ✓ Worker thread fallback working correctly: uses thread ID when no custom name exists" << std::endl;
    });
    
    worker.join();
    std::cout << "Fallback test completed - main thread gets 'main', worker threads get thread ID" << std::endl;
}