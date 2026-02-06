// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#ifndef SPDLOG_HEADER_ONLY
#include <spdlog/details/file_helper.h>
#endif

#include <spdlog/common.h>
#include <spdlog/details/os.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <tuple>
#include <algorithm>

// Platform-specific includes for mmap support
#ifdef _WIN32
    #include <spdlog/details/windows_include.h>
    #include <io.h>
    #include <fcntl.h>
    #include <sys/stat.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
#endif

namespace spdlog {
namespace details {

SPDLOG_INLINE file_helper::file_helper(const file_event_handlers &event_handlers, bool mmap_enabled)
    : event_handlers_(event_handlers), mmap_enabled_(mmap_enabled) {}

SPDLOG_INLINE file_helper::~file_helper() { close(); }

SPDLOG_INLINE void file_helper::open(const filename_t &fname, bool truncate) {
    close();
    filename_ = fname;

    const auto *mode = SPDLOG_FILENAME_T("ab");
    const auto *trunc_mode = SPDLOG_FILENAME_T("wb");

    if (event_handlers_.before_open && !in_event_handler_) {
        in_event_handler_ = true;
        event_handlers_.before_open(filename_);
        in_event_handler_ = false;
    }
    for (int tries = 0; tries < open_tries_; ++tries) {
        // create containing folder if not exists already.
        os::create_dir(os::dir_name(fname));
        if (truncate) {
            // Truncate by opening-and-closing a tmp file in "wb" mode, always
            // opening the actual log-we-write-to in "ab" mode, since that
            // interacts more politely with eternal processes that might
            // rotate/truncate the file underneath us.
            std::FILE *tmp = nullptr;
            if (os::fopen_s(&tmp, fname, trunc_mode)) {
                continue;
            }
            std::fclose(tmp);
        }
        if (!os::fopen_s(&fd_, fname, mode)) {
            if (event_handlers_.after_open && !in_event_handler_) {
                in_event_handler_ = true;
                event_handlers_.after_open(filename_, fd_);
                in_event_handler_ = false;
            }
            
            // Try to initialize mmap if enabled
            if (mmap_enabled_ && !try_init_mmap()) {
                // mmap initialization failed, continue with regular file I/O
                // No need to throw exception, just log a debug message if needed
            }
            
            return;
        }

        details::os::sleep_for_millis(open_interval_);
    }

    throw_spdlog_ex("Failed opening file " + os::filename_to_str(filename_) + " for writing",
                    errno);
}

SPDLOG_INLINE void file_helper::reopen(bool truncate) {
    if (filename_.empty()) {
        throw_spdlog_ex("Failed re opening file - was not opened before");
    }
    this->open(filename_, truncate);
}

SPDLOG_INLINE void file_helper::flush() {
    if (mmap_active_) {
        // For mmap, flush means sync the memory mapping
#ifdef _WIN32
        if (!FlushViewOfFile(mmap_ptr_, mmap_offset_)) {
            // If mmap flush fails, fallback to stdio
            fallback_to_stdio();
        }
#else
        if (msync(mmap_ptr_, mmap_offset_, MS_ASYNC) != 0) {
            // If mmap flush fails, fallback to stdio
            fallback_to_stdio();
        }
#endif
    }
    
    if (std::fflush(fd_) != 0) {
        throw_spdlog_ex("Failed flush to file " + os::filename_to_str(filename_), errno);
    }
}

SPDLOG_INLINE void file_helper::sync() {
    if (mmap_active_) {
        // For mmap, sync means synchronous sync of the memory mapping
#ifdef _WIN32
        if (!FlushViewOfFile(mmap_ptr_, mmap_offset_)) {
            fallback_to_stdio();
        }
#else
        if (msync(mmap_ptr_, mmap_offset_, MS_SYNC) != 0) {
            fallback_to_stdio();
        }
#endif
    }
    
    if (!os::fsync(fd_)) {
        throw_spdlog_ex("Failed to fsync file " + os::filename_to_str(filename_), errno);
    }
}

SPDLOG_INLINE void file_helper::close() {
    if (fd_ != nullptr) {
        if (event_handlers_.before_close && !in_event_handler_) {
            in_event_handler_ = true;
            event_handlers_.before_close(filename_, fd_);
            in_event_handler_ = false;
        }

        // Clean up mmap resources before closing file
        cleanup_mmap();

        std::fclose(fd_);
        fd_ = nullptr;

        if (event_handlers_.after_close && !in_event_handler_) {
            in_event_handler_ = true;
            event_handlers_.after_close(filename_);
            in_event_handler_ = false;
        }
    }
}

SPDLOG_INLINE void file_helper::write(const memory_buf_t &buf) {
    if (fd_ == nullptr) return;
    size_t msg_size = buf.size();
    auto data = buf.data();

    // Try mmap write first if active
    if (mmap_active_) {
        size_t required_size = mmap_offset_ + msg_size;
        
        // Check if we need to expand the mapping
        if (required_size > mmap_size_) {
            if (!expand_mmap(required_size)) {
                // Expansion failed, fallback to stdio
                goto stdio_write;
            }
        }

        // Write to mmap
        try {
            std::memcpy(static_cast<char*>(mmap_ptr_) + mmap_offset_, data, msg_size);
            mmap_offset_ += msg_size;
            return;
        } catch (...) {
            // mmap write failed, fallback to stdio
            fallback_to_stdio();
        }
    }

stdio_write:
    // Standard file I/O fallback
    if (!details::os::fwrite_bytes(data, msg_size, fd_)) {
        throw_spdlog_ex("Failed writing to file " + os::filename_to_str(filename_), errno);
    }
}

SPDLOG_INLINE size_t file_helper::size() const {
    if (fd_ == nullptr) {
        throw_spdlog_ex("Cannot use size() on closed file " + os::filename_to_str(filename_));
    }
    
    // If mmap is active, return the current offset (actual written size)
    if (mmap_active_) {
        // DEBUG: Print mmap_offset_ value
        return mmap_offset_;
    }
    
    return os::filesize(fd_);
}

SPDLOG_INLINE const filename_t &file_helper::filename() const { return filename_; }

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
SPDLOG_INLINE std::tuple<filename_t, filename_t> file_helper::split_by_extension(
    const filename_t &fname) {
    auto ext_index = fname.rfind('.');

    // no valid extension found - return whole path and empty string as
    // extension
    if (ext_index == filename_t::npos || ext_index == 0 || ext_index == fname.size() - 1) {
        return std::make_tuple(fname, filename_t());
    }

    // treat cases like "/etc/rc.d/somelogfile or "/abc/.hiddenfile"
    auto folder_index = fname.find_last_of(details::os::folder_seps_filename);
    if (folder_index != filename_t::npos && folder_index >= ext_index - 1) {
        return std::make_tuple(fname, filename_t());
    }

    // finally - return a valid base and extension tuple
    return std::make_tuple(fname.substr(0, ext_index), fname.substr(ext_index));
}

// mmap helper methods implementation
SPDLOG_INLINE void file_helper::set_mmap_enabled(bool enabled) {
    mmap_enabled_ = enabled;
    if (!enabled && mmap_active_) {
        fallback_to_stdio();
    }
}

SPDLOG_INLINE bool file_helper::is_mmap_enabled() const {
    return mmap_enabled_;
}

SPDLOG_INLINE bool file_helper::try_init_mmap() {
    if (!mmap_enabled_ || fd_ == nullptr) {
        return false;
    }

    // Get current file size BEFORE any modifications
    size_t current_file_size = os::filesize(fd_);
    size_t required_size = (std::max)(current_file_size + initial_mmap_size_, initial_mmap_size_);

#ifdef _WIN32
    // Windows implementation
    file_handle_ = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(fd_)));
    if (file_handle_ == INVALID_HANDLE_VALUE) {
        return false;
    }

