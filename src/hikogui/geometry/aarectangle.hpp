// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file geometry/aarectangle.hpp
 * @ingroup geometry
 */

#pragma once

#include "alignment.hpp"
#include "extent2.hpp"
#include "point2.hpp"
#include "point3.hpp"
#include "margins.hpp"
#include "../utility/utility.hpp"
#include "../concurrency/concurrency.hpp"
#include "../concurrency/unfair_mutex.hpp" // XXX #616
#include "../concurrency/thread.hpp" // XXX #616
#include "../macros.hpp"
#include <hikocpu/hikocpu.hpp>
#include <concepts>
#include <mutex>
#include <exception>
#include <compare>

hi_export_module(hikogui.geometry : aarectangle);

hi_export namespace hi {
inline namespace v1 {

/** Class which represents an axis-aligned rectangle.
 * @ingroup geometry
 */
class aarectangle {
public:
    using array_type = simd<float, 4>;
    using value_type = array_type::value_type;

    constexpr aarectangle() noexcept : v() {}
    constexpr aarectangle(aarectangle const& rhs) noexcept = default;
    constexpr aarectangle& operator=(aarectangle const& rhs) noexcept = default;
    constexpr aarectangle(aarectangle&& rhs) noexcept = default;
    constexpr aarectangle& operator=(aarectangle&& rhs) noexcept = default;

    /** Create a large axis aligned rectangle.
     */
    [[nodiscard]] constexpr static aarectangle large() noexcept
    {
        return {point2{-large_number_v<float>, -large_number_v<float>}, point2{large_number_v<float>, large_number_v<float>}};
    }

    constexpr explicit aarectangle(array_type const& other) noexcept : v(other)
    {
        hi_axiom(holds_invariant());
    }

    /** Create a box from the position and size.
     *
     * @param x The x location of the left-bottom corner of the box
     * @param y The y location of the left-bottom corner of the box
     * @param width The width of the box.
     * @param height The height of the box.
     */
    constexpr aarectangle(float x, float y, float width, float height) noexcept : v{x, y, x + width, y + height}
    {
        hi_axiom(holds_invariant());
    }

    /** Create a rectangle from the size.
     * The rectangle's left bottom corner is at the origin.
     * @param extent The size of the box.
     */
    constexpr explicit aarectangle(extent2 const& extent) noexcept : v(static_cast<array_type>(extent)._00xy())
    {
        hi_axiom(holds_invariant());
    }

    /** Create a rectangle from the left-bottom and right-top points.
     * @param p0 The left bottom point.
     * @param p3 The right opt point.
     */
    constexpr aarectangle(point2 const& p0, point2 const& p3) noexcept :
        v(static_cast<array_type>(p0).xy00() + static_cast<array_type>(p3)._00xy())
    {
        hi_axiom(p0.holds_invariant());
        hi_axiom(p3.holds_invariant());
        hi_axiom(holds_invariant());
    }

    /** Create a rectangle from the size.
     *
     * The rectangle's left bottom corner is at the origin.
     *
     * @param p0 The left-bottom point where the rectangle starts.
     * @param extent The size of the rectangle.
     */
    constexpr aarectangle(point2 const& p0, extent2 const& extent) noexcept :
        v(static_cast<array_type>(p0).xyxy() + static_cast<array_type>(extent)._00xy())
    {
        hi_axiom(holds_invariant());
    }

    constexpr explicit operator array_type() const noexcept
    {
        return v;
    }

    /** Make sure p0 is left/bottom from p3.
     * @return True is p0 is left and below p3.
     */
    [[nodiscard]] constexpr bool holds_invariant() const noexcept
    {
        return (v <= v.zwzw()).mask() == 0b1111;
    }

    /** Check if the rectangle has no area.
     */
    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return (v == v.zwxy()).mask() == 0b1111;
    }

    /** True when the rectangle has an area.
     */
    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return not empty();
    }

    /** Expand the current rectangle to include the new rectangle.
     * This is mostly used for extending bounding a bounding box.
     *
     * @param rhs The new rectangle to include in the current rectangle.
     */
    constexpr aarectangle& operator|=(aarectangle const& rhs) noexcept
    {
        return *this = *this | rhs;
    }

    /** Expand the current rectangle to include the new rectangle.
     * This is mostly used for extending bounding a bounding box.
     *
     * @param rhs A new point to include in the current rectangle.
     */
    constexpr aarectangle& operator|=(point2 const& rhs) noexcept
    {
        return *this = *this | rhs;
    }

    [[nodiscard]] constexpr point2 operator[](std::size_t i) const noexcept
    {
        switch (i) {
        case 0:
            return point2{v.xy01()};
        case 1:
            return point2{v.zy01()};
        case 2:
            return point2{v.xw01()};
        case 3:
            return point2{v.zw01()};
        default:
            hi_no_default();
        }
    }

    template<int I>
    [[nodiscard]] constexpr friend point2 get(aarectangle const& rhs) noexcept
    {
        if constexpr (I == 0) {
            return point2{rhs.v.xy01()};
        } else if constexpr (I == 1) {
            return point2{rhs.v.zy01()};
        } else if constexpr (I == 2) {
            return point2{rhs.v.xw01()};
        } else if constexpr (I == 3) {
            return point2{rhs.v.zw01()};
        } else {
            hi_static_no_default();
        }
    }

    /** Get size of the rectangle
     *
     * @return The (x, y) vector representing the width and height of the rectangle.
     */
    [[nodiscard]] constexpr extent2 size() const noexcept
    {
        return extent2{v.zwzw() - v};
    }

    [[nodiscard]] constexpr value_type x() const noexcept
    {
        return v.x();
    }

    [[nodiscard]] constexpr value_type y() const noexcept
    {
        return v.y();
    }

    [[nodiscard]] constexpr value_type width() const noexcept
    {
        return (v.zwzw() - v).x();
    }

    [[nodiscard]] constexpr value_type height() const noexcept
    {
        return (v.zwzw() - v).y();
    }

    [[nodiscard]] constexpr value_type bottom() const noexcept
    {
        return v.y();
    }

    [[nodiscard]] constexpr value_type top() const noexcept
    {
        return v.w();
    }

    [[nodiscard]] constexpr value_type left() const noexcept
    {
        return v.x();
    }

    [[nodiscard]] constexpr value_type right() const noexcept
    {
        return v.z();
    }

    /** The middle on the y-axis between bottom and top.
     */
    [[nodiscard]] constexpr value_type middle() const noexcept
    {
        return (bottom() + top()) / value_type{2};
    }

    /** The center on the x-axis between left and right.
     */
    [[nodiscard]] constexpr value_type center() const noexcept
    {
        return (left() + right()) / value_type{2};
    }

    /** Get the center of the rectangle.
     */
    [[nodiscard]] constexpr friend point2 midpoint(aarectangle const& rhs) noexcept
    {
        return midpoint(get<0>(rhs), get<3>(rhs));
    }

    constexpr aarectangle& set_width(value_type newWidth) noexcept
    {
        v = v.xyxw() + array_type{value_type{0}, value_type{0}, newWidth, value_type{0}};
        return *this;
    }

    constexpr aarectangle& set_height(value_type newHeight) noexcept
    {
        v = v.xyzy() + array_type{value_type{0}, value_type{0}, value_type{0}, newHeight};
        return *this;
    }

    /** Check if a 2D coordinate is inside the rectangle.
     *
     * @param rhs The coordinate of the point to test.
     */
    [[nodiscard]] constexpr bool contains(point2 const& rhs) const noexcept
    {
        // No need to check with empty due to half open range check.
        return (static_cast<array_type>(rhs).xyxy() >= v).mask() == 0b0011;
    }

    /** Check if a 3D coordinate is inside the rectangle.
     *
     * @param rhs The coordinate of the point to test. This point is
     *            converted to 2D by this function.
     */
    [[nodiscard]] constexpr bool contains(point3 const& rhs) const noexcept
    {
        return contains(point2{rhs});
    }

    /** Align a rectangle within another rectangle.
     * @param haystack The outside rectangle
     * @param needle The size of the rectangle to be aligned.
     * @param alignment How the inside rectangle should be aligned.
     * @return The needle rectangle repositioned and aligned inside the haystack.
     */
    [[nodiscard]] friend constexpr aarectangle align(aarectangle haystack, extent2 needle, alignment alignment) noexcept
    {
        auto x = value_type{0};
        if (alignment == horizontal_alignment::left) {
            x = haystack.left();

        } else if (alignment == horizontal_alignment::right) {
            x = haystack.right() - needle.width();

        } else if (alignment == horizontal_alignment::center) {
            x = haystack.center() - needle.width() / value_type{2};

        } else {
            hi_no_default();
        }

        auto y = value_type{0};
        if (alignment == vertical_alignment::bottom) {
            y = haystack.bottom();

        } else if (alignment == vertical_alignment::top) {
            y = haystack.top() - needle.height();

        } else if (alignment == vertical_alignment::middle) {
            y = haystack.middle() - needle.height() / value_type{2};

        } else {
            hi_no_default();
        }

        return {point2{x, y}, needle};
    }

    /** Align a rectangle within another rectangle.
     * @param haystack The outside rectangle
     * @param needle The inside rectangle; to be aligned.
     * @param alignment How the inside rectangle should be aligned.
     * @return The needle rectangle repositioned and aligned inside the haystack.
     */
    [[nodiscard]] friend constexpr aarectangle align(aarectangle haystack, aarectangle needle, alignment alignment) noexcept
    {
        return align(haystack, needle.size(), alignment);
    }

    /**
     * Aligns a rectangle to the middle of another rectangle based on the
     * specified horizontal alignment and middle position.
     *
     * The rectangle will not move beyond the bounds of the haystack.
     *
     * @param haystack The rectangle to align to.
     * @param needle The rectangle to align.
     * @param alignment The horizontal alignment.
     * @param middle The middle value.
     * @return The aligned rectangle.
     */
    [[nodiscard]] friend constexpr aarectangle
    align_to_middle(aarectangle haystack, extent2 needle, horizontal_alignment alignment, value_type middle) noexcept
    {
        assert(needle.width() <= haystack.width());
        assert(needle.height() <= haystack.height());
        
        auto const needle_half_height = needle.height() / value_type{2};
        auto const needle_half_width = needle.width() / value_type{2};

        auto const x = [&] {
            switch (alignment) {
            case horizontal_alignment::left:
            case horizontal_alignment::flush:
            case horizontal_alignment::justified:
                return haystack.left();
            case horizontal_alignment::right:
                return haystack.right() - needle.width();
            case horizontal_alignment::center:
                return haystack.center() - needle_half_width;
            }
            std::unreachable();
        }();

        middle = std::clamp(
            middle, haystack.bottom() + needle_half_height, haystack.top() - needle_half_height);

        auto const y = middle - needle_half_height;
        return {point2{x, y}, needle};
    }

    /** Need to call the hidden friend function from within another class.
     */
    [[nodiscard]] constexpr static aarectangle _align(aarectangle outside, aarectangle inside, alignment alignment) noexcept
    {
        return align(outside, inside, alignment);
    }

    [[nodiscard]] friend constexpr bool operator==(aarectangle const& lhs, aarectangle const& rhs) noexcept
    {
        return equal(lhs.v, rhs.v);
    }

    [[nodiscard]] friend constexpr bool overlaps(aarectangle const& lhs, aarectangle const& rhs) noexcept
    {
        if (lhs.empty() or rhs.empty()) {
            return false;
        }

        auto const rhs_swap = rhs.v.zwxy();

        // lhs.p0.x > rhs.p3.x | lhs.p0.y > rhs.p3.y
        if (((lhs.v > rhs_swap).mask() & 0b0011) != 0) {
            return false;
        }

        // lhs.p3.x < rhs.p0.x | lhs.p3.y < rhs.p0.y
        if (((lhs.v < rhs_swap).mask() & 0b1100) != 0) {
            return false;
        }

        return true;
    }

    [[nodiscard]] friend constexpr aarectangle operator|(aarectangle const& lhs, aarectangle const& rhs) noexcept
    {
        if (!lhs) {
            return rhs;
        } else if (!rhs) {
            return lhs;
        } else {
            return aarectangle{min(get<0>(lhs), get<0>(rhs)), max(get<3>(lhs), get<3>(rhs))};
        }
    }

    [[nodiscard]] friend constexpr aarectangle operator|(aarectangle const& lhs, point2 const& rhs) noexcept
    {
        if (!lhs) {
            return aarectangle{rhs, rhs};
        } else {
            return aarectangle{min(get<0>(lhs), rhs), max(get<3>(lhs), rhs)};
        }
    }

    /** Expand the rectangle for the same amount in all directions.
     * @param lhs The original rectangle.
     * @param rhs How much the width and height should be scaled by.
     * @return A new rectangle expanded on each side.
     */
    [[nodiscard]] friend constexpr aarectangle operator*(aarectangle const& lhs, value_type rhs) noexcept
    {
        auto const new_extent = lhs.size() * rhs;
        auto const diff = vector2{new_extent} - vector2{lhs.size()};
        auto const offset = diff * 0.5f;

        auto const p0 = get<0>(lhs) - offset;
        auto const p3 = max(get<3>(lhs) + offset, p0);
        return aarectangle{p0, p3};
    }

    [[nodiscard]] friend constexpr aarectangle operator+(aarectangle const& lhs, margins const& rhs) noexcept
    {
        return aarectangle{lhs.v + neg_mask<0b0011>(array_type{rhs})};
    }

    [[nodiscard]] friend constexpr aarectangle operator-(aarectangle const& lhs, margins const& rhs) noexcept
    {
        return aarectangle{lhs.v - neg_mask<0b0011>(array_type{rhs})};
    }

    friend constexpr aarectangle& operator+=(aarectangle& lhs, margins const& rhs) noexcept
    {
        return lhs = lhs + rhs;
    }

    friend constexpr aarectangle& operator-=(aarectangle& lhs, margins const& rhs) noexcept
    {
        return lhs = lhs - rhs;
    }

    /** Expand the rectangle for the same amount in all directions.
     * @param lhs The original rectangle.
     * @param rhs How much should be added on each side of the rectangle,
     *            this value may be zero or negative.
     * @return A new rectangle expanded on each side.
     */
    [[nodiscard]] friend constexpr aarectangle operator+(aarectangle const& lhs, value_type rhs) noexcept
    {
        return lhs + margins{rhs};
    }

    /** Shrink the rectangle for the same amount in all directions.
     * @param lhs The original rectangle.
     * @param rhs How much should be added on each side of the rectangle,
     *            this value may be zero or negative.
     * @return A new rectangle shrank on each side.
     */
    [[nodiscard]] friend constexpr aarectangle operator-(aarectangle const& lhs, value_type rhs) noexcept
    {
        return lhs - margins{rhs};
    }

    friend constexpr aarectangle& operator+=(aarectangle& lhs, value_type rhs) noexcept
    {
        return lhs = lhs + rhs;
    }

    friend constexpr aarectangle& operator-=(aarectangle& lhs, value_type rhs) noexcept
    {
        return lhs = lhs - rhs;
    }

    [[nodiscard]] friend constexpr aarectangle round(aarectangle const& rhs) noexcept
    {
        auto const p0 = round(get<0>(rhs));
        auto const size = round(rhs.size());
        return aarectangle{p0, size};
    }

    /** Round rectangle by expanding to pixel edge.
     */
    [[nodiscard]] friend constexpr aarectangle ceil(aarectangle const& rhs) noexcept
    {
        auto const p0 = floor(get<0>(rhs));
        auto const p3 = ceil(get<3>(rhs));
        return aarectangle{p0, p3};
    }

    /** Round rectangle by expanding to a certain granularity.
     */
    [[nodiscard]] friend constexpr aarectangle ceil(aarectangle const& lhs, extent2 const& rhs) noexcept
    {
        auto const p0 = floor(get<0>(lhs), rhs);
        auto const p3 = ceil(get<3>(lhs), rhs);
        return aarectangle{p0, p3};
    }

    /** Round rectangle by shrinking to pixel edge.
     */
    [[nodiscard]] friend constexpr aarectangle floor(aarectangle const& rhs) noexcept
    {
        auto const p0 = ceil(get<0>(rhs));
        auto const p3 = floor(get<3>(rhs));
        return aarectangle{p0, p3};
    }

    [[nodiscard]] friend constexpr aarectangle bounding_rectangle(aarectangle const& rhs) noexcept
    {
        return rhs;
    }

    /** Return the overlapping part of two rectangles.
     * When the rectangles are not overlapping, the width and height are zero.
     */
    [[nodiscard]] friend constexpr aarectangle intersect(aarectangle const& lhs, aarectangle const& rhs) noexcept
    {
        auto const p0 = max(get<0>(lhs), get<0>(rhs));
        auto const p3 = min(get<3>(lhs), get<3>(rhs));
        if (p0.x() < p3.x() && p0.y() < p3.y()) {
            return {p0, p3};
        } else {
            return {};
        }
    }

    [[nodiscard]] friend value_type distance(aarectangle const& lhs, point2 const& rhs) noexcept
    {
        auto const lhs_ = static_cast<array_type>(lhs);
        auto const rhs_ = static_cast<array_type>(rhs);
        // Only (x,y) of subsequent calculations are valid, (z,w) have garbage values.
        auto const closest_point = max(min(rhs_, lhs_.zwzw()), lhs_);
        auto const v_closest_point = closest_point - rhs_;
        return hypot<0b0011>(v_closest_point).x();
    }

private:
    /** Intrinsic of the rectangle.
     * Elements are assigned as follows:
     *  - (x, y) 2D-coordinate of left-bottom corner of the rectangle
     *  - (z, w) 2D-coordinate of right-top corner of the rectangle
     */
    array_type v;
};

} // namespace v1
} // namespace hi::v1

