// Copyright Take Vos 2020-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "font_font.hpp"
#include "font_id.hpp"
#include "font_glyph_ids.hpp"
#include "font_family_id.hpp"
#include "true_type_font.hpp"
#include "elusive_icon.hpp"
#include "hikogui_icon.hpp"
#include "../unicode/unicode.hpp"
#include "../geometry/geometry.hpp"
#include "../utility/utility.hpp"
#include "../path/path.hpp"
#include <gsl/gsl>
#include <limits>
#include <array>
#include <new>
#include <atomic>
#include <filesystem>

hi_export_module(hikogui.font : font_book);

hi_export namespace hi::inline v1 {
/** font_book keeps track of multiple fonts.
 * The font_book is instantiated during application startup
 * and is available through Foundation_globals->font_book.
 *
 *
 */
class font_book {
public:
    static font_book& global() noexcept;

    ~font_book() = default;
    font_book(font_book const&) = delete;
    font_book(font_book&&) = delete;
    font_book& operator=(font_book const&) = delete;
    font_book& operator=(font_book&&) = delete;
    font_book() = default;

    /** Register a font.
     * Duplicate registrations will be ignored.
     *
     * When a font file is registered the file will be temporarily opened to read and cache a set of properties:
     *  - The English font Family from the 'name' table.
     *  - The weight, width, slant & design-size from the 'fdsc' table.
     *  - The character map 'cmap' table.
     *
     * @param path Location of font.
     */
    font_id register_font_file(std::filesystem::path const& path)
    {
        if (_fonts.size() >= font_id::empty_value) {
            throw std::overflow_error("Too many fonts registered");
        }

        auto const font_id = hi::font_id{gsl::narrow_cast<font_id::value_type>(_fonts.size())};
        auto const& font = *_fonts.emplace_back(std::make_unique<true_type_font>(font_id, path));

        hi_log_info("Parsed font id={} {}: {}", *font_id, path.string(), to_string(font));

        auto const font_family_id = register_family(font.family_name);
        _font_variants[*font_family_id][font.font_variant()] = font_id;

        return font_id;
    }

    /** Register all fonts found in a directory.
     *
     * @see register_font()
     */
    void register_font_directory(std::filesystem::path const& path)
    {
        auto const font_directory_glob = path / "**" / "*.ttf";
        for (auto const& font_path : glob(font_directory_glob)) {
            auto const t = trace<"font_scan">{};

            try {
                register_font_file(font_path);

            } catch (std::exception const& e) {
                hi_log_error("Failed parsing font at {}: \"{}\"", font_path.string(), e.what());
            }
        }
    }

    /** Find font family id.
     */
    [[nodiscard]] font_family_id find_family(std::string const& family_name) const noexcept
    {
        auto it = _family_names.find(to_lower(family_name));
        if (it == _family_names.end()) {
            return std::nullopt;
        } else {
            return it->second;
        }
    }

    /** Register font family id.
     * If the family already exists the existing family_id is returned.
     */
    [[nodiscard]] font_family_id register_family(std::string_view family_name)
    {
        auto name = to_lower(family_name);

        auto it = _family_names.find(name);
        if (it == _family_names.end()) {
            if (_font_variants.size() >= font_family_id::empty_value) {
                throw std::overflow_error("Too many font-family-ids registered");
            }

            auto const family_id = font_family_id{gsl::narrow_cast<font_family_id::value_type>(_font_variants.size())};
            _font_variants.emplace_back();
            _family_names[name] = family_id;
            return family_id;
        } else {
            return it->second;
        }
    }

    /** Find a font closest to the variant.
     * This function will always return a valid font_id.
     *
     * @param family_id a valid family id.
     * @param variant The variant of the font to select.
     * @return a valid font id.
     */
    [[nodiscard]] font_id find_font(font_family_id family_id, font_variant variant) const noexcept
    {
        hi_assert(family_id);
        hi_assert_bounds(*family_id, _font_variants);

        auto const& variants = _font_variants[*family_id];
        for (auto i : alternatives(variant)) {
            if (auto id = variants[i]) {
                return id;
            }
        }

        // If a family exists, there must be at least one font variant available.
        hi_no_default();
    }

    [[nodiscard]] font& get_font(font_id id) const
    {
        return *_fonts.at(*id);
    }

    /** Find a combination of glyphs matching the given grapheme.
     * This function will find a combination of glyphs matching the grapheme
     * in the selected font.
     *
     * @param font The font to use to find the grapheme in.
     * @param grapheme The Unicode grapheme to find in the font.
     * @return A list of glyphs which matched the grapheme.
     */
    [[nodiscard]] font_glyph_ids find_glyph(font_id font, hi::grapheme grapheme) const noexcept
    {
        auto const& f = get_font(font);

        // First try the selected font.
        if (auto const glyphs = f.find_glyphs(grapheme); not glyphs.empty()) {
            return glyphs;
        }

        // If all everything has failed, use the tofu block of the original font.
        return {font, {glyph_id{0}}};
    }

private:
    /** Table of font_family_ids index using the family-name.
     */
    std::unordered_map<std::string, font_family_id> _family_names;

    /** Different fonts; variants of a family.
     */
    std::vector<std::array<font_id, font_variant::size()>> _font_variants;

    std::vector<std::unique_ptr<font>> _fonts;
};

namespace detail {
inline std::unique_ptr<font_book> font_book_global = nullptr;
}

inline font_book& font_book::global() noexcept
{
    if (not detail::font_book_global) {
        detail::font_book_global = std::make_unique<font_book>();
    }
    return *detail::font_book_global;
}

[[nodiscard]] inline font& get_font(font_id id)
{
    return font_book::global().get_font(id);
}

/** Register a font.
 * Duplicate registrations will be ignored.
 *
 * When a font file is registered the file will be temporarily opened to read and cache a set of properties:
 *  - The English font Family from the 'name' table.
 *  - The weight, width, slant & design-size from the 'fdsc' table.
 *  - The character map 'cmap' table.
 *
 * @param path Location of font.
 */
inline font_id register_font_file(std::filesystem::path const& path)
{
    return font_book::global().register_font_file(path);
}

inline void register_font_directory(std::filesystem::path const& path)
{
    return font_book::global().register_font_directory(path);
}

template<typename Range>
inline void register_font_directories(Range&& range) noexcept
{
    for (auto const& path : range) {
        font_book::global().register_font_directory(path);
    }
}

/** Find font family id.
 */
[[nodiscard]] inline font_family_id find_font_family(std::string const& family_name) noexcept
{
    return font_book::global().find_family(family_name);
}

/** Find a font closest to the variant.
 * This function will always return a valid font_id.
 *
 * @param family_id a valid family id.
 * @param variant The variant of the font to select.
 * @return a valid font id.
 */
[[nodiscard]] inline font_id find_font(font_family_id family_id, font_variant variant = font_variant{}) noexcept
{
    return font_book::global().find_font(family_id, variant);
}

/** Find a font closest to the variant.
 * This function will always return a valid font_id.
 *
 * @param family_name a font family name.
 * @param variant The variant of the font to select.
 * @return A pointer to the loaded font.
 */
[[nodiscard]] inline font_id find_font(std::string const& family_name, font_variant variant = font_variant{}) noexcept
{
    if (auto family_id = find_font_family(family_name)) {
        return find_font(family_id, variant);
    } else {
        return font_id{};
    }
}

/** Find a glyph using the given code-point.
 * This function will find a glyph matching the grapheme in the selected font.
 *
 * @param font The font to use to find the grapheme in.
 * @param grapheme The Unicode grapheme to find in the font.
 * @return A list of glyphs which matched the grapheme.
 */
[[nodiscard]] inline font_glyph_ids find_glyph(font_id font, grapheme grapheme) noexcept
{
    return font_book::global().find_glyph(font, grapheme);
}

[[nodiscard]] inline font_glyph_ids find_glyph(elusive_icon rhs) noexcept
{
    auto const id = find_font("elusiveicons", font_variant{font_weight::medium, font_style::normal});
    hi_assert(not id.empty(), "Could not find Elusive icon font");
    return find_glyph(id, std::to_underlying(rhs));
}

[[nodiscard]] inline font_glyph_ids find_glyph(hikogui_icon rhs) noexcept
{
    auto const id = find_font("Hikogui Icons", font_variant{font_weight::regular, font_style::normal});
    hi_assert(not id.empty(), "Could not find HikoGUI icon font");
    return find_glyph(id, std::to_underlying(rhs));
}

/** Find a combination of glyphs matching the given grapheme.
 *
 * @param g The grapheme to find the glyphs for.
 * @param font_chain The chain of fonts to use to find the glyphs.
 * @return The font and the glyphs that match the grapheme. Or an empty
 *         if the grapheme was not found.
 */
[[nodiscard]] inline font_glyph_ids find_glyphs(grapheme g, lean_vector<font_id> const& font_chain) noexcept
{
    for (auto const& font_id : font_chain) {
        auto const& font = get_font(font_id);
        if (auto ids = font.find_glyphs(g); not ids.empty()) {
            return ids;
        }
    }

    return {};
}

/** Find a combination of glyphs matching the given text.
 *
 * For performance reasons, this function returns the glyphs through an
 * output parameter.
 *
 * @param text A run of graphemes to find the glyphs for.
 * @param font_chain The chain of fonts to use to find the glyphs.
 * @param [out] r The font and the glyphs that match the grapheme.
 */
inline void find_glyphs(
    std::span<grapheme const> graphemes,
    lean_vector<font_id> const& font_chain,
    std::span<font_glyph_ids> r) noexcept
{
    assert(not font_chain.empty());
    assert(graphemes.size() == r.size());

    // First try to find the glyphs for the complete text, using a single font.
    for (auto const& font_id : font_chain) {
        auto const& font = get_font(font_id);

        if (font.find_glyphs(graphemes, r)) {
            return;
        }
    }

    // As a fallback, try different fonts for each grapheme separately.
    // And use the tofu block from the first font in the chain when a grapheme
    // is not found in any of the fonts.
    for (auto i = size_t{0}; i != graphemes.size(); ++i) {
        auto const& g = graphemes[i];
        auto& glyph_ids = r[i];

        glyph_ids = find_glyphs(g, font_chain);
        if (not glyph_ids.empty()) {
            continue;
        }

        // Use the tofu block from the first font in the chain.
        glyph_ids = font_glyph_ids{font_chain.front(), lean_vector{glyph_id{0}}};
    }
}

} // namespace hi::inline v1