    // Create file mapping
    LARGE_INTEGER map_size;
    map_size.QuadPart = required_size;
    
    mapping_handle_ = CreateFileMapping(file_handle_, nullptr, PAGE_READWRITE,
                                       map_size.HighPart, map_size.LowPart, nullptr);
    if (mapping_handle_ == nullptr) {
        return false;
    }

    // Map view of file
    mmap_ptr_ = MapViewOfFile(mapping_handle_, FILE_MAP_WRITE, 0, 0, required_size);
    if (mmap_ptr_ == nullptr) {
        CloseHandle(mapping_handle_);
        mapping_handle_ = nullptr;
        return false;
    }

#else
    // Unix/Linux implementation
    file_descriptor_ = fileno(fd_);
    if (file_descriptor_ == -1) {
        return false;
    }

    // Create memory mapping first (without extending file)
    mmap_ptr_ = mmap(nullptr, required_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                     file_descriptor_, 0);
    if (mmap_ptr_ == MAP_FAILED) {
        
        mmap_ptr_ = nullptr;
        return false;
    }

    // Extend file if necessary (only after mmap succeeds)
    if (ftruncate(file_descriptor_, static_cast<off_t>(required_size)) != 0) {
        munmap(mmap_ptr_, required_size);
        mmap_ptr_ = nullptr;
        return false;
    }
#endif

