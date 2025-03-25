
#pragma once

#include "../path/path.hpp"
#include "../i18n/i18n.hpp"
#include "../units/units.hpp"
#include "../telemetry/telemetry.hpp"
#include "../algorithm/algorithm.hpp"
#include "png_loader.hpp"
#include "pixmap.hpp"
#include "sfloat_rgba16.hpp"
#include "../macros.hpp"

hi_export_module(hikogui.image : image_loader);

hi_export namespace hi::inline v1 {

[[nodiscard]] inline std::expected<pixmap<sfloat_rgba16>, std::error_code>
load_image(std::filesystem::path const& path)
{
    if (path.empty()) {
        return std::unexpected{std::make_error_code(std::errc::invalid_argument)};
    }

    auto const scale = file_suffix_get_scale(path);

    auto const ext = to_lower(path.extension().string());
    if (ext == ".png") {
        hi_log_info("Loading PNG image {}", path.string());
        auto image = load_png(path);
        image.set_scale(scale);
        return std::move(image);
    } else {
        return std::unexpected{std::make_error_code(std::errc::not_supported)};
    }
}

/** Load in image from a file.
 *
 * The image loaded is specified by the search parameters in the bookmark.
 * The pixel density is used to determine the most optimal image to load, when
 * multiple versions of the image are available.
 *
 * @param bookmark The bookmark to the image.
 * @param languages The languages to use when resolving the bookmark.
 * @param density The pixel density of the screen.
 * @return The image.
 */
[[nodiscard]] inline std::expected<pixmap<sfloat_rgba16>, std::error_code>
load_image(hi::bookmark const& bookmark, std::vector<language_tag> languages, hi::unit::pixel_density density)
{
    if (auto const resolved_bookmark = bookmark.resolve(languages, density)) {
        return load_image(resolved_bookmark->path());
    } else {
        return std::unexpected{resolved_bookmark.error()};
    }
}

}
