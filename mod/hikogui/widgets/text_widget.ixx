// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/text_widget.hpp Defines text_widget.
 * @ingroup widgets
 */

module;
#include "../macros.hpp"

#include <memory>
#include <string>
#include <array>
#include <optional>
#include <future>
#include <limits>
#include <chrono>

export module hikogui_widgets_text_widget;
import hikogui_GUI;
import hikogui_container;
import hikogui_coroutine;
import hikogui_geometry;
import hikogui_l10n;
import hikogui_observer;
import hikogui_text;
import hikogui_widgets_text_delegate;
import hikogui_widgets_widget;

export namespace hi { inline namespace v1 {

template<typename Context>
concept text_widget_attribute = forward_of<Context, observer<hi::alignment>, observer<hi::semantic_text_style>>;

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
class text_widget final : public widget {
public:
    using super = widget;
    using delegate_type = text_delegate;

    std::shared_ptr<delegate_type> delegate;

    /** The horizontal alignment of the text inside the space of the widget.
     */
    observer<alignment> alignment = hi::alignment::top_flush();

    /** The style of the text.
     */
    observer<semantic_text_style> text_style = semantic_text_style::label;

    ~text_widget()
    {
        hi_assert_not_null(delegate);
        delegate->deinit(*this);
    }

    /** Construct a text widget.
     *
     * @param parent The owner of this widget.
     * @param delegate The delegate to use to control the widget's data.
     */
    text_widget(not_null<widget_intf const *> parent, std::shared_ptr<delegate_type> delegate) noexcept : super(parent), delegate(std::move(delegate))
    {
        mode = widget_mode::select;

        hi_assert_not_null(this->delegate);
        _delegate_cbt = this->delegate->subscribe([&] {
            // On every text edit, immediately/synchronously update the shaped text.
            // This is needed for handling multiple edit commands before the next frame update.
            if (_layout) {
                auto new_layout = _layout;
                hilet old_constraints = _constraints_cache;

                // Constrain and layout according to the old layout.
                hilet new_constraints = update_constraints();
                new_layout.shape.rectangle = aarectangle{
                    new_layout.shape.x(),
                    new_layout.shape.y(),
                    std::max(new_layout.shape.width(), new_constraints.minimum.width()),
                    std::max(new_layout.shape.height(), new_constraints.minimum.height())};
                set_layout(new_layout);

                if (new_constraints != old_constraints) {
                    // The constraints have changed, properly constrain and layout on the next frame.
                    ++global_counter<"text_widget:delegate:constrain">;
                    request_scroll();
                    process_event({gui_event_type::window_reconstrain});
                }
            } else {
                // The layout is incomplete, properly constrain and layout on the next frame.
                ++global_counter<"text_widget:delegate:constrain">;
                request_scroll();
                process_event({gui_event_type::window_reconstrain});
            }
        });

        _text_style_cbt = text_style.subscribe([&](auto...) {
            ++global_counter<"text_widget:text_style:constrain">;
            request_scroll();
            process_event({gui_event_type::window_reconstrain});
        });

        _cursor_state_cbt = _cursor_state.subscribe([&](auto...) {
            ++global_counter<"text_widget:cursor_state:redraw">;
            request_redraw();
        });

        // If the text_widget is used as a label the blink_cursor() co-routine
        // is only waiting on `model` and `focus`, so this is cheap.
        _blink_cursor = blink_cursor();

        this->delegate->init(*this);
    }

    text_widget(not_null<widget_intf const *> parent, std::shared_ptr<delegate_type> delegate, text_widget_attribute auto&&...attributes) noexcept :
        text_widget(parent, std::move(delegate))
    {
        set_attributes(hi_forward(attributes)...);
    }

    /** Construct a text widget.
     *
     * @param parent The owner of this widget.
     * @param text The text to be displayed.
     * @param attributes A set of attributes used to configure the text widget: a `alignment` or `semantic_text_style`.
     */
    text_widget(
        not_null<widget_intf const *> parent,
        incompatible_with<std::shared_ptr<delegate_type>> auto&& text,
        text_widget_attribute auto&&...attributes) noexcept
        requires requires { make_default_text_delegate(hi_forward(text)); }
        : text_widget(parent, make_default_text_delegate(hi_forward(text)), hi_forward(attributes)...)
    {
    }

