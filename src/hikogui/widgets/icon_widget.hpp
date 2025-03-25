// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/icon_widget.hpp Defines icon_widget.
 * @ingroup widgets
 */

#pragma once

#include "widget.hpp"
#include "icon_delegate.hpp"
#include "../GFX/GFX.hpp"
#include "../geometry/geometry.hpp"
#include "../l10n/l10n.hpp"
#include "../macros.hpp"
#include <gsl/gsl>
#include <memory>
#include <string>
#include <array>
#include <optional>
#include <future>

hi_export_module(hikogui.widgets.icon_widget);

hi_export namespace hi {
inline namespace v1 {

/** An simple GUI widget that displays an icon.
 * @ingroup widgets
 *
 * The icon is scaled to the size of the widget,
 * parent widgets will use this scaling to set the correct size.
 */
class icon_widget : public widget {
public:
    using super = widget;
    using delegate_type = icon_delegate;

    /** The color a non-color icon will be displayed with.
     */
    observer<hi::phrasing> phrasing = hi::phrasing::regular;

    template<typename... Args>
    [[nodiscard]] static std::shared_ptr<delegate_type> make_default_delegate(Args&&... args)
    {
        return make_shared_ctad<default_icon_delegate>(std::forward<Args>(args)...);
    }

    template<std::derived_from<delegate_type> Delegate>
    icon_widget(std::shared_ptr<Delegate> delegate) : super(), _delegate(std::move(delegate))
    {
        assert(_delegate != nullptr);

        _delegate_cbt = _delegate->subscribe(this, [this] {
            _icon_has_modified = true;
            ++global_counter<"icon_widget:icon:constrain">;
            request_reconstrain();
        });

        style.set_name("icon");
    }

    template<typename... Args>
    icon_widget(Args&&... args) : icon_widget(make_default_delegate(std::forward<Args>(args)...))
    {
    }

    /// @privatesection
    [[nodiscard]] box_constraints update_constraints() noexcept override
    {
        if (_icon_has_modified.exchange(false)) {
            assert(_delegate != nullptr);
            auto const icon = _delegate->get_icon(this);

            if (auto const optional_bookmark = std::get_if<hi::bookmark>(&icon)) {
                if (auto const pixmap = load_image(*optional_bookmark, os_settings::language_tags(), style.pixel_density)) {
                    auto const width = gsl::narrow<float>(pixmap->width());
                    auto const height = gsl::narrow<float>(pixmap->height());
                    auto const scale = 1.0f / gsl::narrow<float>(pixmap->scale());
 
                    _glyph = {};
                    _icon_type = icon_type::pixmap;
                    _icon_scale = scale;
                    _icon_size = style.concrete_size_px(extent2{width, height}, scale); 
                    _pixmap_backing = gfx_pipeline_image::paged_image{surface(), *pixmap};
                    if (not _pixmap_backing) {
                        // Could not get an image, retry.
                        _icon_has_modified = true;
                        ++global_counter<"icon_widget:no-backing-image:constrain">;
                        request_reconstrain();
                    }

                } else {
                    _glyph = find_glyph(elusive_icon::ExclamationSign);
                    _icon_type = icon_type::glyph;
                    _icon_scale = 1.0f;
                    _icon_size = style.concrete_size_px(_glyph.front_glyph_metrics().bounding_rectangle.size() * style.font_size_px, 1.0f);
                    _pixmap_backing = {};
                }

            } else if (auto const g1 = std::get_if<font_glyph_ids>(&icon)) {
                _glyph = *g1;
                _icon_type = icon_type::glyph;
                _icon_scale = 1.0f;
                _icon_size = style.concrete_size_px(_glyph.front_glyph_metrics().bounding_rectangle.size() * style.font_size_px, 1.0f);
                _pixmap_backing = {};

            } else if (auto const g2 = std::get_if<elusive_icon>(&icon)) {
                _glyph = find_glyph(*g2);
                _icon_type = icon_type::glyph;
                _icon_scale = 1.0f;
                _icon_size = style.concrete_size_px(_glyph.front_glyph_metrics().bounding_rectangle.size() * style.font_size_px, 1.0f);
                _pixmap_backing = {};

            } else if (auto const g3 = std::get_if<hikogui_icon>(&icon)) {
                _glyph = find_glyph(*g3);
                _icon_type = icon_type::glyph;
                _icon_scale = 1.0f;
                _icon_size = style.concrete_size_px(_glyph.front_glyph_metrics().bounding_rectangle.size() * style.font_size_px, 1.0f);
                _pixmap_backing = {};

            } else {
                _glyph = {};
                _icon_type = icon_type::no;
                _icon_scale = 1.0f;
                _icon_size = {};
                _pixmap_backing = {};
            }
        }

        return {_icon_size, _icon_size, _icon_size, style.margins_px};
    }

    void set_layout(widget_layout const& context) noexcept override
    {
        super::set_layout(context);

        if (_icon_type == icon_type::no or not _icon_size) {
            _icon_rectangle = {};

        } else {
            auto const scaled_icon_size = style.concrete_size_px(_icon_size, 1.0f, context.size());

            auto const middle = context.get_middle(style.cap_height);
            //auto const aspect_fit_size = aspect_fit(_icon_size, context.size());
            //auto const font_fit_size = _icon_size * style.font_size_px;

            _icon_rectangle = align_to_middle(
                context.rectangle() + style.vertical_margins_px,
                scaled_icon_size,
                os_settings::alignment(style.horizontal_alignment),
                middle.in(unit::pixels));
        }
    }

    color icon_color() noexcept
    {
        return style.text_style[{*phrasing}].color();
    }

    void draw(draw_context const& context) const noexcept override
    {
        if (overlaps(context, layout())) {
            switch (_icon_type) {
            case icon_type::no:
                break;

            case icon_type::pixmap:
                if (not context.draw_image(layout(), _icon_rectangle, _pixmap_backing)) {
                    // Continue redrawing until the image is loaded.
                    request_redraw();
                }
                break;

            case icon_type::glyph:
                {
                    context.draw_glyph(layout(), _icon_rectangle, _glyph, style.color);
                }
                break;

            default:
                hi_no_default();
            }
        }

        return super::draw(context);
    }
    /// @endprivatesection
private:
    enum class icon_type { no, glyph, pixmap };

    icon_type _icon_type;
    font_glyph_ids _glyph;
    mutable gfx_pipeline_image::paged_image _pixmap_backing;
    std::atomic<bool> _icon_has_modified = true;

    extent2 _icon_size;
    float _icon_scale;
    aarectangle _icon_rectangle;

    std::shared_ptr<delegate_type> _delegate;
    callback<void()> _delegate_cbt;
};

} // namespace v1
} // namespace hi::v1
