/*
 * This content is released under the MIT License as specified in
 * https://raw.githubusercontent.com/gabime/spdlog/master/LICENSE
 */
#include "includes.h"
#include <chrono>
#include <iomanip>
#include <iostream>

#define TEST_FILENAME "test_logs/mmap_file_helper_test.txt"

using spdlog::details::file_helper;

static void write_with_helper(file_helper &helper, size_t howmany) {
    std::cout << "  Writing " << howmany << " bytes of data..." << std::endl;
    spdlog::memory_buf_t formatted;
    spdlog::fmt_lib::format_to(std::back_inserter(formatted), "{}", std::string(howmany, '1'));
    helper.write(formatted);
    helper.flush();
    std::cout << "  Write completed, current file size: " << helper.size() << " bytes" << std::endl;
}

TEST_CASE("file_helper_mmap_disabled_by_default", "[file_helper::mmap]") {
    std::cout << "\n=== Test: mmap disabled by default ===" << std::endl;
    prepare_logdir();
    
    file_helper helper;
    std::cout << "Checking mmap default state..." << std::endl;
    bool mmap_enabled = helper.is_mmap_enabled();
    std::cout << "mmap status: " << (mmap_enabled ? "enabled" : "disabled") << std::endl;
    REQUIRE(helper.is_mmap_enabled() == false);
    std::cout << "✓ Test passed: mmap disabled by default" << std::endl;
}

TEST_CASE("file_helper_mmap_enable_disable", "[file_helper::mmap]") {
    std::cout << "\n=== Test: mmap enable/disable functionality ===" << std::endl;
    prepare_logdir();
    
    file_helper helper;
    std::cout << "Initial mmap status: " << (helper.is_mmap_enabled() ? "enabled" : "disabled") << std::endl;
    
    // Test enable/disable
    std::cout << "Enabling mmap..." << std::endl;
    helper.set_mmap_enabled(true);
    std::cout << "mmap status: " << (helper.is_mmap_enabled() ? "enabled" : "disabled") << std::endl;
    REQUIRE(helper.is_mmap_enabled() == true);
    
    std::cout << "Disabling mmap..." << std::endl;
    helper.set_mmap_enabled(false);
    std::cout << "mmap status: " << (helper.is_mmap_enabled() ? "enabled" : "disabled") << std::endl;
    REQUIRE(helper.is_mmap_enabled() == false);
    std::cout << "✓ Test passed: mmap enable/disable functionality works correctly" << std::endl;
}

TEST_CASE("file_helper_mmap_basic_write", "[file_helper::mmap]") {
    std::cout << "\n=== Test: mmap basic write functionality ===" << std::endl;
    prepare_logdir();
    
    spdlog::filename_t target_filename = SPDLOG_FILENAME_T(TEST_FILENAME);
    size_t expected_size = 123;
    
    std::cout << "Opening file: " << TEST_FILENAME << std::endl;
    {
        file_helper helper;
        helper.set_mmap_enabled(true);  // Enable mmap for this test
        helper.open(target_filename);
        std::cout << "mmap status: " << (helper.is_mmap_enabled() ? "enabled" : "disabled") << std::endl;
        write_with_helper(helper, expected_size);
        REQUIRE(static_cast<size_t>(helper.size()) == expected_size);
    }
    
    // Verify file was written correctly
    size_t actual_size = get_filesize(TEST_FILENAME);
    std::cout << "Verifying file size - expected: " << expected_size << " bytes, actual: " << actual_size << " bytes" << std::endl;
    REQUIRE(get_filesize(TEST_FILENAME) == expected_size);
    std::cout << "✓ Test passed: mmap basic write functionality works correctly" << std::endl;
}

