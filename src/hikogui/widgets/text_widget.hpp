// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/text_widget.hpp Defines text_widget.
 * @ingroup widgets
 */

#pragma once

#include "widget.hpp"
#include "text_delegate.hpp"
#include "../GUI/GUI.hpp"
#include "../text/text.hpp"
#include "../geometry/geometry.hpp"
#include "../l10n/l10n.hpp"
#include "../container/container.hpp"
#include "../observer/observer.hpp"
#include "../macros.hpp"
#include <concepts>
#include <memory>
#include <string>
#include <array>
#include <optional>
#include <future>
#include <limits>
#include <chrono>

hi_export_module(hikogui.widgets.text_widget);

hi_export namespace hi {
inline namespace v1 {

/**
 * Enumeration representing the edit mode of a text widget.
 *
 * The edit mode determines the behavior and capabilities of the text widget when it comes to editing and selecting text.
 */
enum class text_widget_edit_mode {
    label = 0, //< The text widget is used as a label and does not allow editing or selecting text.
    selectable = 1, //< The text widget allows selecting text but does not allow editing.
    line_edit = 2, //< The text widget allows editing a single line of text.
    full_edit = 3 //< The text widget allows editing multiple lines of text.
};

/** A text widget.
 *
 * The text widget is a widget for displaying, selecting and editing text.
 *
 * On its own it can be used to edit multiple lines of text, but it will probably
 * be used embedded inside other widgets, like:
 *  - `label_widget` to display translated text together with an optional icon.
 *  - `text_field_widget` to edit a value of diffent types, including integers, floating point, strings, etc.
 *
 * Features:
 *  - Multiple paragraphs.
 *  - Uses the unicode line break algorithm to wrap lines when not enough horizontal space.
 *  - Used the unicode word break algorithm for selecting and moving through words.
 *  - Uses the unicode sentence break algorithm for selecting and moving through sentences.
 *  - Uses the unicode bidi algorithm for displaying text in mixed left-to-right & right-to-left languages.
 *  - Displays secondary cursor where text in the other language-direction will be inserted.
 *  - Keeps track if the user has just worked in left-to-right or right-to-left language.
 *  - Arrow keys move the cursor visually through the text
 *  - Handles insertion and overwrite mode; showing a caret or box cursor.
 *  - When entering dead-key on the keyboard the dead-key character is displayed underneath a secondary
 *    overwrite cursor.
 *  - Cut, Copy & Paste.
 *  - Undo & Redo.
 *
 * @ingroup widgets
 */
class text_widget : public widget {
public:
    using super = widget;
    using delegate_type = text_delegate;

    std::shared_ptr<delegate_type> delegate;

    template<typename... Args>
    [[nodiscard]] static std::shared_ptr<delegate_type> make_default_delegate(Args&&... args)
    {
        return make_shared_ctad<default_text_delegate>(std::forward<Args>(args)...);
    }

    ~text_widget()
    {
        hi_assert_not_null(delegate);
        delegate->deinit(this);
    }

    /** Construct a text widget.
     *
     * @param parent The owner of this widget.
     * @param delegate The delegate to use to control the widget's data.
     */
    template<std::derived_from<delegate_type> Delegate>
    text_widget(std::shared_ptr<Delegate> delegate) noexcept : super(), delegate(std::move(delegate))
    {
        hi_assert_not_null(this->delegate);

        _delegate_cbt = this->delegate->subscribe(this, [this] {
            _text = this->delegate->get_text(this);
            _selection.resize(_text.size());

            ++global_counter<"text_widget:delegate:reconstrain">;
            request_reconstrain();
        });

        _cursor_state_cbt = _cursor_state.subscribe([&](auto...) {
            ++global_counter<"text_widget:cursor_state:redraw">;
            request_redraw();
        });

        // If the text_widget is used as a label the blink_cursor() co-routine
        // is only waiting on `model` and `focus`, so this is cheap.
        _blink_cursor = blink_cursor();

        this->delegate->init(this);

        style.set_name("text");
        _delegate_cbt();
    }

    /** Construct a text widget.
     *
     * @param parent The owner of this widget.
     * @param text The text to be displayed.
     * @param attributes A set of attributes used to configure the text widget: a `alignment`.
     */
    template<typename... Args>
    text_widget(Args&&... args) noexcept : text_widget(make_default_delegate(std::forward<Args>(args)...))
    {
    }

    [[nodiscard]] text_widget_edit_mode edit_mode() const noexcept
    {
        return _edit_mode;
    }

    void set_edit_mode(text_widget_edit_mode value) noexcept
    {
        _edit_mode = value;
    }

    [[nodiscard]] bool selectable() const noexcept
    {
        return edit_mode() >= text_widget_edit_mode::selectable;
    }

    [[nodiscard]] bool line_edit() const noexcept
    {
        return edit_mode() >= text_widget_edit_mode::line_edit;
    }

    [[nodiscard]] bool full_edit() const noexcept
    {
        return edit_mode() >= text_widget_edit_mode::full_edit;
    }

    /// @privatesection
    [[nodiscard]] box_constraints update_constraints() noexcept override
    {
        assert(window() != nullptr);

        auto const maximum_width = [&] {
            if (auto pixel_width = std::get_if<unit::pixels_f>(&style.width)) {
                return *pixel_width;
            } else {
                return style.pixel_density * au::milli(au::meters)(210.0f);
            }
        }();

        // Read the latest text from the delegate.
        _phase1 = shaper_phase1(_text, style.font_size, style.text_style);

        auto maximum_width = shaper_get_natural_width(_phase1);
        auto maximum_height = shaper_get_natural_height(_phase1);
        auto preferred_width = maximum_width;
        auto preferred_height = maximum_height;
        auto minimum_width = maximum_width;
        auto minimum_height = maximum_height;

        if (maximum_width > style.width) {
            preferred_width = style.width;
            preferred_height = shaper_get_maximum_height(_phase1, maximum_width);
            // The preferred-height may become larger than the maximum-height.
            maximum_height = preferred_height;
        }

        if (maximum_height < style.height) {
            maximum_height = style.height;
            preferred_height = maximum_height;

            // XXX try different widths to find the smallest width that fits the
            // height.
            minimum_width = preferred_width;
            minimum_height = preferred_height;
        }

        return {
            extent2{minimum_width, minimum_height},
            extent2{preferred_width, preferred_height},
            extent2{maximum_width, maximum_height},
            style.margins_px,
            baseline{style.baseline_priority, _phase1.text_metrics.baseline_function}};
    }

    void set_layout(widget_layout const& context) noexcept override
    {
        super::set_layout(context);

        _phase2 = shaper_phase2(_phase1, unit::pixels(context.width()));

        _shaped_text.layout(context.rectangle(), context.get_baseline().in(unit::pixels), context.sub_pixel_size);
    }

    void draw(draw_context const& context) const noexcept override
    {
        using namespace std::literals::chrono_literals;

        // After potential reconstrain and relayout, updating the shaped-text, ask the parent window to scroll if needed.
        auto* mutable_this = const_cast<text_widget*>(this);

        if (std::exchange(mutable_this->_request_scroll, false)) {
            mutable_this->scroll_to_show_selection();
        }

        if (_last_drag_mouse_event) {
            if (_last_drag_mouse_event_next_repeat == utc_nanoseconds{}) {
                mutable_this->_last_drag_mouse_event_next_repeat =
                    context.display_time_point + os_settings::keyboard_repeat_delay();

            } else if (context.display_time_point >= _last_drag_mouse_event_next_repeat) {
                mutable_this->_last_drag_mouse_event_next_repeat =
                    context.display_time_point + os_settings::keyboard_repeat_interval();

                // The last drag mouse event was stored in window coordinate to compensate for scrolling, translate it
                // back to local coordinates before handling the mouse event again.
                auto new_mouse_event = _last_drag_mouse_event;
                new_mouse_event.mouse().position = layout().from_window * _last_drag_mouse_event.mouse().position;

                // When mouse is dragging a selection, start continues redraw and scroll parent views to display the selection.
                mutable_this->handle_event(new_mouse_event);
            }
            mutable_this->scroll_to_show_selection();
            ++global_counter<"text_widget:mouse_drag:redraw">;
            request_redraw();
        }

        if (overlaps(context, layout())) {
            context.draw_text(layout(), _shaped_text);

            context.draw_text_selection(layout(), _shaped_text, _selection, theme().text_select_color());

            if (*_cursor_state == cursor_state_type::on or *_cursor_state == cursor_state_type::busy) {
                context.draw_text_cursors(
                    layout(),
                    _shaped_text,
                    _selection.cursor(),
                    _overwrite_mode,
                    to_bool(_has_dead_character),
                    theme().primary_cursor_color(),
                    theme().secondary_cursor_color());
            }
        }

        return super::draw(context);
    }

    bool handle_event(gui_event const& event) noexcept override
    {
        hi_axiom(loop::main().on_thread());

        switch (event.type()) {
            using enum gui_event_type;

        case gui_widget_next:
        case gui_widget_prev:
        case keyboard_exit:
            // When the next widget is selected due to pressing the Tab key the text should be committed.
            // The `text_widget` does not handle gui_activate, so it will be forwarded to parent widgets,
            // such as `text_field_widget` which does.
            send_to_window(gui_event_type::gui_activate);
            return super::handle_event(event);

        case keyboard_grapheme:
            if (enabled() and edit_mode() >= text_widget_edit_mode::line_edit) {
                reset_state("BDX");
                add_character(event.grapheme(), add_type::append);
                return true;
            }
            break;

        case keyboard_partial_grapheme:
            if (enabled() and edit_mode() >= text_widget_edit_mode::line_edit) {
                reset_state("BDX");
                add_character(event.grapheme(), add_type::dead);
                return true;
            }
            break;

        case text_mode_insert:
            if (enabled() and edit_mode() >= text_widget_edit_mode::line_edit) {
                reset_state("BDX");
                _overwrite_mode = not _overwrite_mode;
                fix_cursor_position();
                return true;
            }
            break;

        case text_edit_paste:
            if (enabled()) {
                if (edit_mode() == text_widget_edit_mode::line_edit) {
                    reset_state("BDX");
                    auto tmp = event.clipboard_data();
                    // Replace all paragraph separators with white-space.
                    std::replace(tmp.begin(), tmp.end(), grapheme{unicode_PS}, grapheme{' '});
                    replace_selection(tmp);
                    return true;

                } else if (edit_mode() == text_widget_edit_mode::full_edit) {
                    reset_state("BDX");
                    replace_selection(event.clipboard_data());
                    return true;
                }
            }
            break;

        case text_edit_copy:
            if (enabled() and edit_mode() >= text_widget_edit_mode::selectable) {
                reset_state("BDX");
                if (auto const selected_text_ = selected_text(); not selected_text_.empty()) {
                    send_to_window(gui_event::make_clipboard_event(gui_event_type::window_set_clipboard, selected_text_));
                }
                return true;
            }
            break;

        case text_edit_cut:
            if (enabled() and edit_mode() >= text_widget_edit_mode::selectable) {
                reset_state("BDX");
                send_to_window(gui_event::make_clipboard_event(gui_event_type::window_set_clipboard, selected_text()));
                if (edit_mode() >= text_widget_edit_mode::line_edit) {
                    replace_selection(gstring{});
                }
                return true;
            }
            break;

        case text_undo:
            if (enabled() and edit_mode() >= text_widget_edit_mode::line_edit) {
                reset_state("BDX");
                undo();
                return true;
            }
            break;

        case text_redo:
            if (enabled() and edit_mode() >= text_widget_edit_mode::line_edit) {
                reset_state("BDX");
                redo();
                return true;
            }
            break;

        case text_insert_line:
            if (enabled() and edit_mode() >= text_widget_edit_mode::full_edit) {
                reset_state("BDX");
                add_character(grapheme{unicode_PS}, add_type::append);
                return true;
            }
            break;

        case text_insert_line_up:
            if (enabled() and edit_mode() >= text_widget_edit_mode::full_edit) {
                reset_state("BDX");
                _selection = _shaped_text.move_begin_paragraph(_selection.cursor());
                add_character(grapheme{unicode_PS}, add_type::insert);
                return true;
            }
            break;

        case text_insert_line_down:
            if (enabled() and edit_mode() >= text_widget_edit_mode::full_edit) {
                reset_state("BDX");
                _selection = _shaped_text.move_end_paragraph(_selection.cursor());
                add_character(grapheme{unicode_PS}, add_type::insert);
                return true;
            }
            break;

        case text_delete_char_next:
            if (enabled() and edit_mode() >= text_widget_edit_mode::line_edit) {
                reset_state("BDX");
                delete_character_next();
                return true;
            }
            break;

        case text_delete_char_prev:
            if (enabled() and edit_mode() >= text_widget_edit_mode::line_edit) {
                reset_state("BDX");
                delete_character_prev();
                return true;
            }
            break;

        case text_delete_word_next:
            if (enabled() and edit_mode() >= text_widget_edit_mode::line_edit) {
                reset_state("BDX");
                delete_word_next();
                return true;
            }
            break;

        case text_delete_word_prev:
            if (enabled() and edit_mode() >= text_widget_edit_mode::line_edit) {
                reset_state("BDX");
                delete_word_prev();
                return true;
            }
            break;

        case text_cursor_left_char:
            if (enabled() and edit_mode() >= text_widget_edit_mode::line_edit) {
                reset_state("BDX");
                _selection = _shaped_text.move_left_char(_selection.cursor(), _overwrite_mode);
                request_scroll();
                return true;
            }
            break;

        case text_cursor_right_char:
            if (enabled() and edit_mode() >= text_widget_edit_mode::line_edit) {
                reset_state("BDX");
                _selection = _shaped_text.move_right_char(_selection.cursor(), _overwrite_mode);
                request_scroll();
                return true;
            }
            break;

        case text_cursor_down_char:
            if (enabled() and edit_mode() >= text_widget_edit_mode::full_edit) {
                reset_state("BD");
                _selection = _shaped_text.move_down_char(_selection.cursor(), _vertical_movement_x);
                request_scroll();
                return true;
            }
            break;

        case text_cursor_up_char:
            if (enabled() and edit_mode() >= text_widget_edit_mode::full_edit) {
                reset_state("BD");
                _selection = _shaped_text.move_up_char(_selection.cursor(), _vertical_movement_x);
                request_scroll();
                return true;
            }
            break;

        case text_cursor_left_word:
            if (enabled() and edit_mode() >= text_widget_edit_mode::line_edit) {
                reset_state("BDX");
                _selection = _shaped_text.move_left_word(_selection.cursor(), _overwrite_mode);
                request_scroll();
                return true;
            }
            break;

        case text_cursor_right_word:
            if (enabled() and edit_mode() >= text_widget_edit_mode::line_edit) {
                reset_state("BDX");
                _selection = _shaped_text.move_right_word(_selection.cursor(), _overwrite_mode);
                request_scroll();
                return true;
            }
            break;

        case text_cursor_begin_line:
            if (enabled() and edit_mode() >= text_widget_edit_mode::line_edit) {
                reset_state("BDX");
                _selection = _shaped_text.move_begin_line(_selection.cursor());
                request_scroll();
                return true;
            }
            break;

        case text_cursor_end_line:
            if (enabled() and edit_mode() >= text_widget_edit_mode::line_edit) {
                reset_state("BDX");
                _selection = _shaped_text.move_end_line(_selection.cursor());
                request_scroll();
                return true;
            }
            break;

        case text_cursor_begin_sentence:
            if (enabled() and edit_mode() >= text_widget_edit_mode::line_edit) {
                reset_state("BDX");
                _selection = _shaped_text.move_begin_sentence(_selection.cursor());
                request_scroll();
                return true;
            }
            break;

        case text_cursor_end_sentence:
            if (enabled() and edit_mode() >= text_widget_edit_mode::line_edit) {
                reset_state("BDX");
                _selection = _shaped_text.move_end_sentence(_selection.cursor());
                request_scroll();
                return true;
            }
            break;

        case text_cursor_begin_document:
            if (enabled() and edit_mode() >= text_widget_edit_mode::line_edit) {
                reset_state("BDX");
                _selection = _shaped_text.move_begin_document(_selection.cursor());
                request_scroll();
                return true;
            }
            break;

        case text_cursor_end_document:
            if (enabled() and edit_mode() >= text_widget_edit_mode::line_edit) {
                reset_state("BDX");
                _selection = _shaped_text.move_end_document(_selection.cursor());
                request_scroll();
                return true;
            }
            break;

        case gui_cancel:
            if (enabled() and edit_mode() >= text_widget_edit_mode::selectable) {
                reset_state("BDX");
                _selection.clear_selection(_shaped_text.size());
                return true;
            }
            break;

        case text_select_left_char:
            if (enabled() and edit_mode() >= text_widget_edit_mode::selectable) {
                reset_state("BDX");
                _selection.drag_selection(_shaped_text.move_left_char(_selection.cursor(), false));
                request_scroll();
                return true;
            }
            break;

        case text_select_right_char:
            if (enabled() and edit_mode() >= text_widget_edit_mode::selectable) {
                reset_state("BDX");
                _selection.drag_selection(_shaped_text.move_right_char(_selection.cursor(), false));
                request_scroll();
                return true;
            }
            break;

        case text_select_down_char:
            if (enabled() and edit_mode() >= text_widget_edit_mode::selectable) {
                reset_state("BD");
                _selection.drag_selection(_shaped_text.move_down_char(_selection.cursor(), _vertical_movement_x));
                request_scroll();
                return true;
            }
            break;

        case text_select_up_char:
            if (enabled() and edit_mode() >= text_widget_edit_mode::selectable) {
                reset_state("BD");
                _selection.drag_selection(_shaped_text.move_up_char(_selection.cursor(), _vertical_movement_x));
                request_scroll();
                return true;
            }
            break;

        case text_select_left_word:
            if (enabled() and edit_mode() >= text_widget_edit_mode::selectable) {
                reset_state("BDX");
                _selection.drag_selection(_shaped_text.move_left_word(_selection.cursor(), false));
                request_scroll();
                return true;
            }
            break;

        case text_select_right_word:
            if (enabled() and edit_mode() >= text_widget_edit_mode::selectable) {
                reset_state("BDX");
                _selection.drag_selection(_shaped_text.move_right_word(_selection.cursor(), false));
                request_scroll();
                return true;
            }
            break;

        case text_select_begin_line:
            if (enabled() and edit_mode() >= text_widget_edit_mode::selectable) {
                reset_state("BDX");
                _selection.drag_selection(_shaped_text.move_begin_line(_selection.cursor()));
                request_scroll();
                return true;
            }
            break;

        case text_select_end_line:
            if (enabled() and edit_mode() >= text_widget_edit_mode::selectable) {
                reset_state("BDX");
                _selection.drag_selection(_shaped_text.move_end_line(_selection.cursor()));
                request_scroll();
                return true;
            }
            break;

        case text_select_begin_sentence:
            if (enabled() and edit_mode() >= text_widget_edit_mode::selectable) {
                reset_state("BDX");
                _selection.drag_selection(_shaped_text.move_begin_sentence(_selection.cursor()));
                request_scroll();
                return true;
            }
            break;

        case text_select_end_sentence:
            if (enabled() and edit_mode() >= text_widget_edit_mode::selectable) {
                reset_state("BDX");
                _selection.drag_selection(_shaped_text.move_end_sentence(_selection.cursor()));
                request_scroll();
                return true;
            }
            break;

        case text_select_begin_document:
            if (enabled() and edit_mode() >= text_widget_edit_mode::selectable) {
                reset_state("BDX");
                _selection.drag_selection(_shaped_text.move_begin_document(_selection.cursor()));
                request_scroll();
                return true;
            }
            break;

        case text_select_end_document:
            if (enabled() and edit_mode() >= text_widget_edit_mode::selectable) {
                reset_state("BDX");
                _selection.drag_selection(_shaped_text.move_end_document(_selection.cursor()));
                request_scroll();
                return true;
            }
            break;

        case text_select_document:
            if (enabled() and edit_mode() >= text_widget_edit_mode::selectable) {
                reset_state("BDX");
                _selection = _shaped_text.move_begin_document(_selection.cursor());
                _selection.drag_selection(_shaped_text.move_end_document(_selection.cursor()));
                request_scroll();
                return true;
            }
            break;

        case mouse_up:
            if (enabled() and edit_mode() >= text_widget_edit_mode::selectable) {
                // Stop the continues redrawing during dragging.
                // Also reset the time, so on drag-start it will initialize the time, which will
                // cause a smooth startup of repeating.
                _last_drag_mouse_event = {};
                _last_drag_mouse_event_next_repeat = {};
                return true;
            }
            break;

        case mouse_down:
            if (enabled() and edit_mode() >= text_widget_edit_mode::selectable) {
                auto const cursor = _shaped_text.get_nearest_cursor(event.mouse().position);
                switch (event.mouse().click_count) {
                case 1:
                    reset_state("BDX");
                    _selection = cursor;
                    break;
                case 2:
                    reset_state("BDX");
                    _selection.start_selection(cursor, _shaped_text.select_word(cursor));
                    break;
                case 3:
                    reset_state("BDX");
                    _selection.start_selection(cursor, _shaped_text.select_sentence(cursor));
                    break;
                case 4:
                    reset_state("BDX");
                    _selection.start_selection(cursor, _shaped_text.select_paragraph(cursor));
                    break;
                case 5:
                    reset_state("BDX");
                    _selection.start_selection(cursor, _shaped_text.select_document(cursor));
                    break;
                default:;
                }

                ++global_counter<"text_widget:mouse_down:relayout">;
                request_relayout();
                request_scroll();
                return true;
            }
            break;

        case mouse_drag:
            if (enabled() and edit_mode() >= text_widget_edit_mode::selectable) {
                auto const cursor = _shaped_text.get_nearest_cursor(event.mouse().position);
                switch (event.mouse().click_count) {
                case 1:
                    reset_state("BDX");
                    _selection.drag_selection(cursor);
                    break;
                case 2:
                    reset_state("BDX");
                    _selection.drag_selection(cursor, _shaped_text.select_word(cursor));
                    break;
                case 3:
                    reset_state("BDX");
                    _selection.drag_selection(cursor, _shaped_text.select_sentence(cursor));
                    break;
                case 4:
                    reset_state("BDX");
                    _selection.drag_selection(cursor, _shaped_text.select_paragraph(cursor));
                    break;
                default:;
                }

                // Drag events must be repeated, so that dragging is continues when it causes scrolling.
                // Normally mouse positions are kept in the local coordinate system, but scrolling
                // causes this coordinate system to shift, so translate it to the window coordinate system here.
                _last_drag_mouse_event = event;
                _last_drag_mouse_event.mouse().position = layout().to_window * event.mouse().position;
                ++global_counter<"text_widget:mouse_drag:redraw">;
                request_redraw();
                return true;
            }
            break;

        default:;
        }

        return super::handle_event(event);
    }

    hitbox hitbox_test(point2 position) const noexcept override
    {
        hi_axiom(loop::main().on_thread());

        if (layout().contains(position)) {
            if (enabled() and edit_mode() >= text_widget_edit_mode::line_edit) {
                return hitbox{id(), layout().elevation, hitbox_type::text_edit};

            } else if (enabled() and edit_mode() >= text_widget_edit_mode::selectable) {
                return hitbox{id(), layout().elevation, hitbox_type::_default};

            } else {
                return hitbox{};
            }
        } else {
            return hitbox{};
        }
    }

    [[nodiscard]] bool accepts_keyboard_focus(keyboard_focus_group group) const noexcept override
    {
        if (enabled() and edit_mode() >= text_widget_edit_mode::line_edit) {
            return to_bool(group & keyboard_focus_group::normal);
        } else if (enabled() and edit_mode() >= text_widget_edit_mode::selectable) {
            return to_bool(group & keyboard_focus_group::mouse);
        } else {
            return false;
        }
    }
    /// @endprivatesection
private:
    enum class add_type { append, insert, dead };

    struct undo_type {
        gstring text;
        text_selection selection;
    };

    enum class cursor_state_type { off, on, busy, none };

    gstring _text;

    shaper_phase1_result _phase1;
    shaper_phase2_result _phase2;

    std::vector<size_t> _line_lengths;
    std::vector<size_t> _display_order;

    hi::margins _margins;

    text_selection _selection;

    scoped_task<> _blink_cursor;

    observer<cursor_state_type> _cursor_state = cursor_state_type::none;

    text_widget_edit_mode _edit_mode = text_widget_edit_mode::selectable;

    /** After layout request scroll from the parent widgets.
     */
    bool _request_scroll = false;

    /** The last drag mouse event.
     *
     * This variable is used to repeatably execute the mouse event
     * even in absent of new mouse events. This must be done to get
     * continues scrolling to work during dragging.
     */
    gui_event _last_drag_mouse_event = {};

    /** When to cause the next mouse drag event repeat.
     */
    utc_nanoseconds _last_drag_mouse_event_next_repeat = {};

    /** The x-coordinate during vertical movement.
     */
    float _vertical_movement_x = std::numeric_limits<float>::quiet_NaN();

    bool _overwrite_mode = false;

    /** The text has a dead character.
     *
     * This variable has the following states:
     *  - std::nullopt: The text-widget is not in dead-char composition mode.
     *  - '\uffff': The text-widget is in dead-char composition, in insert-mode.
     *  - other: The text-widget is in dead-char composition, in overwrite and
     *           the grapheme value is the original character being replaced.
     *           So that is can be restored when cancelling composition.
     */
    std::optional<grapheme> _has_dead_character = std::nullopt;

    undo_stack<undo_type> _undo_stack = {1000};

    callback<void()> _delegate_cbt;
    callback<void(cursor_state_type)> _cursor_state_cbt;

    /** Make parent scroll views, scroll to show the current selection and cursor.
     */
    void scroll_to_show_selection() noexcept
    {
        if (focus()) {
            auto const cursor = _selection.cursor();
            auto const char_it = _shaped_text.begin() + cursor.index();
            if (char_it < _shaped_text.end()) {
                scroll_to_show(char_it->rectangle);
            }
        }
    }

    void request_scroll() noexcept
    {
        // At a minimum we need to request a redraw so that
        // `scroll_to_show_selection()` is called on the next frame.
        _request_scroll = true;
        ++global_counter<"text_widget:request_scroll:redraw">;
        request_redraw();
    }

    /** Reset states.
     *
     * Possible states:
     *  - 'X' x-coordinate for vertical movement.
     *  - 'D' Dead-character state.
     *  - 'B' Reset cursor blink time.
     *
     * @param states The individual states to reset.
     */
    void reset_state(char const* states) noexcept
    {
        hi_assert_not_null(states);

        while (*states != 0) {
            switch (*states) {
            case 'D':
                delete_dead_character();
                break;
            case 'X':
                _vertical_movement_x = std::numeric_limits<float>::quiet_NaN();
                break;
            case 'B':
                if (*_cursor_state == cursor_state_type::on or *_cursor_state == cursor_state_type::off) {
                    _cursor_state = cursor_state_type::busy;
                }
                break;
            default:
                hi_no_default();
            }
            ++states;
        }
    }

    [[nodiscard]] gstring_view selected_text() const noexcept
    {
        auto const [first, last] = _selection.selection_indices();

        return gstring_view{_phase1.text}.substr(first, last - first);
    }

    void undo_push() noexcept
    {
        _undo_stack.emplace(_phase1.text, _selection);
    }

    void undo() noexcept
    {
        if (_undo_stack.can_undo()) {
            auto const& [text, selection] = _undo_stack.undo(_phase1.text, _selection);

            delegate->set_text(this, text);
            _selection = selection;
        }
    }

    void redo() noexcept
    {
        if (_undo_stack.can_redo()) {
            auto const& [text, selection] = _undo_stack.redo();

            delegate->set_text(this, text);
            _selection = selection;
        }
    }

    scoped_task<> blink_cursor() noexcept
    {
        while (true) {
            if (enabled() and edit_mode() >= text_widget_edit_mode::line_edit) {
                switch (*_cursor_state) {
                case cursor_state_type::busy:
                    _cursor_state = cursor_state_type::on;
                    co_await when_any(os_settings::cursor_blink_delay(), *this);
                    break;

                case cursor_state_type::on:
                    _cursor_state = cursor_state_type::off;
                    co_await when_any(os_settings::cursor_blink_interval() / 2, *this);
                    break;

                case cursor_state_type::off:
                    _cursor_state = cursor_state_type::on;
                    co_await when_any(os_settings::cursor_blink_interval() / 2, *this);
                    break;

                default:
                    _cursor_state = cursor_state_type::busy;
                }

            } else {
                _cursor_state = cursor_state_type::none;
                co_await *this;
            }
        }
    }

    /** Fix the cursor position after cursor movement.
     */
    void fix_cursor_position() noexcept
    {
        auto const size = _phase1.text.size();
        if (_overwrite_mode and _selection.empty() and _selection.cursor().after()) {
            _selection = _selection.cursor().before_neighbor(size);
        }
        _selection.resize(size);
    }

    /** This function replaces the current selection with replacement text.
     */
    void replace_selection(gstring const& replacement) noexcept
    {
        undo_push();

        auto const [first, last] = _selection.selection_indices();

        auto text = _phase1.text;
        text.replace(first, last - first, replacement);
        delegate->set_text(this, text);

        _selection = text_cursor{first + replacement.size() - 1, true};
        fix_cursor_position();
    }

    /** Add a character to the text.
     *
     * @param c The character to add at the current position
     * @param mode The mode how to add a character.
     */
    void add_character(grapheme c, add_type keyboard_mode) noexcept
    {
        auto const [start_selection, end_selection] = _selection.selection(_phase1.text.size());
        auto original_grapheme = grapheme{char32_t{0xffff}};

        if (_selection.empty() and _overwrite_mode and start_selection.before()) {
            original_grapheme = _phase1.text[start_selection.index()];

            auto const [first, last] = _shaped_text.select_char(start_selection);
            _selection.drag_selection(last);
        }
        replace_selection(gstring{c});

        if (keyboard_mode == add_type::insert) {
            // The character was inserted, put the cursor back where it was.
            _selection = start_selection;

        } else if (keyboard_mode == add_type::dead) {
            _selection = start_selection.before_neighbor(_phase1.text.size());
            _has_dead_character = original_grapheme;
        }
    }

    void delete_dead_character() noexcept
    {
        if (_has_dead_character) {
            hi_assert(_selection.cursor().before());
            hi_assert_bounds(_selection.cursor().index(), _phase1.text);

            if (_has_dead_character != U'\uffff') {
                auto text = _phase1.text;
                text[_selection.cursor().index()] = *_has_dead_character;
                delegate->set_text(this, text);
            } else {
                auto text = _phase1.text;
                text.erase(_selection.cursor().index(), 1);
                delegate->set_text(this, text);
            }
        }
        _has_dead_character = std::nullopt;
    }

    void delete_character_next() noexcept
    {
        if (_selection.empty()) {
            auto cursor = _selection.cursor();
            cursor = cursor.before_neighbor(_shaped_text.size());

            auto const [first, last] = _shaped_text.select_char(cursor);
            _selection.drag_selection(last);
        }

        return replace_selection(gstring{});
    }

    void delete_character_prev() noexcept
    {
        if (_selection.empty()) {
            auto cursor = _selection.cursor();
            cursor = cursor.after_neighbor(_shaped_text.size());

            auto const [first, last] = _shaped_text.select_char(cursor);
            _selection.drag_selection(first);
        }

        return replace_selection(gstring{});
    }

    void delete_word_next() noexcept
    {
        if (_selection.empty()) {
            auto cursor = _selection.cursor();
            cursor = cursor.before_neighbor(_shaped_text.size());

            auto const [first, last] = _shaped_text.select_word(cursor);
            _selection.drag_selection(last);
        }

        return replace_selection(gstring{});
    }

    void delete_word_prev() noexcept
    {
        if (_selection.empty()) {
            auto cursor = _selection.cursor();
            cursor = cursor.after_neighbor(_shaped_text.size());

            auto const [first, last] = _shaped_text.select_word(cursor);
            _selection.drag_selection(first);
        }

        return replace_selection(gstring{});
    }
};

} // namespace v1
} // namespace hi::v1
