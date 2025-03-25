// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/tab_widget.hpp Defines tab_widget.
 * @ingroup widgets
 */

#pragma once

#include "widget.hpp"
#include "grid_widget.hpp"
#include "tab_delegate.hpp"
#include "../macros.hpp"
#include <coroutine>

hi_export_module(hikogui.widgets.tab_widget);

hi_export namespace hi { inline namespace v1 {

/** A graphical element that shows only one of a predefined set of mutually
 * exclusive child widgets.
 *
 * A tab widget is generally controlled by a `toolbar_tab_button_widget` or
 * another selection widget.
 *
 * @image html toolbar_tab_button_widget.gif
 *
 * In the following example we create three tabs on the window which observes a
 * `value` controlled by a set of toolbar tab buttons. Each tab is configured
 * with a different value: 0, 1 and 2.
 *
 * @snippet widgets/tab_example_impl.cpp Create three tabs
 *
 * @ingroup widgets
 * @note A `tab_button` is not directly controlled by a
 *       `toolbar_tab_button_widget`. This is accomplished by sharing a delegate
 *       or a observer between the toolbar tab button and the tab widget.
 */
class tab_widget : public widget {
public:
    using super = widget;
    using delegate_type = tab_delegate;

    std::shared_ptr<delegate_type> delegate;

    ~tab_widget()
    {
        hi_assert_not_null(delegate);
        delegate->deinit(this);
    }

    /** Construct a tab widget with a delegate.
     *
     * @param parent The owner of this widget.
     * @param delegate The delegate that will control this widget.
     */
    tab_widget(std::shared_ptr<delegate_type> delegate) noexcept : super(), delegate(std::move(delegate))
    {
        hi_axiom(loop::main().on_thread());
        hi_assert_not_null(this->delegate);

        _delegate_cbt = this->delegate->subscribe(this, [this] {
            ++global_counter<"tab_widget:delegate:constrain">;
            request_resize();
        });

        this->delegate->init(this);

        style.set_name("tab-view");
    }

    /** Construct a tab widget with an observer value.
     *
     * @param parent The owner of this widget.
     * @param value The value or observer value to monitor for which child widget
     *              to display.
     */
    template<incompatible_with<std::shared_ptr<delegate_type>> Value>
    tab_widget(Value&& value) noexcept
        requires requires { make_default_tab_delegate(std::forward<Value>(value)); }
        : tab_widget(make_default_tab_delegate(std::forward<Value>(value)))
    {
    }

    void add(size_t index, std::unique_ptr<widget> child)
    {
        hi_assert_not_null(delegate);

        child->set_parent(this);
        delegate->add_tab(this, index, _children.size());
        _children.push_back(std::move(child));

        ++global_counter<"tab_widget:emplace:constrain">;
        request_reconstrain();
    }

    /** Make and add a child widget.
     *
     * @pre A widget with the same @a value must not have been added before.
     * @tparam WidgetType The type of the widget to create.
     * @tparam Key The type of the key, must be convertible to `std::size_t`.
     * @param key The value used as a key to select this newly added widget.
     * @param args The arguments to pass to the constructor of widget to add.
     */
    template<typename WidgetType, typename Key, typename... Args>
    WidgetType& emplace(Key const& key, Args&&...args)
    {
        hi_axiom(loop::main().on_thread());

        auto tmp = std::make_unique<WidgetType>(std::forward<Args>(args)...);
        auto& ref = *tmp;
        add(static_cast<size_t>(key), std::move(tmp));
        return ref;
    }

    /// @privatesection
    [[nodiscard]] generator<widget_intf &> children(bool include_invisible) const noexcept override
    {
        for (auto i = size_t{0}; i < _children.size(); ++i) {
            if (std::cmp_equal(i, delegate->index(this)) or include_invisible) {
                co_yield *_children[i];
            }
        }
    }

    [[nodiscard]] box_constraints update_constraints() noexcept override
    {
        auto r = box_constraints{};

        for (auto &child : visible_children()) {
            r = child.update_constraints();
        }

        return r;
    }

    void set_layout(widget_layout const& context) noexcept override
    {
        super::set_layout(context);

        for (auto &child : visible_children()) {
            child.set_layout(context);
        }
    }

    [[nodiscard]] hitbox hitbox_test(point2 position) const noexcept override
    {
        hi_axiom(loop::main().on_thread());

        if (enabled()) {
            auto r = hitbox{};
            for (auto &child : visible_children()) {
                r = child.hitbox_test_from_parent(position, r);
            }
            return r;
        } else {
            return {};
        }
    }

    [[nodiscard]] widget_id find_next_widget(
        widget_id current_widget,
        keyboard_focus_group group,
        keyboard_focus_direction direction) const noexcept override
    {
        hi_axiom(loop::main().on_thread());
        for (auto &child : visible_children()) {
            // Only one child is visible at a time.
            return child.find_next_widget(current_widget, group, direction);
        }

        // No children, or no children visible.
        return current_widget == id() ? id() : widget_id{};
    }
    /// @endprivatsectopn
private:
    widget const *_previous_selected_child = nullptr;
    std::vector<std::unique_ptr<widget>> _children;
    callback<void()> _delegate_cbt;
};

}} // namespace hi::v1
