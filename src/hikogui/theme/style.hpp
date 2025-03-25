// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "hikogui/units/pixels.hpp"
#include "style_cascade.hpp"
#include "style_path.hpp"
#include "style_properties.hpp"
#include "style_computed_properties.hpp"
#include "style_pseudo_class.hpp"
#include "../text/text.hpp"
#include "../units/units.hpp"
#include "../dispatch/dispatch.hpp"
#include "../macros.hpp"
#include <cstdint>
#include <functional>
#include <utility>
#include <cassert>
#include <array>
#include <vector>
#include <string>
#include <memory>

hi_export_module(hikogui.theme : style);

hi_export namespace hi {
inline namespace v1 {

class style : public style_computed_properties {
public:
    using notifier_type = notifier<void(style_modify_mask, bool)>;
    using callback_type = notifier_type::callback_type;
    using callback_proto = notifier_type::callback_proto;
    using properties_array_type = std::array<style_computed_properties, style_pseudo_class_size>;

    unit::pixel_density pixel_density;

    float width_px;
    float height_px;
    float font_size_px;
    float margin_left_px;
    float margin_bottom_px;
    float margin_right_px;
    float margin_top_px;
    float padding_left_px;
    float padding_bottom_px;
    float padding_right_px;
    float padding_top_px;
    float border_width_px;
    float border_bottom_left_radius_px;
    float border_bottom_right_radius_px;
    float border_top_left_radius_px;
    float border_top_right_radius_px;

    /** The x-height of the primary font.
     */
    unit::pixels_f x_height;

    /** The cap-height of the primary font.
     */
    unit::pixels_f cap_height;

    /** The x-height of the primary font.
     */
    float x_height_px;

    /** The cap-height of the primary font.
     */
    float cap_height_px;

    extent2 size_px;
    hi::margins margins_px;
    hi::margins horizontal_margins_px;
    hi::margins vertical_margins_px;
    hi::margins padding_px;
    hi::margins horizontal_padding_px;
    hi::margins vertical_padding_px;
    hi::corner_radii border_radius_px;

    hi::alignment alignment;

    [[nodiscard]] layout::constraints _make_constraints_helper() const noexcept
    {
    }

    /** Make constraints from layout.
     *
     * This function will return the constraints for a widget with a layout.
     * The constraints will be calculated from the width, height, and alignment
     * properties of the style.
     *
     * The baseline_offset will be calculated based on the vertical alignment
     * property and the alignment within the final layout.
     */
    [[nodiscard]] layout::constraints make_constraints() const noexcept
    {
        auto r = layout::constraints{};

        r.size = size_px;
        r.margins = margins_px;
        r.baseline_offset = [&] {
            switch (vertical_alignment) {
            case vertical_alignment::top:
                return -x_height_px;
            case vertical_alignment::middle:
                return -x_height_px / 2.0f;
            case vertical_alignment::bottom:
                return 0.0f;
            }
            std::unreachable();
        }();

        r.baseline_priority = baseline_priority;
        r.vertical_alignment = vertical_alignment;
        return r; 
    }

    /** Make constraints for a fixed size widget.
     *
     * This function will return the constraints for a widget with a fixed size.
     * The constraints will be calculated from the width, height, and alignment
     * properties of the style.
     *
     * The baseline_offset will be calculated where the baseline is positition
     * so that text is aligned to the middle of the visual part of the widget.
     */
    [[nodiscard]] layout::constraints make_constraints_for_fixed_size() const noexcept
    {
        auto r = make_constraints();

        r.baseline_offset = [&] {
            auto const middle_px = height_px / 2.0f;
            auto const x_middle_px = x_height_px / 2.0f;

            switch (vertical_alignment) {
            case vertical_alignment::top:
                return -middle_px - x_middle_px
            case vertical_alignment::middle:
                return -x_middle_px;
            case vertical_alignment::bottom:
                return middle_px - x_middle_px;
            }
            std::unreachable();
        }();

        return r; 
    }


    

    /** Calculate the concrete object size of an image.
     *
     * This function takes into account:
     * - The natural size of the image.
     * - The width and height specified in the style.
     * - The scale of the image from the image-loader.
     * 
     * @param natural_size The natural size of the image.
     * @param scale When the image loader did not found an image matching the
     *              pixel density of the display, the scale is greater than 1.0
     *              when the image was for a lower resolution display, and less
     *              than 1.0 when the image was for a higher resolution display.
     * @return The natural size of the image after scaling to the specified size.
     */
    [[nodiscard]] extent2 concrete_size_px(extent2 natural_size, float scale) const noexcept
    {
        auto const concrete_width = [&] {
            if (auto const *scalar_width = std::get_if<float>(&width)) {
                return natural_size.width() * *scalar_width * scale;
            } else {
                return width_px;
            }
        }();

        auto const concrete_height = [&] {
            if (auto const *scalar_height = std::get_if<float>(&height)) {
                return natural_size.height() * *scalar_height * scale;
            } else {
                return height_px;
            }
        }(); 

        return {concrete_width, concrete_height};
    }

    /** Calculate the concrete object size of an image to fit inside a box.
     * 
     * This function takes into account:
     * - The natural size of the image.
     * - The scale of the image from the image-loader.
     * - The layout size of the box.
     * - The object-fit property of the style.
     *
     * @param natural_size The natural size of the image.
     * @param scale When the image loader did not found an image matching the
     *              pixel density of the display, the scale is greater than 1.0
     *              when the image was for a lower resolution display, and less
     *              than 1.0 when the image was for a higher resolution display.
     * @param layout_size The size of the box to fit the image in.
     */
    [[nodiscard]] extent2 concrete_size_px(extent2 natural_size, float scale, extent2 layout_size) const noexcept
    {
        if (natural_size.width() == 0.0f or
            natural_size.height() == 0.0f or
            layout_size.width() == 0.0f or
            layout_size.height() == 0.0f) {
            // if the aspect-ratios can not be determined it is as-if object_fit::fill.
            return layout_size;
        }

        auto const natural_aspect_ratio = natural_size.width() / natural_size.height();
        auto const layout_aspect_ratio = layout_size.width() / layout_size.height();

        auto const none_size = natural_size * scale;

        auto const fill_size = layout_size;

        auto const contain_size = natural_aspect_ratio < layout_aspect_ratio ?
                extent2{layout_size.height() * natural_aspect_ratio, layout_size.height()} :
                extent2{layout_size.width(), layout_size.width() / natural_aspect_ratio};

        auto const cover_size = natural_aspect_ratio < layout_aspect_ratio ?
                extent2{layout_size.width(), layout_size.width() / natural_aspect_ratio} :
                extent2{layout_size.height() * natural_aspect_ratio, layout_size.height()};

        auto const scale_down_size = none_size.width() < contain_size.width() ? none_size : contain_size;

        switch (object_fit) {
        case object_fit::none:
            return none_size;
        case object_fit::fill:
            return fill_size;
        case object_fit::contain:
            return contain_size;
        case object_fit::cover:
            return cover_size;
        case object_fit::scale_down:
            return scale_down_size;
        }
        std::unreachable();
    }

    style(style const&) noexcept = delete;
    style(style&&) noexcept = delete;
    style& operator=(style const&) noexcept = delete;
    style& operator=(style&&) noexcept = delete;
    style() noexcept = default;

    /** Give the style a name.
     *
     * @note The first time this function is called the path of child widgets
     *       will not be updated. This is done so that the path can be set
     *       before child widgets are created; before children() must list
     *       these children.
     * @param name The name of the style.
     */
    void set_name(std::string name)
    {
        _name = name;
        _notifier(style_modify_mask::none, true);
    }

    [[nodiscard]] std::string const& name() const noexcept
    {
        return _name;
    }

    void set_id(std::string id)
    {
        _id = id;
        _notifier(style_modify_mask::none, true);
    }

    [[nodiscard]] std::string const& id() const noexcept
    {
        return _id;
    }

    void set_classes(std::vector<std::string> classes)
    {
        _classes = std::move(classes);
        _notifier(style_modify_mask::none, true);
    }

    [[nodiscard]] std::vector<std::string> const& classes() const noexcept
    {
        return _classes;
    }

    /** Parse the given string to configure this style.
     *
     * The @a style_string has the following format:
     * ```
     * style_string := (id | class | attribute )*
     * id := '#' annex-31-minus
     * class := '.' annex-31-minus
     * attribute := annex-31-minus '=' value
     * value := color-value | length-value | horizontal-alignment-value | vertical-alignment-value
     * color-value := hex-color-value | rgb-color-value | rgba-color-value | named-color-value
     * hex-color-value := '#' [0-9A-Fa-f]{6,8}
     * rgb-color-value := 'rgb_color(' number ',' number ',' number ')'
     * rgba-color-value := 'rgb_color(' number ',' number ',' number ',' number ')'
     * named-color-value := annex-31-minus
     * length-value := number ('pd' | 'px' | 'pt' | 'in' | 'cm')?
     * horizontal-alignment-value := 'left' | 'right | 'center' | 'justified' | 'natural' | 'opposite'
     * vertical-alignment-value := 'bottom' | 'middle' | 'top'
     * ```
     */
    style& operator=(std::string style_string)
    {
        if (auto const optional_style = parse_style(style_string)) {
            std::tie(_override_properties, _id, _classes) = *optional_style;
            _notifier(style_modify_mask::none, true);

        } else if (optional_style.has_error()) {
            throw parse_error(optional_style.error());

        } else {
            std::unreachable();
        }
        return *this;
    }

    [[nodiscard]] style_pseudo_class pseudo_class() const noexcept
    {
        return _pseudo_class;
    }

    void set_pseudo_class(style_pseudo_class new_pseudo_class)
    {
        auto const t = trace<"style::set_pseudo_class">{};

        assert(std::to_underlying(new_pseudo_class) < _loaded_properties.size());

        auto const old_pseudo_class = std::exchange(_pseudo_class, new_pseudo_class);

        if (old_pseudo_class != _pseudo_class) {
            auto const i = std::to_underlying(old_pseudo_class);
            auto const j = std::to_underlying(new_pseudo_class);
            auto const mask = _pseudo_class_modifications[i + j * style_pseudo_class_size];

            update_properties(mask);
            _notifier(mask, false);
        }
    }

    [[nodiscard]] std::pair<style_path, properties_array_type const&> restyle(
        unit::pixel_density density,
        style_path const& parent_path,
        properties_array_type const& parent_properties) noexcept
    {
        auto const t = trace<"style::restyle">{};

        this->pixel_density = density;

        auto path = parent_path;
        path.emplace_back(_name, _id, _classes);

        for (auto i = size_t{0}; i != style_pseudo_class_size; ++i) {
            auto p = get_style_properties(path, static_cast<style_pseudo_class>(i));
            p.apply(_override_properties);

            _loaded_properties[i] = p * pixel_density;
            _loaded_properties[i].inherit(parent_properties[i]);
        }

        for (auto i = size_t{0}; i != style_pseudo_class_size; ++i) {
            auto const& src = _loaded_properties[i];
            for (auto j = size_t{0}; j != style_pseudo_class_size; ++j) {
                auto const& dst = _loaded_properties[j];
                _pseudo_class_modifications[i + j * style_pseudo_class_size] = compare(src, dst);
            }
        }

        update_properties(style_modify_mask::all);
        _notifier(style_modify_mask::all, false);
        return {path, _loaded_properties};
    }

    /** Add a callback to the style.
     *
     * After the call the caller will take ownership of the returned callback
     * object.
     *
     * The `callback` object is a move-only RAII object that will automatically
     * unsubscribe the callback when the token is destroyed.
     *
     * @param flags The callback-flags used to determine how the @a callback is called.
     * @param callback A callable object with prototype void(style_modify_mask, bool) being called when the style changes.
     *                 The first argument is the mask of which properties have changed.
     *                 The second argument is true when the path of the style has changed.
     * @return A RAII object which when destroyed will unsubscribe the callback.
     */
    template<forward_of<callback_proto> Func>
    [[nodiscard]] callback_type subscribe(Func&& func, callback_flags flags = callback_flags::synchronous) noexcept
    {
        return _notifier.subscribe(std::forward<Func>(func), flags);
    }

private:
    std::string _name;
    std::string _id;
    std::vector<std::string> _classes;

    style_pseudo_class _pseudo_class;

    /** The properties directly overridden by the developer for this widget's instance.
     */
    style_properties _override_properties;

    /** The properties loaded from the query, with overriden properties applied.
     */
    properties_array_type _loaded_properties;

    /** A table for which properties are modified when switching between pseudo-classes.
     */
    std::array<style_modify_mask, style_pseudo_class_size * style_pseudo_class_size> _pseudo_class_modifications;

    notifier_type _notifier;

    void update_properties(style_modify_mask mask)
    {
        auto const t = trace<"style::update_properties">{};

        this->set_properties(_loaded_properties[std::to_underlying(_pseudo_class)], mask);

        if (to_bool(mask & style_modify_mask::size)) {
            if (auto const *pixel_width = std::get_if<unit::pixels_f>(&width)) {
                width_px = pixel_width->in(unit::pixels);
            } else {
                assert(std::holds_alternative<float>(width));
                width_px = 0.0f;
            }
            
            if (auto const *pixel_height = std::get_if<unit::pixels_f>(&height)) {
                height_px = pixel_height->in(unit::pixels);
            } else {
                assert(std::holds_alternative<float>(height));
                height_px = 0.0f;
            }

            size_px = extent2{width_px, height_px};
            font_size_px = font_size.in(unit::pixels_per_em);

            auto const &style = text_style.front();
            auto const &primary_font = get_font(style.font_chain().front());

            auto const scaled_font_size = font_size * style.scale();
            x_height = round_as(unit::pixels, primary_font.metrics.x_height * scaled_font_size);
            cap_height = round_as(unit::pixels, primary_font.metrics.cap_height * scaled_font_size);

            x_height_px = x_height.in(unit::pixels);
            cap_height_px = cap_height.in(unit::pixels);
        }

        if (to_bool(mask & style_modify_mask::margin)) {
            margin_left_px = margin_left.in(unit::pixels);
            margin_bottom_px = margin_bottom.in(unit::pixels);
            margin_right_px = margin_right.in(unit::pixels);
            margin_top_px = margin_top.in(unit::pixels);
            padding_left_px = padding_left.in(unit::pixels);
            padding_bottom_px = padding_bottom.in(unit::pixels);
            padding_right_px = padding_right.in(unit::pixels);
            padding_top_px = padding_top.in(unit::pixels);
            margins_px = hi::margins{margin_left_px, margin_bottom_px, margin_right_px, margin_top_px};
            horizontal_margins_px = hi::margins{margin_left_px, 0.0f, margin_right_px, 0.0f};
            vertical_margins_px = hi::margins{0.0f, margin_bottom_px, 0.0f, margin_top_px};
            padding_px = hi::margins{padding_left_px, padding_bottom_px, padding_right_px, padding_top_px};
            horizontal_padding_px = hi::margins{padding_left_px, 0.0f, padding_right_px, 0.0f};
            vertical_padding_px = hi::margins{0.0f, padding_bottom_px, 0.0f, padding_top_px};
        }

        if (to_bool(mask & style_modify_mask::weight)) {
            border_width_px = border_width.in(unit::pixels);
            border_bottom_left_radius_px = border_bottom_left_radius.in(unit::pixels);
            border_bottom_right_radius_px = border_bottom_right_radius.in(unit::pixels);
            border_top_left_radius_px = border_top_left_radius.in(unit::pixels);
            border_top_right_radius_px = border_top_right_radius.in(unit::pixels);
            border_radius_px = hi::corner_radii{
                border_bottom_left_radius_px,
                border_bottom_right_radius_px,
                border_top_left_radius_px,
                border_top_right_radius_px};
        }

        if (to_bool(mask & style_modify_mask::alignment)) {
            alignment = hi::alignment{horizontal_alignment, vertical_alignment};
        }
    }
};

} // namespace v1
}
