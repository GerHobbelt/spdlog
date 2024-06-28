//
// Copyright(c) 2015 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

// spdlog usage example

// Uncomment to enable source location support.
// This will add filename/line/column info to the log (and in to the resulting binary so take care).
//#define SPDLOG_NO_SOURCE_LOC

#include <spdlog/common.h>

#include <cstdio>
#include <chrono>

static void load_levels_example(int argc, const char **argv);
static void stdout_logger_example();
static void basic_example();
static void rotating_example();
static void daily_example();
static void callback_example();
static void async_example();
static void binary_example();
static void vector_example();
static void stopwatch_example();
static void trace_example();
static void multi_sink_example();
static void user_defined_example();
static void err_handler_example();
static void syslog_example();
static void udp_example();
static void custom_flags_example();
static void file_events_example();
static void replace_default_logger_example();
static void hierarchical_logger_example();
static void extended_stlying();
#ifndef _WIN32
static void syslog_example();
#endif
#if defined(__ANDROID__)
static void android_example();
#endif
static void mdc_example();

#include "spdlog/spdlog.h"
#include "spdlog/json_formatter.h"
#include "spdlog/cfg/env.h"   // support for loading levels from the environment variable
#include "spdlog/fmt/ostr.h"  // support for user defined types

#include "monolithic_examples.h"


#if defined(BUILD_MONOLITHIC)
#define main(cnt, arr)      spdlog_example_main(cnt, arr)
#endif

