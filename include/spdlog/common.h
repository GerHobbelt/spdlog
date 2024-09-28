// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <spdlog/tweakme.h>

#if !defined(SPDLOG_USE_STD_FORMAT) && defined(_WIN32) && defined(_UNICODE)
//  #if !defined(SPDLOG_WCHAR_FILENAMES)
//    #define SPDLOG_WCHAR_FILENAMES	1
//	#endif
	#if !defined(SPDLOG_WCHAR_TO_UTF8_SUPPORT)
    #define SPDLOG_WCHAR_TO_UTF8_SUPPORT  1
	#endif
	#if !defined(SPDLOG_UTF8_TO_WCHAR_CONSOLE)
		#define SPDLOG_UTF8_TO_WCHAR_CONSOLE  1
	#endif
#endif

#include <spdlog/details/null_mutex.h>
#include <spdlog/details/source_location.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <exception>
#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <type_traits>

#ifdef SPDLOG_USE_STD_FORMAT
    #include <version>
    #if __cpp_lib_format >= 202110L
        #include <format>
    #else
        #include <string_view>
    #endif
#endif

#ifdef SPDLOG_COMPILED_LIB
    #undef SPDLOG_HEADER_ONLY
    #if defined(SPDLOG_SHARED_LIB)
        #if defined(_WIN32)
            #ifdef spdlog_EXPORTS
                #define SPDLOG_API __declspec(dllexport)
            #else  // !spdlog_EXPORTS
                #define SPDLOG_API __declspec(dllimport)
            #endif
        #else  // !defined(_WIN32)
            #define SPDLOG_API __attribute__((visibility("default")))
        #endif
    #else  // !defined(SPDLOG_SHARED_LIB)
        #define SPDLOG_API
    #endif
    #define SPDLOG_INLINE
#else  // !defined(SPDLOG_COMPILED_LIB)
    #define SPDLOG_API
    #define SPDLOG_HEADER_ONLY
    #define SPDLOG_INLINE inline
#endif  // #ifdef SPDLOG_COMPILED_LIB

#include <spdlog/fmt/fmt.h>

#if !defined(SPDLOG_USE_STD_FORMAT) && \
    FMT_VERSION >= 80000  // backward compatibility with fmt versions older than 8
    #define SPDLOG_FMT_RUNTIME(format_string) fmt::runtime(format_string)
    #define SPDLOG_FMT_STRING(format_string) FMT_STRING(format_string)
    #if defined(SPDLOG_WCHAR_FILENAMES) || defined(SPDLOG_WCHAR_TO_UTF8_SUPPORT) || defined(SPDLOG_UTF8_TO_WCHAR_CONSOLE)
        #include <spdlog/fmt/xchar.h>
    #endif
#else
    #define SPDLOG_FMT_RUNTIME(format_string) format_string
    #define SPDLOG_FMT_STRING(format_string) format_string
#endif

// visual studio up to 2013 does not support noexcept nor constexpr
#if defined(_MSC_VER) && (_MSC_VER < 1900)
    #define SPDLOG_NOEXCEPT _NOEXCEPT
    #define SPDLOG_CONSTEXPR
#else
    #define SPDLOG_NOEXCEPT noexcept
    #define SPDLOG_CONSTEXPR constexpr
#endif

#ifdef SPDLOG_LOGGER_NOEXCEPT
    #define SPDLOG_COND_NOEXCEPT SPDLOG_NOEXCEPT
#else
    #define SPDLOG_COND_NOEXCEPT
#endif

// If building with std::format, can just use constexpr, otherwise if building with fmt
// SPDLOG_CONSTEXPR_FUNC needs to be set the same as FMT_CONSTEXPR to avoid situations where
// a constexpr function in spdlog could end up calling a non-constexpr function in fmt
// depending on the compiler
// If fmt determines it can't use constexpr, we should inline the function instead
#ifdef SPDLOG_USE_STD_FORMAT
    #define SPDLOG_CONSTEXPR_FUNC constexpr
