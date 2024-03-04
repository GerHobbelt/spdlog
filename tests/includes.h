#pragma once

#if defined(__GNUC__) && __GNUC__ == 12
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"  // Workaround for GCC 12
#endif
#include <catch2/catch_all.hpp>
#if defined(__GNUC__) && __GNUC__ == 12
    #pragma GCC diagnostic pop
#endif

#include "utils.h"
#include <chrono>
#include <cstdio>
#include <exception>
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <iomanip>
#include <stdlib.h>

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG

#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/details/fmt_helper.h"
#include "spdlog/pattern_formatter.h"

#include "spdlog/json_formatter.h"
#include "spdlog/structured_spdlog.h"

// all om 'em:
#include "spdlog/sinks/android_sink.h"
#include "spdlog/sinks/ansicolor_sink.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/callback_sink.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/dist_sink.h"
#include "spdlog/sinks/dup_filter_sink.h"
#include "spdlog/sinks/hourly_file_sink.h"
//#include "spdlog/sinks/kafka_sink.h"
#include "spdlog/sinks/minutely_file_sink.h"
//#include "spdlog/sinks/mongo_sink.h"
#include "spdlog/sinks/msvc_sink.h"
#include "spdlog/sinks/null_sink.h"
#include "spdlog/sinks/ostream_sink.h"
//#include "spdlog/sinks/qt_sinks.h"
#include "spdlog/sinks/ringbuffer_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/syslog_sink.h"
#include "spdlog/sinks/systemd_sink.h"
#include "spdlog/sinks/tcp_sink.h"
#include "spdlog/sinks/udp_sink.h"
#include "spdlog/sinks/wincolor_sink.h"
#include "spdlog/sinks/win_eventlog_sink.h"
