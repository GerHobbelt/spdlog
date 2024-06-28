//
// Copyright(c) 2015 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

// spdlog usage example

#include <spdlog/common.h>

#include <cstdio>
#include <chrono>

static void load_levels_example(int argc, const char **argv);

#include "spdlog/spdlog.h"
#include "spdlog/json_formatter.h"
#include "spdlog/cfg/env.h"   // support for loading levels from the environment variable
#include "spdlog/fmt/ostr.h"  // support for user defined types
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/all-sinks.h"

#include "monolithic_examples.h"


#if defined(BUILD_MONOLITHIC)
#define main(cnt, arr)      spdlog_example2_main(cnt, arr)
#endif

int main(int argc, const char **argv) {
    // Log levels can be loaded from argv/env using "SPDLOG_LEVEL"
    load_levels_example(argc, argv);
    spdlog::set_automatic_registration(true);
		spdlog::set_pattern("[%H:%M:%S %z] [process %P] [thread %t(%q)] [%n] %v");

    spdlog::default_logger()->log(spdlog::process_info(6789, 44, "TH_foobar"), spdlog::level::critical, "Spoofed pid and thread message");

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
    //spdlog::set_formatter(spdlog::details::make_unique<spdlog::json_formatter>());
    //SPDLOG_INFO({{"field1","value2"}, {"field2",4.0}}, "JSON logging is good for fields");
#endif

		{
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);
        console_sink->set_pattern("[multi_sink_example] [%^%l%$] %v");

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/multisink.txt", true);
        file_sink->set_level(spdlog::level::trace);

        spdlog::logger logger("multi_sink", {console_sink, file_sink});
        logger.set_level(spdlog::level::debug);
        logger.warn("this should appear in both console and file");
        logger.info("this message should appear in the console and in the file");
    }

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

		{
        // store the old logger so we don't break other examples.
        auto old_logger = spdlog::default_logger();

        auto new_logger = spdlog::basic_logger_mt("new_default_logger", "logs/new-default-log.txt", true);
        spdlog::set_default_logger(new_logger);
        spdlog::set_level(spdlog::level::info);
        spdlog::debug("This message should not be displayed!");
        spdlog::set_level(spdlog::level::trace);
        spdlog::debug("This message should be displayed..");

        spdlog::set_default_logger(old_logger);
    }

		{
        // Create color multi threaded logger.
        auto console = spdlog::stdout_color_mt("console");
        // or for stderr:
        // auto console = spdlog::stderr_color_mt("error-logger");
        auto dual_sink = spdlog::dual_color_mt("dual-sink");
        dual_sink->set_level(spdlog::level::debug);
        dual_sink->info("dual-sink");
        dual_sink->error("error");
        dual_sink->critical("critical");
        dual_sink->debug("debug");
    }

    spdlog::set_pattern("[%^%H:%M:%S %z%$] [%^%L%$] [%^thread %t%$] %{fg_green}^%v%$");
    spdlog::set_level(spdlog::level::trace);
    spdlog::trace("I work on other levels?");
    spdlog::debug("I work on other levels?");
    spdlog::info("I work on other levels?");
    spdlog::warn("I work on other levels?");
    spdlog::error("I work on other levels?");
    spdlog::critical("I work on other levels?");
    spdlog::set_pattern("%+");  // back to default format

    try {
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