#else  // Being built with fmt
    #if FMT_USE_CONSTEXPR
        #define SPDLOG_CONSTEXPR_FUNC FMT_CONSTEXPR
    #else
        #define SPDLOG_CONSTEXPR_FUNC inline
    #endif
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define SPDLOG_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
    #define SPDLOG_DEPRECATED __declspec(deprecated)
#else
    #define SPDLOG_DEPRECATED
#endif

// disable thread local on msvc 2013
#ifndef SPDLOG_NO_TLS
    #if (defined(_MSC_VER) && (_MSC_VER < 1900)) || defined(__cplusplus_winrt)
        #define SPDLOG_NO_TLS 1
    #endif
#endif

#ifndef SPDLOG_FUNCTION
    #define SPDLOG_FUNCTION static_cast<const char *>(__FUNCTION__)
#endif

#ifdef SPDLOG_NO_EXCEPTIONS
    #define SPDLOG_TRY
    #define SPDLOG_THROW(ex)                               \
        do {                                               \
            printf("spdlog fatal error: %s\n", ex.what()); \
            std::abort();                                  \
        } while (0)
    #define SPDLOG_CATCH_STD
#else
    #define SPDLOG_TRY try
    #define SPDLOG_THROW(ex) throw(ex)
    #define SPDLOG_CATCH_STD             \
        catch (const std::exception &) { \
        }
#endif

#if SPDLOG_CPLUSPLUS > 201811L
#   define SPDLOG_CONSTEVAL consteval
#elif SPDLOG_CPLUSPLUS < 201402L
#    define SPDLOG_CONSTEVAL 
#else
#    define SPDLOG_CONSTEVAL constexpr
#endif

namespace spdlog {

class formatter;

#if !defined(SPDLOG_USE_STD_FORMAT)
template <typename Char>
#    if FMT_VERSION >= 90101
using fmt_runtime_string = fmt::runtime_format_string<Char>;
#    else
using fmt_runtime_string = fmt::basic_runtime<Char>;
#    endif
#endif

#ifndef SPDLOG_NO_SOURCE_LOC

struct source_loc
{
    SPDLOG_CONSTEXPR source_loc() = default;
    SPDLOG_CONSTEXPR source_loc(const char *filename_in, unsigned line_in, const char *funcname_in)
        : filename{filename_in}
        , line{line_in}
        , funcname{funcname_in}
    {}
    SPDLOG_CONSTEXPR source_loc(details::source_location location)
        : filename{location.file_name()}
        , line{location.line()}
        , funcname{location.function_name()}
    {}
    SPDLOG_CONSTEXPR source_loc static current(details::source_location cur = details::source_location::current())
    {
        return source_loc{cur.file_name(), cur.line(), cur.function_name()};
    }

