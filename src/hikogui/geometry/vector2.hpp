// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <hikocpu/hikocpu.hpp>
#include <exception>
#include <format>
#include <ostream>
#include <compare>

hi_export_module(hikogui.geometry : vector2);

hi_export namespace hi {
inline namespace v1 {

/** A high-level geometric vector
 * Part of the high-level vector, point, mat and color types.
 *
 * A vector, for both 2D or 3D is internally represented
 * as a 4D homogeneous vector. Which can be efficiently implemented
 * as a __m128 SSE register.
 */
class vector2 {
public:
    using array_type = simd<float, 4>;
    using value_type = array_type::value_type;

    constexpr vector2(vector2 const&) noexcept = default;
    constexpr vector2(vector2&&) noexcept = default;
    constexpr vector2& operator=(vector2 const&) noexcept = default;
    constexpr vector2& operator=(vector2&&) noexcept = default;

    /** Convert a vector to its array_type-numeric_array.
     */
    [[nodiscard]] constexpr explicit operator array_type() const noexcept
    {
        return _v;
    }

    /** Construct a vector from a array_type-simd.
     */
    [[nodiscard]] constexpr explicit vector2(array_type const& other) noexcept : _v(other)
    {
        hi_axiom(holds_invariant());
    }

    /** Construct a empty vector / zero length.
     */
    [[nodiscard]] constexpr vector2() noexcept : _v(0.0f, 0.0f, 0.0f, 0.0f) {}

    /** Construct a 3D vector from x, y and z elements.
     * @param x The x element.
     * @param y The y element.
     * @param z The z element.
     */
    [[nodiscard]] constexpr vector2(float x, float y) noexcept : _v(x, y, 0.0f, 0.0f) {}

    /** Access the x element from the vector.
     * @return a reference to the x element.
     */
    [[nodiscard]] constexpr float& x() noexcept
    {
        return _v.x();
    }

    /** Access the y element from the vector.
     * @return a reference to the y element.
     */
    [[nodiscard]] constexpr float& y() noexcept
    {
        return _v.y();
    }

    /** Access the x element from the vector.
     * @return a reference to the x element.
     */
    [[nodiscard]] constexpr float x() const noexcept
    {
        return _v.x();
    }

    /** Access the y element from the vector.
     * @return a reference to the y element.
     */
    [[nodiscard]] constexpr float y() const noexcept
    {
        return _v.y();
    }

    /** Mirror this vector.
     * @return The mirrored vector.
     */
    [[nodiscard]] constexpr vector2 operator-() const noexcept
    {
        return vector2{-_v};
    }

    constexpr vector2& operator+=(vector2 const& rhs) noexcept
    {
        return *this = *this + rhs;
    }

    constexpr vector2& operator-=(vector2 const& rhs) noexcept
    {
        return *this = *this - rhs;
    }

    constexpr vector2& operator*=(float const& rhs) noexcept
    {
        return *this = *this * rhs;
    }

    /** Add two vectors from each other.
     * @param lhs The first vector.
     * @param rhs The second vector.
     * @return A new vector.
     */
    [[nodiscard]] constexpr friend vector2 operator+(vector2 const& lhs, vector2 const& rhs) noexcept
    {
        return vector2{lhs._v + rhs._v};
    }

    /** Subtract two vectors from each other.
     * @param lhs The first vector.
     * @param rhs The second vector.
     * @return A new vector.
     */
    [[nodiscard]] constexpr friend vector2 operator-(vector2 const& lhs, vector2 const& rhs) noexcept
    {
        return vector2{lhs._v - rhs._v};
    }

    /** Scale the vector by a scaler.
     * @param lhs The vector to scale.
     * @param rhs The scaling factor.
     * @return The scaled vector.
     */
    [[nodiscard]] constexpr friend vector2 operator*(vector2 const& lhs, float const& rhs) noexcept
    {
        return vector2{lhs._v * array_type::broadcast(rhs)};
    }

    /** Scale the vector by a scaler.
     * @param lhs The vector to scale.
     * @param rhs The scaling factor.
     * @return The scaled vector.
     */
    [[nodiscard]] constexpr friend vector2 operator*(float const& lhs, vector2 const& rhs) noexcept
    {
        return vector2{array_type::broadcast(lhs) * rhs._v};
    }

    /** Compare if two vectors are equal.
     * @param lhs The first vector.
     * @param rhs The second vector.
     * @return True if both vectors are completely equal to each other.
     */
    [[nodiscard]] constexpr friend bool operator==(vector2 const& lhs, vector2 const& rhs) noexcept
    {
        return equal(lhs._v, rhs._v);
    }

    /** Get the squared length of the vector.
     * @param rhs The vector.
     * @return The length of the vector.
     */
    [[nodiscard]] constexpr friend float squared_hypot(vector2 const& rhs) noexcept
    {
        return dot<0b0011>(rhs._v, rhs._v).x();
    }

    /** Get the length of the vector.
     * @param rhs The vector.
     * @return The length of the vector.
     */
    [[nodiscard]] friend float hypot(vector2 const& rhs) noexcept
    {
        return hypot<0b0011>(rhs._v).x();
    }

    /** Get the length of the vector.
     * @param rhs The vector.
     * @return One over the length of the vector.
     */
    [[nodiscard]] constexpr friend float rcp_hypot(vector2 const& rhs) noexcept
    {
        return rhypot<0b0011>(rhs._v).x();
    }

    /** Normalize a vector to a unit vector.
     * @param rhs The vector.
     * @return A vector with the same direction as the given vector, but its length is 1.0.
     */
    [[nodiscard]] constexpr friend vector2 normalize(vector2 const& rhs) noexcept
    {
        return vector2{normalize<0b0011>(rhs._v)};
    }

    /** Get the dot product between two vectors.
     * @param lhs The first vector.
     * @param rhs The second vector.
     * @return The dot product from the two given vectors.
     */
    [[nodiscard]] constexpr friend float dot(vector2 const& lhs, vector2 const& rhs) noexcept
    {
        return dot<0b0011>(lhs._v, rhs._v).x();
    }

    /** Get the determinate between two vectors.
     * @param lhs The first vector.
     * @param rhs The second vector.
     * @return The dot product from the two given vectors.
     */
    [[nodiscard]] constexpr friend float det(vector2 const& lhs, vector2 const& rhs) noexcept
    {
        return lhs.x() * rhs.y() - lhs.y() * rhs.x();
    }

    /** Get the cross product of one 2D vectors.
     * @param rhs The vector.
     * @return A vector perpendicular to the given vector.
     */
    [[nodiscard]] constexpr friend vector2 cross(vector2 const& rhs) noexcept
    {
        return vector2{-rhs._v.y(), rhs._v.x()};
    }

    /** Get the cross product between two 2D vectors.
     * This function is useful for finding the winding direction of the vectors,
     * when doing ray casting.
     *
     * @param lhs The first vector.
     * @param rhs The second vector.
     * @return A scaler representing the sharpness of the corner between the two vectors.
     */
    [[nodiscard]] constexpr friend float cross(vector2 const& lhs, vector2 const& rhs) noexcept
    {
        auto const tmp1 = rhs._v.yxwz();
        auto const tmp2 = lhs._v * tmp1;
        auto const tmp3 = hsub(tmp2, tmp2);
        return tmp3.x();
    }

    /** Get the normal on a 2D vector.
     * @param rhs The vector.
     * @return A normal on the vector.
     */
    [[nodiscard]] constexpr friend vector2 normal(vector2 const& rhs) noexcept
    {
        return normalize(cross(rhs));
    }

    /** Mix the two vectors and get the lowest value of each element.
     * @param lhs The first vector.
     * @param rhs The first vector.
     * @return A vector that points the most left of both vectors, and most downward of both vectors.
     */
    [[nodiscard]] friend constexpr vector2 min(vector2 const& lhs, vector2 const& rhs) noexcept
    {
        return vector2{min(static_cast<array_type>(lhs), static_cast<array_type>(rhs))};
    }

    /** Mix the two vectors and get the highest value of each element.
     * @param lhs The first vector.
     * @param rhs The first vector.
     * @return A vector that points the most right of both vectors, and most upward of both vectors.
     */
    [[nodiscard]] friend constexpr vector2 max(vector2 const& lhs, vector2 const& rhs) noexcept
    {
        return vector2{max(static_cast<array_type>(lhs), static_cast<array_type>(rhs))};
    }

    /** Round the elements of the vector toward nearest integer.
     */
    [[nodiscard]] friend constexpr vector2 round(vector2 const& rhs) noexcept
    {
        return vector2{round(static_cast<array_type>(rhs))};
    }

    /** Round the elements of the vector toward upward and to the right.
     */
    [[nodiscard]] friend constexpr vector2 ceil(vector2 const& rhs) noexcept
    {
        return vector2{ceil(static_cast<array_type>(rhs))};
    }

    /** Round the elements of the vector toward downward and to the left.
     */
    [[nodiscard]] friend constexpr vector2 floor(vector2 const& rhs) noexcept
    {
        return vector2{floor(static_cast<array_type>(rhs))};
    }

    /** Check if the vector is valid.
     * This function will check if w is zero, and with 2D vector is z is zero.
     */
    [[nodiscard]] constexpr bool holds_invariant() const noexcept
    {
        return _v.z() == 0.0f and _v.w() == 0.0f;
    }

    [[nodiscard]] friend std::string to_string(vector2 const& rhs) noexcept
    {
        return std::format("({}, {})", rhs._v.x(), rhs._v.y());
    }

    friend std::ostream& operator<<(std::ostream& lhs, vector2 const& rhs) noexcept
    {
        return lhs << to_string(rhs);
    }

private:
    array_type _v;
};

} // namespace v1
} // namespace hi::v1

// XXX #617 MSVC bug does not handle partial specialization in modules.
hi_export template<>
struct std::formatter<hi::vector2, char> : std::formatter<std::string, char> {
    auto format(hi::vector2 const& t, auto& fc) const
    {
        return std::formatter<std::string, char>::format(std::format("({}, {})", t.x(), t.y()), fc);
    }
};
