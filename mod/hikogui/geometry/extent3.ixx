// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file geometry/extent3.hpp Defined the geo::extent, extent2 and extent3 types.
 * @ingroup geometry
 */

module;
#include "../macros.hpp"

#include <compare>
#include <concepts>
#include <format>
#include <ostream>
#include <exception>

export module hikogui_geometry : extent3;
import : extent2;
import : vector3;
import hikogui_SIMD;
import hikogui_utility;

export namespace hi { inline namespace v1 {

/** A high-level geometric extent
 * @ingroup geometry
 *
 * A extent, for both 2D or 3D is internally represented
 * as a 4D homogeneous extent. Which can be efficiently implemented
 * as a __m128 SSE register.
 */
class extent3 {
public:
    using array_type = simd<float, 4>;
    using value_type = array_type::value_type;

    constexpr extent3(extent3 const&) noexcept = default;
    constexpr extent3(extent3&&) noexcept = default;
    constexpr extent3& operator=(extent3 const&) noexcept = default;
    constexpr extent3& operator=(extent3&&) noexcept = default;

    /** Construct a extent from a lower dimension extent.
     */
    [[nodiscard]] constexpr extent3(extent2 const& other) noexcept : _v(static_cast<array_type>(other))
    {
        hi_axiom(holds_invariant());
    }

    [[nodiscard]] constexpr explicit operator extent2() const noexcept
    {
        auto tmp = _v;
        tmp.z() = 0.0f;
        return extent2{tmp};
    }

    /** Convert a extent to its array_type-numeric_array.
     */
    [[nodiscard]] constexpr explicit operator array_type() const noexcept
    {
        return _v;
    }

    [[nodiscard]] constexpr explicit extent3(array_type const& other) noexcept : _v(other)
    {
        hi_axiom(holds_invariant());
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return _v.x() != 0.0f or _v.y() != 0.0f or _v.z() != 0.0f;
    }

    [[nodiscard]] constexpr explicit operator vector3() const noexcept
    {
        return vector3{static_cast<array_type>(*this)};
    }

    /** Construct a empty extent / zero length.
     */
    [[nodiscard]] constexpr extent3() noexcept : _v(0.0f, 0.0f, 0.0f, 0.0f) {}

    /** Construct a 3D extent from width, height and depth.
     * @param width The width element.
     * @param height The height element.
     * @param depth The depth element.
     */
    [[nodiscard]] constexpr extent3(float width, float height, float depth = 0.0f) noexcept : _v(width, height, depth, 0.0f)
    {
        hi_axiom(holds_invariant());
    }

    [[nodiscard]] constexpr static extent3 infinity() noexcept
    {
        return extent3{
            std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::infinity()};
    }

    [[nodiscard]] constexpr static extent3 large() noexcept
    {
        return extent3{large_number_v<float>, large_number_v<float>, large_number_v<float>};
    }

