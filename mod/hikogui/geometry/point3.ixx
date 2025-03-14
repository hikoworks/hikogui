// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include "../macros.hpp"

#include <format>
#include <concepts>
#include <exception>
#include <compare>

export module hikogui_geometry : point3;
import : extent3;
import : point2;
import : vector3;
import hikogui_SIMD;
import hikogui_utility;

export namespace hi::inline v1 {

/** A high-level geometric point
 * Part of the high-level vec, point, mat and color types.
 *
 * A point, for both 2D or 3D is internally represented
 * as a 4D homogeneous vector. Which can be efficiently implemented
 * as a __m128 SSE register.
 */
class point3 {
public:
    using array_type = simd<float, 4>;
    using value_type = array_type::value_type;

    constexpr point3(point3 const&) noexcept = default;
    constexpr point3(point3&&) noexcept = default;
    constexpr point3& operator=(point3 const&) noexcept = default;
    constexpr point3& operator=(point3&&) noexcept = default;

    /** Construct a point from a lower dimension point.
     */
    [[nodiscard]] constexpr point3(point2 const& other) noexcept : _v(static_cast<array_type>(other)) {}

    [[nodiscard]] constexpr explicit operator point2() const noexcept
    {
        auto tmp = _v;
        tmp.z() = 0.0f;
        return point2{tmp};
    }

    /** Construct a point from a lower dimension point.
     */
    [[nodiscard]] constexpr point3(point2 const& other, float z) noexcept : _v(static_cast<array_type>(other))
    {
        _v.z() = z;
    }

    /** Convert a point to its array_type-numeric_array.
     */
    [[nodiscard]] constexpr explicit operator array_type() const noexcept
    {
        return _v;
    }

    /** Construct a point from a array_type-simd.
     */
    [[nodiscard]] constexpr explicit point3(array_type const& other) noexcept : _v(other)
    {
        hi_axiom(holds_invariant());
    }

    /** Construct a point at the origin of the coordinate system.
     */
    [[nodiscard]] constexpr point3() noexcept : _v(0.0f, 0.0f, 0.0f, 1.0f) {}

    /** Construct a 3D point from x, y and z elements.
     * @param x The x element.
     * @param y The y element.
     * @param z The z element.
     */
    [[nodiscard]] constexpr point3(float x, float y, float z = 0.0f) noexcept : _v(x, y, z, 1.0f) {}

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

