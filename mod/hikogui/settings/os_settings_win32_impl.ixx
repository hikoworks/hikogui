// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include "../macros.hpp"
#include "../win32_headers.hpp"



export module hikogui_settings_os_settings : impl;
import : intf;
import hikogui_path;
import hikogui_telemetry;
import hikogui_utility;
import hikogui_win32;

export namespace hi { inline namespace v1 {

[[nodiscard]] std::vector<uuid> os_settings::preferred_gpus(hi::policy performance_policy) noexcept
{
    auto r = std::vector<uuid>{};

    auto actual_policy = os_settings::gpu_policy();
    if (actual_policy == hi::policy::unspecified) {
        actual_policy = performance_policy;
    }
    if (actual_policy == hi::policy::unspecified) {
        actual_policy = policy();
    }
    hilet actual_policy_ = actual_policy == hi::policy::low_power ? DXGI_GPU_PREFERENCE_MINIMUM_POWER :
        actual_policy == hi::policy::high_performance             ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE :
                                                                    DXGI_GPU_PREFERENCE_UNSPECIFIED;

    IDXGIFactory *factory = nullptr;
    if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void **)&factory))) {
        hi_log_error("Could not IDXGIFactory. {}", get_last_error_message());
        return r;
    }
    hi_assert_not_null(factory);
    hilet d1 = defer([&] {
        factory->Release();
    });

    IDXGIFactory6 *factory6 = nullptr;
    if (FAILED(factory->QueryInterface(__uuidof(IDXGIFactory6), (void **)&factory6))) {
        hi_log_error("Could not IDXGIFactory::QueryInterface(IDXGIFactory6). {}", get_last_error_message());
        return r;
    }
    hi_assert_not_null(factory6);
    hilet d2 = defer([&] {
        factory6->Release();
    });

    IDXGIAdapter1 *adapter = nullptr;
    for (UINT i = 0;
         SUCCEEDED(factory6->EnumAdapterByGpuPreference(i, actual_policy_, __uuidof(IDXGIAdapter1), (void **)&adapter));
         ++i) {
        hilet d3 = defer([&] {
            adapter->Release();
        });

        DXGI_ADAPTER_DESC1 description;
        if (FAILED(adapter->GetDesc1(&description))) {
            hi_log_error("Could not IDXGIAdapter1::GetDesc1(). {}", get_last_error_message());
            return r;
        }

        static_assert(sizeof(description.AdapterLuid) <= sizeof(uuid));
        r.emplace_back();
        std::memcpy(std::addressof(r.back()), std::addressof(description.AdapterLuid), sizeof(description.AdapterLuid));
    }

    return r;
}

/**
 * GetUserPreferredUILanguages() returns at most two of the selected languages in random order
 * and can not be used to retrieve the preferred languages the user has selected.
 *
 * The winrt GlobalizationPreferences::Languages returns all languages in the correct order.
 * However winrt header files are incompatible with c++20 co-routines.
 *
 * Therefor the only option available is to read the language list from the registry.
 */
[[nodiscard]] std::vector<language_tag> os_settings::gather_languages() noexcept
{
    auto r = std::vector<language_tag>{};

    // The official APIs to get the current languages do not work.
    // Either they return the languages in a random order.
    // Or they return only three languages, but not necessarily the first three
    // Or they do not update at runtime.
    // The only way that works is to get the registry from the Control Panel application.
    if (hilet languages = win32_RegGetValue<std::vector<std::string>>(
            HKEY_CURRENT_USER, "Control Panel\\International\\User Profile", "Languages")) {
        r.reserve(languages->size());
        for (hilet& language : *languages) {
            r.push_back(language_tag{language});
        }
    } else {
        hi_log_error("Could not read languages: {}", std::error_code{languages.error()}.message());
        r.push_back(language_tag{"en"});
    }

    return r;
}

[[nodiscard]] std::expected<std::locale, std::error_code> os_settings::gather_locale() noexcept
{
    if (auto name = win32_GetUserDefaultLocaleName()) {
        return std::locale(*name);

    } else {
        return std::unexpected{std::error_code{name.error()}};
    }
}

[[nodiscard]] bool os_settings::gather_left_to_right() noexcept
{
    if (auto locale = gather_locale()) {
        try {
            // The locale name on windows is the <language-tag>.<collation>.
            auto locale_name = locale->name();

            // Strip off the optional collation algorithm.
            if (auto i = locale_name.find('.'); i != locale_name.npos) {
                locale_name = locale_name.substr(0, i);
            }

            auto locale_tag = language_tag{locale->name()};

            // Expanding will complete the script part of the tag
            // that is needed to get left-to-right.
            return locale_tag.expand().left_to_right();

        } catch (...) {
            // The locale-name may be different from the language-tag
            // So fallthrough here.
        }
    }

    // Use the left-to-right direction of the first configured language.
    if (auto languages = gather_languages(); not languages.empty()) {
        return languages.front().expand().left_to_right();
    }

    // Most languages are left-to-right so it is a good guess.
    return true;
}