    SPDLOG_CONSTEXPR bool empty() const SPDLOG_NOEXCEPT
    {
        return line <= 0;
    }
    const char *filename{nullptr};
    unsigned line{0};
    const char *funcname{nullptr};
};

template<typename T, typename Char>
struct format_string_wrapper
{
    SPDLOG_CONSTEVAL format_string_wrapper(const Char* fmtstr, source_loc loc = source_loc::current())
        : fmt_{fmtstr}
        , loc_{loc}
    {}
#if !defined(SPDLOG_USE_STD_FORMAT) && FMT_VERSION >= 80000
    SPDLOG_CONSTEXPR format_string_wrapper(fmt_runtime_string<Char> fmtstr, source_loc loc = source_loc::current())
        : fmt_{fmtstr}
        , loc_{loc}
    {}
#elif defined(SPDLOG_USE_STD_FORMAT) && SPDLOG_CPLUSPLUS >= 202002L
    template <typename S>
    requires std::is_convertible_v<S, T>
    SPDLOG_CONSTEXPR format_string_wrapper(S fmtstr, source_loc loc = source_loc::current())
        : fmt_{fmtstr}
        , loc_{loc}
    {}
#endif
    T format_string()
    {
        return fmt_;
    }
    source_loc location()
    {
        return loc_;
    }

private:
    T fmt_;
    source_loc loc_;
};

#else

struct source_loc {
    SPDLOG_CONSTEXPR source_loc() = default;
};

template <typename T, typename Char>
struct format_string_wrapper {
    SPDLOG_CONSTEVAL format_string_wrapper(const Char *fmtstr)
        : fmt_{fmtstr} {}
    #if !defined(SPDLOG_USE_STD_FORMAT) && FMT_VERSION >= 80000
    SPDLOG_CONSTEXPR format_string_wrapper(fmt_runtime_string<Char> fmtstr)
        : fmt_{fmtstr} {}
    #elif defined(SPDLOG_USE_STD_FORMAT) && SPDLOG_CPLUSPLUS >= 202002L
    template <typename S>
    requires std::is_convertible_v<S, T>
        SPDLOG_CONSTEXPR format_string_wrapper(S fmtstr)
        : fmt_{fmtstr} {}
    #endif
    T format_string() {
        return fmt_;
    }

private:
    T fmt_;
};

#endif  // SPDLOG_NO_SOURCE_LOC

namespace sinks {
class sink;
}

#if defined(_WIN32) && defined(SPDLOG_WCHAR_FILENAMES)
using filename_t = std::wstring;
    // allow macro expansion to occur in SPDLOG_FILENAME_T
    #define SPDLOG_FILENAME_T_INNER(s) L##s
    #define SPDLOG_FILENAME_T(s) SPDLOG_FILENAME_T_INNER(s)
#else
using filename_t = std::string;
    #define SPDLOG_FILENAME_T(s) s
#endif

using log_clock = std::chrono::system_clock;
using sink_ptr = std::shared_ptr<sinks::sink>;
using sinks_init_list = std::initializer_list<sink_ptr>;
using err_handler = std::function<void(const std::string &err_msg)>;
#ifdef SPDLOG_USE_STD_FORMAT
namespace fmt_lib = std;

using string_view_t = std::string_view;
using memory_buf_t = std::string;

template <typename... Args>
    #if __cpp_lib_format >= 202110L
using format_string_t = format_string_wrapper<std::format_string<Args...>, char>;
    #else
using format_string_t = format_string_wrapper<std::string_view, char>;
    #endif

template <class T, class Char = char>
struct is_convertible_to_basic_format_string
    : std::integral_constant<bool, std::is_convertible<T, std::basic_string_view<Char>>::value> {};

    #if defined(SPDLOG_WCHAR_FILENAMES) || defined(SPDLOG_WCHAR_TO_UTF8_SUPPORT) || defined(SPDLOG_UTF8_TO_WCHAR_CONSOLE)
using wstring_view_t = std::wstring_view;
using wmemory_buf_t = std::wstring;

template <typename... Args>
        #if __cpp_lib_format >= 202110L
using wformat_string_t = format_string_wrapper<std::wformat_string<Args...>, wchar_t>;
        #else
using wformat_string_t = format_string_wrapper<std::wstring_view, wchar_t>;
        #endif
    #endif
    #define SPDLOG_BUF_TO_STRING(x) x
#else  // use fmt lib instead of std::format
namespace fmt_lib = fmt;

using string_view_t = fmt::basic_string_view<char>;
using memory_buf_t = fmt::basic_memory_buffer<char, 250>;

template <typename... Args>
using format_string_t = format_string_wrapper<fmt::format_string<Args...>, char>;

template <class T>
using remove_cvref_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

template <typename Char>
    #if FMT_VERSION >= 90101
using fmt_runtime_string = fmt::runtime_format_string<Char>;
    #else
using fmt_runtime_string = fmt::basic_runtime<Char>;
    #endif

// clang doesn't like SFINAE disabled constructor in std::is_convertible<> so have to repeat the
// condition from basic_format_string here, in addition, fmt::basic_runtime<Char> is only
// convertible to basic_format_string<Char> but not basic_string_view<Char>
template <class T, class Char = char>
struct is_convertible_to_basic_format_string
    : std::integral_constant<bool,
                             std::is_convertible<T, fmt::basic_string_view<Char>>::value ||
                                 std::is_same<remove_cvref_t<T>, fmt_runtime_string<Char>>::value> {
};