    /// @privatesection
    [[nodiscard]] box_constraints update_constraints() noexcept override
    {
        _layout = {};

        // Read the latest text from the delegate.
        hi_assert_not_null(delegate);
        _text_cache = delegate->read(*this);

        // Make sure that the current selection fits the new text.
        _selection.resize(_text_cache.size());

        hilet actual_text_style = theme().text_style(*text_style);

        // Create a new text_shaper with the new text.
        auto alignment_ = os_settings::left_to_right() ? *alignment : mirror(*alignment);

        _shaped_text = text_shaper{_text_cache, actual_text_style, theme().scale, alignment_, os_settings::left_to_right()};

        hilet shaped_text_rectangle = ceil(_shaped_text.bounding_rectangle(std::numeric_limits<float>::infinity()));
        hilet shaped_text_size = shaped_text_rectangle.size();

        if (*mode == widget_mode::partial) {
            // In line-edit mode the text should not wrap.
            return _constraints_cache = {
                       shaped_text_size, shaped_text_size, shaped_text_size, _shaped_text.resolved_alignment(), theme().margin()};

        } else {
            // Allow the text to be 550.0f pixels wide.
            hilet preferred_shaped_text_rectangle = ceil(_shaped_text.bounding_rectangle(550.0f));
            hilet preferred_shaped_text_size = preferred_shaped_text_rectangle.size();

            hilet height = std::max(shaped_text_size.height(), preferred_shaped_text_size.height());
            return _constraints_cache = {
                       extent2{preferred_shaped_text_size.width(), height},
                       extent2{preferred_shaped_text_size.width(), height},
                       extent2{shaped_text_size.width(), height},
                       _shaped_text.resolved_alignment(),
                       theme().margin()};
        }
    }

    void set_layout(widget_layout const& context) noexcept override
    {
        if (compare_store(_layout, context)) {
            hi_assert(context.shape.baseline);

            _shaped_text.layout(context.rectangle(), *context.shape.baseline, context.sub_pixel_size);
        }
    }

    void draw(draw_context const& context) noexcept override
    {
        using namespace std::literals::chrono_literals;

        // After potential reconstrain and relayout, updating the shaped-text, ask the parent window to scroll if needed.
        if (std::exchange(_request_scroll, false)) {
            scroll_to_show_selection();
        }

        if (_last_drag_mouse_event) {
            if (_last_drag_mouse_event_next_repeat == utc_nanoseconds{}) {
                _last_drag_mouse_event_next_repeat = context.display_time_point + os_settings::keyboard_repeat_delay();

            } else if (context.display_time_point >= _last_drag_mouse_event_next_repeat) {
                _last_drag_mouse_event_next_repeat = context.display_time_point + os_settings::keyboard_repeat_interval();

                // The last drag mouse event was stored in window coordinate to compensate for scrolling, translate it
                // back to local coordinates before handling the mouse event again.
                auto new_mouse_event = _last_drag_mouse_event;
                new_mouse_event.mouse().position = _layout.from_window * _last_drag_mouse_event.mouse().position;

                // When mouse is dragging a selection, start continues redraw and scroll parent views to display the selection.
                text_widget::handle_event(new_mouse_event);
            }
            scroll_to_show_selection();
            ++global_counter<"text_widget:mouse_drag:redraw">;
            request_redraw();
        }

        if (*mode > widget_mode::invisible and overlaps(context, layout())) {
            context.draw_text(layout(), _shaped_text);

            context.draw_text_selection(layout(), _shaped_text, _selection, theme().color(semantic_color::text_select));

            if (*_cursor_state == cursor_state_type::on or *_cursor_state == cursor_state_type::busy) {
                context.draw_text_cursors(
                    layout(),
                    _shaped_text,
                    _selection.cursor(),
                    _overwrite_mode,
                    to_bool(_has_dead_character),
                    theme().color(semantic_color::primary_cursor),
                    theme().color(semantic_color::secondary_cursor));
            }
        }
    }

