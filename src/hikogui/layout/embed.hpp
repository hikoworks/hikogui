

#pragma once

#include "constraints.hpp"
#include "shape.hpp"
#include "../units/units.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"

hi_export_module(hikogui.layout : embed);

hi_export namespace hi::inline v1::layout {

/** Embed a single widget.
 */
class embed {
public:
    constexpr embed() noexcept = default;

    constexpr embed &set_margin(hi::margins margins) noexcept
    {
        _margins = margins;
        return *this;
    }

    constexpr embed &set_padding(hi::margins padding) noexcept
    {
        _padding = padding;
        return *this;
    }

    constexpr embed &set_baseline_priority(std::optional<baseline_priority> priority) noexcept
    {
        _priority = priority;
        return *this;
    }

    template<typename F>
    [[nodiscard]] constexpr constraints get_constraints(F const& f) noexcept
        requires std::is_invocable_r_v<constraints, F>
    {
        _child_constraints = f();

        auto const child_left_padding = max(_left_padding, _child_constraints.left);
        auto const child_right_padding = max(_right_padding, _child_constraints.right);
        auto const child_top_padding = max(_top_padding, _child_constraints.top);
        auto const child_bottom_padding = max(_bottom_padding, _child_constraints.bottom);

        auto r = _child_constraints;
        r.width += child_left_padding + child_right_padding;
        r.height += child_top_padding + child_bottom_padding;
        r.left = _left_margin;
        r.right = _right_margin;
        r.top = _top_margin;
        r.bottom = _bottom_margin;

        switch (r._vertical_alignment) {
        case vertical_alignment::top:
            r.baseline_offset += -child_top_padding;
            break;
        case vertical_alignment::middle:
            r.baseline_offset += (-child_top_padding + child_bottom_padding) * 0.5f;
            break;
        case vertical_alignment::bottom:
            r.baseline_offset += child_bottom_padding;
            break;
        }

        if (_priority) {
            r.baseline_priority = *_priority;
        }
        return r;
    }

    template<typename F>
    constexpr void set_layout(shape const& shape, F const& f) noexcept
    {

    }


private:
    unit::pixels_f _left_padding = {};
    unit::pixels_f _right_padding = {};
    unit::pixels_f _top_padding = {};
    unit::pixels_f _bottom_padding = {};
    unit::pixels_f _left_margin = {};
    unit::pixels_f _right_margin = {};
    unit::pixels_f _top_margin = {};
    unit::pixels_f _bottom_margin = {};

    constraints _child_constraints;
};

}