template<>
class std::atomic<hi::aarectangle> {
public:
    using value_type = hi::aarectangle;
    constexpr static bool is_always_lock_free = false;

    constexpr atomic() noexcept = default;
    atomic(atomic const&) = delete;
    atomic(atomic&&) = delete;
    atomic& operator=(atomic const&) = delete;
    atomic& operator=(atomic&&) = delete;

    constexpr atomic(value_type const& rhs) noexcept : _value(rhs) {}
    atomic& operator=(value_type const& rhs) noexcept
    {
        store(rhs);
        return *this;
    }

    operator value_type() const noexcept
    {
        return load();
    }

    [[nodiscard]] bool is_lock_free() const noexcept
    {
        return is_always_lock_free;
    }

    void store(value_type desired, std::memory_order = std::memory_order_seq_cst) noexcept
    {
        auto const lock = std::scoped_lock(_mutex);
        _value = desired;
    }

    value_type load(std::memory_order = std::memory_order_seq_cst) const noexcept
    {
        auto const lock = std::scoped_lock(_mutex);
        return _value;
    }

    value_type exchange(value_type desired, std::memory_order = std::memory_order_seq_cst) noexcept
    {
        auto const lock = std::scoped_lock(_mutex);
        return std::exchange(_value, desired);
    }

