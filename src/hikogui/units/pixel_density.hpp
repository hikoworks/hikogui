// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "pixels_per_inch.hpp"
#include "dips.hpp"
#include "pixels.hpp"
#include "font_size.hpp"
#include "length.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"

namespace hi { inline namespace v1 {
namespace unit {

struct pixel_density {
    pixels_per_inch_f ppi;
    device_type type;

    [[nodiscard]] constexpr friend bool operator==(pixel_density const&, pixel_density const&) noexcept = default;

    template<typename T>
    [[nodiscard]] constexpr friend au::Quantity<Pixels, std::common_type_t<float, T>>
    operator*(pixel_density const& lhs, au::Quantity<Dips, T> const& rhs) noexcept
    {
        return pixels(lhs.density_scale() * rhs.in(dips));
    }

    template<typename T>
    [[nodiscard]] constexpr friend au::Quantity<Pixels, std::common_type_t<float, T>>
    operator*(au::Quantity<Dips, T> const& lhs, pixel_density const& rhs) noexcept
    {
        return rhs * lhs;
    }

    template<typename Unit, typename T>
    [[nodiscard]] constexpr friend au::Quantity<Pixels, std::common_type_t<float, T>>
    operator*(pixel_density const& lhs, au::Quantity<Unit, T> const& rhs) noexcept requires (au::HasSameDimension<Unit, au::Inches>::value)
    {
        return lhs.ppi * rhs;
    }

    template<typename Unit, typename T>
    [[nodiscard]] constexpr friend au::Quantity<Pixels, std::common_type_t<float, T>>
    operator*(au::Quantity<Unit, T> const& lhs, pixel_density const& rhs) noexcept requires (au::HasSameDimension<Unit, au::Inches>::value)
    {
        return rhs * lhs;
    }

    template<typename T>
    [[nodiscard]] constexpr friend au::Quantity<Pixels, std::common_type_t<float, T>>
    operator*(pixel_density const& lhs, au::Quantity<Pixels, T> const& rhs) noexcept
    {
        return rhs;
    }

    template<typename T>
    [[nodiscard]] constexpr friend au::Quantity<Pixels, std::common_type_t<float, T>>
    operator*(au::Quantity<Pixels, T> const& lhs, pixel_density const& rhs) noexcept
    {
        return lhs;
    }

    template<typename T>
    [[nodiscard]] constexpr friend au::Quantity<Pixels, std::common_type_t<float, T>>
    operator*(length_quantity<T> const& lhs, pixel_density const& rhs) noexcept
    {
        if (auto const *pts = std::get_if<au::Quantity<Points, T>>(&lhs)) {
            return *pts * rhs;
        } else if (auto const *dps = std::get_if<au::Quantity<Dips, T>>(&lhs)) {
            return *dps * rhs;
        } else if (auto const *pxs = std::get_if<au::Quantity<Pixels, T>>(&lhs)) {
            return *pxs * rhs;
        } else {
            hi_no_default();
        }
    }

    template<typename T>
    [[nodiscard]] constexpr friend au::Quantity<Pixels, std::common_type_t<float, T>>
    operator*(pixel_density const& lhs, length_quantity<T> const& rhs) noexcept
    {
        return rhs * lhs;
    }

    template<typename T>
    [[nodiscard]] constexpr friend au::Quantity<PixelsPerEm, std::common_type_t<float, T>>
    operator*(pixel_density const& lhs, au::Quantity<DipsPerEm, T> const& rhs) noexcept
    {
        return pixels_per_em(lhs.density_scale() * rhs.in(dips_per_em));
    }

    template<typename T>
    [[nodiscard]] constexpr friend au::Quantity<PixelsPerEm, std::common_type_t<float, T>>
    operator*(au::Quantity<DipsPerEm, T> const& lhs, pixel_density const& rhs) noexcept
    {
        return rhs * lhs;
    }

    template<typename T>
    [[nodiscard]] constexpr friend au::Quantity<PixelsPerEm, std::common_type_t<float, T>>
    operator*(pixel_density const& lhs, au::Quantity<PointsPerEm, T> const& rhs) noexcept
    {
        return lhs.ppi * rhs;
    }

