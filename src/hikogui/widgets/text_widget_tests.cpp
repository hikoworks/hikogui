// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "text_widget.hpp"
#include "../file/file.hpp"
#include "../GUI/GUI.hpp"
#include "../macros.hpp"
#include <gtest/gtest.h>
#include <iostream>
#include <string>

using namespace std;
using namespace hi;

class text_widget_tests : public ::testing::Test {
protected:
    class system_moc : public hi::gui_system {
    public:
        system_moc() : hi::gui_system({}, {}, {}, {}) {}
    };

    class window_moc : public hi::gui_window {
    public:
        hi::theme _theme;

        // clang-format off
        window_moc(gui_system &system) : hi::gui_window(system, {}, {}) {}
        void set_cursor(hi::mouse_cursor) override {}
        void close_window() override {}
        void set_size_state(hi::gui_window_size) noexcept override {}
        hi::aarectangle workspace_rectangle() const noexcept override { return {};}
        hi::aarectangle fullscreen_rectangle() const noexcept override { return {};}
        hi::subpixel_orientation subpixel_orientation() const noexcept override { return hi::subpixel_orientation::unknown; }
        void open_system_menu() override {}
        void set_window_size(hi::extent2) override {}
        std::optional<gstring> get_text_from_clipboard() const noexcept override { return {};}
        void put_text_on_clipboard(gstring_view) const noexcept override {}
        void create_window(hi::extent2) override {}
        // clang-format on
    };

    class window_widget_moc : public hi::widget {
    public:
        window_widget_moc(hi::gui_window *window) noexcept : hi::widget(nullptr), _window(window) {}
    };

    std::unique_ptr<hi::theme_book> theme_book;
    hi::theme theme;

    observer<std::string> text;
    std::unique_ptr<system_moc> system;
    std::unique_ptr<window_moc> window;
    std::unique_ptr<window_widget_moc> window_widget;
    std::unique_ptr<hi::text_widget> widget;

    void SetUp() override
    {
        hi::start_system();
        os_settings::start_subsystem();

        // Cursor movement (including editing) requires the text to be shaped.
        // text shaping requires fonts and text styles.
        register_font_directories(hi::font_dirs());
        theme_book = std::make_unique<hi::theme_book>(std::ranges::to<std::vector>(hi::theme_dirs()));
        theme = theme_book->find("default", theme_mode::light);

        system = std::make_unique<system_moc>();

        window = std::make_unique<window_moc>(*system);
        window->theme = theme;

        window_widget = std::make_unique<window_widget_moc>(window.get());

        widget = std::make_unique<hi::text_widget>(window_widget.get(), text);
        widget->set_mode(widget_mode::enabled);

        auto constraints = widget->update_constraints();
        auto layout = widget_layout{};
        layout.shape.rectangle = aarectangle{constraints.preferred};
        layout.shape.baseline = constraints.preferred.height() / 2;
        // display_time_point is used to check for valid widget_layout.
        layout.display_time_point = std::chrono::utc_clock::now();
        widget->set_layout(layout);
    }
};

TEST_F(text_widget_tests, add_character)
{
    text = std::string{"hllo"};

    widget->handle_event(gui_event{gui_event_type::text_cursor_right_char});
    widget->handle_event(gui_event::keyboard_grapheme(grapheme{'e'}));

    ASSERT_EQ(*text, "hello");
}

TEST_F(text_widget_tests, add_combining_character)
{
    text = std::string{"hllo"};

    widget->handle_event(gui_event{gui_event_type::text_cursor_right_char});
    widget->handle_event(gui_event::keyboard_partial_grapheme(grapheme{'"'}));
    widget->handle_event(gui_event::keyboard_grapheme(grapheme{U'\u00EB'})); // e-umlaut

    ASSERT_EQ(*text, "h\u00EBllo");
}

TEST_F(text_widget_tests, replace_selection_combining_character)
{
    text = std::string{"hello"};

    widget->handle_event(gui_event{gui_event_type::text_cursor_right_char});
    widget->handle_event(gui_event{gui_event_type::text_select_right_char});
    widget->handle_event(gui_event::keyboard_partial_grapheme(grapheme{'"'}));
    widget->handle_event(gui_event::keyboard_grapheme(grapheme{U'\u00EB'})); // e-umlaut

    ASSERT_EQ(*text, "h\u00EBllo");
}
