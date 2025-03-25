// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "cmake_install.hpp"
#include "../utility/utility.hpp"
#include "../metadata/metadata.hpp"
#include "../units/units.hpp"
#include "../i18n/i18n.hpp"
#include "../macros.hpp"
#include <filesystem>
#include <ranges>
#include <fstream>
#include <string>
#include <ranges>
#include <expected>
#include <system_error>

hi_warning_push();
// C4702 unreachable code: False positive, but "not a bug" / "low priority".
// https://developercommunity.visualstudio.com/t/warning-c4702-for-range-based-for-loop/859129
// The ranged for-loop is translated into a normal for-loop. The iterator
// part of the normal for-loop is never executed due to the immediate return
// inside the for-loop.
hi_warning_ignore_msvc(4702);

/** @file path/path_location.hpp functions to locate files and directories.
 * @ingroup path
 */

hi_export_module(hikogui.path.path_location : intf);

hi_export namespace hi {
inline namespace v1 {

enum class path_location { none, executable, data, log, resource, system_font, font, theme };

template<typename Context>
concept path_range = std::ranges::input_range<Context> and
    std::convertible_to<std::ranges::range_value_t<std::remove_cvref_t<Context>>, std::filesystem::path> and
    not std::convertible_to<Context, std::filesystem::path>;

template<typename Context>
concept suffix_range = std::ranges::input_range<Context> and
    std::convertible_to<std::ranges::range_value_t<std::remove_cvref_t<Context>>, std::string> and
    not std::convertible_to<Context, std::string>;



/** Get a string representation of a search-path.
 *
 * @param locations A range of std::filesystem::path elements.
 * @return A string of semicolon ';' separated paths.
 */
template<path_range Locations>
[[nodiscard]] inline std::string to_string(Locations&& locations) noexcept
{
    auto r = std::string{};
    for (auto const& path : locations) {
        if (not r.empty()) {
            r += ";";
        }
        r += path.string();
    }
    return r;
}

/** Get the full path to this executable.
 * @ingroup path
 */
[[nodiscard]] std::filesystem::path executable_file();

[[nodiscard]] inline generator<std::filesystem::path> root_dirs()
{
    co_yield std::filesystem::path{"/"};
}

/** Get the full path to the directory when this executable is located.
 * @ingroup path
 */
[[nodiscard]] inline generator<std::filesystem::path> executable_dirs()
{
    co_yield executable_file().remove_filename();
}

/** Get the full path to the directory where the application should store its data.
 * @ingroup path
 */
[[nodiscard]] generator<std::filesystem::path> data_dirs();

/** Get the full path to the directory where the application should store its log files.
 * @ingroup path
 */
[[nodiscard]] generator<std::filesystem::path> log_dirs();

/** The directories to search for resource files.
 * @ingroup path
 */
[[nodiscard]] inline generator<std::filesystem::path> resource_dirs();

/** The directories to search for system font files.
 * @ingroup path
 */
[[nodiscard]] inline generator<std::filesystem::path> system_font_dirs();

/** The directories to search for font files of both the application and system.
 * @ingroup path
 */
[[nodiscard]] inline generator<std::filesystem::path> font_dirs();

/** The directories to search for theme files of the application.
 * @ingroup path
 */
[[nodiscard]] inline generator<std::filesystem::path> theme_dirs();

/** Get the full path to source code of this executable.
 *
 * @ingroup path
 * @return The path to directory of the source code.
 */
[[nodiscard]] inline generator<std::filesystem::path> source_dirs()
{
    for (auto const& executable_dir : executable_dirs()) {
        // If the cmake_install.cmake file exists then the executable is located in a build directory.
        if (auto tmp = parse_cmake_install(executable_dir / "cmake_install.cmake")) {
            co_yield tmp->source_dir;
        }

        // When using a cmake multi-config generator The executable lives in the
        // ./Debug/, ./Release/ or ./RelWithDebInfo/ directory.
        // So, the cmake_install.cmake file is located one directory up.
        if (auto tmp = parse_cmake_install(executable_dir / ".." / "cmake_install.cmake")) {
            co_yield tmp->source_dir;
        }
    }
}

[[nodiscard]] inline generator<std::filesystem::path> library_source_dirs()
{
    auto path = std::filesystem::path{__FILE__};
    path.replace_filename("../../..");
    co_yield path.lexically_normal();
}

[[nodiscard]] inline generator<std::filesystem::path> library_test_data_dirs()
{
    for (auto const& path : library_source_dirs()) {
        co_yield path / "tests" / "data";
    }
}

[[nodiscard]] inline std::filesystem::path library_test_data_dir()
{
    for (auto const& path: library_test_data_dirs()) {
        return path;
    }
    throw std::system_error(std::make_error_code(std::errc::no_such_file_or_directory));
}

[[nodiscard]] inline generator<std::filesystem::path> location_dirs(path_location location)
{
    auto const g = [&]() -> generator<std::filesystem::path> {
        switch (location) {
        case path_location::executable:
            return executable_dirs();
        case path_location::data:
            return data_dirs();
        case path_location::log:
            return log_dirs();
        case path_location::resource:
            return resource_dirs();
        case path_location::system_font:
            return system_font_dirs();
        case path_location::font:
            return font_dirs();
        case path_location::theme:
            return theme_dirs();
        case path_location::none:
            return root_dirs();
        }
        std::unreachable();
    }();

    for (auto const& path : g) {
        co_yield path;
    }
}

/** Find a path.
 * @ingroup path
 *
 * @param locations The locations to search for filesystem-object.
 * @param ref A relative path to the filesystem-object.
 * @return full paths to the filesystem-objects found in the location.
 */
template<path_range Locations>
[[nodiscard]] inline generator<std::filesystem::path> find_path(Locations&& locations, std::filesystem::path const& ref)
{
    if (ref.is_absolute()) {
        if (std::filesystem::exists(ref)) {
            co_yield ref;
        }

    } else {
        for (auto const& base : locations) {
            auto const path = base / ref;
            if (std::filesystem::exists(path)) {
                co_yield path;
            }
        }
    }
}

template<path_range Locations, suffix_range Suffixes>
[[nodiscard]] inline generator<std::filesystem::path>
find_path(Locations&& locations, std::filesystem::path const& ref, Suffixes&& suffixes)
{
    auto suffix_cache = std::optional<std::vector<std::string>>{std::nullopt};

    for (auto const& path : find_path(std::forward<Locations>(locations), ref)) {
        auto const filename = path.filename().string();
        auto const directory = path.parent_path();
        if (filename.empty() or filename == "." or filename == "..") {
            co_yield path;
            continue;
        }

        // Split the filename into stem and all extensions.
        // This is different from std::filesystem::path::stem() and
        // std::filesystem::path::extension() because these will split
        // on only the last extension.
        auto const [stem, ext] = [&]() -> std::pair<std::string, std::string> {
            auto const dot_i = filename.find('.');
            if (dot_i == std::string::npos) {
                return {filename, {}};
            } else {
                // Include the dot with the extensions.
                return {filename.substr(0, dot_i), filename.substr(dot_i)};
            }
        }();

        if (not suffix_cache) {
            suffix_cache = std::vector<std::string>{};
            for (auto& suffix : suffixes) {
                auto const path_with_suffix = directory / std::format("{}{}{}", stem, suffix, ext);
                if (std::filesystem::exists(path_with_suffix)) {
                    co_yield path_with_suffix;
                }
                // suffixes can only be used once, so we can move them into the
                // cache. We do this after the co_yield, to reduce the chance of
                // an allocation; since this generator's frame will likely be
                // destroyed after the first co_yield.
                suffix_cache->push_back(std::move(suffix));
            }

        } else {
            for (auto const& suffix : *suffix_cache) {
                auto const path_with_suffix = directory / std::format("{}{}{}", stem, suffix, ext);
                if (std::filesystem::exists(path_with_suffix)) {
                    co_yield path_with_suffix;
                }
            }
        }

        co_yield path;
    }
}

[[nodiscard]] inline generator<std::string> file_suffixes(std::vector<language_tag> const& languages, unit::pixel_density density)
{
    auto const scale = density.image_scale();

    for (auto const& language : languages) {
        for (auto s = scale; s != 0; s /= 2) {
            co_yield std::format("-{}@{}x", language, s);
        }
        co_yield std::format("-{}", language);
    }

    for (auto s = scale; s != 0; s /= 2) {
        co_yield std::format("@{}x", s);
    }
}

[[nodiscard]] inline generator<std::string> file_suffixes(std::vector<language_tag> const& languages)
{
    for (auto const& language : languages) {
        co_yield std::format("-{}", language);
    }
}

[[nodiscard]] inline size_t file_suffix_get_scale(std::filesystem::path const &path)
{
    auto const filename = path.filename().string();
    auto const at_i = filename.find('@');
    if (at_i == std::string::npos) {
        return 1;
    }
    auto const at_num = at_i + 1;
    auto const x_i = filename.find('x', at_num);
    if (x_i == std::string::npos) {
        return 1;
    }

    auto const scale_string = filename.substr(at_num, x_i - at_num);

    if (auto const scale = hi::from_string<size_t>(scale_string, 10)) {
        return *scale;
    } else {
        return 1;
    }
}

template<path_range Locations, typename First, typename... Args>
[[nodiscard]] inline generator<std::filesystem::path>
find_path(Locations&& locations, std::filesystem::path const& ref, First&& first, Args&&... args) requires requires {
    find_path(std::forward<Locations>(locations), ref, file_suffixes(std::forward<First>(first), std::forward<Args>(args)...));
}
{
    return find_path(
        std::forward<Locations>(locations), ref, file_suffixes(std::forward<First>(first), std::forward<Args>(args)...));
}

template<typename... Args>
[[nodiscard]] inline generator<std::filesystem::path>
find_path(path_location location, std::filesystem::path const& ref, Args&&... args)
    requires requires { find_path(location_dirs(location), ref, std::forward<Args>(args)...); }
{
    return find_path(location_dirs(location), ref, std::forward<Args>(args)...);
}

template<typename... Args>
[[nodiscard]] inline std::filesystem::path get_path(Args&&... args) requires requires { find_path(std::forward<Args>(args)...); }
{
    for (auto const& path : find_path(std::forward<Args>(args)...)) {
        return path;
    }

    throw std::system_error(std::make_error_code(std::errc::no_such_file_or_directory));
}

} // namespace v1
} // namespace hi::v1

hi_warning_pop();