TEST_CASE("file_helper_mmap_vs_stdio_content", "[file_helper::mmap]") {
    std::cout << "\n=== Test: mmap vs stdio content consistency ===" << std::endl;
    prepare_logdir();
    
    std::string test_content = "Hello mmap world!\nLine 2\nLine 3\n";
    std::cout << "Test content length: " << test_content.size() << " bytes" << std::endl;
    
    // Write with mmap enabled
    std::cout << "Writing file using mmap mode..." << std::endl;
    {
        file_helper helper_mmap;
        helper_mmap.set_mmap_enabled(true);  // Enable mmap for this test
        spdlog::filename_t mmap_filename = SPDLOG_FILENAME_T("test_logs/mmap_test.txt");
        helper_mmap.open(mmap_filename);
        std::cout << "  mmap status: " << (helper_mmap.is_mmap_enabled() ? "enabled" : "disabled") << std::endl;
        
        spdlog::memory_buf_t buf;
        buf.append(test_content.data(), test_content.data() + test_content.size());
        helper_mmap.write(buf);
        helper_mmap.flush();
        std::cout << "  mmap file write completed, size: " << helper_mmap.size() << " bytes" << std::endl;
    }
    
    // Write with stdio
    std::cout << "Writing file using stdio mode..." << std::endl;
    {
        file_helper helper_stdio;
        helper_stdio.set_mmap_enabled(false);
        spdlog::filename_t stdio_filename = SPDLOG_FILENAME_T("test_logs/stdio_test.txt");
        helper_stdio.open(stdio_filename);
        std::cout << "  mmap status: " << (helper_stdio.is_mmap_enabled() ? "enabled" : "disabled") << std::endl;
        
        spdlog::memory_buf_t buf;
        buf.append(test_content.data(), test_content.data() + test_content.size());
        helper_stdio.write(buf);
        helper_stdio.flush();
        std::cout << "  stdio file write completed, size: " << helper_stdio.size() << " bytes" << std::endl;
    }
    
    // Compare file contents
    size_t mmap_size = get_filesize("test_logs/mmap_test.txt");
    size_t stdio_size = get_filesize("test_logs/stdio_test.txt");
    std::cout << "Comparing file sizes - mmap: " << mmap_size << " bytes, stdio: " << stdio_size << " bytes" << std::endl;
    REQUIRE(get_filesize("test_logs/mmap_test.txt") == get_filesize("test_logs/stdio_test.txt"));
    
    // Read and compare actual content
    std::cout << "Reading and comparing file contents..." << std::endl;
    std::ifstream mmap_file("test_logs/mmap_test.txt");
    std::ifstream stdio_file("test_logs/stdio_test.txt");
    
    std::string mmap_content((std::istreambuf_iterator<char>(mmap_file)),
                             std::istreambuf_iterator<char>());
    std::string stdio_content((std::istreambuf_iterator<char>(stdio_file)),
                              std::istreambuf_iterator<char>());
    
    std::cout << "Content comparison result: " << (mmap_content == stdio_content ? "identical" : "different") << std::endl;
    REQUIRE(mmap_content == stdio_content);
    REQUIRE(mmap_content == test_content);
    std::cout << "✓ Test passed: mmap and stdio mode contents are completely identical" << std::endl;
}

TEST_CASE("file_helper_mmap_large_write", "[file_helper::mmap]") {
    std::cout << "\n=== Test: mmap large write ===" << std::endl;
    prepare_logdir();
    
    spdlog::filename_t target_filename = SPDLOG_FILENAME_T(TEST_FILENAME);
    
    file_helper helper;
    helper.set_mmap_enabled(true);  // Enable mmap for this test
    helper.open(target_filename);
    std::cout << "mmap status: " << (helper.is_mmap_enabled() ? "enabled" : "disabled") << std::endl;
    
    // Write multiple chunks to test mmap expansion
    size_t total_size = 0;
    std::cout << "Starting to write 100 lines of test data..." << std::endl;
    for (int i = 0; i < 100; ++i) {
        spdlog::memory_buf_t buf;
        std::string line = "This is test line " + std::to_string(i) + " with some data.\n";
        buf.append(line.data(), line.data() + line.size());
        helper.write(buf);
        total_size += line.size();
        
        if ((i + 1) % 20 == 0) {
            std::cout << "  Written " << (i + 1) << " lines, cumulative size: " << total_size << " bytes" << std::endl;
        }
    }
    
    helper.flush();
    std::cout << "Write completed, total size: " << total_size << " bytes" << std::endl;
    std::cout << "helper reported size: " << helper.size() << " bytes" << std::endl;
    std::cout << "actual file size: " << get_filesize(TEST_FILENAME) << " bytes" << std::endl;
    REQUIRE(static_cast<size_t>(helper.size()) == total_size);
    REQUIRE(get_filesize(TEST_FILENAME) == total_size);
    std::cout << "✓ Test passed: mmap large write functionality works correctly" << std::endl;
}

