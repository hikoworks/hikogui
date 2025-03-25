

#pragma once

#include "path_location.hpp"
#include "../macros.hpp"
#include <string>
#include <string_view>
#include <filesystem>
#include <expected>
#include <system_error>

hi_export_module(hikogui.path : bookmark);

hi_export namespace hi::inline v1 {
class bookmark {
public:
    ~bookmark() = default;

    /** Create an empty bookmark.
     */
    bookmark() = default;

    /** Create a bookmark from a path.
     *
     * @param path The path to the file.
     */
    bookmark(std::filesystem::path path)
        : _path(std::move(path)), _location(path_location::none)
    {
    }

    /** Create a bookmark from a path and location.
     *
     * @param path The path to the file.
     * @param location The location of the file.
     */
    bookmark(std::filesystem::path path, path_location location)
        : _path(std::move(path)), _location(location)
    {
    }

    [[nodiscard]] static bookmark resource(std::filesystem::path path)
    {
        return bookmark{std::move(path), path_location::resource};
    }

    [[nodiscard]] static bookmark data(std::filesystem::path path)
    {
        return bookmark{std::move(path), path_location::data};
    }

    bookmark(bookmark const& other) = default;
    bookmark(bookmark&& other) = default;
    bookmark& operator=(bookmark const& other) = default;
    bookmark& operator=(bookmark&& other) = default;

    [[nodiscard]] friend bool operator==(bookmark const& lhs, bookmark const& rhs) = default;

    /** Deserialize a bookmark including sandbox-tokens.
     *
     * The serialization will include any sandbox-tokens needed
     * to access the file again in a different process without
     * user intervention.
     *
     * @param str The string retrieved from a preferences file.
     */
    [[nodiscard]] static bookmark deserialize(std::string_view str);

    /** Serialize a bookmark including sandbox-tokens.
     *
     * The serialization will include any sandbox-tokens needed
     * to access the file again in a different process without
     * user intervention.
     *
     * @return A string which can be stored in preferences file.
     */
    [[nodiscard]] std::string serialize() const;

    /** Get the path from the bookmark.
     * 
     * @return The path.
     */
    [[nodiscard]] std::filesystem::path path() const noexcept
    {
        return _path;
    }

    /** This file is located outside the sandbox.
     *
     * If the file is located outside the sandbox then the application
     * will need to use `start_access_through_sandbox()` before
     * accessing the file.
     *
     * @retval true The file is located outside the sandbox.
     * @retval false The file is located inside the sandbox and
     *               is freely accessable by the application.
     */
    [[nodiscard]] bool is_outside_sandbox() const noexcept;

    /** Call this function before accessing the file.
     *
     * @param ask If true then attempt is made to get a
     *            new sandbox token, if the current token is
     *            no longer valid. This may happen through
     *            a dialogue box presented to the user.
     * @retval true Access was successful.
     * @retval false The sandbox token is no longer valid.
     */
    [[nodiscard]] bool start_access_through_sandbox(bool ask) const;

    /** Call this function after access is completed on a file.
     *
     * @note Each time `start_access_through_sandbox()` is called
     *       it should be balanced with `stop_access_through_sandbox()`.
     */
    void stop_access_through_sandbox() const;

    /** The bookmark is empty.
     */
    [[nodiscard]] bool empty() const noexcept
    {
        return _path.empty();
    }

    /** Clear the bookmark.
     */
    void clear() noexcept
    {
        _path.clear();
        _location = path_location::none;
    }

    [[nodiscard]] bool is_resolved() const noexcept
    {
        return _path.is_absolute();
    }

    /** Check if the path exists on this disk.
     */
    [[nodiscard]] bool exists() const noexcept
    {
        if (is_resolved()) {
            return std::filesystem::exists(_path);
        } else {
            return false;
        }
    }

    template<suffix_range Suffixes>
    [[nodiscard]] std::expected<bookmark, std::error_code> resolve(Suffixes&& suffixes) const
    {
        if (_path.empty()) {
            return std::unexpected{std::make_error_code(std::errc::invalid_argument)};
        }

        // If the path is absolute then it is already resolved, only need to check
        // if it actually exists. In the future we may want to look if the file
        // has moved by tracking its inode.
        if (_path.is_absolute()) {
            if (std::filesystem::exists(_path)) {
                return *this;
            } else {
                return std::unexpected{std::make_error_code(std::errc::no_such_file_or_directory)};
            }
        }

        try {
            return bookmark{get_path(_location, _path, std::forward<Suffixes>(suffixes)), _location};
        } catch (std::system_error const& e) {
            return std::unexpected{e.code()};
        } catch (...) {
            throw;
        }
    }

    /** Resolve the bookmark to an actual path on the disk.
     *
     * When resolving a bookmark with a given language and scale, a file
     * without a language and scale needs to exist. This will help with
     * searching quickly.
     *
     * Files should have the following naming convention:
     *  - <path>/<name>(-<language>)(@<scale>x).<ext>
     *
     * @param tags When searching for a file, try to find a file
     *             matching the earliest of the languages-tags in the list.
     * @param scale When searching for a file, try to find a file matching
     *              the given scale.
     * @return A bookmark that points to an actual file. or an error.
     */
    template<typename First, typename... Args>
    [[nodiscard]] std::expected<bookmark, std::error_code> resolve(First&& first, Args&&... args) const
        requires requires { resolve(file_suffixes(std::forward<First>(first), std::forward<Args>(args)...)); }
    {
        return resolve(file_suffixes(std::forward<First>(first), std::forward<Args>(args)...));
    }

private:
    std::filesystem::path _path;
    path_location _location;
};
}