int main(int argc, const char **argv) {
    // Log levels can be loaded from argv/env using "SPDLOG_LEVEL"
    load_levels_example(argc, argv);
    spdlog::set_pattern("[%H:%M:%S %z] [process %P] [thread %t] [%n] %v");

    spdlog::default_logger()->log(spdlog::process_info(6789, 44), spdlog::level::critical, "Spoofed pid and thread message");

    spdlog::info("Welcome to spdlog version {}.{}.{}  !", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);

    spdlog::warn("Easy padding in numbers like {:08d}", 12);
    spdlog::critical("Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
    spdlog::info("Support for floats {:03.2f}", 1.23456);
    spdlog::info("Positional args are {1} {0}..", "too", "supported");
    spdlog::info("{:>8} aligned, {:<8} aligned", "right", "left");

    // Runtime log levels
    spdlog::set_level(spdlog::level::info);  // Set global log level to info
    spdlog::debug("This message should not be displayed!");
    spdlog::set_level(spdlog::level::trace);  // Set specific logger's log level
    spdlog::debug("This message should be displayed..");

#ifndef SPDLOG_NO_STRUCTURED_SPDLOG
    // Structured data
    spdlog::info({{"field1","value1"}, {"field2",2.0}}, "You can output data with fields and values");
    spdlog::set_formatter(spdlog::details::make_unique<spdlog::json_formatter>());
    SPDLOG_INFO({{"field1","value2"}, {"field2",4.0}}, "JSON logging is good for fields");
#endif

    // Customize msg format for all loggers
    spdlog::set_pattern("[%H:%M:%S %z] [%^%L%$] [thread %t] %v");
    spdlog::info("This an info message with custom format");
    spdlog::set_pattern("%+");  // back to default format
    spdlog::set_level(spdlog::level::info);

    // Backtrace support
    // Loggers can store in a ring buffer all messages (including debug/trace) for later inspection.
    // When needed, call dump_backtrace() to see what happened:
    spdlog::enable_backtrace(10);  // create ring buffer with capacity of 10  messages
    for (int i = 0; i < 100; i++) {
        spdlog::debug("Backtrace message {}", i);  // not logged..
    }
    // e.g. if some error happened:
    spdlog::dump_backtrace();  // log them now!

    try {
        stdout_logger_example();
        basic_example();
        rotating_example();
        daily_example();
        callback_example();
        async_example();
        binary_example();
        vector_example();
        multi_sink_example();
        user_defined_example();
        err_handler_example();
        trace_example();
        stopwatch_example();
        udp_example();
        custom_flags_example();
        file_events_example();
        replace_default_logger_example();
        hierarchical_logger_example();
        extended_stlying();
#if !defined(_WIN32)
        syslog_example();
#endif
#if defined(__ANDROID__)
        android_example();
#endif
        mdc_example();

        // Flush all *registered* loggers using a worker thread every 3 seconds.
        // note: registered loggers *must* be thread safe for this to work correctly!
        spdlog::flush_every(std::chrono::seconds(3));

        // Apply some function on all registered loggers
        spdlog::apply_all([&](std::shared_ptr<spdlog::logger> l) { l->info("End of example."); });

        // Release all spdlog resources, and drop all loggers in the registry.
        // This is optional (only mandatory if using windows + async log).
        spdlog::shutdown();
		return 0;
    }
    // Exceptions will only be thrown upon failed logger or sink construction (not during logging).
    catch (const spdlog::spdlog_ex &ex) {
        std::printf("Log initialization failed: %s\n", ex.what());
        return 1;
    }
}

#include "spdlog/sinks/stdout_color_sinks.h"
// or #include "spdlog/sinks/stdout_sinks.h" if no colors needed.
static void stdout_logger_example() {
    // Create color multi threaded logger.
    auto console = spdlog::stdout_color_mt("console");
    // or for stderr:
    // auto console = spdlog::stderr_color_mt("error-logger");
    auto dual_sink = spdlog::dual_color_mt("dual-sink");
    dual_sink->set_level(spdlog::level::debug);
    dual_sink->info("dual-sink");
    dual_sink->error("error");
    dual_sink->debug("debug");
}

#include "spdlog/sinks/basic_file_sink.h"
static void basic_example() {
    // Create basic file logger (not rotated).
    auto my_logger = spdlog::basic_logger_mt("file_logger", "logs/basic-log.txt", true);
}

#include "spdlog/sinks/rotating_file_sink.h"
static void rotating_example() {
    // Create a file rotating logger with 5mb size max and 3 rotated files.
    auto rotating_logger =
        spdlog::rotating_logger_mt("some_logger_name", "logs/rotating.txt", 1048576 * 5, 3);
}

#include "spdlog/sinks/daily_file_sink.h"
static void daily_example() {
    // Create a daily logger - a new file is created every day on 2:30am.
    auto daily_logger = spdlog::daily_logger_mt("daily_logger", "logs/daily.txt", 2, 30);
}

#include "spdlog/sinks/callback_sink.h"
static void callback_example() {
    // Create the logger
    auto logger = spdlog::callback_logger_mt("custom_callback_logger",
                                             [](const spdlog::details::log_msg & /*msg*/) {
                                                 // do what you need to do with msg
                                             });
}

#include "spdlog/cfg/env.h"
#include "spdlog/cfg/argv.h"
static void load_levels_example(int argc, const char **argv) {
    // Set the log level to "info" and mylogger to "trace":
    // SPDLOG_LEVEL=info,mylogger=trace && ./example
    spdlog::cfg::load_env_levels();
    // or from command line:
    // ./example SPDLOG_LEVEL=info,mylogger=trace
    // #include "spdlog/cfg/argv.h" // for loading levels from argv
    spdlog::cfg::load_argv_levels(argc, argv);
}

#include "spdlog/async.h"
static void async_example() {
    // Default thread pool settings can be modified *before* creating the async logger:
    // spdlog::init_thread_pool(32768, 1); // queue with max 32k items 1 backing thread.
    auto async_file =
        spdlog::basic_logger_mt<spdlog::async_factory>("async_file_logger", "logs/async_log.txt");
    // alternatively:
    // auto async_file =
    // spdlog::create_async<spdlog::sinks::basic_file_sink_mt>("async_file_logger",
    // "logs/async_log.txt");

    for (int i = 1; i < 101; ++i) {
        async_file->info("Async message #{}", i);
    }
    async_file->flush();

    auto async_logger = static_cast<spdlog::async_logger*>(&(*async_file));
    async_logger->wait();
}

// Log binary data as hex.
// Many types of std::container<char> types can be used.
// Iterator ranges are supported too.
// Format flags:
// {:X} - print in uppercase.
// {:s} - don't separate each byte with space.
// {:p} - don't print the position on each line start.
// {:n} - don't split the output to lines.

#if !defined SPDLOG_USE_STD_FORMAT || defined(_MSC_VER)
    #include "spdlog/fmt/bin_to_hex.h"

static void binary_example() {
    std::vector<char> buf;
    for (int i = 0; i < 80; i++) {
        buf.push_back(static_cast<char>(i & 0xff));
    }
    spdlog::info("Binary example: {}", spdlog::to_hex(buf));
    spdlog::info("Another binary example:{:n}",
                 spdlog::to_hex(std::begin(buf), std::begin(buf) + 10));
    // more examples:
	auto* logger = spdlog::default_logger_raw();
    logger->info("uppercase: {:X}", spdlog::to_hex(buf));
    logger->info("uppercase, no delimiters: {:Xs}", spdlog::to_hex(buf));
    logger->info("uppercase, no delimiters, no position info: {:Xsp}", spdlog::to_hex(buf));
    logger->info("hexdump style: {:a}", spdlog::to_hex(buf));
    logger->info("hexdump style, 20 chars per line {:a}", spdlog::to_hex(buf, 20));
}
#else
static void binary_example() {
    // not supported with std::format yet
}
#endif

// Log a vector of numbers
#ifndef SPDLOG_USE_STD_FORMAT
    #include "spdlog/fmt/ranges.h"
static void vector_example() {
    std::vector<int> vec = {1, 2, 3};
    spdlog::info("Vector example: {}", vec);
}

#else
static void vector_example() {}
#endif

// ! DSPDLOG_USE_STD_FORMAT

// Compile time log levels.
// define SPDLOG_ACTIVE_LEVEL to required level (e.g. SPDLOG_LEVEL_TRACE)
static void trace_example() {
    // trace from default logger
    SPDLOG_TRACE("Some trace message.. {} ,{}", 1, 3.23);
    // debug from default logger
    SPDLOG_DEBUG("Some debug message.. {} ,{}", 1, 3.23);

    // trace from logger object
    auto logger = spdlog::get("file_logger");
    SPDLOG_LOGGER_TRACE(logger, "another trace message");
}

// stopwatch example
#include "spdlog/stopwatch.h"
#include <thread>
static void stopwatch_example() {
    spdlog::stopwatch sw;
    std::this_thread::sleep_for(std::chrono::milliseconds(123));
    spdlog::info("Stopwatch: {} seconds", sw);
}

#include "spdlog/sinks/udp_sink.h"
static void udp_example() {
    spdlog::sinks::udp_sink_config cfg("127.0.0.1", 11091);
    auto my_logger = spdlog::udp_logger_mt("udplog", cfg);
    my_logger->set_level(spdlog::level::debug);
    my_logger->info("hello world");
}

// A logger with multiple sinks (stdout and file) - each with a different format and log level.
static void multi_sink_example() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::warn);
    console_sink->set_pattern("[multi_sink_example] [%^%l%$] %v");

    auto file_sink =
        std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/multisink.txt", true);
    file_sink->set_level(spdlog::level::trace);

    spdlog::logger logger("multi_sink", {console_sink, file_sink});
    logger.set_level(spdlog::level::debug);
    logger.warn("this should appear in both console and file");
    logger.info("this message should not appear in the console, only in the file");
}

