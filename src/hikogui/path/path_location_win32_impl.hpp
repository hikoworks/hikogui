// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../win32_headers.hpp"

#include "path_location_intf.hpp"
#include "../metadata/metadata.hpp"
#include "../telemetry/telemetry.hpp"
#include "../utility/utility.hpp"
#include "../char_maps/char_maps.hpp" // XXX #616
#include "../win32/win32.hpp"
#include "../macros.hpp"
#include <filesystem>
#include <string>
#include <coroutine>

hi_export_module(hikogui.path.path_location : impl);

hi_export namespace hi {
inline namespace v1 {

[[nodiscard]] inline std::filesystem::path executable_file()
{
    if (auto path = win32_GetModuleFileName()) {
        return *path;
    } else {
        throw std::system_error{std::error_code{path.error()}};
    }
}

[[nodiscard]] inline generator<std::filesystem::path> data_dirs()
{
    // "%LOCALAPPDATA%\<Application Vendor>\<Application Name>\"
    // FOLDERID_LocalAppData has the default path: %LOCALAPPDATA% (%USERPROFILE%\AppData\Local)
    if (auto path = win32_SHGetKnownFolderPath(FOLDERID_LocalAppData)) {
        co_yield *path / get_application_vendor() / get_application_name() / "";
    } else {
        throw std::system_error{std::error_code{path.error()}};
    }
}

[[nodiscard]] inline generator<std::filesystem::path> log_dirs()
{
    // "%LOCALAPPDATA%\<Application Vendor>\<Application Name>\Log\"
    for (auto const& path : data_dirs()) {
        co_yield path / "Log" / "";
    }
}

[[nodiscard]] inline generator<std::filesystem::path> resource_dirs()
{
    // Always look at the resource directory where the executable is located.
    for (auto const& path : executable_dirs()) {
        co_yield path / "resources" / "";
    }

    // Also look in the data directory of the application.
    for (auto const& path : data_dirs()) {
        co_yield path / "resources" / "";
    }

    // If the executable of the application is located in the build directory,
    // then check the source directories for resources.
    for (auto const& source_dir : source_dirs()) {
        co_yield source_dir / "resources" / "";

        // Check the in-tree HikoGUI-library build directory.
        if (auto path = library_cmake_build_dir(); not path.empty()) {
            co_yield path / "resources" / "";
        }

        // Check the in-tree HikoGUI-library source directory.
        if (auto path = library_cmake_source_dir(); not path.empty()) {
            co_yield path / "resources" / "";
        }

        for (auto const& library_source_dir : library_source_dirs()) {
            // Check the HikoGUI source directory.
            co_yield library_source_dir / "resources" / "";

            // Check the HikoGUI install directory.
            co_yield library_source_dir / "share" / "hikogui" / "resources" / "";
        }
    }
}

[[nodiscard]] inline generator<std::filesystem::path> system_font_dirs()
{
    if (auto path = win32_SHGetKnownFolderPath(FOLDERID_Fonts)) {
        co_yield *path;
    }
}

[[nodiscard]] inline generator<std::filesystem::path> font_dirs()
{
    for (auto const& path : resource_dirs()) {
        co_yield path;
    }
    for (auto const& path : system_font_dirs()) {
        co_yield path;
    }
}

[[nodiscard]] inline generator<std::filesystem::path> theme_dirs()
{
    for (auto const& path : resource_dirs()) {
        co_yield path;
    }
}

} // namespace v1
} // namespace hi::inline v1