TEST_CASE("file_helper_mmap_reopen", "[file_helper::mmap]") {
    std::cout << "\n=== Test: mmap file reopen ===" << std::endl;
    prepare_logdir();
    
    spdlog::filename_t target_filename = SPDLOG_FILENAME_T(TEST_FILENAME);
    file_helper helper;
    
    // Enable mmap for this test since default is now false
    std::cout << "Enabling mmap for reopen test..." << std::endl;
    helper.set_mmap_enabled(true);
    std::cout << "Opening file and writing data..." << std::endl;
    helper.open(target_filename);
    write_with_helper(helper, 12);
    std::cout << "Current file size: " << helper.size() << " bytes" << std::endl;
    REQUIRE(helper.size() == 12);
    
    // Test reopen with truncate
    std::cout << "Reopening file (truncate mode)..." << std::endl;
    helper.reopen(true);
    std::cout << "File size after reopen: " << helper.size() << " bytes" << std::endl;
    REQUIRE(helper.size() == 0);
    
    // Verify mmap is still enabled after reopen
    std::cout << "mmap status after reopen: " << (helper.is_mmap_enabled() ? "enabled" : "disabled") << std::endl;
    REQUIRE(helper.is_mmap_enabled() == true);
    std::cout << "✓ Test passed: mmap file reopen functionality works correctly" << std::endl;
}

TEST_CASE("file_helper_mmap_disable_during_operation", "[file_helper::mmap]") {
    std::cout << "\n=== Test: runtime mmap disable ===" << std::endl;
    prepare_logdir();
    
    spdlog::filename_t target_filename = SPDLOG_FILENAME_T(TEST_FILENAME);
    file_helper helper;
    
    // Enable mmap first for this test since default is now false
    helper.set_mmap_enabled(true);
    helper.open(target_filename);
    std::cout << "Initial mmap status: " << (helper.is_mmap_enabled() ? "enabled" : "disabled") << std::endl;
    
    // Write some data with mmap
    std::cout << "Writing data using mmap mode..." << std::endl;
    write_with_helper(helper, 50);
    REQUIRE(helper.size() == 50);
    
    // Disable mmap during operation
    std::cout << "Disabling mmap at runtime..." << std::endl;
    helper.set_mmap_enabled(false);
    std::cout << "mmap status: " << (helper.is_mmap_enabled() ? "enabled" : "disabled") << std::endl;
    REQUIRE(helper.is_mmap_enabled() == false);
    
    // Write more data (should use stdio now)
    std::cout << "Continuing to write data using stdio mode..." << std::endl;
    write_with_helper(helper, 50);
    REQUIRE(helper.size() == 100);
    
    size_t final_size = get_filesize(TEST_FILENAME);
    std::cout << "Final file size: " << final_size << " bytes" << std::endl;
    REQUIRE(get_filesize(TEST_FILENAME) == 100);
    std::cout << "✓ Test passed: runtime mmap disable functionality works correctly" << std::endl;
}

