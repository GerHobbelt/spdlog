// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <spdlog/common.h>
#include <tuple>

// Platform-specific includes for mmap support
#ifdef _WIN32
    #include <spdlog/details/windows_include.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
#endif

namespace spdlog {
namespace details {

// Helper class for file sinks.
// When failing to open a file, retry several times(5) with a delay interval(10 ms).
// Throw spdlog_ex exception on errors.
// Supports mmap mode for improved performance with automatic fallback to regular file I/O.

class SPDLOG_API file_helper {
public:
    file_helper() = default;
    explicit file_helper(const file_event_handlers &event_handlers, bool mmap_enabled = false);

    file_helper(const file_helper &) = delete;
    file_helper &operator=(const file_helper &) = delete;
    ~file_helper();

    void open(const filename_t &fname, bool truncate = false);
    void reopen(bool truncate);
    void flush();
    void sync();
    void close();
    void write(const memory_buf_t &buf);
    size_t size() const;
    const filename_t &filename() const;

    // Enable/disable mmap mode (enabled by default)
    void set_mmap_enabled(bool enabled);
    bool is_mmap_enabled() const;

    //
    // return file path and its extension:
    //
    // "mylog.txt" => ("mylog", ".txt")
    // "mylog" => ("mylog", "")
    // "mylog." => ("mylog.", "")
    // "/dir1/dir2/mylog.txt" => ("/dir1/dir2/mylog", ".txt")
    //
    // the starting dot in filenames is ignored (hidden files):
    //
    // ".mylog" => (".mylog". "")
    // "my_folder/.mylog" => ("my_folder/.mylog", "")
    // "my_folder/.mylog.txt" => ("my_folder/.mylog", ".txt")
    static std::tuple<filename_t, filename_t> split_by_extension(const filename_t &fname);

private:
    // Regular file I/O members
    const int open_tries_ = 5;
    const unsigned int open_interval_ = 10;
    std::FILE *fd_{nullptr};
    filename_t filename_;
    file_event_handlers event_handlers_;
    mutable bool in_event_handler_{false}; // Recursion protection flag

    // mmap related members
    bool mmap_enabled_{false};  // Disable mmap by default
    bool mmap_active_{false};  // Whether mmap is currently active
    void* mmap_ptr_{nullptr};  // Pointer to mapped memory
    size_t mmap_size_{0};      // Current size of mapped region
    size_t mmap_offset_{0};    // Current write offset in mapped region
    size_t initial_mmap_size_{1024 * 1024};  // Initial mmap size (1MB)
    size_t max_mmap_size_{100 * 1024 * 1024}; // Max mmap size (100MB)

#ifdef _WIN32
    HANDLE file_handle_{INVALID_HANDLE_VALUE};
    HANDLE mapping_handle_{nullptr};
#else
    int file_descriptor_{-1};
#endif

    // mmap helper methods
    bool try_init_mmap();
    bool expand_mmap(size_t required_size);
    void cleanup_mmap();
    void fallback_to_stdio();
};
}  // namespace details
}  // namespace spdlog

#ifdef SPDLOG_HEADER_ONLY
    #include "file_helper-inl.h"
#endif
