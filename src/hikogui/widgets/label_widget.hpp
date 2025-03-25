// Copyright Take Vos 2020-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/label_widget.hpp Defines label_widget.
 * @ingroup widgets
 */

#pragma once

#include "widget.hpp"
#include "text_widget.hpp"
#include "icon_widget.hpp"
#include "label_delegate.hpp"
#include "../geometry/geometry.hpp"
#include "../layout/layout.hpp"
#include "../l10n/l10n.hpp"
#include "../macros.hpp"
#include <memory>
#include <string>
#include <array>
#include <optional>
#include <future>
#include <coroutine>

hi_export_module(hikogui.widgets : label_widget);

hi_export namespace hi { inline namespace v1 {

/** The GUI widget displays and lays out text together with an icon.
 * @ingroup widgets
 *
 * This widget is often used by other widgets. For example
 * checkboxes display a label representing their state next
 * to the checkbox itself.
 *
 * The alignment of icon and text is shown in the following image:
 * @image html label_widget.png
 *
 * Here is an example on how to create a label:
 * @snippet widgets/checkbox_example_impl.cpp Create a label
 */
class label_widget : public widget {
public:
    using super = widget;
    using delegate_type = label_delegate;

    /** The color of the label's (non-color) icon.
     */
    observer<hi::phrasing> phrasing = hi::phrasing::regular;

    template<typename... Args>
    [[nodiscard]] static std::shared_ptr<delegate_type> make_default_delegate(Args&&... args)
    {
        return make_shared_ctad<default_label_delegate>(std::forward<Args>(args)...);
    }

    ~label_widget()
    {
        assert(_delegate != nullptr);
        _delegate->deinit(this);
    }

    template<std::derived_from<delegate_type> Delegate>
    label_widget(std::shared_ptr<Delegate> delegate) noexcept : super(), _delegate(std::move(delegate))
    {
        assert(_delegate != nullptr);

        _icon_widget = std::make_unique<icon_widget>(_delegate);
        _icon_widget->set_parent(this);
        _icon_widget->phrasing = phrasing;

        _text_widget = std::make_unique<text_widget>(_delegate);
        _text_widget->set_parent(this);
        _text_widget->set_edit_mode(text_widget_edit_mode::selectable);

        style.set_name("label");
        _delegate->init(this);
    }

    template<typename... Args>
    label_widget(Args&&... args) : label_widget(make_default_delegate(std::forward<Args>(args)...))
    {
    }

    /// @privatesection
    [[nodiscard]] generator<widget_intf &> children(bool include_invisible) const noexcept override
    {
        if (not _delegate->empty_icon(this) or include_invisible) {
            co_yield *_icon_widget;
        }
        if (not _delegate->empty_text(this) or include_invisible) {
            co_yield *_text_widget;
        }
    }

    [[nodiscard]] box_constraints update_constraints() noexcept override
    {
        // Resolve as if in left-to-right mode, the grid will flip itself.
        auto const resolved_alignment = resolve(style.alignment, true);

        _grid.clear();
        if (not _delegate->empty_icon(this) and not _delegate->empty_text(this)) {
            // Both of the icon and text are set, so configure the grid to hold both.
            if (resolved_alignment == horizontal_alignment::left) {
                // icon text
                _grid.add_cell(0, 0, _icon_widget.get());
                _grid.add_cell(1, 0, _text_widget.get(), true);
            } else if (resolved_alignment == horizontal_alignment::right) {
                // text icon
                _grid.add_cell(0, 0, _text_widget.get(), true);
                _grid.add_cell(1, 0, _icon_widget.get());
            } else if (resolved_alignment == vertical_alignment::top) {
                // icon
                // text
                _grid.add_cell(0, 0, _icon_widget.get());
                _grid.add_cell(0, 1, _text_widget.get(), true);
            } else if (resolved_alignment == vertical_alignment::bottom) {
                // text
                // icon
                _grid.add_cell(0, 0, _text_widget.get(), true);
                _grid.add_cell(0, 1, _icon_widget.get());
            } else {
                // icon text (buttons want to be middle-center aligned).
                _grid.add_cell(0, 0, _icon_widget.get());
                _grid.add_cell(1, 0, _text_widget.get(), true);
            }
        } else if (not _delegate->empty_icon(this)) {
            // Only the icon-widget is used.
            _grid.add_cell(0, 0, _icon_widget.get());
        } else if (not _delegate->empty_text(this)) {
            // Only the text-widget is used.
            _grid.add_cell(0, 0, _text_widget.get());
        }

        for (auto& cell : _grid) {
            cell.set_constraints(cell.value->update_constraints());
        }

        return _grid.constraints(os_settings::left_to_right(), style.vertical_alignment);
    }

    void set_layout(widget_layout const& context) noexcept override
    {
        super::set_layout(context);

        _grid.set_layout(context.shape);
        for (auto const& cell : _grid) {
            cell.value->set_layout(context.transform(cell.shape, transform_command::level));
        }
    }
    
    [[nodiscard]] hitbox hitbox_test(point2 position) const noexcept override
    {
        hi_axiom(loop::main().on_thread());
        auto r = hitbox{};
        for (auto const &child : visible_children()) {
            r = child.hitbox_test_from_parent(position, r);
        }
        return r;
    }
    /// @endprivatesection
private:
    std::unique_ptr<icon_widget> _icon_widget;
    std::unique_ptr<text_widget> _text_widget;
    grid_layout<widget *> _grid;

    std::shared_ptr<delegate_type> _delegate;
};

}} // namespace hi::v1
