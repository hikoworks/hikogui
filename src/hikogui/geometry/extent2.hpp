// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file geometry/extent2.hpp Defined the geo::extent, extent2 and extent3 types.
 * @ingroup geometry
 */

#pragma once

#include "vector2.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <hikocpu/hikocpu.hpp>
#include <compare>
#include <concepts>
#include <format>
#include <ostream>
#include <exception>

hi_export_module(hikogui.geometry : extent2);

hi_export namespace hi { inline namespace v1 {

/** A high-level geometric extent
 * @ingroup geometry
 *
 * A extent, for both 2D or 3D is internally represented
 * as a 4D homogeneous extent. Which can be efficiently implemented
 * as a __m128 SSE register.
 */
class extent2 {
public:
    using array_type = simd<float, 4>;
    using value_type = array_type::value_type;

    constexpr extent2(extent2 const&) noexcept = default;
    constexpr extent2(extent2&&) noexcept = default;
    constexpr extent2& operator=(extent2 const&) noexcept = default;
    constexpr extent2& operator=(extent2&&) noexcept = default;

    [[nodiscard]] constexpr explicit extent2(array_type const& other) noexcept : _v(other)
    {
        hi_axiom(holds_invariant());
    }

    /** Convert a extent to its array_type-nummeric_array.
     */
    [[nodiscard]] constexpr explicit operator array_type() const noexcept
    {
        return _v;
    }

    [[nodiscard]] constexpr explicit extent2(vector2 const &other) noexcept : _v(f32x4{other})
    {
        hi_axiom(holds_invariant());
    }

    [[nodiscard]] constexpr explicit operator vector2() const noexcept
    {
        return vector2{static_cast<array_type>(*this)};
    }

    /** Construct a empty extent / zero length.
     */
    [[nodiscard]] constexpr extent2() noexcept : _v(0.0f, 0.0f, 0.0f, 0.0f) {}

    /** Construct a 3D extent from width, height and depth.
     * @param width The width element.
     * @param height The height element.
     * @param depth The depth element.
     */
    [[nodiscard]] constexpr extent2(float width, float height) noexcept : _v(width, height, 0.0f, 0.0f)
    {
        hi_axiom(holds_invariant());
    }

    [[nodiscard]] constexpr static extent2 infinity() noexcept
    {
        return extent2{std::numeric_limits<value_type>::infinity(), std::numeric_limits<value_type>::infinity()};
    }

    [[nodiscard]] constexpr static extent2 large() noexcept
    {
        return extent2{large_number_v<float>, large_number_v<float>};
    }

    [[nodiscard]] constexpr static extent2 nan() noexcept
    {
        auto r = extent2{};
        r._v.x() = std::numeric_limits<value_type>::signaling_NaN();
        r._v.y() = std::numeric_limits<value_type>::signaling_NaN();
        return r;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return _v.x() != 0.0f or _v.y() != 0.0f or _v.z() != 0.0f;
    }

    /** Access the x-as-width element from the extent.
     * A extent can be seen as having a width, height and depth,
     * these accessors are aliases for x, y, and z.
     *
     * @return a reference to the x element.
     */
    [[nodiscard]] constexpr float& width() noexcept
    {
        return _v.x();
    }

    /** Access the y-as-height element from the extent.
     * A extent can be seen as having a width, height and depth,
     * these accessors are aliases for x, y, and z.
     *
     * @return a reference to the y element.
     */
    [[nodiscard]] constexpr float& height() noexcept
    {
        return _v.y();
    }

    /** Access the x-as-width element from the extent.
     * A extent can be seen as having a width, height and depth,
     * these accessors are aliases for x, y, and z.
     *
     * @return a reference to the x element.
     */
    [[nodiscard]] constexpr float width() const noexcept
    {
        return _v.x();
    }

    /** Access the y-as-height element from the extent.
     * A extent can be seen as having a width, height and depth,
     * these accessors are aliases for x, y, and z.
     *
     * @return a reference to the y element.
     */
    [[nodiscard]] constexpr float height() const noexcept
    {
        return _v.y();
    }

    [[nodiscard]] constexpr vector2 right() const noexcept
    {
        return vector2{_v.x000()};
    }

    [[nodiscard]] constexpr vector2 up() const noexcept
    {
        return vector2{_v._0y00()};
    }

    constexpr extent2& operator+=(extent2 const& rhs) noexcept
    {
        return *this = *this + rhs;
    }

    /** Add two extents from each other.
     * @param lhs The first extent.
     * @param rhs The second extent.
     * @return A new extent.
     */
    [[nodiscard]] constexpr friend extent2 operator+(extent2 const& lhs, extent2 const& rhs) noexcept
    {
        return extent2{lhs._v + rhs._v};
    }

    /** Subtract two extents from each other.
     * @param lhs The first extent.
     * @param rhs The second extent.
     * @return A new extent.
     */
    [[nodiscard]] constexpr friend extent2 operator-(extent2 const& lhs, extent2 const& rhs) noexcept
    {
        return extent2{lhs._v - rhs._v};
    }

    /** Scale the extent by a scaler.
     * @param lhs The extent to scale.
     * @param rhs The scaling factor.
     * @return The scaled extent.
     */
    [[nodiscard]] constexpr friend extent2 operator*(extent2 const& lhs, float const& rhs) noexcept
    {
        return extent2{lhs._v * array_type::broadcast(rhs)};
    }

    [[nodiscard]] constexpr friend extent2 operator+(extent2 const& lhs, vector2 const& rhs) noexcept
    {
        return extent2{static_cast<array_type>(lhs) + static_cast<array_type>(rhs)};
    }

    [[nodiscard]] constexpr friend vector2 operator+(vector2 const& lhs, extent2 const& rhs) noexcept
    {
        return vector2{static_cast<array_type>(lhs) + static_cast<array_type>(rhs)};
    }

    /** Add a scaler to the extent.
     * @param lhs The extent to scale.
     * @param rhs The scaling factor.
     * @return The scaled extent.
     */
    [[nodiscard]] constexpr friend extent2 operator+(extent2 const& lhs, float const& rhs) noexcept
    {
        auto r = extent2{};
        for (std::size_t i = 0; i != 2; ++i) {
            r._v[i] = lhs._v[i] + rhs;
        }

        return r;
    }

    /** Scale the extent by a scaler.
     * @param lhs The extent to scale.
     * @param rhs The scaling factor.
     * @return The scaled extent.
     */
    [[nodiscard]] constexpr friend extent2 operator*(float const& lhs, extent2 const& rhs) noexcept
    {
        return extent2{array_type::broadcast(lhs) * rhs._v};
    }

    /** Compare if two extents are equal.
     * @param lhs The first extent.
     * @param rhs The second extent.
     * @return True if both extents are completely equal to each other.
     */
    [[nodiscard]] constexpr friend bool operator==(extent2 const& lhs, extent2 const& rhs) noexcept
    {
        return equal(lhs._v, rhs._v);
    }

    [[nodiscard]] constexpr friend std::partial_ordering operator<=>(extent2 const& lhs, extent2 const& rhs) noexcept
    {
        constexpr std::size_t mask = 0b0011;

        auto const equal = (lhs._v == rhs._v).mask() & mask;
        if (equal == mask) {
            // Only equivalent if all elements are equal.
            return std::partial_ordering::equivalent;
        }

        auto const less = (lhs._v < rhs._v).mask() & mask;
        if ((less | equal) == mask) {
            // If one or more elements is less (but none are greater) then the ordering is less.
            return std::partial_ordering::less;
        }

        auto const greater = (lhs._v > rhs._v).mask() & mask;
        if ((greater | equal) == mask) {
            // If one or more elements is greater (but none are less) then the ordering is greater.
            return std::partial_ordering::greater;
        }

        // Some elements are less and others are greater, we don't have an ordering.
        return std::partial_ordering::unordered;
    }

    /** Get the squared length of the extent.
     * @param rhs The extent.
     * @return The length of the extent.
     */
    [[nodiscard]] hi_force_inline constexpr friend float squared_hypot(extent2 const& rhs) noexcept
    {
        return dot<0b0011>(rhs._v, rhs._v).x();
    }

    /** Get the length of the extent.
     * @param rhs The extent.
     * @return The length of the extent.
     */
    [[nodiscard]] friend float hypot(extent2 const& rhs) noexcept
    {
        return hypot<0b0011>(rhs._v).x();
    }

    /** Get the length of the extent.
     * @param rhs The extent.
     * @return One over the length of the extent.
     */
    [[nodiscard]] constexpr friend float rcp_hypot(extent2 const& rhs) noexcept
    {
        return rhypot<0b0011>(rhs._v).x();
    }

    /** Normalize a extent to a unit extent.
     * @param rhs The extent.
     * @return A extent with the same direction as the given extent, but its length is 1.0.
     */
    [[nodiscard]] constexpr friend extent2 normalize(extent2 const& rhs) noexcept
    {
        return extent2{normalize<0b0011>(rhs._v)};
    }

    [[nodiscard]] constexpr friend extent2 ceil(extent2 const& rhs) noexcept
    {
        return extent2{ceil(array_type{rhs})};
    }

    [[nodiscard]] constexpr friend extent2 floor(extent2 const& rhs) noexcept
    {
        return extent2{floor(static_cast<array_type>(rhs))};
    }

    [[nodiscard]] constexpr friend extent2 round(extent2 const& rhs) noexcept
    {
        return extent2{round(static_cast<array_type>(rhs))};
    }

    [[nodiscard]] constexpr friend extent2 min(extent2 const& lhs, extent2 const& rhs) noexcept
    {
        return extent2{min(static_cast<array_type>(lhs), static_cast<array_type>(rhs))};
    }

    [[nodiscard]] constexpr friend extent2 max(extent2 const& lhs, extent2 const& rhs) noexcept
    {
        return extent2{max(static_cast<array_type>(lhs), static_cast<array_type>(rhs))};
    }

    template<std::convertible_to<extent2>... Rest>
    [[nodiscard]] constexpr friend extent2 min(extent2 const& lhs, extent2 const& rhs, Rest const&... rest) noexcept
    {
        return min(lhs, min(rhs, rest...));
    }

    template<std::convertible_to<extent2>... Rest>
    [[nodiscard]] constexpr friend extent2 max(extent2 const& lhs, extent2 const& rhs, Rest const&... rest) noexcept
    {
        return max(lhs, max(rhs, rest...));
    }

    [[nodiscard]] constexpr friend extent2 clamp(extent2 const& value, extent2 const& min, extent2 const& max) noexcept
    {
        return extent2{clamp(static_cast<array_type>(value), static_cast<array_type>(min), static_cast<array_type>(max))};
    }

    /**
     * @brief Clamp an extent to fit inside another extent while maintaining the
     * aspect ratio.
     *
     * This function returns a new extent that fits the @a needle extent into
     * the @a haystack extent without distorting the aspect ratio.
     *
     * @param needle The extent to fit into the @a haystack extent.
     * @param haystack The extent to fit the @a needle extent into.
     * @return The new extent that fits the @a needle extent into the @a haystack extent.
     */
    [[nodiscard]] constexpr friend extent2 aspect_clamp(extent2 const &needle, extent2 const& haystack) noexcept
    {
        auto const needle_aspect = needle.width() / needle.height();

        auto r = needle;

        if (r.width() > haystack.width()) {
            r.width() = haystack.width();
            r.height() = r.width() / needle_aspect;
        }
        if (r.height() > haystack.height()) {
            r.height() = haystack.height();
            r.width() = r.height() * needle_aspect;
        }
        
        return r;
    }

    /**
     * @brief Fit an extent into another extent while maintaining the aspect ratio.
     *
     * This function returns a new extent that fits the @a needle extent into
     * the @a haystack extent without distorting the aspect ratio.
     *
     * @param needle The extent to fit into the @a haystack extent.
     * @param haystack The extent to fit the @a needle extent into.
     * @return The new extent that fits the @a needle extent into the @a haystack extent.
     */
    [[nodiscard]] constexpr friend extent2 aspect_fit(extent2 const& needle, extent2 const &haystack) noexcept
    {
        auto const needle_aspect = needle.width() / needle.height();
        auto const haystack_aspect = haystack.width() / haystack.height();

        auto r = needle;

        if (needle_aspect > haystack_aspect) {
            r.width() = haystack.width();
            r.height() = r.width() / needle_aspect;
        } else {
            r.height() = haystack.height();
            r.width() = r.height() * needle_aspect;
        }

        return r;
    }

    /** Check if the extent is valid.
     * Extends must be positive.
     * This function will check if w is zero, and with 2D extent is z is zero.
     */
    [[nodiscard]] constexpr bool holds_invariant() const noexcept
    {
        return _v.x() >= 0.0f and _v.y() >= 0.0f and _v.z() == 0.0f and _v.w() == 0.0f;
    }

    [[nodiscard]] friend std::string to_string(extent2 const& rhs) noexcept
    {
        return std::format("[{}, {}]", rhs._v.x(), rhs._v.y());
    }

    friend std::ostream& operator<<(std::ostream& lhs, extent2 const& rhs) noexcept
    {
        return lhs << to_string(rhs);
    }

private:
    array_type _v;
};

}} // namespace hi::v1

// XXX #617 MSVC bug does not handle partial specialization in modules.
hi_export template<>
struct std::formatter<hi::extent2, char> : std::formatter<std::string, char> {
    auto format(hi::extent2 const& t, auto& fc) const
    {
        return std::formatter<std::string, char>::format(std::format("[{}, {}]", t.width(), t.height()), fc);
    }
};
