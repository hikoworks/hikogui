// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <string_view>
#include <cstdint>
#include <format>
#include <compare>
#include <string>
#include <string_view>

hi_export_module(hikogui.i18n.iso_15924 : intf);

hi_export namespace hi { inline namespace v1 {

/** ISO-15924 script code.
 * A 4 letter title case script code:
 */
class iso_15924 {
public:
    constexpr iso_15924() noexcept : _v(0) {}
    constexpr iso_15924(iso_15924 const&) noexcept = default;
    constexpr iso_15924(iso_15924&&) noexcept = default;
    constexpr iso_15924& operator=(iso_15924 const&) noexcept = default;
    constexpr iso_15924& operator=(iso_15924&&) noexcept = default;

    constexpr iso_15924(std::integral auto number) : _v(0)
    {
        hi_check_bounds(number, 0, 1000);
        _v = narrow_cast<uint16_t>(number);
    }

    constexpr iso_15924(std::string_view code4);

    /** When any script is allowed.
     */
    [[nodiscard]] constexpr static iso_15924 wildcard() noexcept
    {
        return iso_15924{0};
    }

    /** Common script is used for characters that are common in different scripts.
     *
     * Punctuation for example are of a common script.
     */
    [[nodiscard]] constexpr static iso_15924 common() noexcept
    {
        return iso_15924{998};
    }

    /** Used when the script was not encoded with the text.
     */
    [[nodiscard]] constexpr static iso_15924 uncoded() noexcept
    {
        return iso_15924{999};
    }

    [[nodiscard]] constexpr static iso_15924 inherited() noexcept
    {
        return iso_15924{994};
    }

    constexpr iso_15924(std::in_place_t, uint16_t v) noexcept : _v(v)
    {
        hi_axiom_bounds(_v, 0, 1000);
    }

    [[nodiscard]] constexpr uint16_t const& intrinsic() const noexcept
    {
        return _v;
    }

    [[nodiscard]] constexpr uint16_t& intrinsic() noexcept
    {
        return _v;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return _v == 0;
    }

    constexpr explicit operator bool() const noexcept
    {
        return not empty();
    }

    /** Get the iso-15924 numeric value.
     */
    [[nodiscard]] constexpr uint16_t number() const noexcept
    {
        return _v;
    }

    /** Get the iso-15924 4-letter code.
     */
    [[nodiscard]] constexpr std::string code4() const noexcept;

    /** Get the 4-letter code used by open-type.
     */
    [[nodiscard]] constexpr uint32_t code4_open_type() const noexcept;

    [[nodiscard]] constexpr friend std::string to_string(iso_15924 const &rhs) noexcept
    {
        return rhs.code4();
    }

    /** Is this script written left-to-right.
     */
    [[nodiscard]] constexpr bool left_to_right() const noexcept;

    [[nodiscard]] constexpr friend bool operator==(iso_15924 const& lhs, iso_15924 const& rhs) noexcept = default;
    [[nodiscard]] constexpr friend auto operator<=>(iso_15924 const& lhs, iso_15924 const& rhs) noexcept = default;

    /** Check if rhs matches with lhs.
     *
     * @param lhs The script or wild-card.
     * @param rhs The script.
     * @return True when lhs is a wild-card or when lhs and rhs are equal.
     */
    [[nodiscard]] constexpr friend bool matches(iso_15924 const& lhs, iso_15924 const& rhs) noexcept
    {
        return lhs.empty() or lhs == rhs;
    }

private:
    uint16_t _v;
};

}} // namespace hi::inline v1

hi_export template<>
struct std::hash<hi::iso_15924> {
    [[nodiscard]] size_t operator()(hi::iso_15924 const& rhs) const noexcept
    {
        return std::hash<uint16_t>{}(rhs.number());
    }
};