    bool handle_event(gui_event const& event) noexcept override
    {
        hi_axiom(loop::main().on_thread());

        switch (event.type()) {
            using enum gui_event_type;
            using enum widget_mode;

        case gui_widget_next:
        case gui_widget_prev:
        case keyboard_exit:
            // When the next widget is selected due to pressing the Tab key the text should be committed.
            // The `text_widget` does not handle gui_activate, so it will be forwarded to parent widgets,
            // such as `text_field_widget` which does.
            process_event(gui_event_type::gui_activate);
            return super::handle_event(event);

        case keyboard_grapheme:
            if (*mode >= partial) {
                reset_state("BDX");
                add_character(event.grapheme(), add_type::append);
                return true;
            }
            break;

        case keyboard_partial_grapheme:
            if (*mode >= partial) {
                reset_state("BDX");
                add_character(event.grapheme(), add_type::dead);
                return true;
            }
            break;

        case text_mode_insert:
            if (*mode >= partial) {
                reset_state("BDX");
                _overwrite_mode = not _overwrite_mode;
                fix_cursor_position();
                return true;
            }
            break;

        case text_edit_paste:
            if (*mode >= partial) {
                reset_state("BDX");
                auto tmp = event.clipboard_data();
                // Replace all paragraph separators with white-space.
                std::replace(tmp.begin(), tmp.end(), grapheme{unicode_PS}, grapheme{' '});
                replace_selection(tmp);
                return true;

            } else if (*mode >= enabled) {
                reset_state("BDX");
                replace_selection(event.clipboard_data());
                return true;
            }
            break;

        case text_edit_copy:
            if (*mode >= select) {
                reset_state("BDX");
                if (hilet selected_text_ = selected_text(); not selected_text_.empty()) {
                    process_event(gui_event::make_clipboard_event(gui_event_type::window_set_clipboard, selected_text_));
                }
                return true;
            }
            break;

        case text_edit_cut:
            if (*mode >= select) {
                reset_state("BDX");
                process_event(gui_event::make_clipboard_event(gui_event_type::window_set_clipboard, selected_text()));
                if (*mode >= partial) {
                    replace_selection(gstring{});
                }
                return true;
            }
            break;

        case text_undo:
            if (*mode >= partial) {
                reset_state("BDX");
                undo();
                return true;
            }
            break;

        case text_redo:
            if (*mode >= partial) {
                reset_state("BDX");
                redo();
                return true;
            }
            break;

        case text_insert_line:
            if (*mode >= enabled) {
                reset_state("BDX");
                add_character(grapheme{unicode_PS}, add_type::append);
                return true;
            }
            break;

        case text_insert_line_up:
            if (*mode >= enabled) {
                reset_state("BDX");
                _selection = _shaped_text.move_begin_paragraph(_selection.cursor());
                add_character(grapheme{unicode_PS}, add_type::insert);
                return true;
            }
            break;

        case text_insert_line_down:
            if (*mode >= enabled) {
                reset_state("BDX");
                _selection = _shaped_text.move_end_paragraph(_selection.cursor());
                add_character(grapheme{unicode_PS}, add_type::insert);
                return true;
            }
            break;

        case text_delete_char_next:
            if (*mode >= partial) {
                reset_state("BDX");
                delete_character_next();
                return true;
            }
            break;

        case text_delete_char_prev:
            if (*mode >= partial) {
                reset_state("BDX");
                delete_character_prev();
                return true;
            }
            break;

        case text_delete_word_next:
            if (*mode >= partial) {
                reset_state("BDX");
                delete_word_next();
                return true;
            }
            break;

        case text_delete_word_prev:
            if (*mode >= partial) {
                reset_state("BDX");
                delete_word_prev();
                return true;
            }
            break;

        case text_cursor_left_char:
            if (*mode >= partial) {
                reset_state("BDX");
                _selection = _shaped_text.move_left_char(_selection.cursor(), _overwrite_mode);
                request_scroll();
                return true;
            }
            break;

        case text_cursor_right_char:
            if (*mode >= partial) {
                reset_state("BDX");
                _selection = _shaped_text.move_right_char(_selection.cursor(), _overwrite_mode);
                request_scroll();
                return true;
            }
            break;

        case text_cursor_down_char:
            if (*mode >= partial) {
                reset_state("BD");
                _selection = _shaped_text.move_down_char(_selection.cursor(), _vertical_movement_x);
                request_scroll();
                return true;
            }
            break;

        case text_cursor_up_char:
            if (*mode >= partial) {
                reset_state("BD");
                _selection = _shaped_text.move_up_char(_selection.cursor(), _vertical_movement_x);
                request_scroll();
                return true;
            }
            break;

        case text_cursor_left_word:
            if (*mode >= partial) {
                reset_state("BDX");
                _selection = _shaped_text.move_left_word(_selection.cursor(), _overwrite_mode);
                request_scroll();
                return true;
            }
            break;

        case text_cursor_right_word:
            if (*mode >= partial) {
                reset_state("BDX");
                _selection = _shaped_text.move_right_word(_selection.cursor(), _overwrite_mode);
                request_scroll();
                return true;
            }
            break;

        case text_cursor_begin_line:
            if (*mode >= partial) {
                reset_state("BDX");
                _selection = _shaped_text.move_begin_line(_selection.cursor());
                request_scroll();
                return true;
            }
            break;

        case text_cursor_end_line:
            if (*mode >= partial) {
                reset_state("BDX");
                _selection = _shaped_text.move_end_line(_selection.cursor());
                request_scroll();
                return true;
            }
            break;

        case text_cursor_begin_sentence:
            if (*mode >= partial) {
                reset_state("BDX");
                _selection = _shaped_text.move_begin_sentence(_selection.cursor());
                request_scroll();
                return true;
            }
            break;

        case text_cursor_end_sentence:
            if (*mode >= partial) {
                reset_state("BDX");
                _selection = _shaped_text.move_end_sentence(_selection.cursor());
                request_scroll();
                return true;
            }
            break;

        case text_cursor_begin_document:
            if (*mode >= partial) {
                reset_state("BDX");
                _selection = _shaped_text.move_begin_document(_selection.cursor());
                request_scroll();
                return true;
            }
            break;

        case text_cursor_end_document:
            if (*mode >= partial) {
                reset_state("BDX");
                _selection = _shaped_text.move_end_document(_selection.cursor());
                request_scroll();
                return true;
            }
            break;

        case gui_cancel:
            if (*mode >= select) {
                reset_state("BDX");
                _selection.clear_selection(_shaped_text.size());
                return true;
            }
            break;

        case text_select_left_char:
            if (*mode >= partial) {
                reset_state("BDX");
                _selection.drag_selection(_shaped_text.move_left_char(_selection.cursor(), false));
                request_scroll();
                return true;
            }
            break;

        case text_select_right_char:
            if (*mode >= partial) {
                reset_state("BDX");
                _selection.drag_selection(_shaped_text.move_right_char(_selection.cursor(), false));
                request_scroll();
                return true;
            }
            break;

        case text_select_down_char:
            if (*mode >= partial) {
                reset_state("BD");
                _selection.drag_selection(_shaped_text.move_down_char(_selection.cursor(), _vertical_movement_x));
                request_scroll();
                return true;
            }
            break;

        case text_select_up_char:
            if (*mode >= partial) {
                reset_state("BD");
                _selection.drag_selection(_shaped_text.move_up_char(_selection.cursor(), _vertical_movement_x));
                request_scroll();
                return true;
            }
            break;

        case text_select_left_word:
            if (*mode >= partial) {
                reset_state("BDX");
                _selection.drag_selection(_shaped_text.move_left_word(_selection.cursor(), false));
                request_scroll();
                return true;
            }
            break;

        case text_select_right_word:
            if (*mode >= partial) {
                reset_state("BDX");
                _selection.drag_selection(_shaped_text.move_right_word(_selection.cursor(), false));
                request_scroll();
                return true;
            }
            break;

        case text_select_begin_line:
            if (*mode >= partial) {
                reset_state("BDX");
                _selection.drag_selection(_shaped_text.move_begin_line(_selection.cursor()));
                request_scroll();
                return true;
            }
            break;

        case text_select_end_line:
            if (*mode >= partial) {
                reset_state("BDX");
                _selection.drag_selection(_shaped_text.move_end_line(_selection.cursor()));
                request_scroll();
                return true;
            }
            break;

        case text_select_begin_sentence:
            if (*mode >= partial) {
                reset_state("BDX");
                _selection.drag_selection(_shaped_text.move_begin_sentence(_selection.cursor()));
                request_scroll();
                return true;
            }
            break;

        case text_select_end_sentence:
            if (*mode >= partial) {
                reset_state("BDX");
                _selection.drag_selection(_shaped_text.move_end_sentence(_selection.cursor()));
                request_scroll();
                return true;
            }
            break;

        case text_select_begin_document:
            if (*mode >= partial) {
                reset_state("BDX");
                _selection.drag_selection(_shaped_text.move_begin_document(_selection.cursor()));
                request_scroll();
                return true;
            }
            break;

        case text_select_end_document:
            if (*mode >= partial) {
                reset_state("BDX");
                _selection.drag_selection(_shaped_text.move_end_document(_selection.cursor()));
                request_scroll();
                return true;
            }
            break;

        case text_select_document:
            if (*mode >= partial) {
                reset_state("BDX");
                _selection = _shaped_text.move_begin_document(_selection.cursor());
                _selection.drag_selection(_shaped_text.move_end_document(_selection.cursor()));
                request_scroll();
                return true;
            }
            break;

        case mouse_up:
            if (*mode >= select) {
                // Stop the continues redrawing during dragging.
                // Also reset the time, so on drag-start it will initialize the time, which will
                // cause a smooth startup of repeating.
                _last_drag_mouse_event = {};
                _last_drag_mouse_event_next_repeat = {};
                return true;
            }
            break;

        case mouse_down:
            if (*mode >= select) {
                hilet cursor = _shaped_text.get_nearest_cursor(event.mouse().position);
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
                process_event({gui_event_type::window_relayout});
                request_scroll();
                return true;
            }
            break;

        case mouse_drag:
            if (*mode >= select) {
                hilet cursor = _shaped_text.get_nearest_cursor(event.mouse().position);
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
                _last_drag_mouse_event.mouse().position = _layout.to_window * event.mouse().position;
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
            if (*mode >= widget_mode::partial) {
                return hitbox{id, _layout.elevation, hitbox_type::text_edit};

            } else if (*mode >= widget_mode::select) {
                return hitbox{id, _layout.elevation, hitbox_type::_default};

            } else {
                return hitbox{};
            }
        } else {
            return hitbox{};
        }
    }

    [[nodiscard]] bool accepts_keyboard_focus(keyboard_focus_group group) const noexcept override
    {
        if (*mode >= widget_mode::partial) {
            return to_bool(group & keyboard_focus_group::normal);
        } else if (*mode >= widget_mode::select) {
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

    gstring _text_cache;
    text_shaper _shaped_text;

    mutable box_constraints _constraints_cache;

    text_selection _selection;

    scoped_task<> _blink_cursor;

    observer<cursor_state_type> _cursor_state = cursor_state_type::none;

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
    callback<void(semantic_text_style)> _text_style_cbt;
    callback<void(cursor_state_type)> _cursor_state_cbt;

    void set_attributes() noexcept {}
    void set_attributes(text_widget_attribute auto&& first, text_widget_attribute auto&&...rest) noexcept
    {
        if constexpr (forward_of<decltype(first), observer<hi::alignment>>) {
            alignment = hi_forward(first);
        } else if constexpr (forward_of<decltype(first), observer<hi::semantic_text_style>>) {
            text_style = hi_forward(first);
        } else {
            hi_static_no_default();
        }

        set_attributes(hi_forward(rest)...);
    }

    /** Make parent scroll views, scroll to show the current selection and cursor.
     */
    void scroll_to_show_selection() noexcept
    {
        if (*mode > widget_mode::invisible and *focus) {
            hilet cursor = _selection.cursor();
            hilet char_it = _shaped_text.begin() + cursor.index();
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
    void reset_state(char const *states) noexcept
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
        hilet[first, last] = _selection.selection_indices();

        return gstring_view{_text_cache}.substr(first, last - first);
    }

    void undo_push() noexcept
    {
        _undo_stack.emplace(_text_cache, _selection);
    }

    void undo() noexcept
    {
        if (_undo_stack.can_undo()) {
            hilet & [ text, selection ] = _undo_stack.undo(_text_cache, _selection);

            delegate->write(*this, text);
            _selection = selection;
        }
    }

    void redo() noexcept
    {
        if (_undo_stack.can_redo()) {
            hilet & [ text, selection ] = _undo_stack.redo();

            delegate->write(*this, text);
            _selection = selection;
        }
    }

    scoped_task<> blink_cursor() noexcept
    {
        while (true) {
            if (*mode >= widget_mode::partial and *focus) {
                switch (*_cursor_state) {
                case cursor_state_type::busy:
                    _cursor_state = cursor_state_type::on;
                    co_await when_any(os_settings::cursor_blink_delay(), mode, focus);
                    break;

                case cursor_state_type::on:
                    _cursor_state = cursor_state_type::off;
                    co_await when_any(os_settings::cursor_blink_interval() / 2, mode, focus);
                    break;

                case cursor_state_type::off:
                    _cursor_state = cursor_state_type::on;
                    co_await when_any(os_settings::cursor_blink_interval() / 2, mode, focus);
                    break;

                default:
                    _cursor_state = cursor_state_type::busy;
                }

            } else {
                _cursor_state = cursor_state_type::none;
                co_await when_any(mode, focus);
            }
        }
    }

    /** Fix the cursor position after cursor movement.
     */
    void fix_cursor_position() noexcept
    {
        hilet size = _text_cache.size();
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

        hilet[first, last] = _selection.selection_indices();

        auto text = _text_cache;
        text.replace(first, last - first, replacement);
        delegate->write(*this, text);

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
        hilet[start_selection, end_selection] = _selection.selection(_text_cache.size());
        auto original_grapheme = grapheme{char32_t{0xffff}};

        if (_selection.empty() and _overwrite_mode and start_selection.before()) {
            original_grapheme = _text_cache[start_selection.index()];

            hilet[first, last] = _shaped_text.select_char(start_selection);
            _selection.drag_selection(last);
        }
        replace_selection(gstring{c});

        if (keyboard_mode == add_type::insert) {
            // The character was inserted, put the cursor back where it was.
            _selection = start_selection;

        } else if (keyboard_mode == add_type::dead) {
            _selection = start_selection.before_neighbor(_text_cache.size());
            _has_dead_character = original_grapheme;
        }
    }

    void delete_dead_character() noexcept
    {
        if (_has_dead_character) {
            hi_assert(_selection.cursor().before());
            hi_assert_bounds(_selection.cursor().index(), _text_cache);

            if (_has_dead_character != U'\uffff') {
                auto text = _text_cache;
                text[_selection.cursor().index()] = *_has_dead_character;
                delegate->write(*this, text);
            } else {
                auto text = _text_cache;
                text.erase(_selection.cursor().index(), 1);
                delegate->write(*this, text);
            }
        }
        _has_dead_character = std::nullopt;
    }

    void delete_character_next() noexcept
    {
        if (_selection.empty()) {
            auto cursor = _selection.cursor();
            cursor = cursor.before_neighbor(_shaped_text.size());

            hilet[first, last] = _shaped_text.select_char(cursor);
            _selection.drag_selection(last);
        }

        return replace_selection(gstring{});
    }

    void delete_character_prev() noexcept
    {
        if (_selection.empty()) {
            auto cursor = _selection.cursor();
            cursor = cursor.after_neighbor(_shaped_text.size());

            hilet[first, last] = _shaped_text.select_char(cursor);
            _selection.drag_selection(first);
        }

        return replace_selection(gstring{});
    }

    void delete_word_next() noexcept
    {
        if (_selection.empty()) {
            auto cursor = _selection.cursor();
            cursor = cursor.before_neighbor(_shaped_text.size());

            hilet[first, last] = _shaped_text.select_word(cursor);
            _selection.drag_selection(last);
        }

        return replace_selection(gstring{});
    }

    void delete_word_prev() noexcept
    {
        if (_selection.empty()) {
            auto cursor = _selection.cursor();
            cursor = cursor.after_neighbor(_shaped_text.size());

            hilet[first, last] = _shaped_text.select_word(cursor);
            _selection.drag_selection(first);
        }

        return replace_selection(gstring{});
    }
};

}} // namespace hi::v1
