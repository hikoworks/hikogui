// Copyright Take Vos 2020-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "keyboard_modifiers.hpp"
#include "keyboard_virtual_key_intf.hpp"
#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <string_view>
#include <unordered_map>
#include <format>
#include <string>

hi_export_module(hikogui.GUI : keyboard_key);

hi_export namespace hi::inline v1 {

/** A key in combination with modifiers.
 * This key is based on the actual symbol on the keyboard.
 */
class keyboard_key {
    /** Which modifiers where used on the key when sending a key.
     */
    keyboard_modifiers modifiers;

    /** ASCII code of the key that was pressed when sending a key.
     *
     * All printable ASCII characters are mapped to the equivalent key on the
     * keyboard, after processing of the shift-key.
     *
     * The following non-printable ASCII characters are used
     */
    keyboard_virtual_key virtual_key;

public:
    constexpr keyboard_key() noexcept : modifiers(keyboard_modifiers::none), virtual_key(keyboard_virtual_key::nul) {}

    constexpr keyboard_key(keyboard_modifiers modifiers, keyboard_virtual_key key) noexcept :
        modifiers(modifiers), virtual_key(key)
    {
    }

    keyboard_key(std::string_view key_combination) : modifiers(keyboard_modifiers::none), virtual_key(keyboard_virtual_key::nul)
    {
        auto const modifiers_and_vkey = split_string_view(key_combination, "+");
        assert(not modifiers_and_vkey.empty());

        auto const end_modifiers = modifiers_and_vkey.cend() - 1;
        for (auto i = modifiers_and_vkey.cbegin(); i != end_modifiers; ++i) {
            modifiers |= to_keyboard_modifiers(*i);
        }

        virtual_key = to_keyboard_virtual_key(modifiers_and_vkey.back());
    }

    std::size_t hash() const noexcept
    {
        return hash_mix(modifiers, virtual_key);
    }

    [[nodiscard]] friend bool operator==(keyboard_key const &lhs, keyboard_key const &rhs) noexcept
    {
        return lhs.modifiers == rhs.modifiers && lhs.virtual_key == rhs.virtual_key;
    }

    [[nodiscard]] friend std::string to_string(keyboard_key const &rhs) noexcept
    {
        return std::format("{}{}", rhs.modifiers, rhs.virtual_key);
    }

    friend std::ostream &operator<<(std::ostream &lhs, keyboard_key const &rhs)
    {
        return lhs << to_string(rhs);
    }
};

} // namespace hi::inline v1

template<>
struct std::hash<hi::keyboard_key> {
    [[nodiscard]] std::size_t operator()(hi::keyboard_key const &rhs) const noexcept
    {
        return rhs.hash();
    }
};