    template<typename T>
    [[nodiscard]] constexpr friend au::Quantity<PixelsPerEm, std::common_type_t<float, T>>
    operator*(au::Quantity<PointsPerEm, T> const& lhs, pixel_density const& rhs) noexcept
    {
        return rhs * lhs;
    }

    template<typename T>
    [[nodiscard]] constexpr friend au::Quantity<PixelsPerEm, std::common_type_t<float, T>>
    operator*(pixel_density const& lhs, au::Quantity<PixelsPerEm, T> const& rhs) noexcept
    {
        return rhs;
    }

    template<typename T>
    [[nodiscard]] constexpr friend au::Quantity<PixelsPerEm, std::common_type_t<float, T>>
    operator*(au::Quantity<PixelsPerEm, T> const& lhs, pixel_density const& rhs) noexcept
    {
        return lhs;
    }

    template<typename T>
    [[nodiscard]] constexpr friend au::Quantity<PixelsPerEm, std::common_type_t<float, T>>
    operator*(pixel_density const& lhs, font_size_quantity<T> const& rhs) noexcept
    {
        if (auto const* dips_per_em_ptr = std::get_if<au::Quantity<DipsPerEm, T>>(&rhs)) {
            return *dips_per_em_ptr * lhs;
        } else if (auto const* pixels_per_em_ptr = std::get_if<au::Quantity<PixelsPerEm, T>>(&rhs)) {
            return *pixels_per_em_ptr * lhs;
        } else if (auto const* points_per_em_ptr = std::get_if<au::Quantity<PointsPerEm, T>>(&rhs)) {
            return *points_per_em_ptr * lhs;
        } else {
            hi_no_default();
        }
    }

    template<typename T>
    [[nodiscard]] constexpr friend au::Quantity<PixelsPerEm, std::common_type_t<float, T>>
    operator*(font_size_quantity<T> const& lhs, pixel_density const& rhs) noexcept
    {
        return rhs * lhs;
    }

    /** Return a density-scale to convert device independet pixels to normal pixels.
     */
    [[nodiscard]] constexpr float density_scale() const noexcept
    {
        constexpr auto scale_table = std::array{
            0.5f, // 80 dpi
            0.5f,
            0.5f,

            0.75f, // 120 dpi

            1.0f, // 160 dpi
            1.0f,

            1.5f, // 240 dpi
            1.5f,

            2.0f, // 320 dpi
            2.0f,
            2.0f,
            2.0f,

            3.0f, // 480 dpi
            3.0f,
            3.0f,
            3.0f,
            4.0f, // 640 dpi
        };

        // The base density is based on the device type which determines
        // the viewing distance. We strip off the last three bits as there
        // are multiple devices that need unique values but have the same
        // base density.
        auto const base_density = std::to_underlying(type) & 0xf8;

        // casting to unsigned int is faster than size_t.
        auto const index = static_cast<unsigned int>(ppi.in(pixels_per_inch)) * 4 / base_density;

        // Using a table here is faster than the compiler will generate with
        // a switch statement. The bound check here will result in a conditional
        // move instead of a conditional jump.
        if (index < scale_table.size()) {
            return scale_table[index];
        } else {
            return scale_table.back();
        }
    }

    /** Return the scale of the image used for searching the best image.
     * 
     * @return The scale of the best image to load. The values are 1, 2, 4, or 8.
     */
    [[nodiscard]] int image_scale() const noexcept
    {
        constexpr auto scale_table = std::array{
            1, // 80 dpi
            1,
            1,

            2, // 120 dpi

            2, // 160 dpi
            2,

            4, // 240 dpi
            4,

            4, // 320 dpi
            4,
            4,
            4,

            8, // 480 dpi
            8,
            8,
            8,
            8, // 640 dpi
        };

        // The base density is based on the device type which determines
        // the viewing distance. We strip off the last three bits as there
        // are multiple devices that need unique values but have the same
        // base density.
        auto const base_density = std::to_underlying(type) & 0xf8;

        // casting to unsigned int is faster than size_t.
        auto const index = static_cast<unsigned int>(ppi.in(pixels_per_inch)) * 4 / base_density;

        // Using a table here is faster than the compiler will generate with
        // a switch statement. The bound check here will result in a conditional
        // move instead of a conditional jump.
        if (index < scale_table.size()) {
            return scale_table[index];
        } else {
            return scale_table.back();
        }
    }

};

}}} // namespace hi::v1