    #if defined(SPDLOG_WCHAR_FILENAMES) || defined(SPDLOG_WCHAR_TO_UTF8_SUPPORT) || defined(SPDLOG_UTF8_TO_WCHAR_CONSOLE)
using wstring_view_t = fmt::basic_string_view<wchar_t>;
using wmemory_buf_t = fmt::basic_memory_buffer<wchar_t, 250>;

template <typename... Args>
using wformat_string_t = format_string_wrapper<fmt::wformat_string<Args...>, wchar_t>;
    #endif
    #define SPDLOG_BUF_TO_STRING(x) fmt::to_string(x)
#endif

#ifdef SPDLOG_WCHAR_TO_UTF8_SUPPORT
    #ifndef _WIN32
        #error SPDLOG_WCHAR_TO_UTF8_SUPPORT only supported on windows
    #endif  // _WIN32
#endif      // SPDLOG_WCHAR_TO_UTF8_SUPPORT

template <class T>
struct is_convertible_to_any_format_string
    : std::integral_constant<bool,
                             is_convertible_to_basic_format_string<T, char>::value ||
                                 is_convertible_to_basic_format_string<T, wchar_t>::value> {};

#if defined(SPDLOG_NO_ATOMIC_LEVELS)
using level_t = details::null_atomic_int;
#else
using level_t = std::atomic<int>;
#endif

#define SPDLOG_LEVEL_TRACE 0
#define SPDLOG_LEVEL_DEBUG 1
#define SPDLOG_LEVEL_INFO 2
#define SPDLOG_LEVEL_WARN 3
#define SPDLOG_LEVEL_ERROR 4
#define SPDLOG_LEVEL_CRITICAL 5
#define SPDLOG_LEVEL_OFF 6

#if !defined(SPDLOG_ACTIVE_LEVEL)
    #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif

// Log level enum
namespace level {
enum level_enum : int {
    trace = SPDLOG_LEVEL_TRACE,
    debug = SPDLOG_LEVEL_DEBUG,
    info = SPDLOG_LEVEL_INFO,
    warn = SPDLOG_LEVEL_WARN,
    err = SPDLOG_LEVEL_ERROR,
    critical = SPDLOG_LEVEL_CRITICAL,
    off = SPDLOG_LEVEL_OFF,
    n_levels
};

#define SPDLOG_LEVEL_NAME_TRACE spdlog::string_view_t("trace", 5)
#define SPDLOG_LEVEL_NAME_DEBUG spdlog::string_view_t("debug", 5)
#define SPDLOG_LEVEL_NAME_INFO spdlog::string_view_t("info", 4)
#define SPDLOG_LEVEL_NAME_WARNING spdlog::string_view_t("warning", 7)
#define SPDLOG_LEVEL_NAME_ERROR spdlog::string_view_t("error", 5)
#define SPDLOG_LEVEL_NAME_CRITICAL spdlog::string_view_t("critical", 8)
#define SPDLOG_LEVEL_NAME_OFF spdlog::string_view_t("off", 3)

#if !defined(SPDLOG_LEVEL_NAMES)
    #define SPDLOG_LEVEL_NAMES                                                                  \
        {                                                                                       \
            SPDLOG_LEVEL_NAME_TRACE, SPDLOG_LEVEL_NAME_DEBUG, SPDLOG_LEVEL_NAME_INFO,           \
                SPDLOG_LEVEL_NAME_WARNING, SPDLOG_LEVEL_NAME_ERROR, SPDLOG_LEVEL_NAME_CRITICAL, \
                SPDLOG_LEVEL_NAME_OFF                                                           \
        }
#endif

#if !defined(SPDLOG_SHORT_LEVEL_NAMES)

