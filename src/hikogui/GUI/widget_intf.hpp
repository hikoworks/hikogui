// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "hitbox.hpp"
#include "widget_layout.hpp"
#include "widget_id.hpp"
#include "widget_state.hpp"
#include "keyboard_focus_group.hpp"
#include "../layout/layout.hpp"
#include "../GFX/GFX.hpp"
#include "../telemetry/telemetry.hpp"
#include "../theme/theme.hpp"
#include "../macros.hpp"
#include <coroutine>

hi_export_module(hikogui.GUI : widget_intf);

hi_export namespace hi {
inline namespace v1 {
class gui_window;

class widget_intf {
public:
    /** The style of this widget.
     *
     * You can assign a style-string to this style variable to change
     * the style's id, class and individual style-attributes.
     * @see hi::parse_style().
     */
    hi::style style = {};

    /** Notifier which is called after an action is completed by a widget.
     */
    notifier<void()> notifier;

    virtual ~widget_intf()
    {
        release_widget_id(_id);
    }

    widget_intf() noexcept : _id(make_widget_id())
    {
        ++global_counter<"widget:construct">;

        _style_cbt = style.subscribe([&](style_modify_mask mask, bool restyle) {
            if (restyle) {
                ++global_counter<"widget:style:path">;
                request_restyle();
            }

            if (to_bool(mask & style_modify_mask::layout)) {
                // The layout has changed which means its size may have changed
                // which would require a re-layout and re-constrain of the widget.
                ++global_counter<"widget:style:reconstrain">;
                request_reconstrain();

            } else if (to_bool(mask & style_modify_mask::redraw)) {
                // The color attributes, or border magnitude has changed which
                // should only change the widget visually and not the layout.
                ++global_counter<"widget:style:redraw">;
                request_redraw();
            }
        });
        this->set_phase(widget_phase::enabled);
    }

    /** The numeric identifier of a widget.
     *
     * @note This is a uint32_t equal to the operating system's accessibility identifier.
     */
    [[nodiscard]] virtual widget_id id() const noexcept
    {
        return _id;
    }

    /** Pointer to the parent widget.
     *
     * May be a nullptr only when this is the top level widget, or when
     * the widget is removed from its parent.
     */
    [[nodiscard]] widget_intf* parent() const noexcept
    {
        return _parent;
    }

    /** Set the parent widget.
     *
     * @param new_parent A pointer to an existing parent, or nullptr if the
     *                   widget is removed from the parent.
     */
    virtual void set_parent(widget_intf* new_parent) noexcept
    {
        _parent = new_parent;

        if (new_parent) {
            set_window(new_parent->window());
        } else {
            set_window(nullptr);
        }
    }

    [[nodiscard]] gui_window* window() const noexcept
    {
        return _window;
    }

    virtual void set_window(gui_window* new_window) noexcept
    {
        _window = new_window;
        request_restyle();

        for (auto& child : all_children()) {
            child.set_window(new_window);
        }
    }

    /** Send a event to the window.
     *
     * @param event The event to send to the window.
     * @return True when the event was handled by the window.
     */
    bool send_to_window(gui_event const& event) const noexcept;

    /** Request the window to restyle all the widgets.
     */
    void request_restyle() const noexcept;

    /** Request the window to resize based on the preferred size of the widgets.
     */
    void request_resize() const noexcept;

    /** Request the window to reconstrain all the widgets.
     */
    void request_reconstrain() const noexcept;

    /** Request the window to relayout all the widgets.
     */
    void request_relayout() const noexcept;

    /** Request the window to redraw the area used by the widget.
     */
    void request_redraw() const noexcept;

    /** Request the window to be fully redrawn.
     *
     * Use this function when a widget has changed in such a way that it
     * affects the window outside the area of the widget.
     */
    void request_redraw_window() const noexcept;

    /** Subscribe a callback to be called when an action is completed by the widget.
     */
    template<forward_of<void()> Func>
    [[nodiscard]] callback<void()> subscribe(Func&& func, callback_flags flags = callback_flags::synchronous) noexcept
    {
        return notifier.subscribe(std::forward<Func>(func), flags);
    }

    /** Await until an action is completed by the widget.
     */
    [[nodiscard]] auto operator co_await() const noexcept
    {
        return notifier.operator co_await();
    }

    [[nodiscard]] size_t layer() const noexcept
    {
        return _layer;
    }

    void set_layer(size_t new_layer) noexcept
    {
        _layer = new_layer;
    }

    [[nodiscard]] virtual widget_phase phase() const noexcept
    {
        auto const pc = style.pseudo_class();
        if ((pc & style_pseudo_class::phase_mask) == style_pseudo_class::disabled) {
            return widget_phase::disabled;
        } else if ((pc & style_pseudo_class::phase_mask) == style_pseudo_class::enabled) {
            return widget_phase::enabled;
        } else if ((pc & style_pseudo_class::phase_mask) == style_pseudo_class::hover) {
            return widget_phase::hover;
        } else if ((pc & style_pseudo_class::phase_mask) == style_pseudo_class::active) {
            return widget_phase::active;
        } else {
            std::unreachable();
        }
    }

    virtual void set_phase(widget_phase const& phase) noexcept
    {
        auto pc = style.pseudo_class();
        pc &= ~style_pseudo_class::phase_mask;
        pc |= [&] {
            switch (phase) {
            case widget_phase::disabled: return style_pseudo_class::disabled;
            case widget_phase::enabled: return style_pseudo_class::enabled;
            case widget_phase::hover: return style_pseudo_class::hover;
            case widget_phase::active: return style_pseudo_class::active;
            default: std::unreachable();
            }
        }();
        style.set_pseudo_class(pc);
    }

    [[nodiscard]] virtual bool disabled() const noexcept
    {
        return phase() == widget_phase::disabled;
    }

    [[nodiscard]] virtual bool enabled() const noexcept
    {
        return phase() != widget_phase::disabled;
    }

    [[nodiscard]] virtual bool hover() const noexcept
    {
        return phase() == widget_phase::hover or phase() == widget_phase::active;
    }

    [[nodiscard]] virtual bool active() const noexcept
    {
        return phase() == widget_phase::active;
    }

    void set_enabled(bool value) noexcept
    {
        set_phase(value ? widget_phase::enabled : widget_phase::disabled);
    }

    void set_hover(bool value) noexcept
    {
        if (phase() != widget_phase::disabled) {
            set_phase(value ? widget_phase::hover : widget_phase::enabled);
        }
    }

    void set_active(bool value) noexcept
    {
        if (phase() != widget_phase::disabled) {
            set_phase(value ? widget_phase::active : widget_phase::enabled);
        }
    }

    [[nodiscard]] virtual bool focus() const noexcept
    {
        return (style.pseudo_class() & style_pseudo_class::focus) != style_pseudo_class{};
    }

    virtual void set_focus(bool new_focus) noexcept
    {
        auto pc = style.pseudo_class();
        pc &= ~style_pseudo_class::focus;
        if (new_focus) {
            pc |= style_pseudo_class::focus;
        }
        style.set_pseudo_class(pc);
    }

    [[nodiscard]] virtual bool checked() const noexcept
    {
        return (style.pseudo_class() & style_pseudo_class::checked) != style_pseudo_class{};
    }

    virtual void set_checked(bool new_checked) noexcept
    {
        auto pc = style.pseudo_class();
        pc &= ~style_pseudo_class::checked;
        if (new_checked) {
            pc |= style_pseudo_class::checked;
        }
        style.set_pseudo_class(pc);
    }

    [[nodiscard]] virtual bool front() const noexcept
    {
        return (style.pseudo_class() & style_pseudo_class::front) != style_pseudo_class{};
    }

    virtual void set_front(bool new_front) noexcept
    {
        auto pc = style.pseudo_class();
        pc &= ~style_pseudo_class::front;
        if (new_front) {
            pc |= style_pseudo_class::front;
        }
        style.set_pseudo_class(pc);
    }

    /** Get a list of child widgets.
     */
    [[nodiscard]] virtual generator<widget_intf&> children(bool include_invisible) const noexcept
    {
        co_return;
    }

    [[nodiscard]] generator<widget_intf&> all_children() const noexcept
    {
        return children(true);
    }

    [[nodiscard]] generator<widget_intf&> visible_children() const noexcept
    {
        return children(false);
    }

    /** Restyle the widgets and its children.
     *
     * @param pixel_density The pixel density to use for the restyle.
     * @param path The path of the parent widget.
     * @param properties The properties used when this widget will inherit style properties.
     */
    virtual void restyle(
        unit::pixel_density pixel_density,
        style_path const& path = style_path{},
        style::properties_array_type const& properties = style::properties_array_type{}) noexcept
    {
        auto const [child_path, child_properties] = style.restyle(pixel_density, path, properties);

        for (auto& child : all_children()) {
            child.restyle(pixel_density, child_path, child_properties);
        }
    }

    /** Get the height of a widget.
     *
     * This by default returns the style's height of the widget.
     *
     * @param minimum When false this function returns the preferred height, when
     *                true this function returns the minimum height.
     * @return The height in pixels.
     */
    [[nodiscard]] virtual unit::pixels_f height(bool minimum) const
    {
        assert(std::holds_alternative<unit::pixels_f>(style.height);
        return std::get<unit::pixels_f>(style.height);
    }

    /** Get the minimum width for a certain height.
     *
     * The height for widgets in a row are calculated first and may be
     * larger than the minimum and preferred height of the widget itself.
     * This allows a widget to be more precise with its width.
     *
     * @param height The height to calculate the minimum width for.
     * @return The width in pixels.
     */
    [[nodiscard]] virtual unit::pixels_f width(unit::pixels_f height) const
    {
        assert(std::holds_alternative<unit::pixels_f>(style.width));
        return std::get<unit::pixels_f>(style.width);
    }

    /** The default baseline for this widget at a certain height.
     *
     * The default baseline is based on the vertical alignment and cap-height
     * of the text.
     *
     * @note May be overriden when more accurate alignment is required, or
     *       when embedding widgets.
     * @param height The height of the widget during layout.
     * @return The location of the baseline.
     */
    [[nodiscard]] virtual unit::pixels_f baseline(unit::pixels_f height) const
    {
        switch (style.vertical_alignment) {
        case vertical_alignment::top:
            return height - style.cap_height;
        case vertical_alignment::middle:
            return height * 0.5f - style.cap_height * 0.5f;
        case vertical_alignment::bottom:
            return unit::pixels(0.0f);
        }
        std::unreachable();
    }

    /** The priority for selecting baselines between widgets in a row.
     *
     * @return baseline priority, higher value is higher priority.
     */
    [[nodiscard]] virtual int baseline_priority() const
    {
        return style.baseline_priority;
    }

    /** The margins around this widget.
     *
     * @return The margins that needs to be put around a widget.
     *         Some widgets like labels may actually draw into their margin,
     *         so this is important.
     */
    [[nodiscard]] virtual hi::margins margins() const
    {
        return style.margins;
    }

    /** The weight for extra height distribution.
     *
     * @return The weight used to distribute extra height.
     * @retval 0.0 This widget should not get extra height distributed to it.
     */
    [[nodiscard]] virtual float height_weight() const
    {
        return style.height_weight;
    }

    /** The weight for extra width distribution.
     *
     * @return The weight used to distribute extra width.
     * @retval 0.0 This widget should not get extra width distributed to it.
     */
    [[nodiscard]] virtual float width_weight() const
    {
        return style.width_weight;
    }

    /** Update the internal layout of the widget.
     * This function is called when the size of this widget must change, or if any of the
     * widget request a re-layout.
     *
     * This function may be used for expensive calculations, such as geometry calculations,
     * which should only be done when the data or sizes change; it should cache these calculations.
     *
     * @post This function will change what is returned by `widget::size()` and the transformation
     *       matrices.
     * @param context The layout for this child.
     */
    virtual void set_layout(widget_layout const& context) noexcept
    {
        _layout = context;
    }

    /** Get the current layout for this widget.
     */
    [[nodiscard]] widget_layout const& layout() const noexcept
    {
        return _layout;
    }

    /** Draw the widget.
     *
     * This function is called by the window (optionally) on every frame.
     * It should recursively call this function on every visible child.
     * This function is only called when `updateLayout()` has returned true.
     *
     * The overriding function should call the base class's `draw()`, the place
     * where the call this function will determine the order of the vertices into
     * each buffer. This is important when needing to do the painters algorithm
     * for alpha-compositing. However the pipelines are always drawn in the same
     * order.
     *
     * @param context The context to where the widget will draw.
     */
    virtual void draw(draw_context const& context) const noexcept
    {
        for (auto const &child : visible_children()) {
            child.draw(context);
        }
    }

    /** Find the widget that is under the mouse cursor.
     * This function will recursively test with visual child widgets, when
     * widgets overlap on the screen the hitbox object with the highest elevation is returned.
     *
     * @param position The coordinate of the mouse local to the widget.
     * @return A hit_box object with the cursor-type and a reference to the widget.
     */
    [[nodiscard]] virtual hitbox hitbox_test(point2 position) const noexcept
    {
        return {};
    }

    /** Call hitbox_test from a parent widget.
     *
     * This function will transform the position from parent coordinates to local coordinates.
     *
     * @param position The coordinate of the mouse local to the parent widget.
     */
    [[nodiscard]] virtual hitbox hitbox_test_from_parent(point2 position) const noexcept
    {
        return hitbox_test(layout().from_parent * position);
    }

    /** Call hitbox_test from a parent widget.
     *
     * This function will transform the position from parent coordinates to local coordinates.
     *
     * @param position The coordinate of the mouse local to the parent widget.
     * @param sibling_hitbox The hitbox of a sibling to combine with the hitbox of this widget.
     */
    [[nodiscard]] virtual hitbox hitbox_test_from_parent(point2 position, hitbox sibling_hitbox) const noexcept
    {
        return std::max(sibling_hitbox, hitbox_test(layout().from_parent * position));
    }

    /** Check if the widget will accept keyboard focus.
     *
     */
    [[nodiscard]] virtual bool accepts_keyboard_focus(keyboard_focus_group group) const noexcept = 0;

    /** Handle command.
     * If a widget does not fully handle a command it should pass the
     * command to the super class' `handle_event()`.
     */
    virtual bool handle_event(gui_event const& event) noexcept = 0;

    /** Handle command recursive.
     * Handle a command and pass it to each child.
     *
     * @param event The command to handle by this widget.
     * @param reject_list The widgets that should ignore this command
     * @return True when the command was handled by this widget or recursed child.
     */
    virtual bool handle_event_recursive(
        gui_event const& event,
        std::vector<widget_id> const& reject_list = std::vector<widget_id>{}) noexcept = 0;

    /** Find the next widget that handles keyboard focus.
     * This recursively looks for the current keyboard widget, then returns the
     * next (or previous) widget that returns true from
     * `accepts_keyboard_focus()`.
     *
     * @param current_keyboard_widget The widget that currently has focus; or
     *        nullptr to get the first widget that accepts focus.
     * @param group The group to which the widget must belong.
     * @param direction The direction in which to walk the widget tree.
     * @return A pointer to the next widget.
     * @retval current_keyboard_widget When current_keyboard_widget was found
     *         but no next widget that accepts keyboard focus was found.
     * @retval 0 When current_keyboard_widget is not found in this widget.
     */
    [[nodiscard]] virtual widget_id find_next_widget(
        widget_id current_keyboard_widget,
        keyboard_focus_group group,
        keyboard_focus_direction direction) const noexcept = 0;

    /** Get a list of parents of a given widget.
     * The chain includes the given widget.
     */
    [[nodiscard]] std::vector<widget_id> parent_chain() const noexcept
    {
        std::vector<widget_id> chain;

        if (auto w = this) {
            chain.push_back(w->id());
            while ((w = w->parent())) {
                chain.push_back(w->id());
            }
        }

        return chain;
    }

    /** Scroll to show the given rectangle on the window.
     * This will call parents, until all parents have scrolled
     * the rectangle to be shown on the window.
     *
     * @param rectangle The rectangle in window coordinates.
     */
    virtual void scroll_to_show(hi::aarectangle rectangle) noexcept = 0;

    /** Scroll to show the important part of the widget.
     */
    void scroll_to_show() noexcept
    {
        scroll_to_show(layout().rectangle());
    }

private:
    widget_id _id = {};
    widget_intf* _parent = nullptr;
    gui_window* _window = nullptr;

    size_t _layer = 0;

    hi::style::callback_type _style_cbt;

    widget_layout _layout;
};

inline widget_intf* get_if(widget_intf* start, widget_id id, bool include_invisible) noexcept
{
    hi_assert_not_null(start);

    if (start->id() == id) {
        return start;
    }
    for (auto& child : start->children(include_invisible)) {
        if (auto const r = get_if(&child, id, include_invisible); r != nullptr) {
            return r;
        }
    }
    return nullptr;
}

inline widget_intf& get(widget_intf& start, widget_id id, bool include_invisible)
{
    if (auto r = get_if(std::addressof(start), id, include_invisible); r != nullptr) {
        return *r;
    }
    throw not_found_error("get widget by id");
}

template<std::invocable<widget_intf&> Func>
inline void apply(widget_intf& start, Func&& func, bool include_invisible = true)
{
    auto todo = std::vector<widget_intf*>{&start};

    while (not todo.empty()) {
        auto* tmp = todo.back();
        todo.pop_back();

        func(*tmp);

        for (auto& child : tmp->children(include_invisible)) {
            todo.push_back(&child);
        }
    }
}

} // namespace v1
} // namespace hi
