// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "vector2.hpp"
#include "../macros.hpp"
#include <hikocpu/hikocpu.hpp>
#include <exception>
#include <format>
#include <ostream>
#include <compare>

hi_export_module(hikogui.geometry : vector3);

hi_export namespace hi { inline namespace v1 {

/** A high-level geometric vector
 * Part of the high-level vector, point, mat and color types.
 *
 * A vector, for both 2D or 3D is internally represented
 * as a 4D homogeneous vector. Which can be efficiently implemented
 * as a __m128 SSE register.
 */
class vector3 {
public:
    using array_type = simd<float, 4>;
    using value_type = array_type::value_type;

    constexpr vector3(vector3 const&) noexcept = default;
    constexpr vector3(vector3&&) noexcept = default;
    constexpr vector3& operator=(vector3 const&) noexcept = default;
    constexpr vector3& operator=(vector3&&) noexcept = default;

    /** Construct a vector from a lower dimension vector.
     */
    [[nodiscard]] constexpr vector3(vector2 const& other) noexcept : _v(static_cast<array_type>(other))
    {
        hi_axiom(holds_invariant());
    }

    /** Construct a vector from a higher dimension vector.
     * This will clear the values in the higher dimensions.
     */
    [[nodiscard]] constexpr explicit operator vector2() noexcept
    {
        auto tmp = _v;
        tmp.z() = 0.0f;
        return vector2{tmp};
    }

    /** Convert a vector to its array_type-numeric_array.
     */
    [[nodiscard]] constexpr explicit operator array_type() const noexcept
    {
        return _v;
    }

    /** Construct a vector from a array_type-simd.
     */
    [[nodiscard]] constexpr explicit vector3(array_type const& other) noexcept : _v(other)
    {
        hi_axiom(holds_invariant());
    }

    /** Construct a empty vector / zero length.
     */
    [[nodiscard]] constexpr vector3() noexcept : _v(0.0f, 0.0f, 0.0f, 0.0f) {}

    /** Construct a 3D vector from x, y and z elements.
     * @param x The x element.
     * @param y The y element.
     * @param z The z element.
     */
    [[nodiscard]] constexpr vector3(float x, float y, float z = 0.0f) noexcept : _v(x, y, z, 0.0f) {}

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

    /** Access the z element from the vector.
     * @return a reference to the z element.
     */
    [[nodiscard]] constexpr float& z() noexcept
    {
        return _v.z();
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

    /** Access the z element from the vector.
     * @return a reference to the z element.
     */
    [[nodiscard]] constexpr float z() const noexcept
    {
        return _v.z();
    }

    /** Mirror this vector.
     * @return The mirrored vector.
     */
    [[nodiscard]] constexpr vector3 operator-() const noexcept
    {
        return vector3{-_v};
    }

    constexpr vector3& operator+=(vector3 const& rhs) noexcept
    {
        return *this = *this + rhs;
    }

    constexpr vector3& operator-=(vector3 const& rhs) noexcept
    {
        return *this = *this - rhs;
    }

    constexpr vector3& operator*=(float const& rhs) noexcept
    {
        return *this = *this * rhs;
    }

    /** Add two vectors from each other.
     * @param lhs The first vector.
     * @param rhs The second vector.
     * @return A new vector.
     */
    [[nodiscard]] constexpr friend vector3 operator+(vector3 const& lhs, vector3 const& rhs) noexcept
    {
        return vector3{lhs._v + rhs._v};
    }

    /** Subtract two vectors from each other.
     * @param lhs The first vector.
     * @param rhs The second vector.
     * @return A new vector.
     */
    [[nodiscard]] constexpr friend vector3 operator-(vector3 const& lhs, vector3 const& rhs) noexcept
    {
        return vector3{lhs._v - rhs._v};
    }

    /** Scale the vector by a scaler.
     * @param lhs The vector to scale.
     * @param rhs The scaling factor.
     * @return The scaled vector.
     */
    [[nodiscard]] constexpr friend vector3 operator*(vector3 const& lhs, float const& rhs) noexcept
    {
        return vector3{lhs._v * array_type::broadcast(rhs)};
    }

    /** Scale the vector by a scaler.
     * @param lhs The vector to scale.
     * @param rhs The scaling factor.
     * @return The scaled vector.
     */
    [[nodiscard]] constexpr friend vector3 operator*(float const& lhs, vector3 const& rhs) noexcept
    {
        return vector3{array_type::broadcast(lhs) * rhs._v};
    }

    /** Compare if two vectors are equal.
     * @param lhs The first vector.
     * @param rhs The second vector.
     * @return True if both vectors are completely equal to each other.
     */
    [[nodiscard]] constexpr friend bool operator==(vector3 const& lhs, vector3 const& rhs) noexcept
    {
        return equal(lhs._v, rhs._v);
    }

    /** Get the squared length of the vector.
     * @param rhs The vector.
     * @return The length of the vector.
     */
    [[nodiscard]] constexpr friend float squared_hypot(vector3 const& rhs) noexcept
    {
        return dot<0b0111>(rhs._v, rhs._v).x();
    }

    /** Get the length of the vector.
     * @param rhs The vector.
     * @return The length of the vector.
     */
    [[nodiscard]] friend float hypot(vector3 const& rhs) noexcept
    {
        return hypot<0b0111>(rhs._v).x();
    }

    /** Get the length of the vector.
     * @param rhs The vector.
     * @return One over the length of the vector.
     */
    [[nodiscard]] constexpr friend float rcp_hypot(vector3 const& rhs) noexcept
    {
        return rhypot<0b0111>(rhs._v).x();
    }

    /** Normalize a vector to a unit vector.
     * @param rhs The vector.
     * @return A vector with the same direction as the given vector, but its length is 1.0.
     */
    [[nodiscard]] constexpr friend vector3 normalize(vector3 const& rhs) noexcept
    {
        return vector3{normalize<0b0111>(rhs._v)};
    }

    /** Get the dot product between two vectors.
     * @param lhs The first vector.
     * @param rhs The second vector.
     * @return The dot product from the two given vectors.
     */
    [[nodiscard]] constexpr friend float dot(vector3 const& lhs, vector3 const& rhs) noexcept
    {
        return dot<0b0111>(lhs._v, rhs._v).x();
    }

    /** Get the normal on a 3D vector.
     * @param rhs The vector.
     * @param angle The angle around the vector, only value_type{0} is implemented (xy-plane)
     * @return A normal on the vector.
     */
    [[nodiscard]] constexpr friend vector3 normal(vector3 const& rhs, float angle) noexcept
    {
        if (angle != float{0}) {
            hi_not_implemented();
        }
        return normal(vector2{array_type{rhs}.xy00()});
    }

    /** Get the cross product between two 3D vectors.
     * @param lhs The first vector.
     * @param rhs The second vector.
     * @return A vector that is perpendicular to the given vectors.
     */
    [[nodiscard]] constexpr friend vector3 cross(vector3 const& lhs, vector3 const& rhs) noexcept
    {
        auto const a_left = lhs._v.yzxw();
        auto const b_left = rhs._v.zxyw();
        auto const left = a_left * b_left;

        auto const a_right = lhs._v.zxyw();
        auto const b_right = rhs._v.yzxw();
        auto const right = a_right * b_right;
        return vector3{left - right};
    }

    /** Mix the two vectors and get the lowest value of each element.
     * @param lhs The first vector.
     * @param rhs The first vector.
     * @return A vector that points the most left of both vectors, and most downward of both vectors.
     */
    [[nodiscard]] friend constexpr vector3 min(vector3 const& lhs, vector3 const& rhs) noexcept
    {
        return vector3{min(static_cast<array_type>(lhs), static_cast<array_type>(rhs))};
    }

    /** Mix the two vectors and get the highest value of each element.
     * @param lhs The first vector.
     * @param rhs The first vector.
     * @return A vector that points the most right of both vectors, and most upward of both vectors.
     */
    [[nodiscard]] friend constexpr vector3 max(vector3 const& lhs, vector3 const& rhs) noexcept
    {
        return vector3{max(static_cast<array_type>(lhs), static_cast<array_type>(rhs))};
    }

    /** Round the elements of the vector toward nearest integer.
     */
    [[nodiscard]] friend constexpr vector3 round(vector3 const& rhs) noexcept
    {
        return vector3{round(static_cast<array_type>(rhs))};
    }

    /** Round the elements of the vector toward upward and to the right.
     */
    [[nodiscard]] friend constexpr vector3 ceil(vector3 const& rhs) noexcept
    {
        return vector3{ceil(static_cast<array_type>(rhs))};
    }

    /** Round the elements of the vector toward downward and to the left.
     */
    [[nodiscard]] friend constexpr vector3 floor(vector3 const& rhs) noexcept
    {
        return vector3{floor(static_cast<array_type>(rhs))};
    }

    /** Check if the vector is valid.
     * This function will check if w is zero, and with 2D vector is z is zero.
     */
    [[nodiscard]] constexpr bool holds_invariant() const noexcept
    {
        return _v.w() == 0.0f;
    }

    [[nodiscard]] friend std::string to_string(vector3 const& rhs) noexcept
    {
        return std::format("({}, {}, {})", rhs._v.x(), rhs._v.y(), rhs._v.z());
    }

    friend std::ostream& operator<<(std::ostream& lhs, vector3 const& rhs) noexcept
    {
        return lhs << to_string(rhs);
    }

private:
    array_type _v;
};



}} // namespace hi::v1

// XXX #617 MSVC bug does not handle partial specialization in modules.
hi_export template<>
struct std::formatter<hi::vector3, char> : std::formatter<std::string, char> {
    auto format(hi::vector3 const& t, auto& fc) const
    {
        return std::formatter<std::string, char>::format(std::format("({}, {}, {})", t.x(), t.y(), t.z()), fc);
    }
};