// User defined types logging
struct my_type {
    int i = 0;
    explicit my_type(int i)
        : i(i){};
};

#ifndef SPDLOG_USE_STD_FORMAT  // when using fmtlib
template <>
struct fmt::formatter<my_type> : fmt::formatter<std::string> {
    auto format(my_type my, format_context &ctx) -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "[my_type i={}]", my.i);
    }
};

#else  // when using std::format
template <>
struct std::formatter<my_type> : std::formatter<std::string> {
    auto format(my_type my, format_context &ctx) const -> decltype(ctx.out()) {
        return format_to(ctx.out(), "[my_type i={}]", my.i);
    }
};
#endif

static void user_defined_example() { spdlog::info("user defined type: {}", my_type(14)); }

// Custom error handler. Will be triggered on log failure.
static void err_handler_example() {
    // can be set globally or per logger(logger->set_error_handler(..))
    spdlog::set_error_handler([](const std::string &msg) {
        printf("*** Custom log error handler: %s ***\n", msg.c_str());
    });
}

// syslog example (linux/osx/freebsd)
#ifndef _WIN32
#include "spdlog/sinks/syslog_sink.h"
static void syslog_example() {
    std::string ident = "spdlog-example";
    auto syslog_logger = spdlog::syslog_logger_mt("syslog", ident, LOG_PID);
    syslog_logger->warn("This is warning that will end up in syslog.");
}
#endif

