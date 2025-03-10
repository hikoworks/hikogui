// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "vector2.hpp"
#include "extent2.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <hikocpu/hikocpu.hpp>
#include <format>
#include <concepts>
#include <exception>
#include <compare>

hi_export_module(hikogui.geometry : point2);

hi_export namespace hi::inline v1 {

/** A high-level geometric point
 * Part of the high-level vec, point, mat and color types.
 *
 * A point, for both 2D or 3D is internally represented
 * as a 4D homogeneous vector. Which can be efficiently implemented
 * as a __m128 SSE register.
 */
class point2 {
public:
    using array_type = simd<float, 4>;
    using value_type = array_type::value_type;

    constexpr point2(point2 const&) noexcept = default;
    constexpr point2(point2&&) noexcept = default;
    constexpr point2& operator=(point2 const&) noexcept = default;
    constexpr point2& operator=(point2&&) noexcept = default;

    /** Convert a point to its array_type-numeric_array.
     */
    [[nodiscard]] constexpr explicit operator array_type() const noexcept
    {
        return _v;
    }

    /** Construct a point from a array_type-simd.
     */
    [[nodiscard]] constexpr explicit point2(array_type const& other) noexcept : _v(other)
    {
        hi_axiom(holds_invariant());
    }

    /** Construct a point at the origin of the coordinate system.
     */
    [[nodiscard]] constexpr point2() noexcept : _v(0.0f, 0.0f, 0.0f, 1.0f) {}

    /** Construct a 3D point from x, y and z elements.
     * @param x The x element.
     * @param y The y element.
     */
    [[nodiscard]] constexpr point2(float x, float y) noexcept : _v(x, y, 0.0f, 1.0f) {}

    /** Access the x element from the point.
     * @return a reference to the x element.
     */
    [[nodiscard]] constexpr float& x() noexcept
    {
        return _v.x();
    }

    /** Access the y element from the point.
     * @return a reference to the y element.
     */
    [[nodiscard]] constexpr float& y() noexcept
    {
        return _v.y();
    }

    /** Access the x element from the point.
     * @return a reference to the x element.
     */
    [[nodiscard]] constexpr float x() const noexcept
    {
        return _v.x();
    }

    /** Access the y element from the point.
     * @return a reference to the y element.
     */
    [[nodiscard]] constexpr float y() const noexcept
    {
        return _v.y();
    }

    constexpr point2& operator+=(vector2 const& rhs) noexcept
    {
        return *this = *this + rhs;
    }

    constexpr point2& operator-=(vector2 const& rhs) noexcept
    {
        return *this = *this - rhs;
    }

    /** Move a point along a vector.
     * @param lhs The point to move.
     * @param rhs The vector to move along.
     * @return The moved point.
     */
    [[nodiscard]] constexpr friend point2 operator+(point2 const& lhs, vector2 const& rhs) noexcept
    {
        return point2{lhs._v + array_type{rhs}};
    }

    /** Move a point along a vector.
     * @param lhs The vector to move along.
     * @param rhs The point to move.
     * @return The moved point.
     */
    [[nodiscard]] constexpr friend point2 operator+(vector2 const& lhs, point2 const& rhs) noexcept
    {
        return point2{array_type{lhs} + rhs._v};
    }

    /** Move a point backward along the vector.
     * @param lhs The point to move.
     * @param rhs The vector to move backward.
     * @return The moved point.
     */
    [[nodiscard]] constexpr friend point2 operator-(point2 const& lhs, vector2 const& rhs) noexcept
    {
        return point2{lhs._v - array_type{rhs}};
    }

    /** Find the vector between two points
     * @param lhs The first point.
     * @param rhs The second point.
     * @return The vector from the second to first point.
     */
    [[nodiscard]] constexpr friend vector2 operator-(point2 const& lhs, point2 const& rhs) noexcept
    {
        return vector2{lhs._v - rhs._v};
    }

    /** Compare if two points are equal.
     * @param lhs The first point.
     * @param rhs The second point.
     * @return True if both point are completely equal to each other.
     */
    [[nodiscard]] constexpr friend bool operator==(point2 const& lhs, point2 const& rhs) noexcept
    {
        return equal(lhs._v, rhs._v);
    }

    [[nodiscard]] friend constexpr point2 midpoint(point2 const& lhs, point2 const& rhs) noexcept
    {
        return point2{(lhs._v + rhs._v) * array_type::broadcast(0.5f)};
    }

    /** Reflect a point.
     *
     * @param lhs The point to be reflected
     * @param rhs The anchor point.
     * @return The reflected point.
    */
    [[nodiscard]] friend constexpr point2 reflect(point2 const& lhs, point2 const& rhs) noexcept
    {
        return point2{rhs._v - (lhs._v - rhs._v)};
    }

    /** Mix the two points and get the lowest value of each element.
     * @param lhs The first point.
     * @param rhs The first point.
     * @return A point that is the most left of both points, and most bottom of both points.
     */
    [[nodiscard]] friend constexpr point2 min(point2 const& lhs, point2 const& rhs) noexcept
    {
        return point2{min(lhs._v, rhs._v)};
    }

    /** Mix the two points and get the highest value of each element.
     * @param lhs The first point.
     * @param rhs The first point.
     * @return A point that is the most right of both points, and most top of both points.
     */
    [[nodiscard]] friend constexpr point2 max(point2 const& lhs, point2 const& rhs) noexcept
    {
        return point2{max(lhs._v, rhs._v)};
    }

    /** Round the coordinates of a point toward nearest integer.
     */
    [[nodiscard]] friend constexpr point2 round(point2 const& rhs) noexcept
    {
        return point2{round(rhs._v)};
    }

    /** Round the coordinates of a point toward the right-top.
     */
    [[nodiscard]] friend constexpr point2 ceil(point2 const& rhs) noexcept
    {
        return point2{ceil(rhs._v)};
    }

    /** Round the coordinates of a point toward the left-bottom.
     */
    [[nodiscard]] friend constexpr point2 floor(point2 const& rhs) noexcept
    {
        return point2{floor(rhs._v)};
    }

    /** Round the coordinates of a point toward the top-right with the given granularity.
     */
    [[nodiscard]] friend constexpr point2 ceil(point2 const& lhs, extent2 rhs) noexcept
    {
        auto const rhs_ = array_type{rhs}.xy11();
        return point2{ceil(lhs._v / rhs_) * rhs_};
    }

    /** Round the coordinates of a point toward the left-bottom with the given granularity.
     */
    [[nodiscard]] friend constexpr point2 floor(point2 const& lhs, extent2 rhs) noexcept
    {
        auto const rhs_ = array_type{rhs}.xy11();
        return point2{floor(lhs._v / rhs_) * rhs_};
    }

    [[nodiscard]] friend float distance(point2 const& lhs, point2 const& rhs) noexcept
    {
        return hypot(rhs - lhs);
    }

    /** Check if the point is valid.
     * This function will check if w is not zero, and with a 2D point is z is zero.
     */
    [[nodiscard]] constexpr bool holds_invariant() const noexcept
    {
        return _v.z() == 0.0f and _v.w() != 0.0f;
    }

    [[nodiscard]] friend std::string to_string(point2 const& rhs) noexcept
    {
        return std::format("<{}, {}>", rhs._v.x(), rhs._v.y());
    }

    friend std::ostream& operator<<(std::ostream& lhs, point2 const& rhs) noexcept
    {
        return lhs << to_string(rhs);
    }

private:
    array_type _v;
};

} // namespace hi::inline v1

// XXX #617 MSVC bug does not handle partial specialization in modules.
hi_export template<>
struct std::formatter<hi::point2, char> : std::formatter<std::string, char> {
    auto format(hi::point2 const& t, auto& fc) const
    {
        return std::formatter<std::string, char>::format(std::format("<{}, {}>", t.x(), t.y()), fc);
    }
};