    /** Access the z element from the point.
     * @return a reference to the z element.
     */
    [[nodiscard]] constexpr float& z() noexcept
    {
        return _v.z();
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

    /** Access the z element from the point.
     * @return a reference to the z element.
     */
    [[nodiscard]] constexpr float z() const noexcept
    {
        return _v.z();
    }

    constexpr point3& operator+=(vector3 const& rhs) noexcept
    {
        return *this = *this + rhs;
    }

    constexpr point3& operator-=(vector3 const& rhs) noexcept
    {
        return *this = *this - rhs;
    }

    /** Move a point along a vector.
     * @param lhs The point to move.
     * @param rhs The vector to move along.
     * @return The moved point.
     */
    [[nodiscard]] constexpr friend point3 operator+(point3 const& lhs, vector3 const& rhs) noexcept
    {
        return point3{lhs._v + array_type{rhs}};
    }

    /** Move a point along a vector.
     * @param lhs The vector to move along.
     * @param rhs The point to move.
     * @return The moved point.
     */
    [[nodiscard]] constexpr friend point3 operator+(vector3 const& lhs, point3 const& rhs) noexcept
    {
        return point3{array_type{lhs} + rhs._v};
    }

    /** Move a point backward along the vector.
     * @param lhs The point to move.
     * @param rhs The vector to move backward.
     * @return The moved point.
     */
    [[nodiscard]] constexpr friend point3 operator-(point3 const& lhs, vector3 const& rhs) noexcept
    {
        return point3{lhs._v - array_type{rhs}};
    }

    /** Find the vector between two points
     * @param lhs The first point.
     * @param rhs The second point.
     * @return The vector from the second to first point.
     */
    [[nodiscard]] constexpr friend vector3 operator-(point3 const& lhs, point3 const& rhs) noexcept
    {
        return vector3{lhs._v - rhs._v};
    }

    /** Compare if two points are equal.
     * @param lhs The first point.
     * @param rhs The second point.
     * @return True if both point are completely equal to each other.
     */
    [[nodiscard]] constexpr friend bool operator==(point3 const& lhs, point3 const& rhs) noexcept
    {
        return equal(lhs._v, rhs._v);
    }

    [[nodiscard]] friend constexpr point3 midpoint(point3 const& lhs, point3 const& rhs) noexcept
    {
        return point3{midpoint(lhs._v, rhs._v)};
    }

    [[nodiscard]] friend constexpr point3 reflect(point3 const& lhs, point3 const& rhs) noexcept
    {
        return point3{reflect_point(lhs._v, rhs._v)};
    }

    /** Mix the two points and get the lowest value of each element.
     * @param lhs The first point.
     * @param rhs The first point.
     * @return A point that is the most left of both points, and most bottom of both points.
     */
    [[nodiscard]] friend constexpr point3 min(point3 const& lhs, point3 const& rhs) noexcept
    {
        return point3{min(lhs._v, rhs._v)};
    }

    /** Mix the two points and get the highest value of each element.
     * @param lhs The first point.
     * @param rhs The first point.
     * @return A point that is the most right of both points, and most top of both points.
     */
    [[nodiscard]] friend constexpr point3 max(point3 const& lhs, point3 const& rhs) noexcept
    {
        return point3{max(lhs._v, rhs._v)};
    }

    /** Round the coordinates of a point toward nearest integer.
     */
    [[nodiscard]] friend constexpr point3 round(point3 const& rhs) noexcept
    {
        return point3{round(rhs._v)};
    }

    /** Round the coordinates of a point toward the right-top.
     */
    [[nodiscard]] friend constexpr point3 ceil(point3 const& rhs) noexcept
    {
        return point3{ceil(rhs._v)};
    }

    /** Round the coordinates of a point toward the left-bottom.
     */
    [[nodiscard]] friend constexpr point3 floor(point3 const& rhs) noexcept
    {
        return point3{floor(rhs._v)};
    }

    /** Round the coordinates of a point toward the top-right with the given granularity.
     */
    [[nodiscard]] friend constexpr point3 ceil(point3 const& lhs, extent3 rhs) noexcept
    {
        hilet rhs_ = array_type{rhs}.xyz1();
        return point3{ceil(lhs._v / rhs_) * rhs_};
    }

    /** Round the coordinates of a point toward the left-bottom with the given granularity.
     */
    [[nodiscard]] friend constexpr point3 floor(point3 const& lhs, extent3 rhs) noexcept
    {
        hilet rhs_ = array_type{rhs}.xyz1();
        return point3{floor(lhs._v / rhs_) * rhs_};
    }

    [[nodiscard]] friend float distance(point3 const& lhs, point3 const& rhs) noexcept
    {
        return hypot(rhs - lhs);
    }

    /** Check if the point is valid.
     * This function will check if w is not zero, and with a 2D point is z is zero.
     */
    [[nodiscard]] constexpr bool holds_invariant() const noexcept
    {
        return _v.w() != 0.0f;
    }

    [[nodiscard]] friend std::string to_string(point3 const& rhs) noexcept
    {
        return std::format("<{}, {}, {}>", rhs._v.x(), rhs._v.y(), rhs._v.z());
    }

    friend std::ostream& operator<<(std::ostream& lhs, point3 const& rhs) noexcept
    {
        return lhs << to_string(rhs);
    }

private:
    array_type _v;
};

[[nodiscard]] constexpr point3 operator+(point2 const& lhs, vector3 const& rhs) noexcept
{
    return point3{f32x4{lhs} + f32x4{rhs}};
}

[[nodiscard]] constexpr point3 operator+(vector3 const& lhs, point2 const& rhs) noexcept
{
    return point3{f32x4{lhs} + f32x4{rhs}};
}

[[nodiscard]] constexpr point3 operator-(point2 const& lhs, vector3 const& rhs) noexcept
{
    return point3{f32x4{lhs} - f32x4{rhs}};
}




} // namespace hi::inline v1

// XXX #617 MSVC bug does not handle partial specialization in modules.
export template<>
struct std::formatter<hi::point3, char> {
    auto parse(auto& pc)
    {
        return pc.end();
    }

    auto format(hi::point3 const& t, auto& fc) const
    {
        return std::vformat_to(fc.out(), "<{}, {}, {}>", std::make_format_args(t.x(), t.y(), t.z()));
    }
};
