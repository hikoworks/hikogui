// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../macros.hpp"
#include "../geometry/geometry.hpp"
#include "../units/units.hpp"
#include <functional>
#include <utility>

hi_export_module(hikogui.text : baseline);

hi_export namespace hi::inline v1::layout {
/**
 * @brief Enumeration representing the priority levels for baselines.
 *
 * The baseline_priority enumeration defines the priority levels for baselines.
 * Each priority level represents a different alignment preference for widgets
 * when negotiating the baseline position.
 */
enum class baseline_priority : unsigned int {
    none = 0, //< No priority.
    label = 1, //< Priority for labels.
    small_widget = 10, //< Priority for small widgets.
    large_widget = 100, //< Priority for large widgets.
};

/** The negotiated baseline between multiple widgets with different alignments.
 *
 * This is used when multiple widgets are side by side, for example when they
 * are located in a row of a grid. Each widget will have the same height, but
 * the height will not yet be known when the negotiation starts.
 *
 * Once the negotiation is complete, a single baseline-offset will be calculated
 * for the entire row of widgets; the offset is the distance from the bottom of
 * the row to the baseline.
 * 
 * A widget may ignore the baseline if it is unable to draw within the height
 * of the row. In this case the widget will use its natural baseline.
 *
 */
class baseline {
public:
    constexpr baseline() noexcept = default;
    constexpr baseline(baseline const&) noexcept = default;
    constexpr baseline(baseline&&) noexcept = default;
    constexpr baseline& operator=(baseline const&) noexcept = default;
    constexpr baseline& operator=(baseline&&) noexcept = default;

    /** Construct a baseline.
     *
     * @param priority The priority of the baseline.
     * @param offset The offset of the baseline from the bottom of the row.
     * @param alignment The alignment of the baseline.
     */
    constexpr baseline(baseline_priority priority, float offset, vertical_alignment alignment) noexcept :
        _priority(priority), _offset(offset), _alignment(alignment)
    {
    }

    /** Embed a child baseline into the current baseline.
     *
     * @param child The child baseline.
     * @param padding The padding around the child baseline. The padding should
     *                include the margins of the child widget.
     * @return The new baseline.
     */
    [[nodiscard]] constexpr static baseline embed(baseline const& child, margins padding) noexcept
    {
        auto r = child;

        switch (child._alignment) {
        case vertical_alignment::top:
            r._offset -= padding.top();
            break;
        case vertical_alignment::middle:
            r._offset -= padding.top() * 0.5f;
            r._offset += padding.bottom() * 0.5f;
            break;
        case vertical_alignment::bottom:
            r._offset += padding.bottom();
            break;
        }
        return r;
    }

    /** Embed a child baseline into the current baseline.
     *
     * @param child The child baseline.
     * @param padding The padding around the child baseline. The padding should
     *                include the margins of the child widget.
     * @param new_priority The new priority of the baseline.
     * @return The new baseline.
     */
    [[nodiscard]] constexpr static baseline embed(baseline const& child, margins padding, baseline_priority new_priority) noexcept
    {
        auto r = embed(child, padding);
        r._priority = new_priority;
        return r;
    }

    /** Return the baseline with the highest priority.
     *
     * If the priorities are equal and the alignments are equal, the offset is:
     * - top: The minimum of the two offsets.
     * - middle: The average of the two offsets.
     * - bottom: The maximum of the two offsets.
     * 
     * If the priorities are equal and the alignments are not equal, the first
     * baseline will be returned.
     *
     * @param a The first baseline.
     * @param b The second baseline.
     * @return The baseline with the highest priority.
     */
    [[nodiscard]] constexpr friend baseline max(baseline const& a, baseline const& b) noexcept
    {
        if (a._priority > b._priority) {
            return a;
        } else if (a._priority < b._priority) {
            return b;
        } else if (a._alignment == b._alignment) {
            switch (a._alignment) {
            case vertical_alignment::top:
                return baseline{a._priority, std::min(a._offset, b._offset), a._alignment};
            case vertical_alignment::middle:
                return baseline{a._priority, (a._offset + b._offset) * 0.5f, a._alignment};
            case vertical_alignment::bottom:
                return baseline{a._priority, std::max(a._offset, b._offset), a._alignment};
            }
        } else {
            return a;
        }
    }

    /** Calculate the offset of the baseline from the bottom of the row.
     * 
     * @param height The height of the row.
     * @return The offset of the baseline from the bottom of the row.
     */
    [[nodiscard]] constexpr float offset(float height) const
    {
        auto const offset = [&] {
            switch (_alignment) {
            case vertical_alignment::top:
                return std::round(height + _offset);
            case vertical_alignment::middle:
                return std::round(height * 0.5f + _offset);
            case vertical_alignment::bottom:
                return std::round(_offset);
            }
        }();

        assert(offset >= 0.0f);
        assert(offset <= height);
        return offset;
    }

    [[nodiscard]] constexpr baseline_priority priority() const noexcept
    {
        return _priority;
    }

    constexpr baseline &set_priority(baseline_priority priority) noexcept
    {
        _priority = priority;
        return *this;
    }

    [[nodiscard]] constexpr vertical_alignment alignment() const noexcept
    {
        return _alignment;
    }

private:
    float _offset = 0.0f;
    vertical_alignment _alignment = vertical_alignment::top;
    baseline_priority _priority = baseline_priority::none;
};

}