    [[nodiscard]] constexpr static extent3 nan() noexcept
    {
        auto r = extent3{};
        r._v.x() = std::numeric_limits<float>::signaling_NaN();
        r._v.y() = std::numeric_limits<float>::signaling_NaN();
        r._v.z() = std::numeric_limits<float>::signaling_NaN();
        return r;
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

    /** Access the z-as-depth element from the extent.
     * A extent can be seen as having a width, height and depth,
     * these accessors are aliases for x, y, and z.
     *
     * @return a reference to the z element.
     */
    [[nodiscard]] constexpr float& depth() noexcept
    {
        return _v.z();
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

    /** Access the z-as-depth element from the extent.
     * A extent can be seen as having a width, height and depth,
     * these accessors are aliases for x, y, and z.
     *
     * @return a reference to the z element.
     */
    [[nodiscard]] constexpr float depth() const noexcept
    {
        return _v.z();
    }

    [[nodiscard]] constexpr vector3 right() const noexcept
    {
        return vector3{_v.x000()};
    }

    [[nodiscard]] constexpr vector3 up() const noexcept
    {
        return vector3{_v._0y00()};
    }

    constexpr extent3& operator+=(extent3 const& rhs) noexcept
    {
        return *this = *this + rhs;
    }

    /** Add two extents from each other.
     * @param lhs The first extent.
     * @param rhs The second extent.
     * @return A new extent.
     */
    [[nodiscard]] constexpr friend extent3 operator+(extent3 const& lhs, extent3 const& rhs) noexcept
    {
        return extent3{lhs._v + rhs._v};
    }

    /** Subtract two extents from each other.
     * @param lhs The first extent.
     * @param rhs The second extent.
     * @return A new extent.
     */
    [[nodiscard]] constexpr friend extent3 operator-(extent3 const& lhs, extent3 const& rhs) noexcept
    {
        return extent3{lhs._v - rhs._v};
    }

    /** Scale the extent by a scaler.
     * @param lhs The extent to scale.
     * @param rhs The scaling factor.
     * @return The scaled extent.
     */
    [[nodiscard]] constexpr friend extent3 operator*(extent3 const& lhs, float const& rhs) noexcept
    {
        return extent3{lhs._v * rhs};
    }

    [[nodiscard]] constexpr friend extent3 operator+(extent3 const& lhs, vector2 const& rhs) noexcept
    {
        return extent3{static_cast<array_type>(lhs) + static_cast<array_type>(rhs)};
    }

    [[nodiscard]] constexpr friend extent3 operator+(extent3 const& lhs, vector3 const& rhs) noexcept
    {
        return extent3{static_cast<array_type>(lhs) + static_cast<array_type>(rhs)};
    }

    [[nodiscard]] constexpr friend vector3 operator+(vector3 const& lhs, extent3 const& rhs) noexcept
    {
        return vector3{static_cast<array_type>(lhs) + static_cast<array_type>(rhs)};
    }

    /** Add a scaler to the extent.
     * @param lhs The extent to scale.
     * @param rhs The scaling factor.
     * @return The scaled extent.
     */
    [[nodiscard]] constexpr friend extent3 operator+(extent3 const& lhs, float const& rhs) noexcept
    {
        auto r = extent3{};
        for (std::size_t i = 0; i != 3; ++i) {
            r._v[i] = lhs._v[i] + rhs;
        }

        return r;
    }

    /** Scale the extent by a scaler.
     * @param lhs The extent to scale.
     * @param rhs The scaling factor.
     * @return The scaled extent.
     */
    [[nodiscard]] constexpr friend extent3 operator*(float const& lhs, extent3 const& rhs) noexcept
    {
        return extent3{lhs * rhs._v};
    }

    /** Compare if two extents are equal.
     * @param lhs The first extent.
     * @param rhs The second extent.
     * @return True if both extents are completely equal to each other.
     */
    [[nodiscard]] constexpr friend bool operator==(extent3 const& lhs, extent3 const& rhs) noexcept
    {
        return equal(lhs._v, rhs._v);
    }

    [[nodiscard]] constexpr friend std::partial_ordering operator<=>(extent3 const& lhs, extent3 const& rhs) noexcept
    {
        constexpr std::size_t mask = 0b0111;

        hilet equal = (lhs._v == rhs._v).mask() & mask;
        if (equal == mask) {
            // Only equivalent if all elements are equal.
            return std::partial_ordering::equivalent;
        }

        hilet less = (lhs._v < rhs._v).mask() & mask;
        if ((less | equal) == mask) {
            // If one or more elements is less (but none are greater) then the ordering is less.
            return std::partial_ordering::less;
        }

        hilet greater = (lhs._v < rhs._v).mask() & mask;
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
    [[nodiscard]] hi_force_inline constexpr friend float squared_hypot(extent3 const& rhs) noexcept
    {
        return squared_hypot<0b0111>(rhs._v);
    }

    /** Get the length of the extent.
     * @param rhs The extent.
     * @return The length of the extent.
     */
    [[nodiscard]] friend float hypot(extent3 const& rhs) noexcept
    {
        return hypot<0b0111>(rhs._v);
    }

    /** Get the length of the extent.
     * @param rhs The extent.
     * @return One over the length of the extent.
     */
    [[nodiscard]] constexpr friend float rcp_hypot(extent3 const& rhs) noexcept
    {
        return rcp_hypot<0b0111>(rhs._v);
    }

    /** Normalize a extent to a unit extent.
     * @param rhs The extent.
     * @return A extent with the same direction as the given extent, but its length is 1.0.
     */
    [[nodiscard]] constexpr friend extent3 normalize(extent3 const& rhs) noexcept
    {
        return extent3{normalize<0b0111>(rhs._v)};
    }

    [[nodiscard]] constexpr friend extent3 ceil(extent3 const& rhs) noexcept
    {
        return extent3{ceil(array_type{rhs})};
    }

    [[nodiscard]] constexpr friend extent3 floor(extent3 const& rhs) noexcept
    {
        return extent3{floor(static_cast<array_type>(rhs))};
    }

    [[nodiscard]] constexpr friend extent3 round(extent3 const& rhs) noexcept
    {
        return extent3{round(static_cast<array_type>(rhs))};
    }

    [[nodiscard]] constexpr friend extent3 min(extent3 const& lhs, extent3 const& rhs) noexcept
    {
        return extent3{min(static_cast<array_type>(lhs), static_cast<array_type>(rhs))};
    }

    [[nodiscard]] constexpr friend extent3 max(extent3 const& lhs, extent3 const& rhs) noexcept
    {
        return extent3{max(static_cast<array_type>(lhs), static_cast<array_type>(rhs))};
    }

    [[nodiscard]] constexpr friend extent3 clamp(extent3 const& value, extent3 const& min, extent3 const& max) noexcept
    {
        return extent3{clamp(static_cast<array_type>(value), static_cast<array_type>(min), static_cast<array_type>(max))};
    }

    /** Check if the extent is valid.
     * Extends must be positive.
     * This function will check if w is zero, and with 2D extent is z is zero.
     */
    [[nodiscard]] constexpr bool holds_invariant() const noexcept
    {
        return _v.x() >= 0.0f and _v.y() >= 0.0f and _v.z() >= 0.0f and _v.w() == 0.0f;
    }

    [[nodiscard]] friend std::string to_string(extent3 const& rhs) noexcept
    {
        return std::format("[{}, {}, {}]", rhs._v.x(), rhs._v.y(), rhs._v.z());
    }

    friend std::ostream& operator<<(std::ostream& lhs, extent3 const& rhs) noexcept
    {
        return lhs << to_string(rhs);
    }

private:
    array_type _v;
};

}} // namespace hi::v1

// XXX #617 MSVC bug does not handle partial specialization in modules.
export template<>
struct std::formatter<hi::extent3, char> {
    auto parse(auto& pc)
    {
        return pc.end();
    }

    auto format(hi::extent3 const& t, auto& fc) const
    {
        return std::vformat_to(fc.out(), "[{}, {}, {}]", std::make_format_args(t.width(), t.height(), t.depth()));
    }
};
