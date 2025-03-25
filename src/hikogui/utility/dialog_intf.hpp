// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../macros.hpp"
#include <string_view>
#include <iostream>
#include <format>
#include <concepts>
#include <expected>
#include <system_error>

hi_export_module(hikogui.utility.dialog : intf);

hi_export namespace hi {
inline namespace v1 {

enum class dialog_button {no, yes, cancel, ok, retry, _continue};

template<std::integral T>
[[nodiscard]] constexpr unsigned long long operator<<(T const &lhs, dialog_button const &rhs) noexcept
{
    return 1ULL << std::to_underlying(rhs);
}

enum class dialog_button_mask : uint64_t {
    no = 1 << dialog_button::no,
    yes = 1 << dialog_button::yes,

    cancel = 1 << dialog_button::cancel,

    /** A dialog box with just a "ok" button.
     * 
     * There was a serious error, but the user can only accept the dialog as
     * a notification.
     * 
     * @note Valid on windows.
     */
    ok = 1 << dialog_button::ok,
    retry = 1 << dialog_button::retry,
    _continue = 1 << dialog_button::_continue,

    /** A dialog box with "cancel", "retry" and "continue" buttons.
     * 
     * There was a serious error, but the user has some interation:
     *  - "cancel": Cancel the processing the list of jobs.
     *  - "retry": Retry the current job and when successful continue with the list of jobs.
     *  - "continue": Skip the current job and continue with the next job in the list.
     * 
     * @note Valid on windows.
     */
    cancel_retry_continue = cancel | retry | _continue,

    /** A dialog box with "yes" and "no" buttons.
     * 
     * An important yes/no question:
     *  - "yes"
     *  - "no"
     * 
     * @note Valid on windows.
     */
    yes_no = yes | no,

    /** A dialog box with "ok" and "cancel" buttons.
     * 
     * About to perform a dangerous operation:
     *  - "ok": Perform the dangerous operation.
     *  - "cancel": Cancel the dangerous operation.
     * 
     * @note Valid on windows.
     */
    ok_cancel = ok | cancel,

    /** A dialog box with "retry" and "cancel" buttons.
     * 
     * A error during processing:
     *  - "retry": Retry the operation.
     *  - "cancel": Cancel the operation.
     * 
     * @note Valid on windows.
     */
    retry_cancel = retry | cancel,

    /** A dialog box with "yes", "no" and "cancel" buttons.
     * 
     * An important yes/no question, but we can still cancel:
     *  - "yes"
     *  - "no"
     *  - "cancel": Cancel the operation
     * 
     * @note Valid on windows.
     */
    yes_no_cancel = yes | no | cancel,
};

[[nodiscard]] constexpr bool to_bool(dialog_button_mask const &rhs) noexcept
{
    return std::to_underlying(rhs) != 0;
}

[[nodiscard]] constexpr dialog_button_mask operator&(dialog_button_mask const &lhs, dialog_button_mask const &rhs) noexcept
{
    return static_cast<dialog_button_mask>(std::to_underlying(lhs) & std::to_underlying(rhs));
}

[[nodiscard]] constexpr dialog_button_mask operator|(dialog_button_mask const &lhs, dialog_button_mask const &rhs) noexcept
{
    return static_cast<dialog_button_mask>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

[[nodiscard]] constexpr dialog_button_mask &operator&=(dialog_button_mask &lhs, dialog_button_mask const &rhs) noexcept
{
    return lhs = lhs & rhs;
}

[[nodiscard]] constexpr dialog_button_mask &operator|=(dialog_button_mask &lhs, dialog_button_mask const &rhs) noexcept
{
    return lhs = lhs | rhs;
}

[[nodiscard]] constexpr dialog_button_mask operator&(dialog_button_mask const &lhs, dialog_button const &rhs) noexcept
{
    return lhs & dialog_button_mask{1 << rhs};
}

[[nodiscard]] constexpr dialog_button_mask operator|(dialog_button_mask const &lhs, dialog_button const &rhs) noexcept
{
    return lhs | dialog_button_mask{1 << rhs};
}

[[nodiscard]] constexpr dialog_button_mask &operator&=(dialog_button_mask &lhs, dialog_button const &rhs) noexcept
{
    return lhs = lhs & rhs;
}

[[nodiscard]] constexpr dialog_button_mask &operator|=(dialog_button_mask &lhs, dialog_button const &rhs) noexcept
{
    return lhs = lhs | rhs;
}

/** Display a modal dialog.
 * You should never display a modal dialog unless it is absolutely
 * necessary for the user to respond right now, or on a catastrophic failure.
 *
 * @param title The title of the dialog window.
 * @param text The text to display in the dialog window.
 * @param button_mask The set of buttons to show in the dialog.
 * @return The button that was pressed by the user.
 */
std::expected<dialog_button, std::error_code> dialog(std::string_view title, std::string_view text, dialog_button_mask button_mask = dialog_button_mask::ok) noexcept;

} // namespace v1
} // namespace hi::inline v1
