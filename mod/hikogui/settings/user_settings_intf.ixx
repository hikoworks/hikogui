

module;
#include "../macros.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <cstdlib>
#include <expected>
#include <system_error>

export module hikogui_settings_user_settings : intf;
import hikogui_utility;

export namespace hi { inline namespace v1 {

[[nodiscard]] std::expected<std::string, std::error_code> get_user_setting_string(std::string_view key) noexcept;
[[nodiscard]] std::expected<long long, std::error_code> get_user_setting_integral(std::string_view key) noexcept;

/** Get a user-setting for the application.
 *
 * Keys starting with two consecutive underscores are reserved by the HikoGUI library.
 *
 * @param key A key for the user setting.
 * @return The value from the user-settings.
 * @retval std::nullopt The key was not found in the user-settings.
 * @throws std::invalid_argument When the key is not valid.
 * @throws std::out_of_range if the value in the default does not fit in the return value.
 */
template<typename T>
[[nodiscard]] std::expected<T, std::error_code> get_user_setting(std::string_view key) noexcept = delete;

/** Set a user-setting for the application.
 *
 * Keys starting with two consecutive underscores are reserved by the HikoGUI library.
 *
 * @param key A key for the user setting.
 * @param value The value to set.
 * @throws std::invalid_argument When the key is not valid.
 * @throws std::out_of_range if the value in the default does not fit in the return value.
 */
std::error_code set_user_setting(std::string_view key, std::string_view value) noexcept;

/** Set a user-setting for the application.
 *
 * Keys starting with two consecutive underscores are reserved by the HikoGUI library.
 *
 * @param key A key for the user setting.
 * @param value The value to set.
 * @throws std::invalid_argument When the key is not valid.
 * @throws std::out_of_range if the value in the default does not fit in the return value.
 */
std::error_code set_user_setting(std::string_view key, long long value) noexcept;

/** Delete a user-setting for the application.
 *
 * Keys starting with two consecutive underscores are reserved by the HikoGUI library.
 *
 * @param key A key for the user setting.
 * @throws std::invalid_argument When the key is not valid.
 * @throws std::out_of_range if the value in the default does not fit in the return value.
 */
std::error_code delete_user_setting(std::string_view key) noexcept;

/** Delete all user-setting for the application.
 */
std::error_code delete_user_settings() noexcept;

template<>
[[nodiscard]] std::expected<std::string, std::error_code> get_user_setting(std::string_view key) noexcept
{
    return get_user_setting_string(key);
}

template<std::integral T>
[[nodiscard]] std::expected<T, std::error_code> get_user_setting(std::string_view key) noexcept
{
    if (hilet value = get_user_setting_integral(key)) {
        if (can_narrow_cast<T>(*value)) {
            return narrow_cast<T>(*value);
        } else {
            return std::unexpected{std::make_error_code(std::errc::result_out_of_range)};
        }
    } else {
        return std::unexpected{std::error_code{value.error()}};
    }
}

}} // namespace hi::v1
