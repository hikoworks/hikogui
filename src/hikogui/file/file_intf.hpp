// Copyright Take Vos 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file file/file.hpp Defines the file class.
 * @ingroup file
 */

#pragma once

#include "../container/container.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"
#include "access_mode.hpp"
#include "seek_whence.hpp"
#include "file_win32_impl.hpp"
#include <mutex>
#include <cstdint>
#include <map>
#include <span>
#include <filesystem>

hi_export_module(hikogui.file.file);

hi_export namespace hi { inline namespace v1 {
namespace detail {
class file_impl;
}

/** A File object.
 * @ingroup file
 */
hi_export class file {
public:
    ~file() = default;
    file(file const& other) noexcept = default;
    file(file&& other) noexcept = default;
    file& operator=(file const& other) noexcept = default;
    file& operator=(file&& other) noexcept = default;

    /** Open a file at location.
     * @param path The path to the file to open.
     * @param access_mode access-mode to open the file.
     */
    file(std::filesystem::path const& path, access_mode access_mode = access_mode::open_for_read) :
        _pimpl(std::make_shared<detail::file_impl>(path, access_mode))
    {
    }

    [[nodiscard]] hi::access_mode access_mode() const noexcept
    {
        return _pimpl->access_mode();
    }

    [[nodiscard]] std::shared_ptr<detail::file_impl> pimpl() const noexcept
    {
        return _pimpl;
    }

    /** Close the file.
     */
    void close()
    {
        return _pimpl->close();
    }

    /** Flush and block until all data is physically written to disk.
     * Flushing is required before renaming a file, to prevent
     * data corruption when the computer crashes during the rename.
     */
    void flush()
    {
        return _pimpl->flush();
    }

    /** Rename an open file.
     * This function will rename an open file atomically.
     *
     * @param destination The destination file name.
     * @param overwrite_existing Overwrite an existing file.
     * @throw io_error When failing to rename.
     */
    void rename(std::filesystem::path const& destination, bool overwrite_existing = true)
    {
        return _pimpl->rename(destination, overwrite_existing);
    }

    /** Return the size of the file.
     */
    [[nodiscard]] std::size_t size() const
    {
        return _pimpl->size();
    }

    /** Set the seek location.
     * @param offset To move the file pointer.
     * @param whence Where to seek from: begin, current or end
     * @return The new seek position relative to the beginning of the file.
     */
    std::size_t seek(std::ptrdiff_t offset, seek_whence whence = seek_whence::begin)
    {
        return _pimpl->seek(offset, whence);
    }

    /** Get the current seek location.
     */
    std::size_t get_seek()
    {
        return seek(0, seek_whence::current);
    }

    /** Write data to a file.
     *
     * @param data Pointer to data to be written.
     * @param size The number of bytes to write.
     * @throw io_error
     */
    void write(void const *data, std::size_t size)
    {
        return _pimpl->write(data, size);
    }

    /** Read data from a file.
     *
     * @param data Pointer to a buffer to read into.
     * @param size The number of bytes to read.
     * @return The number of bytes read.
     * @throw io_error
     */
    [[nodiscard]] std::size_t read(void *data, std::size_t size)
    {
        return _pimpl->read(data, size);
    }

    /** Write data to a file.
     *
     * @param bytes The byte string to write
     * @throw io_error
     */
    void write(std::span<std::byte const> bytes)
    {
        return write(bytes.data(), ssize(bytes));
    }

    /** Write data to a file.
     *
     * @param text The byte string to write
     * @throw io_error
     */
    void write(bstring_view text)
    {
        return write(text.data(), ssize(text));
    }

    /** Write data to a file.
     *
     * @param text The byte string to write
     * @throw io_error
     */
    void write(bstring const& text)
    {
        return write(text.data(), ssize(text));
    }

    /** Write data to a file.
     *
     * @param text The UTF-8 string to write
     * @throw io_error
     */
    void write(std::string_view text)
    {
        return write(text.data(), ssize(text));
    }

    /** Read bytes from the file.
     *
     * @param max_size The maximum number of bytes to read.
     * @return Data as a byte string, may return less then the requested size.
     * @throws io_error On IO error.
     */
    [[nodiscard]] bstring read_bstring(std::size_t max_size = 10'000'000)
    {
        auto const offset = get_seek();
        auto const size_ = std::min(max_size, this->size() - offset);

        auto r = bstring{};
        // XXX c++23 resize_and_overwrite()
        r.resize(size_);
        auto const bytes_read = read(r.data(), size_);
        r.resize(bytes_read);
        return r;
    }

    /** Read a UTF-8 string from the file.
     *
     * Because of complications with reading UTF-8 string with sequences
     * it is only allowed to read from the start of the file.
     *
     * @note It is undefined bahavior when the seek pointer is not zero.
     * @param max_size The maximum number of bytes to read.
     * @return Data as a UTF-8 string, may return less then the requested size.
     * @throws io_error On IO error.
     */
    [[nodiscard]] std::string read_string(std::size_t max_size = 10'000'000)
    {
        hi_assert(get_seek() == 0);

        auto const size_ = size();
        if (size_ > max_size) {
            throw io_error("read_string() requires the file size to be smaler than max_size.");
        }

        auto r = std::string{};
        // XXX c++23 resize_and_overwrite()
        r.resize(size_);
        auto const bytes_read = read(r.data(), size_);
        r.resize(bytes_read);
        return r;
    }

private:
    std::shared_ptr<detail::file_impl> _pimpl;
};

}} // namespace hi::v1