    #define SPDLOG_SHORT_LEVEL_NAMES \
        { "T", "D", "I", "W", "E", "C", "O" }
#endif

SPDLOG_API const string_view_t &to_string_view(spdlog::level::level_enum l) SPDLOG_NOEXCEPT;
SPDLOG_API const char *to_short_c_str(spdlog::level::level_enum l) SPDLOG_NOEXCEPT;
SPDLOG_API spdlog::level::level_enum from_str(const std::string &name) SPDLOG_NOEXCEPT;

}  // namespace level

//
// Color mode used by sinks with color support.
//
enum class color_mode { always, automatic, never };

//
// Pattern time - specific time getting to use for pattern_formatter.
// local time by default
//
enum class pattern_time_type {
    local,  // log localtime
    utc     // log utc
};
        
//
// rotate file mode used by rotate sink
//
enum class rotate_file_mode
{
    desc,
    asc,
};


//
// Log exception
//
class SPDLOG_API spdlog_ex : public std::exception {
public:
    explicit spdlog_ex(std::string msg);
    spdlog_ex(const std::string &msg, int last_errno);
    const char *what() const SPDLOG_NOEXCEPT override;

private:
    std::string msg_;
};

[[noreturn]] SPDLOG_API void throw_spdlog_ex(const std::string &msg, int last_errno);
[[noreturn]] SPDLOG_API void throw_spdlog_ex(std::string msg);

struct process_info
{
    SPDLOG_CONSTEXPR process_info() = default;
    SPDLOG_CONSTEXPR process_info(int process_id)
        : process_id(process_id)
    {}
    SPDLOG_CONSTEXPR process_info(int process_id, size_t thread_id)
        : process_id(process_id)
			  , thread_id(thread_id)
		{}
    SPDLOG_CONSTEXPR process_info(int process_id, size_t thread_id, const std::string &thread_name)
        : process_id(process_id)
        , thread_id(thread_id)
        , thread_name(thread_name)
    {}

    int process_id{0};
    size_t thread_id{0};
    std::string thread_name{};
};

struct file_event_handlers {
    file_event_handlers()
        : before_open(nullptr),
          after_open(nullptr),
          before_close(nullptr),
          after_close(nullptr) {}

    std::function<void(const filename_t &filename)> before_open;
    std::function<void(const filename_t &filename, std::FILE *file_stream)> after_open;
    std::function<void(const filename_t &filename, std::FILE *file_stream)> before_close;
    std::function<void(const filename_t &filename)> after_close;
};

// Cover all fundamental types for C++ plus string_view
//   see https://en.cppreference.com/w/cpp/language/types
// This would be better handled by a C++14 std::variant type, but we want to stay C++11-compatible
enum class FieldValueType {
    STRING_VIEW,
    SHORT, USHORT, INT, UINT, LONG, ULONG, LONGLONG, ULONGLONG,
    BOOL,
    CHAR, UCHAR, WCHAR,
    FLOAT, DOUBLE, LONGDOUBLE
};

struct Field {
    spdlog::string_view_t name;
    FieldValueType        value_type;
    union  {
        string_view_t string_view_;
        short short_; unsigned short ushort_; int int_; unsigned int uint_;
        long long_; unsigned long ulong_; long long longlong_; unsigned long long ulonglong_;
        bool bool_;
        char char_; unsigned char uchar_; wchar_t wchar_;
        float float_; double double_; long double longdouble_;
    };
    Field(const string_view_t &field_name, FieldValueType field_type) :
        name(field_name), value_type(field_type) {};
    Field() : name{}, value_type(FieldValueType::INT), int_(0) {}

