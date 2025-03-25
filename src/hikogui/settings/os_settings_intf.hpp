// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "theme_mode.hpp"
#include "subpixel_orientation.hpp"
#include "../i18n/i18n.hpp"
#include "../geometry/geometry.hpp"
#include "../utility/utility.hpp"
#include "../numeric/numeric.hpp"
#include "../observer/observer.hpp"
#include "../dispatch/dispatch.hpp"
#include "../concurrency/concurrency.hpp"
#include "../telemetry/telemetry.hpp"
#include "../time/time.hpp"
#include "../char_maps/char_maps.hpp" // XXX #616
#include "../macros.hpp"
#include <vector>
#include <mutex>
#include <expected>
#include <system_error>
#include <chrono>
#include <atomic>

hi_export_module(hikogui.settings.os_settings : intf);

hi_export namespace hi::inline v1 {

class os_settings {
public:
    /** Get the device this application is running on.
     * 
     * This is used to detect the viewing distance from the screen and
     * combined with the screen's DPI to determine how device independent
     * pixels (dips) are scaled.
     * 
     * If you need to know how to layout the application based on the device
     * type use `device_mode()`; as some operating systems allow emulation or
     * switching of device-mode during run-time. 
     */
    [[nodiscard]] static device_type device_type() noexcept
    {
        hi_axiom(_populated.load(std::memory_order::acquire));
        return _device_type;
    }

    // [[nodiscard]] static device_type device_mode() noexcept

    /** Get the language tags for the configured languages.
     *
     * @return A list of language tags in order of priority. The tags included
     *         are for all variants of the languages configured in the operating
     *         system, in order of specificity.
     */
    [[nodiscard]] static std::vector<language_tag> language_tags() noexcept
    {
        hi_axiom(_populated.load(std::memory_order::acquire));
        auto const lock = std::scoped_lock(_mutex);
        return _language_tags;
    }

    /** Get the current local.
     *
     * @return The current locale.
     */
    [[nodiscard]] static std::locale locale() noexcept
    {
        hi_axiom(_populated.load(std::memory_order::acquire));
        auto const lock = std::scoped_lock(_mutex);
        return _locale;
    }


    /** Check if the configured writing direction is left-to-right.
     *
     * @retval true If the writing direction is left-to-right.
     */
    [[nodiscard]] static bool left_to_right() noexcept
    {
        hi_axiom(_populated.load(std::memory_order::acquire));
        return _left_to_right.load(std::memory_order::relaxed);
    }

    /** Get the alignment based on the writing direction.
     * 
     * @param rhs The alignment that may need to be flipped to match the writing direction.
     * @retval The alignment, possibly flipped.
     */
    [[nodiscard]] static hi::alignment alignment(hi::alignment rhs) noexcept
    {
        return left_to_right() ? rhs : mirror(rhs);
    }

    /** Get the alignment based on the writing direction.
     * 
     * @param rhs The alignment that may need to be flipped to match the writing direction.
     * @retval The alignment, possibly flipped.
     */
    [[nodiscard]] static hi::horizontal_alignment alignment(hi::horizontal_alignment rhs) noexcept
    {
        return left_to_right() ? rhs : mirror(rhs);
    }

    /** Get the configured light/dark theme mode
     */
    [[nodiscard]] static hi::theme_mode theme_mode() noexcept
    {
        hi_axiom(_populated.load(std::memory_order::acquire));
        return _theme_mode.load(std::memory_order::relaxed);
    }

    /** Get the configured light/dark theme mode
     */
    [[nodiscard]] static hi::subpixel_orientation subpixel_orientation() noexcept
    {
        hi_axiom(_populated.load(std::memory_order::acquire));
        return _subpixel_orientation.load(std::memory_order::relaxed);
    }

    /** Whether SDR and HDR application can coexists on the same display.
     *
     * Microsoft Windows 10 and at least early versions of Windows 11 will
     * have set this to false, because if an application opens a HDR surface
     * it will switch the display mode to HDR, this switching may cause a
     * significant change in color and brightness of the display, including
     * other (SDR) applications that where already running. This would be
     * surprising for most users and we can not expect users to have calibrated
     * colors to match HDR with SDR.
     */
    [[nodiscard]] static bool uniform_HDR() noexcept
    {
        hi_axiom(_populated.load(std::memory_order::acquire));
        return _uniform_HDR.load(std::memory_order::relaxed);
    }

    /** Get the mouse double click interval.
     */
    [[nodiscard]] static std::chrono::milliseconds double_click_interval() noexcept
    {
        hi_axiom(_populated.load(std::memory_order::acquire));
        return _double_click_interval.load(std::memory_order::relaxed);
    }

    /** Get the distance from the previous mouse position to detect double click.
     */
    [[nodiscard]] static float double_click_distance() noexcept
    {
        hi_axiom(_populated.load(std::memory_order::acquire));
        return _double_click_distance.load(std::memory_order::relaxed);
    }

    /** Get the delay before the keyboard starts repeating.
     *
     * @note Also used to determine the scroll delay when selecting text.
     */
    [[nodiscard]] static std::chrono::milliseconds keyboard_repeat_delay() noexcept
    {
        hi_axiom(_populated.load(std::memory_order::acquire));
        return _keyboard_repeat_delay.load(std::memory_order::relaxed);
    }

    /** Get the keyboard repeat interval
     *
     * @note Also used to determine the scroll speed when selecting text.
     */
    [[nodiscard]] static std::chrono::milliseconds keyboard_repeat_interval() noexcept
    {
        hi_axiom(_populated.load(std::memory_order::acquire));
        return _keyboard_repeat_interval.load(std::memory_order::relaxed);
    }

    /** Get the cursor blink delay.
     *
     * @note This delay is used to determine when to blink after cursor movement.
     */
    [[nodiscard]] static std::chrono::milliseconds cursor_blink_delay() noexcept
    {
        hi_axiom(_populated.load(std::memory_order::acquire));
        return _cursor_blink_delay.load(std::memory_order::relaxed);
    }

    /** Get the cursor blink interval.
     *
     * @note The interval is the complete period of the cursor blink, from on-to-on.
     * @return The cursor blink interval, or `std::chrono::milliseconds::max()` when blinking is turned off.
     */
    [[nodiscard]] static std::chrono::milliseconds cursor_blink_interval() noexcept
    {
        hi_axiom(_populated.load(std::memory_order::acquire));
        return _cursor_blink_interval.load(std::memory_order::relaxed);
    }

    /** The minimum width a window is allowed to be.
     *
     * @return The minimum window width.
     */
    [[nodiscard]] static float minimum_window_width() noexcept
    {
        hi_axiom(_populated.load(std::memory_order::acquire));
        return _minimum_window_width.load(std::memory_order::relaxed);
    }

    /** The minimum height a window is allowed to be.
     *
     * @return The minimum window height.
     */
    [[nodiscard]] static float minimum_window_height() noexcept
    {
        hi_axiom(_populated.load(std::memory_order::acquire));
        return _minimum_window_height.load(std::memory_order::relaxed);
    }

    /** The maximum width a window is allowed to be.
     *
     * @return The maximum window width.
     */
    [[nodiscard]] static float maximum_window_width() noexcept
    {
        hi_axiom(_populated.load(std::memory_order::acquire));
        return _maximum_window_width.load(std::memory_order::relaxed);
    }

    /** The maximum height a window is allowed to be.
     *
     * @return The maximum window height.
     */
    [[nodiscard]] static float maximum_window_height() noexcept
    {
        hi_axiom(_populated.load(std::memory_order::acquire));
        return _maximum_window_height.load(std::memory_order::relaxed);
    }

    /** Get the rectangle of the primary monitor.
     *
     * @return The rectangle describing the size and location inside the desktop.
     */
    [[nodiscard]] static aarectangle primary_monitor_rectangle() noexcept
    {
        hi_axiom(_populated.load(std::memory_order::acquire));
        auto const lock = std::scoped_lock(_mutex);
        return _primary_monitor_rectangle;
    }

    /** Get an opaque id of the primary monitor.
     */
    [[nodiscard]] static uintptr_t primary_monitor_id() noexcept
    {
        hi_axiom(_populated.load(std::memory_order::acquire));
        return _primary_monitor_id.load(std::memory_order::relaxed);
    }

    /** Get the rectangle describing the desktop.
     *
     * @return The bounding rectangle around the desktop. With the origin being equal to the origin of the primary monitor.
     */
    [[nodiscard]] static aarectangle desktop_rectangle() noexcept
    {
        hi_axiom(_populated.load(std::memory_order::acquire));
        auto const lock = std::scoped_lock(_mutex);
        return _desktop_rectangle;
    }

    /** Get the global performance policy.
     *
     * @return The performance policy selected by the operating system.
     */
    [[nodiscard]] static hi::policy policy() noexcept
    {
        return policy::unspecified;
    }

    /** Get the policy for selecting a GPU.
     *
     * @return The performance policy for selecting a GPU.
     */
    [[nodiscard]] static hi::policy gpu_policy() noexcept
    {
        hi_axiom(_populated.load(std::memory_order::acquire));
        return _gpu_policy.load(std::memory_order::relaxed);
    }

    /** Get a list of GPUs ordered best to worst.
     *
     * The performance policy is calculated from several sources,
     * in order from high priority to low priority:
     *  1. os_settings::gpu_policy() if not policy::unspecified.
     *  2. performance_policy argument if not policy::unspecified.
     *  3. os_settings::policy()
     *
     * On win32 the GPU identifiers returned are LUIDs which are smaller
     * then UUIDs. Vulkan specifically includes VkPhysicalDeviceIDProperties::deviceLUID
     * to match with these return values.
     *
     * On other operating systems the return value here is a UUID which will
     * match with VkPhysicalDeviceIDProperties::deviceUUID.
     *
     * Use VkPhysicalDeviceIDProperties::deviceLUIDValid to know which one to match
     * and VK_LUID_SIZE for the size of the comparison.
     *
     * @param performance_policy The performance policy of the application.
     * @return A list of GPU identifiers ordered best to worst.
     */
    [[nodiscard]] static std::vector<uuid> preferred_gpus(hi::policy performance_policy) noexcept;

    /** The number of logical processors in the system.
     *
     * Logical processors include all the physical processors, cores and hyperthreading.
     *
     * @return The number of logical processors.
     */
    [[nodiscard]] static size_t num_logical_processors() noexcept
    {
        hi_axiom(_populated.load(std::memory_order::acquire));
        return _num_logical_processors.load(std::memory_order::relaxed);
    }

    template<forward_of<void()> Func>
    [[nodiscard]] static callback<void()> subscribe(Func &&func, callback_flags flags = callback_flags::synchronous) noexcept
    {
        auto const lock = std::scoped_lock(_mutex);
        return _notifier.subscribe(std::forward<Func>(func), flags);
    }

    /** Get the global os_settings instance.
     *
     * @return True on success.
     */
    static bool start_subsystem() noexcept
    {
        return hi::start_subsystem(_started, false, subsystem_init, subsystem_deinit);
    }

    /** Gather the settings from the operating system now.
     */
    static void gather() noexcept
    {
        auto const lock = std::scoped_lock(_mutex);
        auto setting_has_changed = false;

        auto const current_time = std::chrono::utc_clock::now();
        if (current_time < _gather_last_time + gather_minimum_interval) {
            return;
        }
        _gather_last_time = current_time;

        if (auto optional_device_type = gather_device_type()) {
            if (compare_store(_device_type, *optional_device_type)) {
                setting_has_changed = true;
                hi_log_info("OS device type has changed: {}", *optional_device_type);
            }

        } else {
            hi_log_error("Failed to get device type: {}", optional_device_type.error().message());
        }

        try {
            auto language_tags = gather_languages();
            if (language_tags.empty()) {
                // If not language is configured on the system, use English as default.
                language_tags.emplace_back("en-Latn-US");
            }

            // Add all the variants of languages, for searching into translations.
            language_tags = variants(language_tags);

            if (compare_store(_language_tags, language_tags)) {
                setting_has_changed = true;
                hi_log_info("OS language order has changed: {}", _language_tags);
            }
        } catch (std::exception const& e) {
            hi_log_error("Failed to get OS language: {}", e.what());
        }

        if (auto optional_locale = gather_locale()) {
            if (compare_store(_locale, *optional_locale)) {
                setting_has_changed = true;
                hi_log_info("OS locale has changed: {}", _locale.name());
            }
        } else {
            hi_log_error("Failed to get OS locale: {}", optional_locale.error().message());
        }

        if (compare_store(_left_to_right, gather_left_to_right())) {
            setting_has_changed = true;
            hi_log_info("OS mirrored-GUI has changed: {}", not _left_to_right);
        }

        try {
            if (compare_store(_theme_mode, gather_theme_mode())) {
                setting_has_changed = true;
                hi_log_info("OS theme-mode has changed: {}", _theme_mode.load());
            }
        } catch (std::exception const& e) {
            hi_log_error("Failed to get OS theme-mode: {}", e.what());
        }

        try {
            if (compare_store(_subpixel_orientation, gather_subpixel_orientation())) {
                setting_has_changed = true;
                hi_log_info("OS sub-pixel orientation has changed: {}", _subpixel_orientation.load());
            }
        } catch (std::exception const& e) {
            hi_log_error("Failed to get OS sub-pixel orientation: {}", e.what());
        }

        try {
            if (compare_store(_uniform_HDR, gather_uniform_HDR())) {
                setting_has_changed = true;
                hi_log_info("OS uniform-HDR has changed: {}", _uniform_HDR.load());
            }
        } catch (std::exception const& e) {
            hi_log_error("Failed to get OS uniform-HDR: {}", e.what());
        }

        try {
            if (compare_store(_double_click_interval, gather_double_click_interval())) {
                setting_has_changed = true;
                hi_log_info("OS double click interval has changed: {}", _double_click_interval.load());
            }
        } catch (std::exception const& e) {
            hi_log_error("Failed to get OS double click interval: {}", e.what());
        }

        try {
            if (compare_store(_double_click_distance, gather_double_click_distance())) {
                setting_has_changed = true;
                hi_log_info("OS double click distance has changed: {}", _double_click_distance.load());
            }
        } catch (std::exception const& e) {
            hi_log_error("Failed to get OS double click distance: {}", e.what());
        }

        try {
            if (compare_store(_keyboard_repeat_delay, gather_keyboard_repeat_delay())) {
                setting_has_changed = true;
                hi_log_info("OS keyboard repeat delay has changed: {}", _keyboard_repeat_delay.load());
            }
        } catch (std::exception const& e) {
            hi_log_error("Failed to get OS keyboard repeat delay: {}", e.what());
        }

        try {
            if (compare_store(_keyboard_repeat_interval, gather_keyboard_repeat_interval())) {
                setting_has_changed = true;
                hi_log_info("OS keyboard repeat interval has changed: {}", _keyboard_repeat_interval.load());
            }
        } catch (std::exception const& e) {
            hi_log_error("Failed to get OS keyboard repeat interval: {}", e.what());
        }

        try {
            if (compare_store(_cursor_blink_interval, gather_cursor_blink_interval())) {
                setting_has_changed = true;
                if (_cursor_blink_interval.load() < std::chrono::minutes(1)) {
                    hi_log_info("OS cursor blink interval has changed: {}", _cursor_blink_interval.load());
                } else {
                    hi_log_info("OS cursor blink interval has changed: no-blinking");
                }
            }
        } catch (std::exception const& e) {
            hi_log_error("Failed to get OS cursor blink interval: {}", e.what());
        }

        try {
            if (compare_store(_cursor_blink_delay, gather_cursor_blink_delay())) {
                setting_has_changed = true;
                hi_log_info("OS cursor blink delay has changed: {}", _cursor_blink_delay.load());
            }
        } catch (std::exception const& e) {
            hi_log_error("Failed to get OS cursor blink delay: {}", e.what());
        }

        try {
            if (compare_store(_minimum_window_width, gather_minimum_window_width())) {
                setting_has_changed = true;
                hi_log_info("OS minimum window width has changed: {}", _minimum_window_width.load());
            }
        } catch (std::exception const& e) {
            hi_log_error("Failed to get OS minimum window width: {}", e.what());
        }

        try {
            if (compare_store(_minimum_window_height, gather_minimum_window_height())) {
                setting_has_changed = true;
                hi_log_info("OS minimum window height has changed: {}", _minimum_window_height.load());
            }
        } catch (std::exception const& e) {
            hi_log_error("Failed to get OS minimum window height: {}", e.what());
        }

        try {
            if (compare_store(_maximum_window_width, gather_maximum_window_width())) {
                setting_has_changed = true;
                hi_log_info("OS maximum window width has changed: {}", _maximum_window_width.load());
            }
        } catch (std::exception const& e) {
            hi_log_error("Failed to get OS maximum window width: {}", e.what());
        }

        try {
            if (compare_store(_maximum_window_height, gather_maximum_window_height())) {
                setting_has_changed = true;
                hi_log_info("OS maximum window height has changed: {}", _maximum_window_height.load());
            }
        } catch (std::exception const& e) {
            hi_log_error("Failed to get OS maximum window height: {}", e.what());
        }

        try {
            auto const primary_monitor_id = gather_primary_monitor_id();
            if (compare_store(_primary_monitor_id, primary_monitor_id)) {
                setting_has_changed = true;
                hi_log_info("OS primary monitor id has changed: {}, updating vsync source", primary_monitor_id);
                loop::main().set_vsync_monitor_id(primary_monitor_id);
            }
        } catch (std::exception const& e) {
            hi_log_error("Failed to get OS primary monitor id: {}", e.what());
        }

        try {
            if (compare_store(_primary_monitor_rectangle, gather_primary_monitor_rectangle())) {
                setting_has_changed = true;
                hi_log_info("OS primary monitor rectangle has changed: {}", _primary_monitor_rectangle);
            }
        } catch (std::exception const& e) {
            hi_log_error("Failed to get OS primary monitor rectangle: {}", e.what());
        }

        try {
            if (compare_store(_desktop_rectangle, gather_desktop_rectangle())) {
                setting_has_changed = true;
                hi_log_info("OS desktop rectangle has changed: {}", _desktop_rectangle);
            }
        } catch (std::exception const& e) {
            hi_log_error("Failed to get OS desktop rectangle: {}", e.what());
        }

        try {
            if (compare_store(_gpu_policy, gather_gpu_policy())) {
                setting_has_changed = true;
                hi_log_info("GPU policy has changed: {}", _gpu_policy.load());
            }
        } catch (std::exception const& e) {
            hi_log_error("Failed to get the GPU policy: {}", e.what());
        }

        if (compare_store(_num_logical_processors, gather_num_logical_processors())) {
            setting_has_changed = true;
            hi_log_info("Number of logical processors have changed: {}", _num_logical_processors.load());
        }

        _populated.store(true, std::memory_order::release);
        if (setting_has_changed) {
            _notifier();
        }
    }

private:
    constexpr static std::chrono::duration gather_interval = std::chrono::seconds(5);
    constexpr static std::chrono::duration gather_minimum_interval = std::chrono::seconds(1);

    static inline std::atomic<bool> _started = false;
    static inline std::atomic<bool> _populated = false;
    static inline unfair_mutex _mutex;
    static inline utc_nanoseconds _gather_last_time;

    static inline notifier<void()> _notifier;

    static inline std::atomic<hi::device_type> _device_type = hi::device_type::desktop;
    static inline std::vector<language_tag> _language_tags = {};
    static inline std::locale _locale = std::locale{""};
    static inline std::atomic<bool> _left_to_right = true;
    static inline std::atomic<hi::theme_mode> _theme_mode = theme_mode::dark;
    static inline std::atomic<bool> _uniform_HDR = false;
    static inline std::atomic<hi::subpixel_orientation> _subpixel_orientation = hi::subpixel_orientation::unknown;
    static inline std::atomic<std::chrono::milliseconds> _double_click_interval = std::chrono::milliseconds(500);
    static inline std::atomic<float> _double_click_distance = 4.0f;
    static inline std::atomic<std::chrono::milliseconds> _keyboard_repeat_delay = std::chrono::milliseconds(250);
    static inline std::atomic<std::chrono::milliseconds> _keyboard_repeat_interval = std::chrono::milliseconds(33);
    static inline std::atomic<std::chrono::milliseconds> _cursor_blink_interval = std::chrono::milliseconds(1000);
    static inline std::atomic<std::chrono::milliseconds> _cursor_blink_delay = std::chrono::milliseconds(1000);
    static inline std::atomic<float> _minimum_window_width = 40.0f;
    static inline std::atomic<float> _minimum_window_height = 25.0f;
    static inline std::atomic<float> _maximum_window_width = 1920.0f;
    static inline std::atomic<float> _maximum_window_height = 1080.0f;
    static inline std::atomic<uintptr_t> _primary_monitor_id = 0;
    static inline aarectangle _primary_monitor_rectangle = aarectangle{0.0f, 0.0f, 1920.0f, 1080.0f};
    static inline aarectangle _desktop_rectangle = aarectangle{0.0f, 0.0f, 1920.0f, 1080.0f};
    static inline std::atomic<hi::policy> _gpu_policy = policy::unspecified;
    static inline std::atomic<size_t> _num_logical_processors = 1;

    static inline callback<void()> _gather_cbt;

    [[nodiscard]] static bool subsystem_init() noexcept
    {
        _gather_cbt = loop::timer().repeat_function(gather_interval, [] {
            os_settings::gather();
        });

        return true;
    }

    static void subsystem_deinit() noexcept
    {
        if (_started.exchange(false)) {
            _gather_cbt = nullptr;
        }
    }

    [[nodiscard]] static std::expected<hi::device_type, std::error_code> gather_device_type() noexcept;
    [[nodiscard]] static std::vector<language_tag> gather_languages() noexcept;
    [[nodiscard]] static std::expected<std::locale, std::error_code> gather_locale() noexcept;
    [[nodiscard]] static bool gather_left_to_right() noexcept;
    [[nodiscard]] static hi::theme_mode gather_theme_mode();
    [[nodiscard]] static hi::subpixel_orientation gather_subpixel_orientation();
    [[nodiscard]] static bool gather_uniform_HDR();
    [[nodiscard]] static std::chrono::milliseconds gather_double_click_interval();
    [[nodiscard]] static float gather_double_click_distance();
    [[nodiscard]] static std::chrono::milliseconds gather_keyboard_repeat_delay();
    [[nodiscard]] static std::chrono::milliseconds gather_keyboard_repeat_interval();
    [[nodiscard]] static std::chrono::milliseconds gather_cursor_blink_interval();
    [[nodiscard]] static std::chrono::milliseconds gather_cursor_blink_delay();
    [[nodiscard]] static float gather_minimum_window_width();
    [[nodiscard]] static float gather_minimum_window_height();
    [[nodiscard]] static float gather_maximum_window_width();
    [[nodiscard]] static float gather_maximum_window_height();
    [[nodiscard]] static uintptr_t gather_primary_monitor_id();
    [[nodiscard]] static aarectangle gather_primary_monitor_rectangle();
    [[nodiscard]] static aarectangle gather_desktop_rectangle();
    [[nodiscard]] static hi::policy gather_gpu_policy();
    [[nodiscard]] static size_t gather_num_logical_processors();
};

} // namespace hi::inline v1
