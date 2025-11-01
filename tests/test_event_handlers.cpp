/*
 * This content is released under the MIT License as specified in
 * https://raw.githubusercontent.com/gabime/spdlog/master/LICENSE
 */
#include "includes.h"

#define TEST_FILENAME "test_logs/event_handlers_test.txt"

using spdlog::details::file_helper;

// Global counter to track event handler calls
static int handler_call_count = 0;
static bool recursion_attempted = false;

TEST_CASE("file_event_handlers_basic", "[file_helper::event_handlers]") {
    prepare_logdir();
    
    handler_call_count = 0;
    
    // Ensure default logger uses console output to avoid recursion
    spdlog::set_default_logger(spdlog::stdout_color_mt("console"));
    
    spdlog::file_event_handlers handlers;
    
    handlers.before_open = [](const spdlog::filename_t& filename) {
        handler_call_count++;
    };
    
    handlers.after_open = [](const spdlog::filename_t& filename, std::FILE* file) {
        handler_call_count++;
    };
    
    handlers.before_close = [](const spdlog::filename_t& filename, std::FILE* file) {
        handler_call_count++;
    };
    
    handlers.after_close = [](const spdlog::filename_t& filename) {
        handler_call_count++;
    };
    
    {
        file_helper helper(handlers);
        spdlog::filename_t target_filename = SPDLOG_FILENAME_T(TEST_FILENAME);
        helper.open(target_filename);
        REQUIRE(handler_call_count == 2); // before_open + after_open
    }
    REQUIRE(handler_call_count == 4); // + before_close + after_close
    
    // Clean up default logger
    spdlog::set_default_logger(nullptr);
}

TEST_CASE("file_event_handlers_logging_in_callbacks", "[file_helper::event_handlers]") {
    prepare_logdir();
    
    handler_call_count = 0;
    recursion_attempted = false;
    
    // Ensure default logger uses console output to avoid recursion
    spdlog::set_default_logger(spdlog::stdout_color_mt("console"));
    
    spdlog::file_event_handlers handlers;
    
    // Create a logger that will be used in event handlers to test recursion protection
    auto test_logger = spdlog::basic_logger_mt("event_test_logger", "test_logs/event_test.log");
    
    handlers.before_open = [&](const spdlog::filename_t& filename) {
        handler_call_count++;
        // Attempt to log from within the event handler (potential recursion)
        test_logger->info("Logging from before_open handler");
        recursion_attempted = true;
    };
    
    handlers.after_open = [&](const spdlog::filename_t& filename, std::FILE* file) {
        handler_call_count++;
        // Attempt to log from within the event handler (potential recursion)
        test_logger->info("Logging from after_open handler");
    };
    
    handlers.before_close = [&](const spdlog::filename_t& filename, std::FILE* file) {
        handler_call_count++;
        // Attempt to log from within the event handler (potential recursion)
        test_logger->info("Logging from before_close handler");
    };
    
    handlers.after_close = [&](const spdlog::filename_t& filename) {
        handler_call_count++;
        // Attempt to log from within the event handler (potential recursion)
        test_logger->info("Logging from after_close handler");
    };
    
    {
        file_helper helper(handlers);
        spdlog::filename_t target_filename = SPDLOG_FILENAME_T(TEST_FILENAME);
        helper.open(target_filename);
        
        // Should have called both open handlers without infinite recursion
        REQUIRE(handler_call_count == 2);
        REQUIRE(recursion_attempted == true);
    }
    // Should have called all 4 handlers without infinite recursion
    REQUIRE(handler_call_count == 4);
    
    // Clean up
    spdlog::drop("event_test_logger");
    spdlog::set_default_logger(nullptr);
}

TEST_CASE("file_event_handlers_spdlog_api_in_callbacks", "[file_helper::event_handlers]") {
    prepare_logdir();
    
    handler_call_count = 0;
    
    // Ensure default logger uses console output to avoid recursion
    spdlog::set_default_logger(spdlog::stdout_color_mt("console"));
    
    spdlog::file_event_handlers handlers;
    
    handlers.before_open = [](const spdlog::filename_t& filename) {
        handler_call_count++;
        // Use spdlog API directly in callback (potential recursion)
        spdlog::info("Direct spdlog call from before_open");
    };
    
    handlers.after_open = [](const spdlog::filename_t& filename, std::FILE* file) {
        handler_call_count++;
        // Use spdlog API directly in callback (potential recursion)
        spdlog::info("Direct spdlog call from after_open");
    };
    
    handlers.before_close = [](const spdlog::filename_t& filename, std::FILE* file) {
        handler_call_count++;
        // Use spdlog API directly in callback (potential recursion)
        spdlog::info("Direct spdlog call from before_close");
    };
    
    handlers.after_close = [](const spdlog::filename_t& filename) {
        handler_call_count++;
        // Use spdlog API directly in callback (potential recursion)
        spdlog::info("Direct spdlog call from after_close");
    };
    
    {
        file_helper helper(handlers);
        spdlog::filename_t target_filename = SPDLOG_FILENAME_T(TEST_FILENAME);
        helper.open(target_filename);
        
        // Should have called both open handlers without infinite recursion
        REQUIRE(handler_call_count == 2);
    }
    // Should have called all 4 handlers without infinite recursion
    REQUIRE(handler_call_count == 4);
    
    // Clean up default logger
    spdlog::set_default_logger(nullptr);
}

TEST_CASE("file_event_handlers_null_handlers", "[file_helper::event_handlers]") {
    prepare_logdir();
    
    // Ensure default logger uses console output to avoid recursion
    spdlog::set_default_logger(spdlog::stdout_color_mt("console"));
    
    // Test with empty handlers (all null)
    spdlog::file_event_handlers handlers;
    
    {
        file_helper helper(handlers);
        spdlog::filename_t target_filename = SPDLOG_FILENAME_T(TEST_FILENAME);
        
        // Should not crash with null handlers
        REQUIRE_NOTHROW(helper.open(target_filename));
    }
    
    // Clean up default logger
    spdlog::set_default_logger(nullptr);
}

TEST_CASE("file_event_handlers_partial_handlers", "[file_helper::event_handlers]") {
    prepare_logdir();
    
    handler_call_count = 0;
    
    // Ensure default logger uses console output to avoid recursion
    spdlog::set_default_logger(spdlog::stdout_color_mt("console"));
    
    spdlog::file_event_handlers handlers;
    
    // Only set some handlers
    handlers.before_open = [](const spdlog::filename_t& filename) {
        handler_call_count++;
    };
    
    handlers.after_close = [](const spdlog::filename_t& filename) {
        handler_call_count++;
    };
    
    {
        file_helper helper(handlers);
        spdlog::filename_t target_filename = SPDLOG_FILENAME_T(TEST_FILENAME);
        helper.open(target_filename);
        REQUIRE(handler_call_count == 1); // only before_open called
    }
    REQUIRE(handler_call_count == 2); // + after_close called
    
    // Clean up default logger
    spdlog::set_default_logger(nullptr);
}