[[nodiscard]] hi::theme_mode os_settings::gather_theme_mode()
{
    if (hilet result = win32_RegGetValue<uint32_t>(
            HKEY_CURRENT_USER,
            "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            "AppsUseLightTheme")) {
        return *result ? theme_mode::light : theme_mode::dark;

    } else {
        hi_log_error("Could not read theme mode: {}", std::error_code{result.error()}.message());
        return theme_mode::light;
    }
}

[[nodiscard]] hi::subpixel_orientation os_settings::gather_subpixel_orientation()
{
    {
        BOOL has_font_smoothing;
        if (not SystemParametersInfoW(SPI_GETFONTSMOOTHING, 0, &has_font_smoothing, 0)) {
            throw os_error(std::format("Could not get system parameter SPI_GETFONTSMOOTHING: {}", get_last_error_message()));
        }

        if (has_font_smoothing == FALSE) {
            // Font smoothing is disabled.
            return hi::subpixel_orientation::unknown;
        }
    }

    {
        UINT font_smooth_type;
        if (not SystemParametersInfoW(SPI_GETFONTSMOOTHINGTYPE, 0, &font_smooth_type, 0)) {
            throw os_error(std::format("Could not get system parameter SPI_GETFONTSMOOTHINGTYPE: {}", get_last_error_message()));
        }

        if (font_smooth_type != FE_FONTSMOOTHINGCLEARTYPE) {
            // Font smoothing is not clear type.
            return hi::subpixel_orientation::unknown;
        }
    }

    {
        BOOL has_clear_type;
        if (not SystemParametersInfoW(SPI_GETCLEARTYPE, 0, &has_clear_type, 0)) {
            throw os_error(std::format("Could not get system parameter SPI_GETCLEARTYPE: {}", get_last_error_message()));
        }

        if (has_clear_type == FALSE) {
            // ClearType is disabled.
            return hi::subpixel_orientation::unknown;
        }
    }

    {
        UINT font_smooth_orientation;
        if (not SystemParametersInfoW(SPI_GETFONTSMOOTHINGORIENTATION, 0, &font_smooth_orientation, 0)) {
            throw os_error(
                std::format("Could not get system parameter SPI_GETFONTSMOOTHINGORIENTATION: {}", get_last_error_message()));
        }

        if (font_smooth_orientation == FE_FONTSMOOTHINGORIENTATIONBGR) {
            // Font smoothing is not clear type.
            return hi::subpixel_orientation::horizontal_bgr;
        } else if (font_smooth_orientation == FE_FONTSMOOTHINGORIENTATIONRGB) {
            // Font smoothing is not clear type.
            return hi::subpixel_orientation::horizontal_rgb;
        } else {
            throw os_error(std::format("Unknown result from SPI_GETFONTSMOOTHINGORIENTATION: {}", font_smooth_orientation));
        }
    }
}

[[nodiscard]] bool os_settings::gather_uniform_HDR()
{
    // Microsoft Windows 10 switches display mode when getting a HDR surface
    // The switching causes all application to display using a different color and brightness calibration.
    return false;
}

[[nodiscard]] std::chrono::milliseconds os_settings::gather_double_click_interval()
{
    return std::chrono::milliseconds{GetDoubleClickTime()};
}

[[nodiscard]] float os_settings::gather_double_click_distance()
{
    hilet width = GetSystemMetrics(SM_CXDOUBLECLK);
    if (width <= 0) {
        throw os_error("Could not retrieve SM_CXDOUBLECLK");
    }

    hilet height = GetSystemMetrics(SM_CYDOUBLECLK);
    if (height <= 0) {
        throw os_error("Could not retrieve SM_CYDOUBLECLK");
    }

    hilet diameter = std::max(width, height);
    return diameter * 0.5f;
}

[[nodiscard]] std::chrono::milliseconds os_settings::gather_keyboard_repeat_delay()
{
    using namespace std::literals::chrono_literals;

    INT r;
    if (not SystemParametersInfoW(SPI_GETKEYBOARDDELAY, 0, &r, 0)) {
        throw os_error(std::format("Could not get system parameter SPI_GETKEYBOARDDELAY: {}", get_last_error_message()));
    }

    // SPI_GETKEYBOARDDELAY values are between 0 (250ms) to 3 (1s).
    constexpr auto bias = 250ms;
    constexpr auto gain = 250ms;

    return bias + r * gain;
}

[[nodiscard]] std::chrono::milliseconds os_settings::gather_keyboard_repeat_interval()
{
    using namespace std::literals::chrono_literals;

    INT r;
    if (not SystemParametersInfoW(SPI_GETKEYBOARDSPEED, 0, &r, 0)) {
        throw os_error(std::format("Could not get system parameter SPI_GETKEYBOARDSPEED: {}", get_last_error_message()));
    }

    // SPI_GETKEYBOARDSPEED values are between 0 (2.5 per/sec) to 31 (30 per/sec).
    constexpr auto bias = 2.5f;
    constexpr auto gain = 0.887f;
    hilet rate = bias + r * gain;
    return std::chrono::duration_cast<std::chrono::milliseconds>(1000ms / rate);
}

[[nodiscard]] std::chrono::milliseconds os_settings::gather_cursor_blink_interval()
{
    using namespace std::literals::chrono_literals;

    hilet r = GetCaretBlinkTime();
    if (r == 0) {
        throw os_error(std::format("Could not get caret blink time: {}", get_last_error_message()));

    } else if (r == INFINITE) {
        return std::chrono::milliseconds::max();

    } else {
        // GetGaretBlinkTime() gives the time for a half-period.
        return std::chrono::milliseconds{r} * 2;
    }
}

[[nodiscard]] std::chrono::milliseconds os_settings::gather_cursor_blink_delay()
{
    // The blink delay is not available in the OS, we can use the keyboard repeat delay.
    return std::max(gather_keyboard_repeat_delay(), gather_keyboard_repeat_interval());
}

[[nodiscard]] float os_settings::gather_minimum_window_width()
{
    hilet width = GetSystemMetrics(SM_CXMINTRACK);
    if (width == 0) {
        throw os_error("Could not retrieve SM_CXMINTRACK");
    }
    return narrow_cast<float>(width);
}

[[nodiscard]] float os_settings::gather_minimum_window_height()
{
    hilet height = GetSystemMetrics(SM_CYMINTRACK);
    if (height == 0) {
        throw os_error("Could not retrieve SM_CYMINTRACK");
    }

    return narrow_cast<float>(height);
}

[[nodiscard]] float os_settings::gather_maximum_window_width()
{
    hilet width = GetSystemMetrics(SM_CXMAXTRACK);
    if (width == 0) {
        throw os_error("Could not retrieve SM_CXMAXTRACK");
    }
    return narrow_cast<float>(width);
}

[[nodiscard]] float os_settings::gather_maximum_window_height()
{
    hilet height = GetSystemMetrics(SM_CYMAXTRACK);
    if (height == 0) {
        throw os_error("Could not retrieve SM_CYMAXTRACK");
    }

    return narrow_cast<float>(height);
}

[[nodiscard]] uintptr_t os_settings::gather_primary_monitor_id()
{
    hilet origin = POINT{0, 0};
    hilet monitor = MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY);
    return std::bit_cast<uintptr_t>(monitor);
}

