// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file GUI/gui_event.hpp Definition of GUI event types.
 * @ingroup GUI
 */

#pragma once

#include "widget_id.hpp"
#include "gui_event_type.hpp"
#include "gui_event_variant.hpp"
#include "keyboard_virtual_key_intf.hpp"
#include "keyboard_state.hpp"
#include "keyboard_modifiers.hpp"
#include "keyboard_focus_group.hpp"
#include "keyboard_focus_direction.hpp"
#include "mouse_buttons.hpp"
#include "hitbox.hpp"
#include "../unicode/unicode.hpp"
#include "../geometry/geometry.hpp"
#include "../time/time.hpp"
#include "../macros.hpp"
#include <chrono>
#include <memory>

hi_export_module(hikogui.GUI : gui_event);

hi_export namespace hi { inline namespace v1 {

/** Information for a mouse event.
 * @ingroup GUI
 */
struct mouse_event_data {
    /** Information about which widget is underneath the mouse pointer.
     */ 
    hi::hitbox hitbox = {};

    /** The current position of the mouse pointer.
     *
     * @note The event system will convert these in widget-local coordinates.
     */
    point2 position = {};

    /** The position the last time a button was pressed.
     *
     * This can be used as the position at the start of a drag event.
     *
     * @note The event system will convert these in widget-local coordinates.
     */
    point2 down_position = {};

    /** Change in wheel rotation, in points (pt).
     *
     * Some mice have two dimensional mouse wheels.
     */
    vector2 wheel_delta = {};

    /** Buttons which have caused this event.
     */
    mouse_buttons cause = {};

    /** Buttons that are also held down.
     */
    mouse_buttons down = {};

    /** Number of clicks from the last button clicked.
     */
    uint8_t click_count = 0;
};

struct keyboard_target_data {
    hi::widget_id widget_id = {};
    keyboard_focus_group group = keyboard_focus_group::normal;
    keyboard_focus_direction direction = keyboard_focus_direction::here;
};

/** A user interface event.
 * @ingroup GUI
 */
class gui_event {
public:
    /** The time when the event was created.
     */
    utc_nanoseconds time_point;

    /** Keyboard modifiers: shift, ctrl, alt, etc.
     *
     * This may be used for in combination with both keyboard and mouse events.
     */
    keyboard_modifiers keyboard_modifiers;

    /** State of the keyboard; caps-lock, num-lock, scroll-lock.
     */
    keyboard_state keyboard_state;

    /** Create a GUI event.
     *
     * @param type The type of the event.
     * @param time_point The time when the was received.
     * @param keyboard_modifiers A list of modifiers key that where hold down: alt, ctrl, shift.
     * @param keyboard_state The state of the keyboard: scroll-lock, num-lock, caps-lock.
     */
    constexpr gui_event(
        gui_event_type type,
        utc_nanoseconds time_point,
        hi::keyboard_modifiers keyboard_modifiers,
        hi::keyboard_state keyboard_state) noexcept :
        _type(gui_event_type::none),
        time_point(time_point),
        keyboard_modifiers(keyboard_modifiers),
        keyboard_state(keyboard_state)
    {
        set_type(type);
    }

    /** Create an empty GUI event.
     */
    constexpr gui_event() noexcept :
        gui_event(gui_event_type::none, utc_nanoseconds{}, keyboard_modifiers::none, keyboard_state::idle)
    {
    }

    /** Create am empty GUI event.
     *
     * @param type The type of the event.
     */
    gui_event(gui_event_type type) noexcept :
        gui_event(type, std::chrono::utc_clock::now(), keyboard_modifiers::none, keyboard_state::idle)
    {
    }

    /** Create a GUI event.
     *
     * @param type The type of the key event.
     * @param key The virtual key that was pressed/released
     * @param keyboard_modifiers A list of modifiers key that where hold down: alt, ctrl, shift.
     * @param keyboard_state The state of the keyboard: scroll-lock, num-lock, caps-lock.
     */
    gui_event(
        gui_event_type type,
        keyboard_virtual_key key,
        hi::keyboard_modifiers keyboard_modifiers = keyboard_modifiers::none,
        hi::keyboard_state keyboard_state = keyboard_state::idle) noexcept :
        gui_event(type, std::chrono::utc_clock::now(), keyboard_modifiers, keyboard_state)
    {
        hi_assert(variant() == gui_event_variant::keyboard);
        this->key() = key;
    }

    constexpr ~gui_event() = default;
    constexpr gui_event(gui_event const&) noexcept = default;
    constexpr gui_event(gui_event&&) noexcept = default;
    constexpr gui_event& operator=(gui_event const&) noexcept = default;
    constexpr gui_event& operator=(gui_event&&) noexcept = default;

    /** Create a mouse enter event.
     *
     * @param position The position where the mouse entered.
     */
    [[nodiscard]] static gui_event make_mouse_enter(point2 position) noexcept
    {
        auto r = gui_event{gui_event_type::mouse_enter};
        r.mouse().position = position;
        return r;
    }

    [[nodiscard]] static gui_event keyboard_grapheme(hi::grapheme grapheme) noexcept
    {
        auto r = gui_event{gui_event_type::keyboard_grapheme};
        r.grapheme() = grapheme;
        return r;
    }

    [[nodiscard]] static gui_event keyboard_partial_grapheme(hi::grapheme grapheme) noexcept
    {
        auto r = gui_event{gui_event_type::keyboard_partial_grapheme};
        r.grapheme() = grapheme;
        return r;
    }

    [[nodiscard]] static gui_event window_set_keyboard_target(
        widget_id id,
        keyboard_focus_group group = keyboard_focus_group::normal,
        keyboard_focus_direction direction = keyboard_focus_direction::here) noexcept
    {
        auto r = gui_event{gui_event_type::window_set_keyboard_target};
        r.keyboard_target().widget_id = id;
        r.keyboard_target().group = group;
        r.keyboard_target().direction = direction;
        return r;
    }

    /** Create clipboard event.
     *
     * @param type Either `gui_event_type::text_edit_paste` or `gui_event_type::window_set_clipboard`.
     * @param text The clipboard data in text form.
     */
    [[nodiscard]] static gui_event make_clipboard_event(gui_event_type type, gstring_view text) noexcept
    {
        auto r = gui_event{type};
        r.clipboard_data() = text;
        return r;
    }

    /** Get the event type.
     */
    [[nodiscard]] constexpr gui_event_type type() const noexcept
    {
        return _type;
    }

    /** Change the type of the gui_event.
     *
     * @note If the variant changes of this event the associated data is cleared.
     * @param type The new type for the gui_event.
     */
    constexpr void set_type(gui_event_type type) noexcept
    {
        auto const previous_variant = variant();

        _type = type;
        if (previous_variant != variant()) {
            switch (variant()) {
            case gui_event_variant::mouse:
                _data = mouse_event_data{};
                break;
            case gui_event_variant::grapheme:
                _data = hi::grapheme{};
                break;
            case gui_event_variant::keyboard:
                _data = keyboard_virtual_key{};
                break;
            case gui_event_variant::keyboard_target:
                _data = keyboard_target_data{};
                break;
            case gui_event_variant::clipboard_data:
                _data = gstring{};
            default:;
            }
        }
    }

    /** Get the mouse event information.
     *
     * @return a referene to the mouse data.
     */
    [[nodiscard]] mouse_event_data& mouse() noexcept
    {
        hi_assert(variant() == gui_event_variant::mouse);
        return std::get<mouse_event_data>(_data);
    }

    /** Get the mouse event information.
     *
     * @return a referene to the mouse data.
     */
    [[nodiscard]] mouse_event_data const& mouse() const noexcept
    {
        hi_assert(variant() == gui_event_variant::mouse);
        return std::get<mouse_event_data>(_data);
    }

    /** Get the key from the keyboard event
     *
     * @return a referene to the key.
     */
    [[nodiscard]] keyboard_virtual_key& key() noexcept
    {
        hi_assert(variant() == gui_event_variant::keyboard);
        return std::get<keyboard_virtual_key>(_data);
    }

    /** Get the key from the keyboard event
     *
     * @return a referene to the key.
     */
    [[nodiscard]] keyboard_virtual_key const& key() const noexcept
    {
        hi_assert(variant() == gui_event_variant::keyboard);
        return std::get<keyboard_virtual_key>(_data);
    }

    /** Get the grapheme entered on the keyboard.
     *
     * @return a referene to the grapheme.
     */
    [[nodiscard]] hi::grapheme& grapheme() noexcept
    {
        hi_assert(variant() == gui_event_variant::grapheme);
        return std::get<hi::grapheme>(_data);
    }

    /** Get the grapheme entered on the keyboard.
     *
     * @return a referene to the grapheme.
     */
    [[nodiscard]] hi::grapheme const& grapheme() const noexcept
    {
        hi_assert(variant() == gui_event_variant::grapheme);
        return std::get<hi::grapheme>(_data);
    }

    [[nodiscard]] keyboard_target_data& keyboard_target() noexcept
    {
        hi_assert(variant() == gui_event_variant::keyboard_target);
        return std::get<keyboard_target_data>(_data);
    }

    [[nodiscard]] keyboard_target_data const& keyboard_target() const noexcept
    {
        hi_assert(variant() == gui_event_variant::keyboard_target);
        return std::get<keyboard_target_data>(_data);
    }

    [[nodiscard]] gstring& clipboard_data() noexcept
    {
        hi_assert(variant() == gui_event_variant::clipboard_data);
        return std::get<gstring>(_data);
    }

    [[nodiscard]] gstring const& clipboard_data() const noexcept
    {
        hi_assert(variant() == gui_event_variant::clipboard_data);
        return std::get<gstring>(_data);
    }

    [[nodiscard]] constexpr bool operator==(gui_event_type event_type) const noexcept
    {
        return type() == event_type;
    }

    [[nodiscard]] constexpr bool operator==(gui_event_variant event_variant) const noexcept
    {
        return variant() == event_variant;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return type() == gui_event_type::none;
    }

    constexpr operator bool() const noexcept
    {
        return not empty();
    }

    [[nodiscard]] constexpr gui_event_variant variant() const noexcept
    {
        return to_gui_event_variant(type());
    }

    /** Check if this event is for a left-button-up event while the mouse pointer is in the given area.
     */
    [[nodiscard]] constexpr bool is_left_button_up(aarectangle active_area) const noexcept
    {
        using enum gui_event_type;
        return type() == mouse_up and mouse().cause.left_button and active_area.contains(mouse().position);
    }

    /** Get the location of the mouse relative to the start of a drag.
     */
    [[nodiscard]] constexpr vector2 drag_delta() const noexcept
    {
        using enum gui_event_type;
        return type() == mouse_drag ? mouse().position - mouse().down_position : vector2{};
    }

    /** Transform a gui-event to another coordinate system.
     *
     * This operations is used mostly to transform mouse evens to a widget's local coordinate system.
     *
     * @param transform The transform object
     * @param rhs The event to transform.
     * @return The transformed event.
     */
    [[nodiscard]] constexpr friend gui_event operator*(translate2 const& transform, gui_event const& rhs) noexcept
    {
        auto r = rhs;
        if (rhs == gui_event_variant::mouse) {
            r.mouse().position = transform * rhs.mouse().position;
            r.mouse().down_position = transform * rhs.mouse().down_position;
        }
        return r;
    }

private:
    using data_type =
        std::variant<mouse_event_data, keyboard_virtual_key, keyboard_target_data, hi::grapheme, gstring>;

    gui_event_type _type;
    data_type _data;
};

}} // namespace hi::v1

// XXX #617 MSVC bug does not handle partial specialization in modules.
hi_export template<>
struct std::formatter<hi::gui_event, char> : std::formatter<std::string_view, char> {
    auto format(hi::gui_event const& t, auto& fc) const
    {
        return std::formatter<std::string_view, char>::format(hi::gui_event_type_metadata[t.type()], fc);
    }
};
