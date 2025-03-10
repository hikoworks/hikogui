// Copyright Take Vos 2020-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../settings/settings.hpp"
#include "../text/text.hpp"
#include "../utility/utility.hpp"
#include "../color/color.hpp"
#include "../geometry/geometry.hpp"
#include "../codec/codec.hpp"
#include "../theme/theme.hpp"
#include "../macros.hpp"
#include <gsl/gsl>
#include <array>
#include <filesystem>
#include <string>
#include <vector>

hi_export_module(hikogui.GUI : theme);

hi_export namespace hi::inline v1 {
class theme {
public:
    /** The PPI of the size values.
     */
    unit::pixel_density pixel_density = unit::pixel_density{unit::pixels_per_inch(72.0f), device_type::desktop};

    std::string name;
    theme_mode mode = theme_mode::light;

    theme() noexcept = default;
    theme(theme const&) noexcept = default;
    theme(theme&&) noexcept = default;
    theme& operator=(theme const&) noexcept = default;
    theme& operator=(theme&&) noexcept = default;

    /** Open and parse a theme file.
     */
    theme(std::filesystem::path const& path)
    {
        try {
            hi_log_info("Parsing theme at {}", path.string());
            auto const data = parse_JSON(path);
            parse(data);
        } catch (std::exception const& e) {
            throw io_error(std::format("{}: Could not load theme.\n{}", path.string(), e.what()));
        }
    }

    /** Distance between widgets and between widgets and the border of the container.
     */
    template<typename T = hi::margins>
    [[nodiscard]] constexpr T margin() const noexcept
    {
        if constexpr (std::is_same_v<T, hi::margins>) {
            return hi::margins{_margin};
        } else if constexpr (std::is_same_v<T, float>) {
            return _margin;
        } else {
            hi_static_not_implemented();
        }
    }

    /** The line-width of a border.
     */
    [[nodiscard]] constexpr float border_width() const noexcept
    {
        return _border_width;
    }

    /** The rounding radius of boxes with rounded corners.
     */
    template<typename T = hi::corner_radii>
    [[nodiscard]] constexpr T rounding_radius() const noexcept
    {
        if constexpr (std::is_same_v<T, hi::corner_radii>) {
            return T{_rounding_radius};
        } else if constexpr (std::is_same_v<T, float>) {
            return _rounding_radius;
        } else {
            hi_static_not_implemented();
        }
    }

    /** The size of small square widgets.
     */
    [[nodiscard]] constexpr float size() const noexcept
    {
        return _size;
    }

    /** The size of large widgets. Such as the minimum scroll bar size.
     */
    [[nodiscard]] constexpr float large_size() const noexcept
    {
        return _large_size;
    }

    /** Size of icons inside a widget.
     */
    [[nodiscard]] constexpr float icon_size() const noexcept
    {
        return _icon_size;
    }

    /** Size of icons representing the length of am average word of a label's text.
     */
    [[nodiscard]] constexpr float large_icon_size() const noexcept
    {
        return _large_icon_size;
    }

    /** Size of icons being inline with a label's text.
     */
    [[nodiscard]] constexpr float label_icon_size() const noexcept
    {
        return _label_icon_size;
    }

    /** The amount the base-line needs to be moved downwards when a label is aligned to top.
     */
    [[nodiscard]] constexpr float baseline_adjustment() const noexcept
    {
        return _baseline_adjustment;
    }

    /** Create a transformed copy of the theme.
     *
     * This function is used by the window, to make a specific version of
     * the theme scaled to the PPI of the window.
     *
     * It can also create a different version when the window becomes active/inactive
     * mostly this will de-saturate the colors in the theme.
     *
     * @param new_ppi The PPI of the window.
     */
    [[nodiscard]] theme transform(unit::pixel_density new_pixel_density) const noexcept
    {
        auto r = *this;

        auto delta_scale = new_pixel_density.ppi / pixel_density.ppi;
        r.pixel_density = new_pixel_density;

        // Scale each size, and round so that everything will stay aligned on pixel boundaries.
        r._margin = std::round(delta_scale * _margin);
        r._border_width = std::round(delta_scale * _border_width);
        r._rounding_radius = std::round(delta_scale * _rounding_radius);
        r._size = std::round(delta_scale * _size);
        r._large_size = std::round(delta_scale * _large_size);
        r._icon_size = std::round(delta_scale * _icon_size);
        r._large_icon_size = std::round(delta_scale * _large_icon_size);
        r._label_icon_size = std::round(delta_scale * _label_icon_size);
        // Cap height is not rounded, since the text-shaper will align the text to sub-pixel boundaries.
        r._baseline_adjustment = std::round(delta_scale * _baseline_adjustment);

        return r;
    }

    [[nodiscard]] hi::color accent_color(size_t nesting_level = 0) const noexcept
    {
        hi_assert(not _accent_colors.empty());
        return _accent_colors[nesting_level % _accent_colors.size()];
    }

    [[nodiscard]] hi::color foreground_color(size_t nesting_level = 0) const noexcept
    {
        hi_assert(not _foreground_colors.empty());
        return _foreground_colors[nesting_level % _foreground_colors.size()];
    }

    [[nodiscard]] hi::color border_color(size_t nesting_level = 0) const noexcept
    {
        hi_assert(not _border_colors.empty());
        return _border_colors[nesting_level % _border_colors.size()];
    }

    [[nodiscard]] hi::color fill_color(size_t nesting_level = 0) const noexcept
    {
        hi_assert(not _fill_colors.empty());
        return _fill_colors[nesting_level % _fill_colors.size()];
    }

    [[nodiscard]] hi::color text_select_color(size_t nesting_level = 0) const noexcept
    {
        hi_assert(not _text_select_colors.empty());
        return _text_select_colors[nesting_level % _text_select_colors.size()];
    }

    [[nodiscard]] hi::color primary_cursor_color(size_t nesting_level = 0) const noexcept
    {
        hi_assert(not _primary_cursor_colors.empty());
        return _primary_cursor_colors[nesting_level % _primary_cursor_colors.size()];
    }

    [[nodiscard]] hi::color secondary_cursor_color(size_t nesting_level = 0) const noexcept
    {
        hi_assert(not _secondary_cursor_colors.empty());
        return _secondary_cursor_colors[nesting_level % _secondary_cursor_colors.size()];
    }

    [[nodiscard]] hi::text_style_set const &text_style_set() const noexcept
    {
        return _text_style_set;
    }

    [[nodiscard]] style::attributes_from_theme_type attributes_from_theme_function() const noexcept
    {
        return [](style_path const &path, style_pseudo_class const &pseudo_class) -> style_attributes {
            return style_attributes{};
        };
    }

private:
    /** Distance between widgets and between widgets and the border of the container.
     */
    float _margin = 5.0f;

    /** The line-width of a border.
     */
    float _border_width = 1.0f;

    /** The rounding radius of boxes with rounded corners.
     */
    float _rounding_radius = 4.0f;

    /** The size of small square widgets.
     */
    float _size = 11.0f;

    /** The size of large widgets. Such as the minimum scroll bar size.
     */
    float _large_size = 19.0f;

    /** Size of icons inside a widget.
     */
    float _icon_size = 8.0f;

    /** Size of icons representing the length of am average word of a label's text.
     */
    float _large_icon_size = 23.0f;

    /** Size of icons being inline with a label's text.
     */
    float _label_icon_size = 15.0f;

    /** The amount the base-line needs to be moved downwards when a label is aligned to top.
     */
    float _baseline_adjustment = 9.0f;

    std::vector<hi::color> _foreground_colors;
    std::vector<hi::color> _border_colors;
    std::vector<hi::color> _fill_colors;
    std::vector<hi::color> _accent_colors;
    std::vector<hi::color> _text_select_colors;
    std::vector<hi::color> _primary_cursor_colors;
    std::vector<hi::color> _secondary_cursor_colors;

    hi::text_style_set _text_style_set;

    [[nodiscard]] float parse_float(datum const& data, char const* object_name)
    {
        if (!data.contains(object_name)) {
            throw parse_error(std::format("Missing '{}'", object_name));
        }

        auto const object = data[object_name];
        if (auto f = get_if<double>(object)) {
            return static_cast<float>(*f);
        } else if (auto ll = get_if<long long>(object)) {
            return static_cast<float>(*ll);
        } else {
            throw parse_error(
                std::format("'{}' attribute must be a floating point number, got {}.", object_name, object.type_name()));
        }
    }

    [[nodiscard]] long long parse_long_long(datum const& data, char const* object_name)
    {
        if (!data.contains(object_name)) {
            throw parse_error(std::format("Missing '{}'", object_name));
        }

        auto const object = data[object_name];
        if (auto f = get_if<long long>(object)) {
            return static_cast<long long>(*f);
        } else {
            throw parse_error(std::format("'{}' attribute must be a integer, got {}.", object_name, object.type_name()));
        }
    }

    [[nodiscard]] int parse_int(datum const& data, char const* object_name)
    {
        auto const value = parse_long_long(data, object_name);
        if (value > std::numeric_limits<int>::max() or value < std::numeric_limits<int>::min()) {
            throw parse_error(std::format("'{}' attribute is out of range, got {}.", object_name, value));
        }
        return narrow_cast<int>(value);
    }

    [[nodiscard]] bool parse_bool(datum const& data, char const* object_name)
    {
        if (!data.contains(object_name)) {
            throw parse_error(std::format("Missing '{}'", object_name));
        }

        auto const object = data[object_name];
        if (!holds_alternative<bool>(object)) {
            throw parse_error(std::format("'{}' attribute must be a boolean, got {}.", object_name, object.type_name()));
        }

        return to_bool(object);
    }

    [[nodiscard]] std::string parse_string(datum const& data, char const* object_name)
    {
        // Extract name
        if (!data.contains(object_name)) {
            throw parse_error(std::format("Missing '{}'", object_name));
        }
        auto const object = data[object_name];
        if (!holds_alternative<std::string>(object)) {
            throw parse_error(std::format("'{}' attribute must be a string, got {}.", object_name, object.type_name()));
        }
        return static_cast<std::string>(object);
    }

    [[nodiscard]] hi::color parse_color_value(datum const& data)
    {
        if (holds_alternative<datum::vector_type>(data)) {
            if (data.size() != 3 && data.size() != 4) {
                throw parse_error(std::format("Expect 3 or 4 values for a color, got {}.", data));
            }
            auto const r = data[0];
            auto const g = data[1];
            auto const b = data[2];
            auto const a = data.size() == 4 ? data[3] : (holds_alternative<long long>(r) ? datum{255} : datum{1.0});

            if (holds_alternative<long long>(r) and holds_alternative<long long>(g) and holds_alternative<long long>(b) and
                holds_alternative<long long>(a)) {
                auto const r_ = get<long long>(r);
                auto const g_ = get<long long>(g);
                auto const b_ = get<long long>(b);
                auto const a_ = get<long long>(a);

                hi_check(r_ >= 0 and r_ <= 255, "integer red-color value not within 0 and 255");
                hi_check(g_ >= 0 and g_ <= 255, "integer green-color value not within 0 and 255");
                hi_check(b_ >= 0 and b_ <= 255, "integer blue-color value not within 0 and 255");
                hi_check(a_ >= 0 and a_ <= 255, "integer alpha-color value not within 0 and 255");

                return color_from_sRGB(
                    static_cast<uint8_t>(r_), static_cast<uint8_t>(g_), static_cast<uint8_t>(b_), static_cast<uint8_t>(a_));

            } else if (
                holds_alternative<double>(r) and holds_alternative<double>(g) and holds_alternative<double>(b) and
                holds_alternative<double>(a)) {
                auto const r_ = static_cast<float>(get<double>(r));
                auto const g_ = static_cast<float>(get<double>(g));
                auto const b_ = static_cast<float>(get<double>(b));
                auto const a_ = static_cast<float>(get<double>(a));

                return hi::color(r_, g_, b_, a_);

            } else {
                throw parse_error(std::format("Expect all integers or all floating point numbers in a color, got {}.", data));
            }

        } else if (auto const* color_name = get_if<std::string>(data)) {
            auto const color_name_ = to_lower(*color_name);
            if (color_name_.starts_with("#")) {
                return color_from_sRGB(color_name_);
            } else if (auto color_ptr = color::find(color_name_)) {
                return *color_ptr;
            } else {
                throw parse_error(std::format("Unable to parse color, got {}.", data));
            }
        } else {
            throw parse_error(std::format("Unable to parse color, got {}.", data));
        }
    }

    [[nodiscard]] hi::color parse_color(datum const& data, char const* object_name)
    {
        if (!data.contains(object_name)) {
            throw parse_error(std::format("Missing color '{}'", object_name));
        }

        auto const color_object = data[object_name];

        return parse_color_value(color_object);
    }

    [[nodiscard]] std::vector<hi::color> parse_color_list(datum const& data, char const* object_name)
    {
        // Extract name
        if (!data.contains(object_name)) {
            throw parse_error(std::format("Missing color list '{}'", object_name));
        }

        auto const color_list_object = data[object_name];
        if (holds_alternative<datum::vector_type>(color_list_object) and not color_list_object.empty() and
            holds_alternative<datum::vector_type>(color_list_object[0])) {
            auto r = std::vector<hi::color>{};
            ssize_t i = 0;
            for (auto const& color : color_list_object) {
                try {
                    r.push_back(parse_color_value(color));
                } catch (parse_error const& e) {
                    throw parse_error(
                        std::format("Could not parse {}nd entry of color list '{}'\n{}", i + 1, object_name, e.what()));
                }
            }
            return r;

        } else {
            try {
                return {parse_color_value(data[object_name])};
            } catch (parse_error const& e) {
                throw parse_error(std::format("Could not parse color '{}'\n{}", object_name, e.what()));
            }
        }
    }

    [[nodiscard]] hi::text_style parse_text_style_value(datum const& data)
    {
        if (not holds_alternative<datum::map_type>(data)) {
            throw parse_error(std::format("Expect a text-style to be an object, got '{}'", data));
        }

        auto r = hi::text_style{};

        auto const family_id = find_font_family(parse_string(data, "family"));

        auto variant = font_variant{};
        if (data.contains("weight")) {
            variant.set_weight(parse_font_weight(data, "weight"));
        } else {
            variant.set_weight(font_weight::regular);
        }

        if (data.contains("italic")) {
            variant.set_style(parse_bool(data, "italic") ? font_style::italic : font_style::normal);
        } else {
            variant.set_style(font_style::normal);
        }

        auto font_id = find_font(family_id, variant);

        r.set_font_chain({font_id});
        r.set_size(unit::points_per_em(gsl::narrow<short>(parse_float(data, "size"))));
        r.set_color(parse_color(data, "color"));
        r.set_line_spacing(1.0f);
        r.set_paragraph_spacing(1.5f);
        return r;
    }

    [[nodiscard]] font_weight parse_font_weight(datum const& data, char const* object_name)
    {
        if (!data.contains(object_name)) {
            throw parse_error(std::format("Missing '{}'", object_name));
        }

        auto const object = data[object_name];
        if (auto i = get_if<long long>(object)) {
            return font_weight_from_int(*i);
        } else if (auto s = get_if<std::string>(object)) {
            return font_weight_from_string(*s);
        } else {
            throw parse_error(std::format("Unable to parse font weight, got {}.", object.type_name()));
        }
    }

    [[nodiscard]] hi::text_style parse_text_style(datum const& data, char const* object_name)
    {
        // Extract name
        if (!data.contains(object_name)) {
            throw parse_error(std::format("Missing text-style '{}'", object_name));
        }

        auto const textStyleObject = data[object_name];
        try {
            return parse_text_style_value(textStyleObject);
        } catch (parse_error const& e) {
            throw parse_error(std::format("Could not parse text-style '{}'\n{}", object_name, e.what()));
        }
    }

    void parse(datum const& data)
    {
        hi_assert(holds_alternative<datum::map_type>(data));

        name = parse_string(data, "name");

        auto const mode_name = to_lower(parse_string(data, "mode"));
        if (mode_name == "light") {
            mode = theme_mode::light;
        } else if (mode_name == "dark") {
            mode = theme_mode::dark;
        } else {
            throw parse_error(std::format("Attribute 'mode' must be \"light\" or \"dark\", got \"{}\".", mode_name));
        }

        named_color<"blue"> = parse_color(data, "blue");
        named_color<"green"> = parse_color(data, "green");
        named_color<"indigo"> = parse_color(data, "indigo");
        named_color<"orange"> = parse_color(data, "orange");
        named_color<"pink"> = parse_color(data, "pink");
        named_color<"purple"> = parse_color(data, "purple");
        named_color<"red"> = parse_color(data, "red");
        named_color<"teal"> = parse_color(data, "teal");
        named_color<"yellow"> = parse_color(data, "yellow");

        named_color<"gray0"> = parse_color(data, "gray0");
        named_color<"gray1"> = parse_color(data, "gray1");
        named_color<"gray2"> = parse_color(data, "gray2");
        named_color<"gray3"> = parse_color(data, "gray3");
        named_color<"gray4"> = parse_color(data, "gray4");
        named_color<"gray5"> = parse_color(data, "gray5");
        named_color<"gray6"> = parse_color(data, "gray6");
        named_color<"gray7"> = parse_color(data, "gray7");
        named_color<"gray8"> = parse_color(data, "gray8");
        named_color<"gray9"> = parse_color(data, "gray9");
        named_color<"gray10"> = parse_color(data, "gray10");

        _accent_colors = parse_color_list(data, "accent-color");
        _border_colors = parse_color_list(data, "border-color");
        _fill_colors = parse_color_list(data, "fill-color");
        _text_select_colors = parse_color_list(data, "text-select-color");
        _primary_cursor_colors = parse_color_list(data, "primary-cursor-color");
        _secondary_cursor_colors = parse_color_list(data, "secondary-cursor-color");

        _text_style_set.clear();
        _text_style_set.push_back({}, parse_text_style(data, "label-style"));
        _text_style_set.push_back({phrasing_mask::warning}, parse_text_style(data, "warning-label-style"));
        _text_style_set.push_back({phrasing_mask::error}, parse_text_style(data, "error-label-style"));
        _text_style_set.push_back({phrasing_mask::example}, parse_text_style(data, "help-label-style"));
        _text_style_set.push_back({phrasing_mask::placeholder}, parse_text_style(data, "placeholder-label-style"));

        _margin = narrow_cast<float>(parse_int(data, "margin"));
        _border_width = narrow_cast<float>(parse_int(data, "border-width"));
        _rounding_radius = narrow_cast<float>(parse_int(data, "rounding-radius"));
        _size = narrow_cast<float>(parse_int(data, "size"));
        _large_size = narrow_cast<float>(parse_int(data, "large-size"));
        _icon_size = narrow_cast<float>(parse_int(data, "icon-size"));
        _large_icon_size = narrow_cast<float>(parse_int(data, "large-icon-size"));
        _label_icon_size = narrow_cast<float>(parse_int(data, "label-icon-size"));

        auto const base_font = _text_style_set.front().font_chain()[0];
        auto const base_size = _text_style_set.front().size();
        auto const base_cap_height = std::get<unit::points_per_em_s>(base_size) * base_font->metrics.cap_height;
        _baseline_adjustment = ceil_in(unit::points, base_cap_height);
    }

    [[nodiscard]] friend std::string to_string(theme const& rhs) noexcept
    {
        return std::format("{}:{}", rhs.name, rhs.mode);
    }

    friend std::ostream& operator<<(std::ostream& lhs, theme const& rhs)
    {
        return lhs << to_string(rhs);
    }
};

} // namespace hi::inline v1
