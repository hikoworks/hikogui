// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** geometry/margins.hpp
 * @ingroup geometry
 */

#pragma once

#include "extent2.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <hikocpu/hikocpu.hpp>
#include <concepts>
#include <exception>
#include <compare>

hi_export_module(hikogui.geometry : margins);

hi_export namespace hi { inline namespace v1 {

/** The left, bottom, right and top margins.
 * @ingroup geometry
 */
class margins {
public:
    using array_type = simd<float, 4>;
    using value_type = array_type::value_type;

    constexpr margins(margins const&) noexcept = default;
    constexpr margins(margins&&) noexcept = default;
    constexpr margins& operator=(margins const&) noexcept = default;
    constexpr margins& operator=(margins&&) noexcept = default;

    [[nodiscard]] constexpr margins() noexcept : _v() {}
    [[nodiscard]] constexpr margins(float margin) noexcept : _v(margin, margin, margin, margin) {}
    [[nodiscard]] constexpr margins(float left, float bottom, float right, float top) noexcept :
        _v(left, bottom, right, top)
    {
    }
    [[nodiscard]] constexpr explicit margins(array_type v) noexcept : _v(v) {}

    [[nodiscard]] constexpr explicit operator array_type() const noexcept
    {
        return _v;
    }

    [[nodiscard]] constexpr float left() const noexcept
    {
        return _v.x();
    }

    [[nodiscard]] constexpr float& left() noexcept
    {
        return _v.x();
    }

    [[nodiscard]] constexpr float bottom() const noexcept
    {
        return _v.y();
    }

    [[nodiscard]] constexpr float& bottom() noexcept
    {
        return _v.y();
    }

    [[nodiscard]] constexpr float right() const noexcept
    {
        return _v.z();
    }

    [[nodiscard]] constexpr float& right() noexcept
    {
        return _v.z();
    }

    [[nodiscard]] constexpr float top() const noexcept
    {
        return _v.w();
    }

    [[nodiscard]] constexpr float& top() noexcept
    {
        return _v.w();
    }

    template<int I>
    [[nodiscard]] constexpr friend float get(margins const& rhs) noexcept
    {
        return get<I>(rhs._v);
    }

    [[nodiscard]] constexpr float operator[](std::size_t i) const noexcept
    {
        return _v[i];
    }

    /** The widths and heights added up together.
     */
    [[nodiscard]] constexpr extent2 size() const noexcept
    {
        return extent2{_v.xy00() + _v.zw00()};
    }

    [[nodiscard]] constexpr float width() const noexcept
    {
        return _v.x() + _v.z();
    }

    [[nodiscard]] constexpr float height() const noexcept
    {
        return _v.y() + _v.w();
    }

    [[nodiscard]] constexpr friend margins operator+(margins const& lhs, margins const& rhs) noexcept
    {
        return margins{lhs._v + rhs._v};
    }

    constexpr margins& operator+=(margins const& rhs) noexcept
    {
        _v += rhs._v;
        return *this;
    }

    [[nodiscard]] constexpr friend margins max(margins const& a, margins const& b) noexcept
    {
        return margins{max(a._v, b._v)};
    }

    template<std::convertible_to<margins>... Rest>
    [[nodiscard]] constexpr friend margins max(margins const& a, margins const& b, margins const& c, Rest const&... rest) noexcept
    {
        return max(a, max(b, c, rest...));
    }

    [[nodiscard]] constexpr friend bool operator==(margins const& lhs, margins const& rhs) noexcept
    {
        return equal(lhs._v, rhs._v);
    }

    [[nodiscard]] constexpr friend extent2 operator+(extent2 const& lhs, margins const& rhs) noexcept
    {
        return lhs + rhs.size();
    }

    [[nodiscard]] constexpr friend extent2 operator-(extent2 const& lhs, margins const& rhs) noexcept
    {
        return lhs - rhs.size();
    }

private:
    array_type _v;
};

}} // namespace hi::v1