    mmap_size_ = required_size;
    mmap_offset_ = 0;  // Always start from beginning for new mmap
    mmap_active_ = true;
    
    return true;
}

SPDLOG_INLINE bool file_helper::expand_mmap(size_t required_size) {
    if (!mmap_active_ || required_size <= mmap_size_) {
        return true;
    }

    // Calculate new size (double current size or required size, whichever is larger)
    size_t new_size = (std::max)(mmap_size_ * 2, required_size);
    new_size = (std::min)(new_size, max_mmap_size_);

    if (new_size <= mmap_size_) {
        // Cannot expand further, fallback to stdio
        fallback_to_stdio();
        return false;
    }

    // Save current offset before cleanup
    size_t saved_offset = mmap_offset_;
    cleanup_mmap();

#ifdef _WIN32
    // Windows re-mapping
    LARGE_INTEGER map_size;
    map_size.QuadPart = new_size;
    
    mapping_handle_ = CreateFileMapping(file_handle_, nullptr, PAGE_READWRITE,
                                       map_size.HighPart, map_size.LowPart, nullptr);
    if (mapping_handle_ == nullptr) {
        fallback_to_stdio();
        return false;
    }

    mmap_ptr_ = MapViewOfFile(mapping_handle_, FILE_MAP_WRITE, 0, 0, new_size);
    if (mmap_ptr_ == nullptr) {
        CloseHandle(mapping_handle_);
        mapping_handle_ = nullptr;
        fallback_to_stdio();
        return false;
    }

#else
    // Unix/Linux re-mapping
    if (ftruncate(file_descriptor_, static_cast<off_t>(new_size)) != 0) {
        fallback_to_stdio();
        return false;
    }

    mmap_ptr_ = mmap(nullptr, new_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                     file_descriptor_, 0);
    if (mmap_ptr_ == MAP_FAILED) {
        mmap_ptr_ = nullptr;
        fallback_to_stdio();
        return false;
    }
#endif

    mmap_size_ = new_size;
    mmap_offset_ = saved_offset;  // Restore the saved offset
    mmap_active_ = true;
    return true;
}

SPDLOG_INLINE void file_helper::cleanup_mmap() {
    if (!mmap_active_) {
        return;
    }

#ifdef _WIN32
    if (mmap_ptr_ != nullptr) {
        UnmapViewOfFile(mmap_ptr_);
        mmap_ptr_ = nullptr;
    }
    if (mapping_handle_ != nullptr) {
        CloseHandle(mapping_handle_);
        mapping_handle_ = nullptr;
    }
#else
    if (mmap_ptr_ != nullptr) {
        munmap(mmap_ptr_, mmap_size_);
        mmap_ptr_ = nullptr;
    }
#endif

    mmap_active_ = false;
    mmap_size_ = 0;
    mmap_offset_ = 0;
}

SPDLOG_INLINE void file_helper::fallback_to_stdio() {
    if (mmap_active_) {
        // Sync any pending data
        sync();
        cleanup_mmap();
    }
    mmap_enabled_ = false;  // Disable mmap for this file
}

}  // namespace details
}  // namespace spdlog