// Android example.
#if defined(__ANDROID__)
#include "spdlog/sinks/android_sink.h"
static void android_example() {
    std::string tag = "spdlog-android";
    auto android_logger = spdlog::android_logger_mt("android", tag);
    android_logger->critical("Use \"adb shell logcat\" to view this message.");
}
#endif

// Log patterns can contain custom flags.
// this will add custom flag '%*' which will be bound to a <my_formatter_flag> instance
#include "spdlog/pattern_formatter.h"
class my_formatter_flag : public spdlog::custom_flag_formatter {
public:
    void format(const spdlog::details::log_msg &,
                const std::tm &,
                spdlog::memory_buf_t &dest) override {
        std::string some_txt = "custom-flag";
        dest.append(some_txt.data(), some_txt.data() + some_txt.size());
    }

    std::unique_ptr<custom_flag_formatter> clone() const override {
        return spdlog::details::make_unique<my_formatter_flag>();
    }
};

static void custom_flags_example() {
    using spdlog::details::make_unique;  // for pre c++14
    auto formatter = make_unique<spdlog::pattern_formatter>();
    formatter->add_flag<my_formatter_flag>('*').set_pattern("[%n] [%*] [%^%l%$] %v");
    // set the new formatter using spdlog::set_formatter(formatter) or
    // logger->set_formatter(formatter) spdlog::set_formatter(std::move(formatter));
}

static void file_events_example() {
    // pass the spdlog::file_event_handlers to file sinks for open/close log file notifications
    spdlog::file_event_handlers handlers;
    handlers.before_open = [](spdlog::filename_t filename) {
        spdlog::info("Before opening {}", filename);
    };
    handlers.after_open = [](spdlog::filename_t filename, std::FILE *fstream) {
        spdlog::info("After opening {}", filename);
        fputs("After opening\n", fstream);
    };
    handlers.before_close = [](spdlog::filename_t filename, std::FILE *fstream) {
        spdlog::info("Before closing {}", filename);
        fputs("Before closing\n", fstream);
    };
    handlers.after_close = [](spdlog::filename_t filename) {
        spdlog::info("After closing {}", filename);
    };
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/events-sample.txt",
                                                                         true, handlers);
    spdlog::logger my_logger("some_logger", file_sink);
    my_logger.info("Some log line");
}

static void replace_default_logger_example() {
    // store the old logger so we don't break other examples.
    auto old_logger = spdlog::default_logger();

    auto new_logger =
        spdlog::basic_logger_mt("new_default_logger", "logs/new-default-log.txt", true);
    spdlog::set_default_logger(new_logger);
    spdlog::set_level(spdlog::level::info);
    spdlog::debug("This message should not be displayed!");
    spdlog::set_level(spdlog::level::trace);
    spdlog::debug("This message should be displayed..");

    spdlog::set_default_logger(old_logger);
}

static void hierarchical_logger_example()
{
    spdlog::default_logger()->set_formatter(spdlog::details::make_unique<spdlog::pattern_formatter>("root [%n] [%^%l%$] %v"));

    auto lvl1 = spdlog::stdout_color_mt("propagate_lvl1");

    lvl1->set_formatter(spdlog::details::make_unique<spdlog::pattern_formatter>("lvl1 [%n] [%^%l%$] %v"));
    lvl1->set_level(spdlog::level::debug);
    auto lvl2 = std::make_shared<spdlog::logger>("propagate_lvl1.lvl2");
    spdlog::register_logger(lvl2);
    // skip level 3
    auto lvl4 = std::make_shared<spdlog::logger>("propagate_lvl1.lvl2.lvl3.lvl4");
    spdlog::register_logger(lvl4);


    lvl4->debug("I am a debug message at Level 4 but will be printed by Level 1 logger");
    lvl2->debug("I am a debug message at Level 2 but will be printed by Level 1 logger");


    auto multi_lvl1 = spdlog::stdout_color_mt("multi_lvl1");
    multi_lvl1->set_level(spdlog::level::debug);
    multi_lvl1->set_formatter(spdlog::details::make_unique<spdlog::pattern_formatter>("lvl1 [%n] [%^%l%$] %v"));
    auto multi_lvl2 = spdlog::stdout_color_mt("multi_lvl1.lvl2");
    multi_lvl2->set_level(spdlog::level::info);
    multi_lvl2->set_formatter(spdlog::details::make_unique<spdlog::pattern_formatter>("lvl2 [%n] [%^%l%$] %v"));
    // skip level 3
    auto multi_lvl4 = std::make_shared<spdlog::logger>("multi_lvl1.lvl2.lvl3.lvl4");
    spdlog::register_logger(multi_lvl4);


    multi_lvl4->debug("I am a debug message at Level 4 but will be printed by Level 1 logger");
    multi_lvl4->info("I am an info message at Level 4 but will be printed by Level 2, Level 1 and root logger");
}

