// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "dips_per_em.hpp"
#include "dips.hpp"
#include "em_squares.hpp"
#include "length.hpp"
#include "pixels_per_em.hpp"
#include "pixels.hpp"
#include "points_per_em.hpp"
#include "points.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <hikothird/au.hh>
#include <concepts>
#include <variant>

hi_export_module(hikogui.unit : pixels_per_inch);

hi_export namespace hi {
inline namespace v1 {
namespace unit {

template<typename T>
using font_size_variant = std::variant<au::Quantity<PointsPerEm, T>, au::Quantity<PixelsPerEm, T>, au::Quantity<DipsPerEm, T>>;

template<typename T>
class font_size_quantity : public font_size_variant<T> {
    using super = font_size_variant<T>;

    using super::super;

    template<typename O>
    constexpr explicit font_size_quantity(au::Quantity<Dips, O> const& other) noexcept : super(dips_per_em(other.in(dips)))
    {
    }

    template<typename O>
    constexpr explicit font_size_quantity(au::Quantity<Pixels, O> const& other) noexcept : super(pixels_per_em(other.in(pixels)))
    {
    }

    template<typename O>
    constexpr explicit font_size_quantity(au::Quantity<Points, O> const& other) noexcept : super(points_per_em(other.in(points)))
    {
    }

    template<typename O>
    constexpr explicit font_size_quantity(length_quantity<O> const& other) : super(font_size_quantity_conversion(other))
    {
    }

    template<typename O>
    [[nodiscard]] constexpr friend length_quantity<O>
    operator*(au::Quantity<EmSquares, O> const& lhs, font_size_quantity const& rhs) noexcept
    {
        if (auto* dips_ptr = std::get_if<au::Quantity<DipsPerEm, T>>(&rhs)) {
            return (*dips_ptr * lhs).as(dips);

        } else if (auto* pixel_ptr = std::get_if<au::Quantity<PixelsPerEm, T>>(&rhs)) {
            return (*pixel_ptr * lhs).as(pixels);

        } else if (auto* points_ptr = std::get_if<au::Quantity<PointsPerEm, T>>(&rhs)) {
            return (*points_ptr * lhs).as(points);

        } else {
            hi_no_default();
        }
    }

    template<typename O>
    [[nodiscard]] constexpr friend length_quantity<O>
    operator*(font_size_quantity const& lhs, au::Quantity<EmSquares, O> const& rhs) noexcept
    {
        return rhs * lhs;
    }

    template<typename O>
    [[nodiscard]] constexpr friend font_size_quantity<std::common_type_t<T, O>>
    operator*(O const& lhs, font_size_quantity const& rhs) noexcept
    {
        if (auto* dips_ptr = std::get_if<au::Quantity<DipsPerEm, T>>(&rhs)) {
            return *dips_ptr * lhs;

        } else if (auto* pixel_ptr = std::get_if<au::Quantity<PixelsPerEm, T>>(&rhs)) {
            return *pixel_ptr * lhs;

        } else if (auto* points_ptr = std::get_if<au::Quantity<PointsPerEm, T>>(&rhs)) {
            return *points_ptr * lhs;

        } else {
            hi_no_default();
        }
    }

    template<typename O>
    [[nodiscard]] constexpr friend font_size_quantity<std::common_type_t<T, O>>
    operator*(font_size_quantity const& lhs, O const& rhs) noexcept
    {
        return rhs * lhs;
    }

private:
    template<typename O>
    [[nodiscard]] constexpr static font_size_quantity font_size_quantity_conversion(length_quantity<O> const& other)
    {
        if (auto* dips_ptr = std::get_if<au::Quantity<Dips, O>>(&other)) {
            return dips_per_em(other.in(dips));
        } else if (auto* pixel_ptr = std::get_if<au::Quantity<Pixels, O>>(&other)) {
            return pixels_per_em(other.in(pixels));
        } else if (auto* points_ptr = std::get_if<au::Quantity<Points, O>>(&other)) {
            return points_per_em(other.in(points));
        } else {
            throw std::bad_variant_access();
        }
    }
};

/** Round to font size in pixels-per-em.
 * 
 * This function is used to round the font size @a by the length
 * in Em. For example to round the font-size so that the x-height will be
 * aligned to pixel boundaries for sharper display of text.
 */
template<typename FontSizeT, typename ByT>
[[nodiscard]] au::Quantity<PixelsPerEm, std::common_type_t<FontSizeT, ByT>>
round(au::Quantity<PixelsPerEm, FontSizeT> font_size, au::Quantity<EmSquares, ByT> const& by)
{
    auto const rounded_x_height = round_as(pixels, by * font_size);
    return rounded_x_height / by;
}

using font_size_f = font_size_quantity<float>;
using font_size_s = font_size_quantity<short>;

}} // namespace v1
}

template<std::integral T>
struct std::hash<hi::unit::font_size_quantity<T>> {
    [[nodiscard]] size_t operator()(hi::unit::font_size_quantity<T> const& rhs) const noexcept
    {
        auto h = std::hash<size_t>{}(rhs.index());
        h ^= std::visit(
            [](auto&& rhs_) {
                return std::hash<T>{}(rhs_.in(rhs_.unit));
            },
            rhs);
        return h;
    }
};
