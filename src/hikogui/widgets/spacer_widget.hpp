// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/spacer_widget.hpp Defines spacer_widget.
 * @ingroup widgets
 */

#pragma once

#include "widget.hpp"
#include "../macros.hpp"

hi_export_module(hikogui.widgets.spacer_widget);

hi_export namespace hi { inline namespace v1 {

/** This GUI widget is used as a spacer between other widget for layout purposes.
 * @ingroup widgets
 *
 */
class spacer_widget : public widget {
public:
    using super = widget;

    spacer_widget() noexcept : super()
    {
        style.set_name("spacer");
    }

    [[nodiscard]] generator<widget_intf &> children(bool include_invisible) const noexcept override
    {
        co_return;
    }

    [[nodiscard]] box_constraints update_constraints() noexcept override
    {
        auto r = box_constraints{};
        r.maximum = extent2::large();
        return r;
    }

    [[nodiscard]] hitbox hitbox_test(point2 position) const noexcept override
    {
        return hitbox{};
    }
};

}} // namespace hi::v1