TEST_CASE("file_helper_mmap_sync", "[file_helper::mmap]") {
    std::cout << "\n=== Test: mmap sync operation ===" << std::endl;
    prepare_logdir();
    
    spdlog::filename_t target_filename = SPDLOG_FILENAME_T(TEST_FILENAME);
    file_helper helper;
    
    helper.set_mmap_enabled(true);  // Enable mmap for this test
    helper.open(target_filename);
    std::cout << "mmap status: " << (helper.is_mmap_enabled() ? "enabled" : "disabled") << std::endl;
    write_with_helper(helper, 123);
    
    // Test sync operation
    std::cout << "Performing sync operation..." << std::endl;
    REQUIRE_NOTHROW(helper.sync());
    std::cout << "Sync operation completed, file size: " << helper.size() << " bytes" << std::endl;
    REQUIRE(helper.size() == 123);
    std::cout << "✓ Test passed: mmap sync operation works correctly" << std::endl;
}

TEST_CASE("file_helper_mmap_filename_consistency", "[file_helper::mmap]") {
    std::cout << "\n=== Test: mmap filename consistency ===" << std::endl;
    prepare_logdir();
    
    file_helper helper;
    spdlog::filename_t target_filename = SPDLOG_FILENAME_T(TEST_FILENAME);
    helper.open(target_filename);
    
    std::cout << "Target filename: " << TEST_FILENAME << std::endl;
    std::cout << "helper reported filename: " << spdlog::details::os::filename_to_str(helper.filename()) << std::endl;
    
    // Filename should be consistent regardless of mmap mode
    std::cout << "Checking filename consistency with mmap disabled (default)..." << std::endl;
    REQUIRE(helper.filename() == target_filename);
    
    std::cout << "Checking filename consistency after enabling mmap..." << std::endl;
    helper.set_mmap_enabled(true);
    std::cout << "mmap status: " << (helper.is_mmap_enabled() ? "enabled" : "disabled") << std::endl;
    std::cout << "helper reported filename: " << spdlog::details::os::filename_to_str(helper.filename()) << std::endl;
    REQUIRE(helper.filename() == target_filename);
    std::cout << "✓ Test passed: filename remains consistent across different mmap modes" << std::endl;
}

