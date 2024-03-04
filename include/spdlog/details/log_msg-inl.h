// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#ifndef SPDLOG_HEADER_ONLY
    #include <spdlog/details/log_msg.h>
#endif

#include <spdlog/details/os.h>

namespace spdlog {
namespace details {

SPDLOG_INLINE log_msg::log_msg(spdlog::log_clock::time_point log_time,
                               spdlog::source_loc loc,
							   spdlog::process_info pinfo,
                               string_view_t a_logger_name,
                               spdlog::level::level_enum lvl,
                               spdlog::string_view_t msg,
							   const Field * fields, 
							   size_t field_count)
    : logger_name(a_logger_name)
    , level(lvl)
    , time(log_time)
    , process_info(pinfo)
    , source(loc)
    , payload(msg)
#ifndef SPDLOG_NO_STRUCTURED_SPDLOG
	, field_data(const_cast<Field *>(fields))
    , field_data_count(field_count)
    , context_field_data(threadlocal_context_head()
#else
	//, field_data(nullptr)
	//, field_data_count(0)
	, context_field_data(threadlocal_context_head())
#pragma message("You are building spdlog with SPDLOG_NO_STRUCTURED_SPDLOG turned on and thus structured logging DISabled. Is this what you want, really?")
#endif
	)
{}

SPDLOG_INLINE log_msg::log_msg(spdlog::log_clock::time_point log_time,
                               spdlog::source_loc loc,
							   spdlog::process_info pinfo,
                               string_view_t a_logger_name,
                               spdlog::level::level_enum lvl,
                               spdlog::string_view_t msg)
    : logger_name(a_logger_name)
    , level(lvl)
    , time(log_time)
    , process_info(pinfo)
    , source(loc)
    , payload(msg)
    //, field_data(nullptr)
    //, field_data_count(0)
    , context_field_data(threadlocal_context_head())
{}

SPDLOG_INLINE log_msg::log_msg(spdlog::log_clock::time_point log_time, source_loc loc, string_view_t logger_name, level::level_enum lvl, string_view_t msg, const Field * fields, size_t field_count)
    : log_msg(log_time, loc, os::pinfo(), logger_name, lvl, msg, fields, field_count)
{}

SPDLOG_INLINE log_msg::log_msg(spdlog::log_clock::time_point log_time, 
                               spdlog::source_loc loc, 
                               string_view_t a_logger_name,
                               spdlog::level::level_enum lvl,
                               spdlog::string_view_t msg)
    : log_msg(log_time, loc, os::pinfo(), a_logger_name, lvl, msg)
{}

SPDLOG_INLINE log_msg::log_msg(source_loc loc, spdlog::process_info pinfo, string_view_t logger_name, level::level_enum lvl, string_view_t msg, const Field * fields, size_t field_count)
    : log_msg(os::now(), loc, pinfo, logger_name, lvl, msg, fields, field_count)
{}

SPDLOG_INLINE log_msg::log_msg(spdlog::source_loc loc, spdlog::process_info pinfo, string_view_t a_logger_name,
    spdlog::level::level_enum lvl, spdlog::string_view_t msg)
    : log_msg(os::now(), loc, pinfo, a_logger_name, lvl, msg)
{}

SPDLOG_INLINE log_msg::log_msg(source_loc loc, string_view_t logger_name, level::level_enum lvl, string_view_t msg, const Field * fields, size_t field_count)
    : log_msg(os::now(), loc, os::pinfo(), logger_name, lvl, msg, fields, field_count)
{}

SPDLOG_INLINE log_msg::log_msg(
    spdlog::source_loc loc, string_view_t a_logger_name, spdlog::level::level_enum lvl, spdlog::string_view_t msg)
    : log_msg(os::now(), loc, os::pinfo(), a_logger_name, lvl, msg)
{}

SPDLOG_INLINE log_msg::log_msg(spdlog::process_info pinfo, string_view_t logger_name, level::level_enum lvl, string_view_t msg, const Field * fields, size_t field_count)
    : log_msg(os::now(), source_loc{}, pinfo, logger_name, lvl, msg, fields, field_count)
{}

SPDLOG_INLINE log_msg::log_msg(spdlog::process_info pinfo, string_view_t a_logger_name, spdlog::level::level_enum lvl, 
    spdlog::string_view_t msg)
    : log_msg(os::now(), source_loc{}, pinfo, a_logger_name, lvl, msg)
{}

SPDLOG_INLINE log_msg::log_msg(string_view_t logger_name, level::level_enum lvl, string_view_t msg, const Field * fields, size_t field_count)
    : log_msg(os::now(), source_loc{}, os::pinfo(), logger_name, lvl, msg, fields, field_count)
{}

SPDLOG_INLINE log_msg::log_msg(string_view_t a_logger_name, spdlog::level::level_enum lvl, spdlog::string_view_t msg)
    : log_msg(os::now(), source_loc{}, os::pinfo(), a_logger_name, lvl, msg)
{}

} // namespace details
} // namespace spdlog
