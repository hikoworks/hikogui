

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

    constexpr embed &set_elevation_offset(float offset) noexcept
    {
        _elevation_offset = offset;
        return *this;
    }

    template<typename F>
    [[nodiscard]] constexpr constraints get_constraints(F const& f) noexcept
        requires std::is_invocable_r_v<constraints, F>
    {
        _child_constraints = f();

        auto const child_padding = max(_padding, _child_constraints.margins);

        auto r = _child_constraints;
        r.width += child_padding.horizontal();
        r.height += child_padding.vertical();
        r.margins = _margins;

        switch (r.vertical_alignment) {
        case vertical_alignment::top:
            r.baseline_offset += -child_padding.top();
            break;
        case vertical_alignment::middle:
            r.baseline_offset += (-child_padding.top() + child_padding.bottom()) * 0.5f;
            break;
        case vertical_alignment::bottom:
            r.baseline_offset += child_padding.bottom();
            break;
        }

        // The parent will likely have a higher priority than the child, when
        // it is embedding the child with extra padding. This will make low
        // priority labels in neighboring cells align with the baseline of
        // a larger widget with a label in a different position.
        if (_priority) {
            r.baseline_priority = *_priority;
        }
        return r;
    }

    template<typename F>
    constexpr void set_layout(layout::shape const& shape, F const& f) noexcept
        requires std::is_invocable_r_v<void, F, shape>
    {
        auto child_shape = layout::shape{};
        child_shape.rectangle = shape.rectangle - max(_padding, _child_constraints.margins);
        child_shape.clipping_rectangle = intersect(shape.clipping_rectangle, child_shape.rectangle + _child_constraints.margins);
        child_shape.elevation = shape.elevation + _elevation_offset;
        
        child_shape.baseline = [&] {
            if (shape.baseline_priority <= _child_constraints.baseline_priority) {
                switch (_child_constraints._vertical_alignment) {
                case vertical_alignment::top:
                    return child_shape.rectangle.top() + _child_constraints.baseline_offset;
                case vertical_alignment::middle:
                    return child_shape.rectangle.middle() + _child_constraints.baseline_offset;
                case vertical_alignment::bottom:
                    return child_shape.rectangle.bottom() + _child_constraints.baseline_offset;
                }
            }
        }();

        return f(child_shape);
    }

private:
    hi::margins _margins = {};
    hi::margins _padding = {};
    std::optional<baseline_priority> _priority = {};
    float _elevation_offset = 0.0f;

    constraints _child_constraints;
    shape _child_shape;
};

}

