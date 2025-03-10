// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../color/color.hpp"
#include "../utility/utility.hpp"
#include "../font/font.hpp"
#include "../container/container.hpp"
#include "../macros.hpp"
#include <ostream>
#include <vector>
#include <algorithm>

hi_export_module(hikogui.text : text_style);

hi_export namespace hi::inline v1 {
/** The text-style that a run-of-text must be displayed in.
 */
class text_style {
public:
    constexpr text_style() noexcept = default;
    text_style(text_style const&) noexcept = default;
    text_style(text_style&&) noexcept = default;
    text_style& operator=(text_style const&) noexcept = default;
    text_style& operator=(text_style&&) noexcept = default;
    [[nodiscard]] friend bool operator==(text_style const&, text_style const&) noexcept = default;

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        uint32_t tmp = 0;
        tmp |= _font_valid;
        tmp |= _color_valid;
        tmp |= _size_valid;
        tmp |= _line_spacing_valid;
        tmp |= _paragraph_spacing_valid;
        return not tmp;
    }

    [[nodiscard]] constexpr bool complete() const noexcept
    {
        uint32_t tmp = 1;
        tmp &= _font_valid;
        tmp &= _color_valid;
        tmp &= _size_valid;
        tmp &= _line_spacing_valid;
        tmp &= _paragraph_spacing_valid;
        return tmp;
    }

    [[nodiscard]] lean_vector<font_id> const& font_chain() const
    {
        hi_axiom(_font_valid);
        return _font_chain;
    }

    /** Set the font-chain for this text style.
     *
     * The new chain is prepended in front of the current chain. Unless
     * @a important is set, in which case the font-chain is replaced.
     *
     * @param font_chain The chain of fonts to search glyphs in.
     */
    void set_font_chain(lean_vector<font_id> font_chain, bool important = false)
    {
        if (important or not _font_important) {
            _font_important |= static_cast<uint32_t>(important);
            _font_valid = 1;
            if (important) {
                _font_chain = std::move(font_chain);
            } else {
                _font_chain.insert(_font_chain.begin(), font_chain.begin(), font_chain.end());
            }
        }
    }

    [[nodiscard]] constexpr hi::color color() const
    {
        hi_axiom(_color_valid);
        return _color;
    }

    constexpr void set_color(hi::color color, bool important = false)
    {
        if (important or not _color_important) {
            _color_important |= static_cast<uint32_t>(important);
            _color_valid = 1;
            _color = color;
        }
    }

    [[nodiscard]] constexpr unit::font_size_s size() const
    {
        hi_axiom(_size_valid);
        return _size;
    }

    constexpr void set_size(unit::font_size_s size, bool important = false)
    {
        if (important or not _size_important) {
            _size_important |= static_cast<uint32_t>(important);
            _size_valid = 1;
            _size = size;
        }
    }

    [[nodiscard]] constexpr float line_spacing() const
    {
        hi_axiom(_line_spacing_valid);
        return _line_spacing;
    }

    constexpr void set_line_spacing(float line_spacing, bool important = false)
    {
        if (important or not _line_spacing_important) {
            _line_spacing_important |= static_cast<uint32_t>(important);
            _line_spacing_valid = 1;
            _line_spacing = line_spacing;
        }
    }

    [[nodiscard]] constexpr float paragraph_spacing() const
    {
        hi_axiom(_paragraph_spacing_valid);
        return _paragraph_spacing;
    }

    constexpr void set_paragraph_spacing(float paragraph_spacing, bool important = false)
    {
        if (important or not _paragraph_spacing_important) {
            _paragraph_spacing_important |= static_cast<uint32_t>(important);
            _paragraph_spacing_valid = 1;
            _paragraph_spacing = paragraph_spacing;
        }
    }

    void clear() noexcept
    {
        *this = text_style{};
    }

    /** Apply the given text-style on top of this style.
     */
    void apply(text_style const& other) noexcept
    {
        if (other._font_valid) {
            set_font_chain(other._font_chain, other._font_important);
        }

        if (other._color_valid) {
            set_color(other._color, other._color_important);
        }

        if (other._size_valid) {
            set_size(other._size, other._size_important);
        }

        if (other._line_spacing_valid) {
            set_line_spacing(other._line_spacing, other._line_spacing_important);
        }

        if (other._paragraph_spacing_valid) {
            set_paragraph_spacing(other._paragraph_spacing, other._paragraph_spacing_important);
        }
    }

private:
    lean_vector<font_id> _font_chain = {};
    hi::color _color = {};
    unit::font_size_s _size = {};
    float _line_spacing = 1.0f;
    float _paragraph_spacing = 1.5f;

    uint32_t _color_valid : 1 = 0;
    uint32_t _color_important : 1 = 0;
    uint32_t _font_valid : 1 = 0;
    uint32_t _font_important : 1 = 0;
    uint32_t _size_valid : 1 = 0;
    uint32_t _size_important : 1 = 0;
    uint32_t _line_spacing_valid : 1 = 0;
    uint32_t _line_spacing_important : 1 = 0;
    uint32_t _paragraph_spacing_valid : 1 = 0;
    uint32_t _paragraph_spacing_important : 1 = 0;
};

} // namespace hi::inline v1