TEST_CASE("file_helper_mmap_vs_stdio_performance", "[file_helper::mmap][performance]") {
    std::cout << "\n=== Test: mmap vs stdio performance comparison ===" << std::endl;
    prepare_logdir();
    
    const size_t test_iterations = 1000;
    const size_t data_size_per_write = 256; // 256 bytes per write
    const std::string test_data(data_size_per_write - 1, 'A');
    const std::string test_data_with_newline = test_data + "\n";
    
    std::cout << "Performance test configuration:" << std::endl;
    std::cout << "  Test iterations: " << test_iterations << " times" << std::endl;
    std::cout << "  Data size per write: " << data_size_per_write << " bytes" << std::endl;
    std::cout << "  Total data volume: " << (test_iterations * data_size_per_write) << " bytes (" 
              << (test_iterations * data_size_per_write / 1024.0) << " KB)" << std::endl;
    
    // Test mmap performance
    std::cout << "\n--- Testing mmap mode performance ---" << std::endl;
    auto start_mmap = std::chrono::high_resolution_clock::now();
    {
        file_helper helper_mmap;
        helper_mmap.set_mmap_enabled(true);  // Enable mmap for performance test
        spdlog::filename_t mmap_filename = SPDLOG_FILENAME_T("test_logs/performance_mmap.txt");
        helper_mmap.open(mmap_filename);
        std::cout << "mmap status: " << (helper_mmap.is_mmap_enabled() ? "enabled" : "disabled") << std::endl;
        
        for (size_t i = 0; i < test_iterations; ++i) {
            spdlog::memory_buf_t buf;
            buf.append(test_data_with_newline.data(), test_data_with_newline.data() + test_data_with_newline.size());
            helper_mmap.write(buf);
            
            if ((i + 1) % 200 == 0) {
                std::cout << "  Completed " << (i + 1) << "/" << test_iterations << " writes" << std::endl;
            }
        }
        helper_mmap.flush();
    }
    auto end_mmap = std::chrono::high_resolution_clock::now();
    auto duration_mmap = std::chrono::duration_cast<std::chrono::microseconds>(end_mmap - start_mmap);
    
    // Test stdio performance
    std::cout << "\n--- Testing stdio mode performance ---" << std::endl;
    auto start_stdio = std::chrono::high_resolution_clock::now();
    {
        file_helper helper_stdio;
        helper_stdio.set_mmap_enabled(false);
        spdlog::filename_t stdio_filename = SPDLOG_FILENAME_T("test_logs/performance_stdio.txt");
        helper_stdio.open(stdio_filename);
        std::cout << "mmap status: " << (helper_stdio.is_mmap_enabled() ? "enabled" : "disabled") << std::endl;
        
        for (size_t i = 0; i < test_iterations; ++i) {
            spdlog::memory_buf_t buf;
            buf.append(test_data_with_newline.data(), test_data_with_newline.data() + test_data_with_newline.size());
            helper_stdio.write(buf);
            
            if ((i + 1) % 200 == 0) {
                std::cout << "  Completed " << (i + 1) << "/" << test_iterations << " writes" << std::endl;
            }
        }
        helper_stdio.flush();
    }
    auto end_stdio = std::chrono::high_resolution_clock::now();
    auto duration_stdio = std::chrono::duration_cast<std::chrono::microseconds>(end_stdio - start_stdio);
    
    // Performance results analysis
    std::cout << "\n=== Performance Test Results ===" << std::endl;
    std::cout << "mmap mode duration: " << duration_mmap.count() << " microseconds (" 
              << (static_cast<double>(duration_mmap.count()) / 1000.0) << " milliseconds)" << std::endl;
    std::cout << "stdio mode duration: " << duration_stdio.count() << " microseconds ("
              << (static_cast<double>(duration_stdio.count()) / 1000.0) << " milliseconds)" << std::endl;
    
    double performance_ratio = static_cast<double>(duration_stdio.count()) / static_cast<double>(duration_mmap.count());
    if (duration_mmap.count() < duration_stdio.count()) {
        std::cout << "mmap mode is " << std::fixed << std::setprecision(2) 
                  << performance_ratio << " times faster than stdio mode" << std::endl;
    } else if (duration_mmap.count() > duration_stdio.count()) {
        std::cout << "stdio mode is " << std::fixed << std::setprecision(2) 
                  << (static_cast<double>(duration_mmap.count()) / static_cast<double>(duration_stdio.count())) << " times faster than mmap mode" << std::endl;
    } else {
        std::cout << "Both modes have similar performance" << std::endl;
    }
    
    // Calculate throughput
    double total_mb = (test_iterations * data_size_per_write) / (1024.0 * 1024.0);
    double mmap_throughput = total_mb / (static_cast<double>(duration_mmap.count()) / 1000000.0); // MB/s
    double stdio_throughput = total_mb / (static_cast<double>(duration_stdio.count()) / 1000000.0); // MB/s
    
    std::cout << "\nThroughput comparison:" << std::endl;
    std::cout << "mmap mode throughput: " << std::fixed << std::setprecision(2) 
              << mmap_throughput << " MB/s" << std::endl;
    std::cout << "stdio mode throughput: " << std::fixed << std::setprecision(2) 
              << stdio_throughput << " MB/s" << std::endl;
    
    // Verify file sizes
    size_t expected_size = test_iterations * data_size_per_write;
    size_t mmap_file_size = get_filesize("test_logs/performance_mmap.txt");
    size_t stdio_file_size = get_filesize("test_logs/performance_stdio.txt");
    
    std::cout << "\nFile size verification:" << std::endl;
    std::cout << "Expected file size: " << expected_size << " bytes" << std::endl;
    std::cout << "mmap file size: " << mmap_file_size << " bytes" << std::endl;
    std::cout << "stdio file size: " << stdio_file_size << " bytes" << std::endl;
    
    REQUIRE(mmap_file_size == expected_size);
    REQUIRE(stdio_file_size == expected_size);
    REQUIRE(mmap_file_size == stdio_file_size);
    
    std::cout << "✓ Test passed: performance comparison test completed, both modes wrote data correctly" << std::endl;
}