[[nodiscard]] aarectangle os_settings::gather_primary_monitor_rectangle()
{
    hilet width = GetSystemMetrics(SM_CXSCREEN);
    if (width == 0) {
        throw os_error("Could not retrieve SM_CXSCREEN");
    }

    hilet height = GetSystemMetrics(SM_CYSCREEN);
    if (height == 0) {
        throw os_error("Could not retrieve SM_CYSCREEN");
    }

    // The origin of the primary monitor is also the origin of the desktop.
    return aarectangle{extent2{narrow_cast<float>(width), narrow_cast<float>(height)}};
}

[[nodiscard]] aarectangle os_settings::gather_desktop_rectangle()
{
    hilet primary_monitor_height = GetSystemMetrics(SM_CYSCREEN);
    if (primary_monitor_height == 0) {
        throw os_error("Could not retrieve SM_CYSCREEN");
    }

    hilet left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    hilet top = GetSystemMetrics(SM_YVIRTUALSCREEN);

    hilet width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    if (width == 0) {
        throw os_error("Could not retrieve SM_CXVIRTUALSCREEN");
    }

    hilet height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (height == 0) {
        throw os_error("Could not retrieve SM_CYVIRTUALSCREEN");
    }

    hilet bottom = top + height;

    // Calculate the bottom as compared to a y-axis up coordinate system.
    hilet inv_bottom = primary_monitor_height - bottom; // 0, 600
    return aarectangle{
        narrow_cast<float>(left), narrow_cast<float>(inv_bottom), narrow_cast<float>(width), narrow_cast<float>(height)};
}

[[nodiscard]] policy os_settings::gather_gpu_policy()
{
    using namespace std::literals;

    hilet executable_path = executable_file().string();
    hilet user_gpu_preferences_key = "Software\\Microsoft\\DirectX\\UserGpuPreferences";

    if (hilet result = win32_RegGetValue<std::string>(HKEY_CURRENT_USER, user_gpu_preferences_key, executable_path)) {
        for (auto entry : std::views::split(std::string_view{*result}, ";"sv)) {
            auto entry_sv = std::string_view{entry};
            if (entry_sv.starts_with("GpuPreference=")) {
                if (entry_sv.ends_with("=0")) {
                    return policy::unspecified;
                } else if (entry_sv.ends_with("=1")) {
                    return policy::low_power;
                } else if (entry_sv.ends_with("=2")) {
                    return policy::high_performance;
                } else {
                    hi_log_error("Unexpected GpuPreference value \"{}\".", entry_sv);
                    return policy::unspecified;
                }
            }
        }

        hi_log_error("Could not find GpuPreference entry.");
        return policy::unspecified;

    } else if (result.error() == win32_error::file_not_found) {
        return policy::unspecified;

    } else{
        hi_log_error("Could not read gpu profile policy: {}", std::error_code{result.error()}.message());
        return policy::unspecified;
    }
}

}} // namespace hi::v1
