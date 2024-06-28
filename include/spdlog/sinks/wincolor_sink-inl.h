// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#ifndef SPDLOG_HEADER_ONLY
    #include <spdlog/sinks/wincolor_sink.h>
#endif

#include <spdlog/details/windows_include.h>
#include <spdlog/details/log_msg.h>
#include <wincon.h>

#include <spdlog/common.h>
#include <spdlog/pattern_formatter.h>

namespace spdlog {
namespace sinks {

#if defined(SPDLOG_EXTENDED_STYLING)

#define WINCOLOR_RESET       ~0x0000
#define WINCOLOR_NEGATE_BIT  0x10000

namespace details {

static spdlog::details::winstyle_codes wincolor_table{
    0,  // null_style

    // font style
    WINCOLOR_RESET,  // reset
    FOREGROUND_INTENSITY,  // bold
    ~FOREGROUND_INTENSITY,  // dark
    COMMON_LVB_UNDERSCORE,  // underline
    0,  // blink
    COMMON_LVB_REVERSE_VIDEO,  // reverse

    // font foreground colors
    ~(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE),  // fg_black
    FOREGROUND_RED,  // fg_red
    FOREGROUND_GREEN,                                         // fg_green
    FOREGROUND_RED | FOREGROUND_GREEN,                       // fg_yellow
    FOREGROUND_BLUE,                                         // fg_blue
    FOREGROUND_RED | FOREGROUND_BLUE,                       // fg_magenta
    FOREGROUND_GREEN | FOREGROUND_BLUE,                       // fg_cyan
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,     // fg_white
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,     // fg_default

    // font  background colors
    ~(BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE),  // bg_black
    BACKGROUND_RED,     // bg_red
    BACKGROUND_GREEN,     // bg_green
    BACKGROUND_RED | BACKGROUND_GREEN,     // bg_yellow
    BACKGROUND_BLUE,     // bg_blue
    BACKGROUND_RED | BACKGROUND_BLUE,     // bg_magenta
    BACKGROUND_GREEN | BACKGROUND_BLUE,     // bg_cyan
    BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,     // bg_white
    ~(BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE),  // bg_default
};

}  // namespace details

#endif

template <typename ConsoleMutex>
SPDLOG_INLINE wincolor_sink<ConsoleMutex>::wincolor_sink(void *out_handle, color_mode mode)
    : out_handle_(out_handle),
      mutex_(ConsoleMutex::mutex()),
      formatter_(spdlog::details::make_unique<spdlog::pattern_formatter>()) {
    set_color_mode_impl(mode);
    // set level colors
    colors_[level::trace] = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;  // white
    colors_[level::debug] = FOREGROUND_GREEN | FOREGROUND_BLUE;                   // cyan
    colors_[level::info] = FOREGROUND_GREEN;                                      // green
    colors_[level::warn] =
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;  // intense yellow
    colors_[level::err] = FOREGROUND_RED | FOREGROUND_INTENSITY;   // intense red
    colors_[level::critical] = BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN |
                               FOREGROUND_BLUE |
                               FOREGROUND_INTENSITY;  // intense white on red background
    colors_[level::off] = 0;
}

template <typename ConsoleMutex>
SPDLOG_INLINE wincolor_sink<ConsoleMutex>::~wincolor_sink() {
    this->flush();
}

// change the color for the given level
template <typename ConsoleMutex>
void SPDLOG_INLINE wincolor_sink<ConsoleMutex>::set_color(level::level_enum level,
                                                          std::uint16_t color) {
    std::lock_guard<mutex_t> lock(mutex_);
    colors_[static_cast<size_t>(level)] = color;
}

template <typename ConsoleMutex>
void SPDLOG_INLINE wincolor_sink<ConsoleMutex>::log(const spdlog::details::log_msg &msg) {
    if (out_handle_ == nullptr || out_handle_ == INVALID_HANDLE_VALUE) {
        return;
    }

    std::lock_guard<mutex_t> lock(mutex_);
#if !defined(SPDLOG_EXTENDED_STYLING)
    msg.color_range_start = 0;
    msg.color_range_end = 0;
    memory_buf_t formatted;
    formatter_->format(msg, formatted);
    if (should_do_colors_ && msg.color_range_end > msg.color_range_start) {
        // before color range
        print_range_(formatted, 0, msg.color_range_start);
        // in color range
        auto orig_attribs =
            static_cast<WORD>(set_foreground_color_(colors_[static_cast<size_t>(msg.level)]));
        print_range_(formatted, msg.color_range_start, msg.color_range_end);
        // reset to orig colors
        ::SetConsoleTextAttribute(static_cast<HANDLE>(out_handle_), orig_attribs);
        print_range_(formatted, msg.color_range_end, formatted.size());
    } else  // print without colors if color range is invalid (or color is disabled)
#else
    msg.styling_ranges.clear();
    memory_buf_t formatted;
    formatter_->format(msg, formatted);
    if (should_do_colors_ && msg.styling_ranges.size() != 0) {
        spdlog::details::styling_info &style = msg.styling_ranges.at(0);
        // before color range
        auto orig_attribs = get_foreground_color_();
        print_range_(formatted, 0, style.position);
        for (auto &next_style : msg.styling_ranges) {
            if (style.is_start) {
                // in color range

                // each styling formatter can contain multiple styles in its style spec.
                // loop through each style and print the style code.
                auto styles = style.styles;
                WORD attribs = 0;
                try {
                    for (int i = 0; styles[i] != spdlog::details::style_type::null_style; i++) {
                        //  in style range
                        auto v = details::wincolor_table.at(static_cast<size_t>(styles[i]));
                        if (v & WINCOLOR_NEGATE_BIT)
                            attribs &= ~v;
                        else
                            attribs |= v;
                    }
                }
								catch (const std::out_of_range &ex) {  // full style spec output looks either amazing or trash
                }

                // it is possible to not provide a style spec with the formatter, in that
                // case the first style spec will be null_style therefore default to the
                // generic log level style
                if (styles[0] == spdlog::details::style_type::null_style) {
                    attribs = colors_[static_cast<size_t>(msg.level)];
                }

								set_foreground_color_(attribs);
                print_range_(formatted, style.position, next_style.position);
            } else {
                ::SetConsoleTextAttribute(static_cast<HANDLE>(out_handle_), orig_attribs);
                print_range_(formatted, style.position, next_style.position);
            }
            style = next_style;
        }
        // reset to orig colors
        ::SetConsoleTextAttribute(static_cast<HANDLE>(out_handle_), orig_attribs);
        print_range_(formatted, style.position, formatted.size());
    } else  // print without colors if color range is invalid (or color is disabled)
#endif
    {
        write_to_file_(formatted);
    }
}

template <typename ConsoleMutex>
void SPDLOG_INLINE wincolor_sink<ConsoleMutex>::set_output(FILE *override_output)
{
    std::lock_guard<mutex_t> guard(mutex_);
    out_handle_ = override_output;
	set_color_mode_impl(color_mode::automatic);
}

template<typename ConsoleMutex>
void SPDLOG_INLINE wincolor_sink<ConsoleMutex>::flush() {
    // windows console always flushed?
}

template <typename ConsoleMutex>
void SPDLOG_INLINE wincolor_sink<ConsoleMutex>::set_pattern(const std::string &pattern) {
    std::lock_guard<mutex_t> lock(mutex_);
    formatter_ = std::unique_ptr<spdlog::formatter>(new pattern_formatter(pattern));
}

template <typename ConsoleMutex>
void SPDLOG_INLINE
wincolor_sink<ConsoleMutex>::set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) {
    std::lock_guard<mutex_t> lock(mutex_);
    formatter_ = std::move(sink_formatter);
}

template <typename ConsoleMutex>
void SPDLOG_INLINE wincolor_sink<ConsoleMutex>::set_color_mode(color_mode mode) {
    std::lock_guard<mutex_t> lock(mutex_);
    set_color_mode_impl(mode);
}

template <typename ConsoleMutex>
void SPDLOG_INLINE wincolor_sink<ConsoleMutex>::set_color_mode_impl(color_mode mode) {
    if (mode == color_mode::automatic) {
        // should do colors only if out_handle_ points to actual console.
        DWORD console_mode;
        bool in_console = ::GetConsoleMode(static_cast<HANDLE>(out_handle_), &console_mode) != 0;
        should_do_colors_ = in_console;
    } else {
        should_do_colors_ = mode == color_mode::always ? true : false;
    }
}

// set foreground color and return the orig console attributes (for resetting later)
template <typename ConsoleMutex>
std::uint16_t SPDLOG_INLINE
wincolor_sink<ConsoleMutex>::set_foreground_color_(std::uint16_t attribs) {
    CONSOLE_SCREEN_BUFFER_INFO orig_buffer_info;
    if (!::GetConsoleScreenBufferInfo(static_cast<HANDLE>(out_handle_), &orig_buffer_info)) {
        // just return white if failed getting console info
        return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }

    // change only the foreground bits (lowest 4 bits)
    auto new_attribs = static_cast<WORD>(attribs) | (orig_buffer_info.wAttributes & 0xfff0);
    auto ignored =
        ::SetConsoleTextAttribute(static_cast<HANDLE>(out_handle_), static_cast<WORD>(new_attribs));
    (void)(ignored);
    return static_cast<std::uint16_t>(orig_buffer_info.wAttributes);  // return orig attribs
}

// return the current console attributes (for resetting later)
template <typename ConsoleMutex>
std::uint16_t SPDLOG_INLINE
wincolor_sink<ConsoleMutex>::get_foreground_color_() {
    CONSOLE_SCREEN_BUFFER_INFO orig_buffer_info;
    if (!::GetConsoleScreenBufferInfo(static_cast<HANDLE>(out_handle_), &orig_buffer_info)) {
        // just return white if failed getting console info
        return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }
    return static_cast<std::uint16_t>(orig_buffer_info.wAttributes);  // return orig attribs
}

// print a range of formatted message to console
template <typename ConsoleMutex>
void SPDLOG_INLINE wincolor_sink<ConsoleMutex>::print_range_(const memory_buf_t &formatted,
                                                             size_t start,
                                                             size_t end) {
    if (end > start) {
#if defined(SPDLOG_UTF8_TO_WCHAR_CONSOLE)
        wmemory_buf_t wformatted;
        spdlog::details::os::utf8_to_wstrbuf(string_view_t(formatted.data() + start, end - start),
                                     wformatted);
        auto size = static_cast<DWORD>(wformatted.size());
        auto ignored = ::WriteConsoleW(static_cast<HANDLE>(out_handle_), wformatted.data(), size,
                                       nullptr, nullptr);
    #else
        auto size = static_cast<DWORD>(end - start);
        auto ignored = ::WriteConsoleA(static_cast<HANDLE>(out_handle_), formatted.data() + start,
                                       size, nullptr, nullptr);
    #endif
        (void)(ignored);
    }
}

template <typename ConsoleMutex>
void SPDLOG_INLINE wincolor_sink<ConsoleMutex>::write_to_file_(const memory_buf_t &formatted) {
    auto size = static_cast<DWORD>(formatted.size());
    DWORD bytes_written = 0;
    auto ignored = ::WriteFile(static_cast<HANDLE>(out_handle_), formatted.data(), size,
                               &bytes_written, nullptr);
    (void)(ignored);
}

// wincolor_stdout_sink
template <typename ConsoleMutex>
SPDLOG_INLINE wincolor_stdout_sink<ConsoleMutex>::wincolor_stdout_sink(color_mode mode)
    : wincolor_sink<ConsoleMutex>(::GetStdHandle(STD_OUTPUT_HANDLE), mode) {}

// wincolor_stderr_sink
template <typename ConsoleMutex>
SPDLOG_INLINE wincolor_stderr_sink<ConsoleMutex>::wincolor_stderr_sink(color_mode mode)
    : wincolor_sink<ConsoleMutex>(::GetStdHandle(STD_ERROR_HANDLE), mode) {}

// wincolor_dual_sink
template<typename ConsoleMutex>
SPDLOG_INLINE wincolor_dual_sink<ConsoleMutex>::wincolor_dual_sink(color_mode mode)
    : wincolor_sink<ConsoleMutex>(stdout, mode)
{}

template<typename ConsoleMutex>
SPDLOG_INLINE void wincolor_dual_sink<ConsoleMutex>::log(const spdlog::details::log_msg &msg) {
    if (msg.level <= level::warn && msg.level > level::debug)
    {
        wincolor_sink<ConsoleMutex>::set_output(stdout);
    }
    else
    {
        wincolor_sink<ConsoleMutex>::set_output(stderr);
    }

    wincolor_sink<ConsoleMutex>::log(msg);
}
}  // namespace sinks
}  // namespace spdlog