static void extended_stlying()
{
#if defined(SPDLOG_EXTENDED_STYLING)
    // with extended styling you may use the multiple color
    // area formatter "%^" in more than one spot in your pattern.
    // in addition there are syntax extensions to the color formatter
    // they are defined by squirley braces { } after the '%' but before
    // the '^'. (example: "%{bold}^")
    //
    // mutliple stylings can apply to a single area by delimiting the key
    // words with a ';'. (example: "%{bold;fg_blue}^")
    //
    // styling key words come in three flavors: font style, font foreground
    // color, and font background color
    //
    // font styles:
    // reset bold dark underline blink reverse
    //
    // font foreground colors:
    // fg_black fg_red fg_green fg_yellow fg_blue fg_magenta fg_cyan fg_white fg_default
    //
    // font background colors:
    // bg_black bg_red bg_green bg_yellow bg_blue bg_magenta bg_cyan bg_white bg_default


    spdlog::set_pattern("[%H:%M:%S %z] [%^%L%$] [thread %t] %v");
    spdlog::info("regular log message");

    spdlog::set_pattern("[%H:%M:%S %z] [%^%L%$] [thread %t] %^%v%$");
    spdlog::info("multiple color areas");

    spdlog::set_pattern("[%H:%M:%S %z] [%^%L%$] [%{fg_yellow}^thread %t%$] %^%v%$");
    spdlog::info("multiple color areas with more than one color");

    spdlog::set_pattern("[%{bold;fg_red}^%H:%M:%S %z%$] [%^%L%$] [%{fg_yellow}^thread %t%$] %{bold;fg_cyan}^%v%$");
    spdlog::info("bold colors?");

    spdlog::set_pattern(
        "[%{bold;fg_red}^%H:%M:%S %z%$] "
        "[%^%L%$] [%{fg_yellow}^thread %t%$] "
        "even %{bold;blink;fg_magenta;bg_black}^%v%$ colors?"
    );
    spdlog::info("blinking");

    spdlog::set_pattern("[%{fg_yellow;bold}^%H:%M:%S %z%$] [%^%L%$] [%^thread %t%$] %^%v%$");
    spdlog::set_level(spdlog::level::trace);
    spdlog::trace("I work on other levels?");
    spdlog::debug("I work on other levels?");
    spdlog::info("I work on other levels?");
    spdlog::warn("I work on other levels?");
    spdlog::error("I work on other levels?");
    spdlog::critical("I work on other levels?");
    spdlog::set_pattern("%+"); // back to default format
#endif
}

// Mapped Diagnostic Context (MDC) is a map that stores key-value pairs (string values) in thread local storage.
// Each thread maintains its own MDC, which loggers use to append diagnostic information to log outputs.
// Note: it is not supported in asynchronous mode due to its reliance on thread-local storage.

#include "spdlog/mdc.h"
static void mdc_example()
{
    spdlog::mdc::put("key1", "value1");
    spdlog::mdc::put("key2", "value2");
    // if not using the default format, you can use the %& formatter to print mdc data as well
    spdlog::set_pattern("[%H:%M:%S %z] [%^%L%$] [%&] %v");
    spdlog::info("Some log message with context");
}

