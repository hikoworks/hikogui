

#pragma once

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

    [[nodiscard]] constexpr constraints get_constraints(constraints const& child_constraints) noexcept
    {
        _child_constraints = child_constraints;

        auto const child_padding = max(_padding, _child_constraints._margins);

        auto r = constraints{};
        r._size = _child_constraints._size + child_padding.size();
        r._weight = _child_constraints._weight;
        r._margins = _margins;
        r._baseline = hi::baseline::embed(child_constraints.baseline(), child_padding);
        if (_priority) {
            r._baseline.set_priority(_priority);
        }
        return r;
    }

    template<typename F>
    constexpr void set_layout(shape const& shape, F const& f) noexcept
    {

    }


private:
    hi::margins _margins;
    hi::margins _padding;
    std::optional<baseline_priority> _priority;
    constraints _child_constraints;
};

}