    bool compare_exchange_weak(value_type& expected, value_type desired, std::memory_order, std::memory_order) noexcept
    {
        auto const lock = std::scoped_lock(_mutex);
        if (_value == expected) {
            _value = desired;
            return true;
        } else {
            expected = _value;
            return false;
        }
    }

    bool compare_exchange_strong(
        value_type& expected,
        value_type desired,
        std::memory_order success,
        std::memory_order failure) noexcept
    {
        return compare_exchange_weak(expected, desired, success, failure);
    }

    bool
    compare_exchange_weak(value_type& expected, value_type desired, std::memory_order order = std::memory_order_seq_cst) noexcept
    {
        return compare_exchange_weak(expected, desired, order, order);
    }

    bool compare_exchange_strong(
        value_type& expected,
        value_type desired,
        std::memory_order order = std::memory_order_seq_cst) noexcept
    {
        return compare_exchange_strong(expected, desired, order, order);
    }

    value_type fetch_or(value_type arg, std::memory_order = std::memory_order_seq_cst) noexcept
    {
        auto const lock = std::scoped_lock(_mutex);
        auto tmp = _value;
        _value = tmp | arg;
        return tmp;
    }

    value_type operator|=(value_type arg) noexcept
    {
        auto const lock = std::scoped_lock(_mutex);
        return _value |= arg;
    }

private:
    value_type _value;
    mutable hi::unfair_mutex _mutex;
};

// XXX #617 MSVC bug does not handle partial specialization in modules.
hi_export template<>
struct std::formatter<hi::aarectangle, char> : std::formatter<std::string, char> {
    auto format(hi::aarectangle const& t, auto& fc) const
    {
        return std::formatter<std::string, char>::format(std::format("{}:{}", get<0>(t), t.size()), fc);
    }
};