    Field(const string_view_t &field_name, string_view_t      val): name(field_name), value_type(FieldValueType::STRING_VIEW), string_view_(val) {}
    Field(const string_view_t &field_name, short              val): name(field_name), value_type(FieldValueType::SHORT),       short_      (val) {}
    Field(const string_view_t &field_name, unsigned short     val): name(field_name), value_type(FieldValueType::USHORT),      ushort_     (val) {}
    Field(const string_view_t &field_name, int                val): name(field_name), value_type(FieldValueType::INT),         int_        (val) {}
    Field(const string_view_t &field_name, unsigned int       val): name(field_name), value_type(FieldValueType::UINT),        uint_       (val) {}
    Field(const string_view_t &field_name, long               val): name(field_name), value_type(FieldValueType::LONG),        long_       (val) {}
    Field(const string_view_t &field_name, unsigned long      val): name(field_name), value_type(FieldValueType::ULONG),       ulong_      (val) {}
    Field(const string_view_t &field_name, long long          val): name(field_name), value_type(FieldValueType::LONGLONG),    longlong_   (val) {}
    Field(const string_view_t &field_name, unsigned long long val): name(field_name), value_type(FieldValueType::ULONGLONG),   ulonglong_  (val) {}
    Field(const string_view_t &field_name, bool               val): name(field_name), value_type(FieldValueType::BOOL),        bool_       (val) {}
    Field(const string_view_t &field_name, char               val): name(field_name), value_type(FieldValueType::CHAR),        char_       (val) {}
    Field(const string_view_t &field_name, unsigned char      val): name(field_name), value_type(FieldValueType::UCHAR),       uchar_      (val) {}
    Field(const string_view_t &field_name, wchar_t            val): name(field_name), value_type(FieldValueType::WCHAR),       wchar_      (val) {}
    Field(const string_view_t &field_name, float              val): name(field_name), value_type(FieldValueType::FLOAT),       float_      (val) {}
    Field(const string_view_t &field_name, double             val): name(field_name), value_type(FieldValueType::DOUBLE),      double_     (val) {}
    Field(const string_view_t &field_name, long double        val): name(field_name), value_type(FieldValueType::LONGDOUBLE),  longdouble_ (val) {}

    // Catch static strings so they don't get converted to bools
    template <size_t N>
    Field(const string_view_t &field_name, const char (&val)[N]): name(field_name), value_type(FieldValueType::STRING_VIEW), string_view_{val, N-1} {}
};
using F=Field;

namespace details {
    struct context_data;
    SPDLOG_API std::shared_ptr<context_data>& threadlocal_context_head();
}

namespace details {

// to_string_view

SPDLOG_CONSTEXPR_FUNC spdlog::string_view_t to_string_view(const memory_buf_t &buf)
    SPDLOG_NOEXCEPT {
    return spdlog::string_view_t{buf.data(), buf.size()};
}

SPDLOG_CONSTEXPR_FUNC spdlog::string_view_t to_string_view(spdlog::string_view_t str)
    SPDLOG_NOEXCEPT {
    return str;
}

#if defined(SPDLOG_WCHAR_FILENAMES) || defined(SPDLOG_WCHAR_TO_UTF8_SUPPORT) || defined(SPDLOG_UTF8_TO_WCHAR_CONSOLE)
SPDLOG_CONSTEXPR_FUNC spdlog::wstring_view_t to_string_view(const wmemory_buf_t &buf)
    SPDLOG_NOEXCEPT {
    return spdlog::wstring_view_t{buf.data(), buf.size()};
}

SPDLOG_CONSTEXPR_FUNC spdlog::wstring_view_t to_string_view(spdlog::wstring_view_t str)
    SPDLOG_NOEXCEPT {
    return str;
}
#endif

#ifndef SPDLOG_USE_STD_FORMAT
template <typename T, typename... Args>
inline fmt::basic_string_view<T> to_string_view(fmt::basic_format_string<T, Args...> fmt) {
    return fmt;
}
#elif __cpp_lib_format >= 202110L
template <typename T, typename... Args>
SPDLOG_CONSTEXPR_FUNC std::basic_string_view<T> to_string_view(
    std::basic_format_string<T, Args...> fmt) SPDLOG_NOEXCEPT {
    return fmt.get();
}
#endif

// make_unique support for pre c++14
#if __cplusplus >= 201402L  // C++14 and beyond
using std::enable_if_t;
using std::make_unique;
#else
template <bool B, class T = void>
using enable_if_t = typename std::enable_if<B, T>::type;

template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&...args) {
    static_assert(!std::is_array<T>::value, "arrays not supported");
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#endif

// to avoid useless casts (see https://github.com/nlohmann/json/issues/2893#issuecomment-889152324)
template <typename T, typename U, enable_if_t<!std::is_same<T, U>::value, int> = 0>
constexpr T conditional_static_cast(U value) {
    return static_cast<T>(value);
}

template <typename T, typename U, enable_if_t<std::is_same<T, U>::value, int> = 0>
constexpr T conditional_static_cast(U value) {
    return value;
}

}  // namespace details
}  // namespace spdlog

#ifdef SPDLOG_HEADER_ONLY
    #include "common-inl.h"
#endif
