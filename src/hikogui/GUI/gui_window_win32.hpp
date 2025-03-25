// Copyright Take Vos 2019, 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../win32_headers.hpp"

#include "gui_event.hpp"
#include "gui_window_size.hpp"
#include "hitbox.hpp"
#include "keyboard_bindings.hpp"
#include "theme_book.hpp"
#include "widget_intf.hpp"
#include "mouse_cursor.hpp"
#include "../l10n/l10n.hpp"
#include "../GFX/GFX.hpp"
#include "../crt/crt.hpp"
#include "../macros.hpp"
#include <unordered_map>

hi_export_module(hikogui.GUI : gui_window);

hi_export namespace hi::inline v1 {

class gui_window {
public:
    HWND win32Window = nullptr;

    std::unique_ptr<gfx_surface> surface;

    /** The current rectangle of the window relative to the screen.
     * The screen rectangle is set by the operating system event loop.
     *
     * This rectangle is used by the operating system event loop hit-testing
     * to determine the position of screen coordinates to window coordinates.
     *
     * The size of this rectangle is used to laying out widgets and setting
     * the size of the gfx_surface during rendering.
     */
    aarectangle rectangle;

    /** The current cursor.
     * Used for optimizing when the operating system cursor is updated.
     * Set to mouse_cursor::None at the start (for the wait icon) and when the
     * operating system is going to display another icon to make sure
     * when it comes back in the application the cursor will be updated
     * correctly.
     */
    mouse_cursor current_mouse_cursor = mouse_cursor::None;

    /*! The window is currently being resized by the user.
     * We can disable expensive redraws during rendering until this
     * is false again.
     */
    bool resizing = false;

    /** Pixel density of the screen where the window is located.
     * If the window is located on multiple screens then one of the screens is used as
     * the source for the pixel-density.
     */
    unit::pixel_density pixel_density = {unit::pixels_per_inch(96.0f), device_type::desktop};

    /** Theme to use to draw the widgets on this window.
     * The sizes and colors of the theme have already been adjusted to the window's state and ppi.
     */
    hi::theme theme = {};

    /** The size of the widget.
     */
    extent2 widget_size;

    /** Notifier used when the window is closing.
     * It is expected that after notifying these callbacks the instance of this class is destroyed.
     */
    notifier<void()> closing;

    gui_window(gui_window const&) = delete;
    gui_window& operator=(gui_window const&) = delete;
    gui_window(gui_window&&) = delete;
    gui_window& operator=(gui_window&&) = delete;

    gui_window(std::unique_ptr<widget_intf> widget) noexcept : _widget(std::move(widget)), track_mouse_leave_event_parameters()
    {
        hi_assert_not_null(_widget);

        if (_first_window) {
            if (not os_settings::start_subsystem()) {
                hi_log_fatal("Could not start the os_settings subsystem.");
            }

            register_font_file(URL{"resource:elusiveicons-webfont.ttf"});
            register_font_file(URL{"resource:hikogui_icons.ttf"});
            register_font_directories(font_dirs());

            register_theme_directories(theme_dirs());

            try {
                load_system_keyboard_bindings(URL{"resource:win32.keybinds.json"});
            } catch (std::exception const& e) {
                hi_log_fatal("Could not load keyboard bindings. \"{}\"", e.what());
            }

            SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

            _first_window = true;
        }

        // Reset the keyboard target to not focus anything.
        update_keyboard_target({});

        // For changes in setting on the OS we should reconstrain/layout/redraw the window
        // For example when the language or theme changes.
        _setting_change_cbt = os_settings::subscribe(
            [this] {
                ++global_counter<"gui_window:os_setting:constrain">;
                request_restyle();
            },
            callback_flags::main);

        // Subscribe on theme changes.
        _selected_theme_cbt = theme_book::global().selected_theme.subscribe(
            [this](auto...) {
                ++global_counter<"gui_window:selected_theme:constrain">;
                request_restyle();
            },
            callback_flags::main);

        _render_cbt = loop::main().subscribe_render([this](utc_nanoseconds display_time, bool redraw_window) {
            this->render(display_time, redraw_window);
        });

        // Delegate has been called, layout of widgets has been calculated for the
        // minimum and maximum size of the window.
        auto const new_position = point2{500.0f, 500.0f}; 
        create_window(new_position);

        theme = get_selected_theme().transform(pixel_density);
        theme.apply_as_styles();
        _widget->set_window(this);

        _restyle = false;
        _widget->restyle(pixel_density);
        assert(not _restyle);

        // Execute a constraint check to determine initial window size.
        _reconstrain = false;
        _widget_constraints = _widget->update_constraints();
        assert(not _reconstrain);

        show_window(_widget_constraints.preferred);
    }

    ~gui_window()
    {
        try {
            if (win32Window != nullptr) {
                DestroyWindow(win32Window);
                hi_assert(win32Window == nullptr);
                // hi_log_fatal("win32Window was not destroyed before Window '{}' was destructed.", title);
            }

        } catch (std::exception const& e) {
            hi_log_fatal("Could not properly destruct gui_window. '{}'", e.what());
        }

        // Destroy the top-level widget, before Window-members that the widgets require from the window during their destruction.
        _widget = {};

        try {
            surface.reset();
            hi_log_info("Window '{}' has been properly destructed.", _title);

        } catch (std::exception const& e) {
            hi_log_fatal("Could not properly destruct gui_window. '{}'", e.what());
        }
    }

    template<typename Widget>
    [[nodiscard]] Widget& widget() const noexcept
    {
        return up_cast<Widget>(*_widget);
    }

    void set_title(label title) noexcept
    {
        _title = std::move(title);
    }

    void request_resize() noexcept
    {
        _resize = true;
    }

    void request_restyle() noexcept
    {
        _restyle = true;
    }

    void request_reconstrain() noexcept
    {
        _reconstrain = true;
    }

    void request_relayout() noexcept
    {
        _relayout = true;
    }

    void request_redraw(aarectangle const& dirty_rectangle) noexcept
    {
        _redraw_rectangle |= dirty_rectangle;
    }

    void request_redraw_window() noexcept
    {
        _redraw_rectangle |= aarectangle{widget_size};
    }

    /** Update window.
     * This will update animations and redraw all widgets managed by this window.
     */
    void render(utc_nanoseconds display_time_point, bool redraw_window)
    {
        if (surface->device() == nullptr) {
            // If there is no device configured for the surface don't try to render.
            return;
        }

        auto const t1 = trace<"window::render">();

        hi_axiom(loop::main().on_thread());
        hi_assert_not_null(surface);
        hi_assert_not_null(_widget);

        if (std::exchange(_restyle, false)) {
            auto const _ = trace<"window::restyle">();

            theme = get_selected_theme().transform(pixel_density);
            theme.apply_as_styles();
            _widget->restyle(pixel_density);
            request_reconstrain();
        }

        auto const resize = std::exchange(_resize, false);

        // When a widget requests it or a window-wide event like language change
        // has happened all the widgets will be set_constraints().
        if (std::exchange(_reconstrain, false) or resize) {
            auto const _ = trace<"window::constrain">();
            _widget_constraints = _widget->update_constraints();
            request_relayout();
        }

        // Check if the window size matches the preferred size of the window_widget.
        // If not ask the operating system to change the size of the window, which is
        // done asynchronously.
        //
        // We need to continue drawing into the incorrectly sized window, otherwise
        // Vulkan will not detect the change of drawing surface's size.
        //
        // Make sure the widget does have its window rectangle match the constraints, otherwise
        // the logic for layout and drawing becomes complicated.
        if (resize) {
            // If a widget asked for a resize, change the size of the window to the preferred size of the widgets.
            auto const new_size = _widget_constraints.preferred;
            if (new_size != rectangle.size()) {
                hi_log_info("A new preferred window size {} was requested by one of the widget.", new_size);
                set_window_size(new_size);
                request_redraw_window();
            }

        } else {
            // Check if the window size matches the minimum and maximum size of the widgets, otherwise resize.
            auto const current_size = rectangle.size();
            auto const new_size = clamp(current_size, _widget_constraints.minimum, _widget_constraints.maximum);
            if (new_size != current_size and size_state() != gui_window_size::minimized) {
                hi_log_info("The current window size {} must grow or shrink to {} to fit the widgets.", current_size, new_size);
                set_window_size(new_size);
                request_redraw_window();
            }
        }

        if (rectangle.size() < _widget_constraints.minimum or rectangle.size() > _widget_constraints.maximum) {
            // Even after the resize above it is possible to have an incorrect window size.
            // For example when minimizing the window.
            // Stop processing rendering for this window here.
            return;
        }

        // Update the graphics' surface to the current size of the window.
        surface->update(rectangle.size());

        if (widget_size != rectangle.size()) {
            // The window size has changed, we need to re-layout the widgets.
            widget_size = rectangle.size();
            request_relayout();
        }

        if (std::exchange(_relayout, false)) {
            auto const _ = trace<"window::layout">();

            // Guarantee that the layout size is always at least the minimum size.
            // We do this because it simplifies calculations if no minimum checks are necessary inside widget.
            auto const widget_layout_size = max(_widget_constraints.minimum, widget_size);
            _widget->set_layout(widget_layout{widget_layout_size, _size_state, subpixel_orientation(), display_time_point});

            // After layout do a complete redraw.
            request_redraw_window();
        }

        // If the window redraw is requested by the loop, redraw the entire window.
        if (redraw_window) {
            request_redraw_window();
        }

        // Draw widgets if the _redraw_rectangle was set.
        if (auto draw_context = surface->render_start(std::exchange(_redraw_rectangle, aarectangle{}))) {
            draw_context.display_time_point = display_time_point;
            draw_context.subpixel_orientation = subpixel_orientation();
            draw_context.saturation = 1.0f;

            {
                auto const t2 = trace<"window::draw">();
                _widget->draw(draw_context);
            }
            {
                auto const t2 = trace<"window::submit">();
                surface->render_finish(draw_context);
            }
        }
    }

    /** Set the mouse cursor icon.
     */
    void set_cursor(mouse_cursor cursor) noexcept
    {
        hi_axiom(loop::main().on_thread());

        if (current_mouse_cursor == cursor) {
            return;
        }
        current_mouse_cursor = cursor;

        if (cursor == mouse_cursor::None) {
            return;
        }

        static auto idcAppStarting = LoadCursorW(nullptr, IDC_APPSTARTING);
        static auto idcArrow = LoadCursorW(nullptr, IDC_ARROW);
        static auto idcHand = LoadCursorW(nullptr, IDC_HAND);
        static auto idcIBeam = LoadCursorW(nullptr, IDC_IBEAM);
        static auto idcNo = LoadCursorW(nullptr, IDC_NO);

        auto idc = idcNo;
        switch (cursor) {
        case mouse_cursor::None:
            idc = idcAppStarting;
            break;
        case mouse_cursor::Default:
            idc = idcArrow;
            break;
        case mouse_cursor::Button:
            idc = idcHand;
            break;
        case mouse_cursor::TextEdit:
            idc = idcIBeam;
            break;
        default:
            hi_no_default();
        }

        SetCursor(idc);
    }

    /** Ask the operating system to close this window.
     */
    void close_window()
    {
        hi_axiom(loop::main().on_thread());
        if (not PostMessageW(win32Window, WM_CLOSE, 0, 0)) {
            hi_log_error("Could not send WM_CLOSE to window {}: {}", _title, get_last_error_message());
        }
    }

    /** Set the size-state of the window.
     *
     * This function is used to change the size of the window to one
     * of the predefined states: normal, minimized, maximized or full-screen.
     */
    void set_size_state(gui_window_size state) noexcept
    {
        hi_axiom(loop::main().on_thread());

        if (_size_state == state) {
            return;
        }

        if (_size_state == gui_window_size::normal) {
            _restore_rectangle = rectangle;
        } else if (_size_state == gui_window_size::minimized) {
            ShowWindow(win32Window, SW_RESTORE);
            _size_state = gui_window_size::normal;
        }

        if (state == gui_window_size::normal) {
            auto const left = round_cast<int>(_restore_rectangle.left());
            auto const top = round_cast<int>(_restore_rectangle.top());
            auto const width = round_cast<int>(_restore_rectangle.width());
            auto const height = round_cast<int>(_restore_rectangle.height());
            auto const inv_top = round_cast<int>(os_settings::primary_monitor_rectangle().height()) - top;
            SetWindowPos(win32Window, HWND_TOP, left, inv_top, width, height, 0);
            _size_state = gui_window_size::normal;

        } else if (state == gui_window_size::minimized) {
            ShowWindow(win32Window, SW_MINIMIZE);
            _size_state = gui_window_size::minimized;

        } else if (state == gui_window_size::maximized) {
            auto const workspace = workspace_rectangle();
            auto const max_size = _widget_constraints.maximum;

            // Try to resize the window while keeping the toolbar in the same location.
            auto const width = std::min(max_size.width(), workspace.width());
            auto const height = std::min(max_size.height(), workspace.height());
            auto const left = std::clamp(rectangle.left(), workspace.left(), workspace.right() - width);
            auto const top = std::clamp(rectangle.top(), workspace.bottom() + height, workspace.top());
            auto const inv_top = os_settings::primary_monitor_rectangle().height() - top;
            SetWindowPos(
                win32Window,
                HWND_TOP,
                round_cast<int>(left),
                round_cast<int>(inv_top),
                round_cast<int>(width),
                round_cast<int>(height),
                0);
            _size_state = gui_window_size::maximized;

        } else if (state == gui_window_size::fullscreen) {
            auto const fullscreen = fullscreen_rectangle();
            auto const max_size = _widget_constraints.maximum;
            if (fullscreen.width() > max_size.width() or fullscreen.height() > max_size.height()) {
                // Do not go full screen if the widget is unable to go that large.
                return;
            }

            auto const left = round_cast<int>(fullscreen.left());
            auto const top = round_cast<int>(fullscreen.top());
            auto const width = round_cast<int>(fullscreen.width());
            auto const height = round_cast<int>(fullscreen.height());
            auto const inv_top = round_cast<int>(os_settings::primary_monitor_rectangle().height()) - top;
            SetWindowPos(win32Window, HWND_TOP, left, inv_top, width, height, 0);
            _size_state = gui_window_size::fullscreen;
        }
    }

    /** The rectangle of the workspace of the screen where the window is currently located.
     */
    [[nodiscard]] aarectangle workspace_rectangle() const noexcept
    {
        auto const monitor = MonitorFromWindow(win32Window, MONITOR_DEFAULTTOPRIMARY);
        if (monitor == NULL) {
            hi_log_error("Could not get monitor for the window.");
            return {0, 0, 1920, 1080};
        }

        MONITORINFO info;
        info.cbSize = sizeof(MONITORINFO);
        if (not GetMonitorInfo(monitor, &info)) {
            hi_log_error("Could not get monitor info for the window.");
            return {0, 0, 1920, 1080};
        }

        auto const left = narrow_cast<float>(info.rcWork.left);
        auto const top = narrow_cast<float>(info.rcWork.top);
        auto const right = narrow_cast<float>(info.rcWork.right);
        auto const bottom = narrow_cast<float>(info.rcWork.bottom);
        auto const width = right - left;
        auto const height = bottom - top;
        auto const inv_bottom = os_settings::primary_monitor_rectangle().height() - bottom;
        return aarectangle{left, inv_bottom, width, height};
    }

    /** The rectangle of the screen where the window is currently located.
     */
    [[nodiscard]] aarectangle fullscreen_rectangle() const noexcept
    {
        auto const monitor = MonitorFromWindow(win32Window, MONITOR_DEFAULTTOPRIMARY);
        if (monitor == NULL) {
            hi_log_error("Could not get monitor for the window.");
            return {0, 0, 1920, 1080};
        }

        MONITORINFO info;
        info.cbSize = sizeof(MONITORINFO);
        if (not GetMonitorInfo(monitor, &info)) {
            hi_log_error("Could not get monitor info for the window.");
            return {0, 0, 1920, 1080};
        }

        auto const left = narrow_cast<float>(info.rcMonitor.left);
        auto const top = narrow_cast<float>(info.rcMonitor.top);
        auto const right = narrow_cast<float>(info.rcMonitor.right);
        auto const bottom = narrow_cast<float>(info.rcMonitor.bottom);
        auto const width = right - left;
        auto const height = bottom - top;
        auto const inv_bottom = os_settings::primary_monitor_rectangle().height() - bottom;
        return aarectangle{left, inv_bottom, width, height};
    }

    /** Get the size-state of the window.
     */
    gui_window_size size_state() const noexcept
    {
        return _size_state;
    }

    [[nodiscard]] hi::subpixel_orientation subpixel_orientation() const noexcept
    {
        // The table for viewing distance are:
        //
        // - Phone/Watch: 10 inch
        // - Tablet: 15 inch
        // - Notebook/Desktop: 20 inch
        //
        // Pixels Per Degree = PPD = 2 * viewing_distance * resolution * tan(0.5 degree)
        constexpr auto tan_half_degree = 0.00872686779075879f;
        constexpr auto viewing_distance = 20.0f;

        auto const ppd = 2 * viewing_distance * pixel_density.ppi * tan_half_degree;

        if (ppd > unit::pixels_per_inch(55.0f)) {
            // High resolution displays do not require subpixel-aliasing.
            return hi::subpixel_orientation::unknown;
        } else {
            // The win32 API does not have a per-monitor subpixel-orientation.
            return os_settings::subpixel_orientation();
        }
    }

    /** Open the system menu of the window.
     *
     * On windows 10 this is activated by pressing Alt followed by Spacebar.
     */
    void open_system_menu()
    {
        hi_axiom(loop::main().on_thread());

        // Position the system menu on the left side, below the system menu button.
        auto const left = rectangle.left();
        auto const top = rectangle.top() - 30.0f;

        // Convert to y-axis down coordinate system
        auto const inv_top = os_settings::primary_monitor_rectangle().height() - top;

        // Open the system menu window and wait.
        auto const system_menu = GetSystemMenu(win32Window, false);
        auto const cmd =
            TrackPopupMenu(system_menu, TPM_RETURNCMD, round_cast<int>(left), round_cast<int>(inv_top), 0, win32Window, NULL);
        if (cmd > 0) {
            SendMessage(win32Window, WM_SYSCOMMAND, narrow_cast<WPARAM>(cmd), LPARAM{0});
        }
    }

    /** Ask the operating system to set the size of this window.
     */
    void set_window_size(extent2 new_extent)
    {
        hi_axiom(loop::main().on_thread());

        RECT original_rect;
        if (not GetWindowRect(win32Window, &original_rect)) {
            hi_log_error("Could not get the window's rectangle on the screen.");
        }

        auto const new_width = round_cast<int>(new_extent.width());
        auto const new_height = round_cast<int>(new_extent.height());
        auto const new_x = os_settings::left_to_right() ? narrow_cast<int>(original_rect.left) :
                                                     narrow_cast<int>(original_rect.right - new_width);
        auto const new_y = narrow_cast<int>(original_rect.top);

        SetWindowPos(
            win32Window,
            HWND_NOTOPMOST,
            new_x,
            new_y,
            new_width,
            new_height,
            SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOREDRAW | SWP_DEFERERASE | SWP_NOCOPYBITS | SWP_FRAMECHANGED);
    }

    void update_mouse_target(widget_id new_target_id, point2 position = {}) noexcept
    {
        hi_axiom(loop::main().on_thread());

        if (_mouse_target_id != 0) {
            if (new_target_id == _mouse_target_id) {
                // Focus does not change.
                return;
            }

            // The mouse target needs to be updated, send exit to previous target.
            send_events_to_widget(_mouse_target_id, std::vector{gui_event{gui_event_type::mouse_exit}});
        }

        if (new_target_id != 0) {
            _mouse_target_id = new_target_id;
            send_events_to_widget(new_target_id, std::vector{gui_event::make_mouse_enter(position)});
        } else {
            _mouse_target_id = {};
        }
    }

    /** Change the keyboard focus to the given widget.
     * If the group of the widget is incorrect then no widget will be in focus.
     *
     * @param widget The new widget to focus, or empty to remove all keyboard focus.
     * @param group The group the widget must belong to.
     */
    void update_keyboard_target(widget_id new_target_id, keyboard_focus_group group = keyboard_focus_group::normal) noexcept
    {
        hi_axiom(loop::main().on_thread());

        auto new_target_widget = get_if(_widget.get(), new_target_id, false);

        // Before we are going to make new_target_widget empty, due to the rules below;
        // capture which parents there are.
        auto new_target_parent_chain = new_target_widget ? new_target_widget->parent_chain() : std::vector<widget_id>{};

        // If the new target widget does not accept focus, for example when clicking
        // on a disabled widget, or empty part of a window.
        // In that case no widget will get focus.
        if (new_target_widget == nullptr or not new_target_widget->accepts_keyboard_focus(group)) {
            new_target_widget = nullptr;
        }

        if (auto const *const keyboard_target_widget = get_if(_widget.get(), _keyboard_target_id, false)) {
            // keyboard target still exists and visible.
            if (new_target_widget == keyboard_target_widget) {
                // Focus does not change.
                return;
            }

            send_events_to_widget(_keyboard_target_id, std::vector{gui_event{gui_event_type::keyboard_exit}});
        }

        // Tell "escape" to all the widget that are not parents of the new widget
        _widget->handle_event_recursive(gui_event_type::gui_cancel, new_target_parent_chain);

        // Tell the new widget that keyboard focus was entered.
        if (new_target_widget != nullptr) {
            _keyboard_target_id = new_target_widget->id();
            send_events_to_widget(_keyboard_target_id, std::vector{gui_event{gui_event_type::keyboard_enter}});
        } else {
            _keyboard_target_id = {};
        }
    }

    /** Change the keyboard focus to the previous or next widget from the given widget.
     * This function will find the closest widget from the given widget which belongs to the given
     * group; if none is found, or if the original selected widget is found, then no widget will be in focus.
     *
     * @param start_widget The widget to use as the start point for a new widget to select.
     * @param group The group the widget must belong to.
     * @param direction The direction to search in, or current to select the current widget.
     */
    void update_keyboard_target(widget_id start_widget, keyboard_focus_group group, keyboard_focus_direction direction) noexcept
    {
        hi_axiom(loop::main().on_thread());

        if (auto tmp = _widget->find_next_widget(start_widget, group, direction); tmp != start_widget) {
            update_keyboard_target(tmp, group);

        } else if (group == keyboard_focus_group::normal) {
            // Could not find a next widget, loop around.
            // menu items should not loop back.
            tmp = _widget->find_next_widget({}, group, direction);
            update_keyboard_target(tmp, group);
        }
    }

    /** Change the keyboard focus to the given, previous or next widget.
     * This function will find the closest widget from the current widget which belongs to the given
     * group; if none is found, or if the original selected widget is found, then no widget will be in focus.
     *
     * @param group The group the widget must belong to.
     * @param direction The direction to search in, or current to select the current widget.
     */
    void update_keyboard_target(keyboard_focus_group group, keyboard_focus_direction direction) noexcept
    {
        return update_keyboard_target(_keyboard_target_id, group, direction);
    }

    /** Get text from the clipboard.
     *
     * @note This is part of the window as some operating systems need to know from which window the text was posted.
     * @return The text from the clipboard.
     * @retval empty When the clipboard is locked by another application, on error, if the data on the clipboard can not
     *               be converted to text or if the clipboard is empty.
     */
    [[nodiscard]] std::optional<gstring> get_text_from_clipboard() const noexcept
    {
        if (not OpenClipboard(win32Window)) {
            // Another application could have the clipboard locked.
            hi_log_info("Could not open win32 clipboard '{}'", get_last_error_message());
            return std::nullopt;
        }

        auto const defer_CloseClipboard = defer([] {
            CloseClipboard();
        });

        UINT format = 0;
        while ((format = EnumClipboardFormats(format)) != 0) {
            switch (format) {
            case CF_TEXT:
            case CF_OEMTEXT:
            case CF_UNICODETEXT:
                {
                    auto const cb_data = GetClipboardData(CF_UNICODETEXT);
                    if (cb_data == nullptr) {
                        hi_log_error("Could not get clipboard data: '{}'", get_last_error_message());
                        return std::nullopt;
                    }

                    auto const *const wstr_c = static_cast<wchar_t const *>(GlobalLock(cb_data));
                    if (wstr_c == nullptr) {
                        hi_log_error("Could not lock clipboard data: '{}'", get_last_error_message());
                        return std::nullopt;
                    }

                    auto const defer_GlobalUnlock = defer([cb_data] {
                        if (not GlobalUnlock(cb_data) and GetLastError() != ERROR_SUCCESS) {
                            hi_log_error("Could not unlock clipboard data: '{}'", get_last_error_message());
                        }
                    });

                    auto r = to_gstring(hi::to_string(std::wstring_view(wstr_c)));
                    hi_log_debug("get_text_from_clipboard '{}'", to_string(r));
                    return {std::move(r)};
                }
                break;

            default:;
            }
        }

        if (GetLastError() != ERROR_SUCCESS) {
            hi_log_error("Could not enumerator clipboard formats: '{}'", get_last_error_message());
        }

        return std::nullopt;
    }

    /** Put text on the clipboard.
     *
     * @note This is part of the window as some operating systems need to know from which window the text was posted.
     * @param text The text to place on the clipboard.
     */
    void put_text_on_clipboard(gstring_view text) const noexcept
    {
        if (not OpenClipboard(win32Window)) {
            // Another application could have the clipboard locked.
            hi_log_info("Could not open win32 clipboard '{}'", get_last_error_message());
            return;
        }

        auto const defer_CloseClipboard = defer([] {
            CloseClipboard();
        });

        if (not EmptyClipboard()) {
            hi_log_error("Could not empty win32 clipboard '{}'", get_last_error_message());
            return;
        }

        auto wtext = hi::to_wstring(unicode_normalize(to_u32string(text), unicode_normalize_config::NFC_CRLF_noctr()));

        auto wtext_handle = GlobalAlloc(GMEM_MOVEABLE, (wtext.size() + 1) * sizeof(wchar_t));
        if (wtext_handle == nullptr) {
            hi_log_error("Could not allocate clipboard data '{}'", get_last_error_message());
            return;
        }

        auto const defer_GlobalFree([&wtext_handle] {
            if (wtext_handle != nullptr) {
                GlobalFree(wtext_handle);
            }
        });

        {
            auto wtext_c = static_cast<wchar_t *>(GlobalLock(wtext_handle));
            if (wtext_c == nullptr) {
                hi_log_error("Could not lock string data '{}'", get_last_error_message());
                return;
            }

            auto const defer_GlobalUnlock = defer([wtext_handle] {
                if (not GlobalUnlock(wtext_handle) and GetLastError() != ERROR_SUCCESS) {
                    hi_log_error("Could not unlock string data '{}'", get_last_error_message());
                }
            });

            std::memcpy(wtext_c, wtext.c_str(), (wtext.size() + 1) * sizeof(wchar_t));
        }

        if (SetClipboardData(CF_UNICODETEXT, wtext_handle) == nullptr) {
            hi_log_error("Could not set clipboard data '{}'", get_last_error_message());
            return;
        } else {
            // Data was transferred to clipboard.
            wtext_handle = nullptr;
        }
    }

    [[nodiscard]] translate2 window_to_screen() const noexcept
    {
        return translate2{rectangle.left(), rectangle.bottom()};
    }

    [[nodiscard]] translate2 screen_to_window() const noexcept
    {
        return ~window_to_screen();
    }

    /** Process the event.
     *
     * This is called by the event handler to start processing events.
     * The events are translated and then uses `send_event_to_widget()` to send the
     * events to the widgets in some priority ordering.
     *
     * It may also be called from within the `event_handle()` of widgets.
     */
    bool handle_event(gui_event event) noexcept
    {
        using enum gui_event_type;

        hi_axiom(loop::main().on_thread());

        switch (event.type()) {
        case window_minimize:
            set_size_state(gui_window_size::minimized);
            return true;

        case window_maximize:
            set_size_state(gui_window_size::maximized);
            return true;

        case window_normalize:
            set_size_state(gui_window_size::normal);
            return true;

        case window_close:
            close_window();
            return true;

        case window_open_sysmenu:
            open_system_menu();
            return true;

        case window_set_keyboard_target:
            {
                auto const& target = event.keyboard_target();
                if (target.widget_id == 0) {
                    update_keyboard_target(target.group, target.direction);
                } else if (target.direction == keyboard_focus_direction::here) {
                    update_keyboard_target(target.widget_id, target.group);
                } else {
                    update_keyboard_target(target.widget_id, target.group, target.direction);
                }
            }
            return true;

        case window_set_clipboard:
            put_text_on_clipboard(event.clipboard_data());
            return true;

        case mouse_exit_window: // Mouse left window.
            update_mouse_target({});
            break;

        case mouse_up:
        case mouse_drag:
        case mouse_down:
        case mouse_move:
            event.mouse().hitbox = _widget->hitbox_test(event.mouse().position);
            if (event == mouse_down or event == mouse_move) {
                update_mouse_target(event.mouse().hitbox.widget_id, event.mouse().position);
            }
            if (event == mouse_down) {
                update_keyboard_target(event.mouse().hitbox.widget_id, keyboard_focus_group::all);
            }
            break;

        default:;
        }

        // Translate keyboard events, using the keybindings.
        auto events = std::vector<gui_event>{event};
        if (event.type() == keyboard_down) {
            for (auto& e : translate_keyboard_event(event)) {
                events.push_back(e);
            }
        }

        for (auto& event_ : events) {
            if (event_.type() == gui_event_type::text_edit_paste) {
                // The text-edit-paste operation was generated by keyboard bindings,
                // it needs the actual text to be pasted added.
                if (auto optional_text = get_text_from_clipboard()) {
                    event_.clipboard_data() = *optional_text;
                }
            }
        }

        // Send the event to the correct widget.
        auto const handled = send_events_to_widget(
            events.front().variant() == gui_event_variant::mouse ? _mouse_target_id : _keyboard_target_id, events);

        // Intercept the keyboard generated escape.
        // A keyboard generated escape should always remove keyboard focus.
        // The update_keyboard_target() function will send gui_keyboard_exit and a
        // potential duplicate gui_cancel messages to all widgets that need it.
        for (auto const event_ : events) {
            if (event_ == gui_cancel) {
                update_keyboard_target({}, keyboard_focus_group::all);
            }
        }

        return handled;
    }

private:
    constexpr static UINT_PTR move_and_resize_timer_id = 2;
    constexpr static std::chrono::nanoseconds _animation_duration = std::chrono::milliseconds(150);

    inline static bool _first_window = true;
    inline static const wchar_t *win32WindowClassName = nullptr;
    inline static WNDCLASSW win32WindowClass = {};
    inline static bool win32WindowClassIsRegistered = false;
    inline static bool firstWindowHasBeenOpened = false;

    /** The label of the window that is passed to the operating system.
     */
    label _title;

    /** The widget covering the complete window.
     */
    std::unique_ptr<widget_intf> _widget;

    box_constraints _widget_constraints = {};

    aarectangle _redraw_rectangle = aarectangle{};
    bool _restyle = false;
    bool _resize = false;
    bool _reconstrain = false;
    bool _relayout = false;

    /** Current size state of the window.
     */
    gui_window_size _size_state = gui_window_size::normal;

    /** When the window is minimized, maximized or made full-screen the original size is stored here.
     */
    aarectangle _restore_rectangle;

    /** The time of the last forced redraw.
     * A forced redraw may happen when needing to draw outside
     * of the event-loop. For example when win32 moving or resizing
     * the event-loop is stuck, so forced redraws are happening on
     * the WM_MOVING and WM_SIZING messages that are generated outside
     * the event loop, but on the same thread as the event loop.
     */
    utc_nanoseconds last_forced_redraw = {};

    /** Target of the mouse
     * Since any mouse event will change the target this is used
     * to check if the target has changed, to send exit events to the previous mouse target.
     */
    widget_id _mouse_target_id;

    /** Target of the keyboard
     * widget where keyboard events are sent to.
     */
    widget_id _keyboard_target_id;

    TRACKMOUSEEVENT track_mouse_leave_event_parameters;
    bool tracking_mouse_leave_event = false;
    char32_t high_surrogate = 0;
    gui_event mouse_button_event;
    utc_nanoseconds multi_click_time_point;
    point2 multi_click_position;
    uint8_t multi_click_count;

    bool keymenu_pressed = false;

    callback<void()> _setting_change_cbt;
    callback<void(std::string)> _selected_theme_cbt;
    callback<void(utc_nanoseconds, bool)> _render_cbt;

    /** Send event to a target widget.
     *
     * The commands are send in order, until the command is handled, then processing stops immediately.
     * All commands are tried in a batch to the following handlers:
     *  - The target widget
     *  - The parents of the widget up to and including the root widget.
     *  - The window itself.
     */
    bool send_events_to_widget(widget_id target_id, std::vector<gui_event> const& events) noexcept
    {
        if (target_id == 0) {
            // If there was no target, send the event to the window's widget.
            target_id = _widget->id();
        }

        auto target_widget = get_if(_widget.get(), target_id, false);
        while (target_widget) {
            // Each widget will try to handle the first event it can.
            for (auto const& event : events) {
                if (target_widget->handle_event(target_widget->layout().from_window * event)) {
                    return true;
                }
            }

            // Forward the events to the parent of the target.
            target_widget = target_widget->parent();
        }

        return false;
    }

    void setOSWindowRectangleFromRECT(RECT new_rectangle) noexcept
    {
        hi_axiom(loop::main().on_thread());

        // Convert bottom to y-axis up coordinate system.
        auto const inv_bottom = os_settings::primary_monitor_rectangle().height() - new_rectangle.bottom;

        auto const new_screen_rectangle = aarectangle{
            narrow_cast<float>(new_rectangle.left),
            narrow_cast<float>(inv_bottom),
            narrow_cast<float>(new_rectangle.right - new_rectangle.left),
            narrow_cast<float>(new_rectangle.bottom - new_rectangle.top)};

        if (rectangle.size() != new_screen_rectangle.size()) {
            ++global_counter<"gui_window:os-resize:relayout">;
            request_relayout();
        }

        rectangle = new_screen_rectangle;
    }

    [[nodiscard]] keyboard_state get_keyboard_state() noexcept
    {
        auto r = keyboard_state::idle;

        if (GetKeyState(VK_CAPITAL) != 0) {
            r |= keyboard_state::caps_lock;
        }
        if (GetKeyState(VK_NUMLOCK) != 0) {
            r |= keyboard_state::num_lock;
        }
        if (GetKeyState(VK_SCROLL) != 0) {
            r |= keyboard_state::scroll_lock;
        }
        return r;
    }

    [[nodiscard]] keyboard_modifiers get_keyboard_modifiers() noexcept
    {
        // Documentation of GetAsyncKeyState() says that the held key is in the most-significant-bit.
        // Make sure it is signed, so that we can do a less-than 0 check. It looks like this function
        // was designed to be used this way.
        static_assert(std::is_signed_v<decltype(GetAsyncKeyState(VK_SHIFT))>);

        auto r = keyboard_modifiers::none;

        if (GetAsyncKeyState(VK_SHIFT) < 0) {
            r |= keyboard_modifiers::shift;
        }
        if (GetAsyncKeyState(VK_CONTROL) < 0) {
            r |= keyboard_modifiers::control;
        }
        if (GetAsyncKeyState(VK_MENU) < 0) {
            r |= keyboard_modifiers::alt;
        }
        if (GetAsyncKeyState(VK_LWIN) < 0 or GetAsyncKeyState(VK_RWIN) < 0) {
            r |= keyboard_modifiers::super;
        }

        return r;
    }

    [[nodiscard]] char32_t handle_suragates(char32_t c) noexcept
    {
        hi_axiom(loop::main().on_thread());

        if (c >= 0xd800 && c <= 0xdbff) {
            high_surrogate = ((c - 0xd800) << 10) + 0x10000;
            return 0;

        } else if (c >= 0xdc00 && c <= 0xdfff) {
            c = high_surrogate ? high_surrogate | (c - 0xdc00) : 0xfffd;
        }
        high_surrogate = 0;
        return c;
    }

    [[nodiscard]] gui_event create_mouse_event(unsigned int uMsg, uint64_t wParam, int64_t lParam) noexcept
    {
        hi_axiom(loop::main().on_thread());

        auto r = gui_event{gui_event_type::mouse_move};
        r.keyboard_modifiers = get_keyboard_modifiers();
        r.keyboard_state = get_keyboard_state();

        auto const x = narrow_cast<float>(GET_X_LPARAM(lParam));
        auto const y = narrow_cast<float>(GET_Y_LPARAM(lParam));

        // Convert to y-axis up coordinate system, y is in window-local.
        auto const inv_y = rectangle.height() - y;

        // On Window 7 up to and including Window10, the I-beam cursor hot-spot is 2 pixels to the left
        // of the vertical bar. But most applications do not fix this problem.
        r.mouse().position = point2{x, inv_y};
        r.mouse().wheel_delta = {};
        if (uMsg == WM_MOUSEWHEEL) {
            r.mouse().wheel_delta.y() = GET_WHEEL_DELTA_WPARAM(wParam) * 10.0f / WHEEL_DELTA;
        } else if (uMsg == WM_MOUSEHWHEEL) {
            r.mouse().wheel_delta.x() = GET_WHEEL_DELTA_WPARAM(wParam) * 10.0f / WHEEL_DELTA;
        }

        // Track which buttons are down, in case the application wants to track multiple buttons being pressed down.
        r.mouse().down.left_button = (GET_KEYSTATE_WPARAM(wParam) & MK_LBUTTON) > 0;
        r.mouse().down.middle_button = (GET_KEYSTATE_WPARAM(wParam) & MK_MBUTTON) > 0;
        r.mouse().down.right_button = (GET_KEYSTATE_WPARAM(wParam) & MK_RBUTTON) > 0;
        r.mouse().down.x1_button = (GET_KEYSTATE_WPARAM(wParam) & MK_XBUTTON1) > 0;
        r.mouse().down.x2_button = (GET_KEYSTATE_WPARAM(wParam) & MK_XBUTTON2) > 0;

        // Check which buttons caused the mouse event.
        switch (uMsg) {
        case WM_LBUTTONUP:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            r.mouse().cause.left_button = true;
            break;
        case WM_RBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
            r.mouse().cause.right_button = true;
            break;
        case WM_MBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
            r.mouse().cause.middle_button = true;
            break;
        case WM_XBUTTONUP:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONDBLCLK:
            r.mouse().cause.x1_button = (GET_XBUTTON_WPARAM(wParam) & XBUTTON1) > 0;
            r.mouse().cause.x2_button = (GET_XBUTTON_WPARAM(wParam) & XBUTTON2) > 0;
            break;
        case WM_MOUSEMOVE:
            if (mouse_button_event == gui_event_type::mouse_down) {
                r.mouse().cause = mouse_button_event.mouse().cause;
            }
            break;
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
        case WM_MOUSELEAVE:
            break;
        default:
            hi_no_default();
        }

        auto const a_button_is_pressed = r.mouse().down.left_button or r.mouse().down.middle_button or r.mouse().down.right_button or
            r.mouse().down.x1_button or r.mouse().down.x2_button;

        switch (uMsg) {
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        case WM_XBUTTONUP:
            r.set_type(gui_event_type::mouse_up);
            if (mouse_button_event) {
                r.mouse().down_position = mouse_button_event.mouse().down_position;
            }
            r.mouse().click_count = 0;

            if (!a_button_is_pressed) {
                ReleaseCapture();
            }
            break;

        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_XBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_XBUTTONDOWN:
            {
                auto const within_double_click_time = r.time_point - multi_click_time_point < os_settings::double_click_interval();
                auto const double_click_distance =
                    std::sqrt(narrow_cast<float>(squared_hypot(r.mouse().position - multi_click_position)));
                auto const within_double_click_distance = double_click_distance < os_settings::double_click_distance();

                multi_click_count = within_double_click_time and within_double_click_distance ? multi_click_count + 1 : 1;
                multi_click_time_point = r.time_point;
                multi_click_position = r.mouse().position;

                r.set_type(gui_event_type::mouse_down);
                r.mouse().down_position = r.mouse().position;
                r.mouse().click_count = multi_click_count;

                // Track draging past the window borders.
                hi_assert_not_null(win32Window);
                SetCapture(win32Window);
            }
            break;

        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
            r.set_type(gui_event_type::mouse_wheel);
            break;

        case WM_MOUSEMOVE:
            {
                // XXX Make sure the mouse is moved enough for this to cause a drag event.
                r.set_type(a_button_is_pressed ? gui_event_type::mouse_drag : gui_event_type::mouse_move);
                if (mouse_button_event) {
                    r.mouse().down_position = mouse_button_event.mouse().down_position;
                    r.mouse().click_count = mouse_button_event.mouse().click_count;
                }
            }
            break;

        case WM_MOUSELEAVE:
            r.set_type(gui_event_type::mouse_exit_window);
            if (mouse_button_event) {
                r.mouse().down_position = mouse_button_event.mouse().down_position;
            }
            r.mouse().click_count = 0;

            // After this event we need to ask win32 to track the mouse again.
            tracking_mouse_leave_event = false;

            // Force current_mouse_cursor to None so that the Window is in a fresh
            // state when the mouse reenters it.
            current_mouse_cursor = mouse_cursor::None;
            break;

        default:
            hi_no_default();
        }

        // Make sure we start tracking mouse events when the mouse has entered the window again.
        // So that once the mouse leaves the window we receive a WM_MOUSELEAVE event.
        if (not tracking_mouse_leave_event and uMsg != WM_MOUSELEAVE) {
            auto *track_mouse_leave_event_parameters_p = &track_mouse_leave_event_parameters;
            if (not TrackMouseEvent(track_mouse_leave_event_parameters_p)) {
                hi_log_error("Could not track leave event '{}'", get_last_error_message());
            }
            tracking_mouse_leave_event = true;
        }

        // Remember the last time a button was pressed or released, so that we can convert
        // a move into a drag event.
        if (r == gui_event_type::mouse_down or r == gui_event_type::mouse_up or r == gui_event_type::mouse_exit_window) {
            mouse_button_event = r;
        }

        return r;
    }

    /** Create a window at a position on the virtual-screen.
     * 
     * We can not know the DPI of the window before creating it at a position
     * in the virtual screen. Use show_window() to complete the creation of the window.
     * 
     * @param position The position of the window on the virtual screen.
    */
    void create_window(point2 position)
    {
        // This function should be called during init(), and therefor should not have a lock on the window.
        hi_assert(loop::main().on_thread());

        createWindowClass();

        auto u16title = to_wstring(std::format("{}", _title));

        hi_log_info("Create window with title '{}'", _title);

        // Recommended to set the dpi-awareness before opening any window.
        SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        // We are opening a popup window with a caption bar to cause drop-shadow to appear around
        // the window.
        win32Window = CreateWindowExW(
            0, // Optional window styles.
            win32WindowClassName, // Window class
            u16title.data(), // Window text
            WS_OVERLAPPEDWINDOW, // Window style
            // Size and position
            round_cast<int>(position.x()),
            round_cast<int>(position.y()),
            0, // Width: we don't know the DPI so we can't calculate the width.
            0, // height: we don't know the DPI so we can't calculate the width.
            NULL, // Parent window
            NULL, // Menu
            reinterpret_cast<HINSTANCE>(crt_application_instance), // Instance handle
            this);
        if (win32Window == nullptr) {
            hi_log_fatal("Could not open a win32 window: {}", get_last_error_message());
        }

        // Now we extend the drawable area over the titlebar and and border, excluding the drop shadow.
        // At least one value needs to be postive for the drop-shadow to be rendered.
        MARGINS m{0, 0, 0, 1};
        DwmExtendFrameIntoClientArea(win32Window, &m);

        // Force WM_NCCALCSIZE to be send to the window.
        SetWindowPos(
            win32Window, nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

        if (not firstWindowHasBeenOpened) {
            auto const win32_window_ = win32Window;
            switch (gui_window_size::normal) {
            case gui_window_size::normal:
                ShowWindow(win32_window_, SW_SHOWNORMAL);
                break;
            case gui_window_size::minimized:
                ShowWindow(win32_window_, SW_SHOWMINIMIZED);
                break;
            case gui_window_size::maximized:
                ShowWindow(win32_window_, SW_SHOWMAXIMIZED);
                break;
            default:
                hi_no_default();
            }
            firstWindowHasBeenOpened = true;
        }

        track_mouse_leave_event_parameters.cbSize = sizeof(track_mouse_leave_event_parameters);
        track_mouse_leave_event_parameters.dwFlags = TME_LEAVE;
        track_mouse_leave_event_parameters.hwndTrack = win32Window;
        track_mouse_leave_event_parameters.dwHoverTime = HOVER_DEFAULT;

        auto ppi_ = GetDpiForWindow(win32Window);
        if (ppi_ == 0) {
            throw gui_error("Could not retrieve dpi for window.");
        }
        pixel_density = {unit::pixels_per_inch(ppi_), os_settings::device_type()};
        surface = make_unique_gfx_surface(crt_application_instance, win32Window);
    }

    /** Complete the creation of the window by showing it.
     * 
     * @param size The size of the window.
     */
    void show_window(extent2 size) noexcept
    {
        hi_log_info("Show window with title '{}' with size {}", _title, size);
        SetWindowPos(
            win32Window, nullptr, 0, 0, round_cast<int>(size.width()), round_cast<int>(size.width()), SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOREDRAW | SWP_DEFERERASE | SWP_NOCOPYBITS | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

        //ShowWindow(win32Window, SW_SHOW);
    }

    int windowProc(unsigned int uMsg, uint64_t wParam, int64_t lParam) noexcept
    {
        using namespace std::chrono_literals;

        gui_event mouse_event;
        auto const current_time = std::chrono::utc_clock::now();

        switch (uMsg) {
        case WM_CLOSE:
            // WM_DESTROY is handled inside `_windowProc` since it has to deal with lifetime of `this`.
            break;

        case WM_DESTROY:
            // WM_DESTROY is handled inside `_windowProc` since it has to deal with lifetime of `this`.
            break;

        case WM_CREATE:
            {
                auto const createstruct_ptr = std::launder(std::bit_cast<CREATESTRUCT *>(lParam));
                RECT new_rectangle;
                new_rectangle.left = createstruct_ptr->x;
                new_rectangle.top = createstruct_ptr->y;
                new_rectangle.right = createstruct_ptr->x + createstruct_ptr->cx;
                new_rectangle.bottom = createstruct_ptr->y + createstruct_ptr->cy;
                setOSWindowRectangleFromRECT(new_rectangle);
            }
            break;

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
            {
                auto const height = [this]() {
                    hi_axiom(loop::main().on_thread());
                    return rectangle.height();
                }();

                PAINTSTRUCT ps;
                BeginPaint(win32Window, &ps);

                auto const update_rectangle = aarectangle{
                    narrow_cast<float>(ps.rcPaint.left),
                    narrow_cast<float>(height - ps.rcPaint.bottom),
                    narrow_cast<float>(ps.rcPaint.right - ps.rcPaint.left),
                    narrow_cast<float>(ps.rcPaint.bottom - ps.rcPaint.top)};

                {
                    hi_axiom(loop::main().on_thread());
                    request_redraw(update_rectangle);
                }

                EndPaint(win32Window, &ps);
            }
            break;

        case WM_NCPAINT:
            hi_axiom(loop::main().on_thread());
            request_redraw_window();
            break;

        case WM_SIZE:
            // This is called when the operating system is changing the size of the window.
            // However we do not support maximizing by the OS.
            hi_axiom(loop::main().on_thread());
            switch (wParam) {
            case SIZE_MAXIMIZED:
                ShowWindow(win32Window, SW_RESTORE);
                set_size_state(gui_window_size::maximized);
                break;
            case SIZE_MINIMIZED:
                _size_state = gui_window_size::minimized;
                break;
            case SIZE_RESTORED:
                _size_state = gui_window_size::normal;
                break;
            default:
                break;
            }
            break;

        case WM_TIMER:
            if (last_forced_redraw + 16.7ms < current_time) {
                // During sizing the event loop is blocked.
                // Render at about 60fps.
                loop::main().resume_once();
                last_forced_redraw = current_time;
            }
            break;

        case WM_SIZING:
            {
                auto const& rect_ptr = *std::launder(std::bit_cast<RECT *>(lParam));
                if (rect_ptr.right < rect_ptr.left or rect_ptr.bottom < rect_ptr.top) {
                    hi_log_error(
                        "Invalid RECT received on WM_SIZING: left={}, right={}, bottom={}, top={}",
                        rect_ptr.left,
                        rect_ptr.right,
                        rect_ptr.bottom,
                        rect_ptr.top);

                } else {
                    setOSWindowRectangleFromRECT(rect_ptr);
                }
            }
            break;

        case WM_MOVING:
            {
                auto const& rect_ptr = *std::launder(std::bit_cast<RECT *>(lParam));
                if (rect_ptr.right < rect_ptr.left or rect_ptr.bottom < rect_ptr.top) {
                    hi_log_error(
                        "Invalid RECT received on WM_MOVING: left={}, right={}, bottom={}, top={}",
                        rect_ptr.left,
                        rect_ptr.right,
                        rect_ptr.bottom,
                        rect_ptr.top);

                } else {
                    setOSWindowRectangleFromRECT(rect_ptr);
                }
            }
            break;

        case WM_WINDOWPOSCHANGED:
            {
                auto const windowpos_ptr = std::launder(std::bit_cast<WINDOWPOS *>(lParam));
                RECT new_rectangle;
                new_rectangle.left = windowpos_ptr->x;
                new_rectangle.top = windowpos_ptr->y;
                new_rectangle.right = windowpos_ptr->x + windowpos_ptr->cx;
                new_rectangle.bottom = windowpos_ptr->y + windowpos_ptr->cy;
                setOSWindowRectangleFromRECT(new_rectangle);
            }
            break;

        case WM_ENTERSIZEMOVE:
            hi_axiom(loop::main().on_thread());
            if (SetTimer(win32Window, move_and_resize_timer_id, 16, NULL) != move_and_resize_timer_id) {
                hi_log_error("Could not set timer before move/resize. {}", get_last_error_message());
            }
            resizing = true;
            break;

        case WM_EXITSIZEMOVE:
            hi_axiom(loop::main().on_thread());
            if (not KillTimer(win32Window, move_and_resize_timer_id)) {
                hi_log_error("Could not kill timer after move/resize. {}", get_last_error_message());
            }
            resizing = false;
            // After a manual move of the window, it is clear that the window is in normal mode.
            _restore_rectangle = rectangle;
            _size_state = gui_window_size::normal;
            request_redraw_window();
            break;

        case WM_ACTIVATE:
            hi_axiom(loop::main().on_thread());
            switch (wParam) {
            case 1: // WA_ACTIVE
            case 2: // WA_CLICKACTIVE
                this->handle_event({gui_event_type::window_activate});
                break;
            case 0: // WA_INACTIVE
                this->handle_event({gui_event_type::window_deactivate});
                break;
            default:
                hi_log_error("Unknown WM_ACTIVE value.");
            }
            ++global_counter<"gui_window:WM_ACTIVATE:constrain">;
            request_reconstrain();
            break;

        case WM_GETMINMAXINFO:
            {
                hi_axiom(loop::main().on_thread());
                auto const minmaxinfo = std::launder(std::bit_cast<MINMAXINFO *>(lParam));
                minmaxinfo->ptMaxSize.x = round_cast<LONG>(_widget_constraints.maximum.width());
                minmaxinfo->ptMaxSize.y = round_cast<LONG>(_widget_constraints.maximum.height());
                minmaxinfo->ptMinTrackSize.x = round_cast<LONG>(_widget_constraints.minimum.width());
                minmaxinfo->ptMinTrackSize.y = round_cast<LONG>(_widget_constraints.minimum.height());
                minmaxinfo->ptMaxTrackSize.x = round_cast<LONG>(_widget_constraints.maximum.width());
                minmaxinfo->ptMaxTrackSize.y = round_cast<LONG>(_widget_constraints.maximum.height());
            }
            break;

        case WM_UNICHAR:
            if (auto c = char_cast<char32_t>(wParam); c == UNICODE_NOCHAR) {
                // Tell the 3rd party keyboard handler application that we support WM_UNICHAR.
                return 1;

            } else if (auto const gc = ucd_get_general_category(c); not is_C(gc) and not is_M(gc)) {
                // Only pass code-points that are non-control and non-mark.
                handle_event(gui_event::keyboard_grapheme(grapheme{c}));
            }
            break;

        case WM_DEADCHAR:
            if (auto c = handle_suragates(char_cast<char32_t>(wParam))) {
                if (auto const gc = ucd_get_general_category(c); not is_C(gc) and not is_M(gc)) {
                    // Only pass code-points that are non-control and non-mark.
                    handle_event(gui_event::keyboard_partial_grapheme(grapheme{c}));
                }
            }
            break;

        case WM_CHAR:
            if (auto c = handle_suragates(char_cast<char32_t>(wParam))) {
                if (auto const gc = ucd_get_general_category(c); not is_C(gc) and not is_M(gc)) {
                    // Only pass code-points that are non-control and non-mark.
                    handle_event(gui_event::keyboard_grapheme(grapheme{c}));
                }
            }
            break;

        case WM_SYSCOMMAND:
            if (wParam == SC_KEYMENU) {
                keymenu_pressed = true;
                handle_event(gui_event{gui_event_type::keyboard_down, keyboard_virtual_key::menu});
                return 0;
            }
            break;

        case WM_KEYDOWN:
        case WM_KEYUP:
            {
                auto const extended = (narrow_cast<uint32_t>(lParam) & 0x01000000) != 0;
                auto const key_code = narrow_cast<int>(wParam);
                auto const key_modifiers = get_keyboard_modifiers();
                auto virtual_key = to_keyboard_virtual_key(key_code, extended, key_modifiers);

                if (std::exchange(keymenu_pressed, false) and uMsg == WM_KEYDOWN and virtual_key == keyboard_virtual_key::space) {
                    // On windows, Alt followed by Space opens the menu of the window, which is called the system menu.
                    virtual_key = keyboard_virtual_key::sysmenu;
                }

                if (virtual_key != keyboard_virtual_key::nul) {
                    auto const key_state = get_keyboard_state();
                    auto const event_type = uMsg == WM_KEYDOWN ? gui_event_type::keyboard_down : gui_event_type::keyboard_up;
                    handle_event(gui_event{event_type, virtual_key, key_modifiers, key_state});
                }
            }
            break;

        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_XBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        case WM_XBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_XBUTTONDBLCLK:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
        case WM_MOUSEMOVE:
        case WM_MOUSELEAVE:
            keymenu_pressed = false;
            handle_event(create_mouse_event(uMsg, wParam, lParam));
            break;

        case WM_NCCALCSIZE:
            if (wParam == TRUE) {
                // When wParam is TRUE, simply returning 0 without processing the NCCALCSIZE_PARAMS rectangles
                // will cause the client area to resize to the size of the window, including the window frame.
                // This will remove the window frame and caption items from your window, leaving only the client area displayed.
                //
                // Starting with Windows Vista, removing the standard frame by simply
                // returning 0 when the wParam is TRUE does not affect frames that are
                // extended into the client area using the DwmExtendFrameIntoClientArea function.
                // Only the standard frame will be removed.
                return 0;
            }

            break;

        case WM_NCHITTEST:
            {
                hi_axiom(loop::main().on_thread());

                auto const x = narrow_cast<float>(GET_X_LPARAM(lParam));
                auto const y = narrow_cast<float>(GET_Y_LPARAM(lParam));

                // Convert to y-axis up coordinate system.
                auto const inv_y = os_settings::primary_monitor_rectangle().height() - y;

                auto const hitbox_type = _widget->hitbox_test(screen_to_window() * point2{x, inv_y}).type;

                switch (hitbox_type) {
                case hitbox_type::bottom_resize_border:
                    set_cursor(mouse_cursor::None);
                    return HTBOTTOM;
                case hitbox_type::top_resize_border:
                    set_cursor(mouse_cursor::None);
                    return HTTOP;
                case hitbox_type::left_resize_border:
                    set_cursor(mouse_cursor::None);
                    return HTLEFT;
                case hitbox_type::right_resize_border:
                    set_cursor(mouse_cursor::None);
                    return HTRIGHT;
                case hitbox_type::bottom_left_resize_corner:
                    set_cursor(mouse_cursor::None);
                    return HTBOTTOMLEFT;
                case hitbox_type::bottom_right_resize_corner:
                    set_cursor(mouse_cursor::None);
                    return HTBOTTOMRIGHT;
                case hitbox_type::top_left_resize_corner:
                    set_cursor(mouse_cursor::None);
                    return HTTOPLEFT;
                case hitbox_type::top_right_resize_corner:
                    set_cursor(mouse_cursor::None);
                    return HTTOPRIGHT;
                case hitbox_type::application_icon:
                    set_cursor(mouse_cursor::None);
                    return HTSYSMENU;
                case hitbox_type::move_area:
                    set_cursor(mouse_cursor::None);
                    return HTCAPTION;
                case hitbox_type::text_edit:
                    set_cursor(mouse_cursor::TextEdit);
                    return HTCLIENT;
                case hitbox_type::button:
                    set_cursor(mouse_cursor::Button);
                    return HTCLIENT;
                case hitbox_type::scroll_bar:
                    set_cursor(mouse_cursor::Default);
                    return HTCLIENT;
                case hitbox_type::_default:
                    set_cursor(mouse_cursor::Default);
                    return HTCLIENT;
                case hitbox_type::outside:
                    set_cursor(mouse_cursor::None);
                    return HTCLIENT;
                default:
                    hi_no_default();
                }
            }
            break;

        case WM_SETTINGCHANGE:
            hi_axiom(loop::main().on_thread());
            os_settings::gather();
            break;

        case WM_DPICHANGED:
            {
                hi_axiom(loop::main().on_thread());
                // x-axis dpi value.
                pixel_density = {unit::pixels_per_inch(LOWORD(wParam)), os_settings::device_type()};

                // Use the recommended rectangle to resize and reposition the window
                auto const new_rectangle = std::launder(reinterpret_cast<RECT *>(lParam));
                SetWindowPos(
                    win32Window,
                    NULL,
                    new_rectangle->left,
                    new_rectangle->top,
                    new_rectangle->right - new_rectangle->left,
                    new_rectangle->bottom - new_rectangle->top,
                    SWP_NOZORDER | SWP_NOACTIVATE);
                ++global_counter<"gui_window:WM_DPICHANGED:constrain">;
                request_reconstrain();

                // XXX #667 use mp-units formatting.
                hi_log_info("DPI has changed to {} ppi", pixel_density.ppi.in(unit::pixels_per_inch));

                hi_assert_not_null(_widget);
                request_restyle();
            }
            break;

        default:
            break;
        }

        // Let DefWindowProc() handle it.
        return -1;
    }

    /** The win32 window message handler.
     * This function should not take any locks as _WindowProc is called recursively.
     */
    static LRESULT CALLBACK _WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
    {
        if (uMsg == WM_CREATE && lParam) {
            auto const createData = std::launder(std::bit_cast<CREATESTRUCT *>(lParam));

            SetLastError(0);
            auto r = SetWindowLongPtrW(hwnd, GWLP_USERDATA, std::bit_cast<LONG_PTR>(createData->lpCreateParams));
            if (r != 0 || GetLastError() != 0) {
                hi_log_fatal("Could not set GWLP_USERDATA on window. '{}'", get_last_error_message());
            }
        }

        // It is assumed that GWLP_USERDATA is zero when the window is created. Because messages to
        // this window are send before WM_CREATE and there is no way to figure out to which actual window
        // these messages belong.
        auto window_userdata = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (window_userdata == 0) {
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }

        auto& window = *std::launder(std::bit_cast<gui_window *>(window_userdata));
        hi_axiom(loop::main().on_thread());

        // WM_CLOSE and WM_DESTROY will re-enter and run the destructor for `window`.
        // We can no longer call virtual functions on the `window` object.
        if (uMsg == WM_CLOSE) {
            // Listeners can close the window by calling the destructor on `window`.
            window.closing();
            return 0;

        } else if (uMsg == WM_DESTROY) {
            // Remove the window now, before DefWindowProc, which could recursively
            // Reuse the window as it is being cleaned up.
            SetLastError(0);
            auto r = SetWindowLongPtrW(hwnd, GWLP_USERDATA, NULL);
            if (r == 0 || GetLastError() != 0) {
                hi_log_fatal("Could not set GWLP_USERDATA on window. '{}'", get_last_error_message());
            }

            // Also remove the win32Window from the window, so that we don't get double DestroyWindow().
            window.win32Window = nullptr;
            return 0;

        } else {
            if (auto result = window.windowProc(uMsg, wParam, lParam); result != -1) {
                return result;
            }
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }

    static void createWindowClass()
    {
        if (!win32WindowClassIsRegistered) {
            // Register the window class.
            win32WindowClassName = L"HikoGUI Window Class";

            std::memset(&win32WindowClass, 0, sizeof(WNDCLASSW));
            win32WindowClass.style = CS_DBLCLKS;
            win32WindowClass.lpfnWndProc = _WindowProc;
            win32WindowClass.hInstance = static_cast<HINSTANCE>(crt_application_instance);
            win32WindowClass.lpszClassName = win32WindowClassName;
            win32WindowClass.hCursor = nullptr;
            RegisterClassW(&win32WindowClass);
        }
        win32WindowClassIsRegistered = true;
    }
};

bool widget_intf::send_to_window(gui_event const& event) const noexcept
{
    if (auto w = window()) {
        return w->handle_event(event);
    } else {
        // Pretend the event was handled, even though there is no window.
        return true;
    }
}

void widget_intf::request_restyle() const noexcept
{
    if (auto *w = window()) {
        w->request_restyle();
    }
}

void widget_intf::request_resize() const noexcept
{
    if (auto *w = window()) {
        w->request_resize();
    }
}

void widget_intf::request_reconstrain() const noexcept
{
    if (auto *w = window()) {
        w->request_reconstrain();
    }
}

void widget_intf::request_relayout() const noexcept
{
    if (auto *w = window()) {
        w->request_relayout();
    }
}

void widget_intf::request_redraw() const noexcept
{
    if (auto *w = window()) {
        w->request_redraw(layout().clipping_rectangle_on_window());
    }
}

void widget_intf::request_redraw_window() const noexcept
{
    if (auto *w = window()) {
        w->request_redraw_window();
    }
}


} // namespace hi::inline v1
