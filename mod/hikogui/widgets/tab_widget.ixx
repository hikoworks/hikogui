// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/tab_widget.hpp Defines tab_widget.
 * @ingroup widgets
 */

module;
#include "../macros.hpp"

#include <coroutine>

export module hikogui_widgets_tab_widget;
import hikogui_coroutine;
import hikogui_widgets_grid_widget;
import hikogui_widgets_tab_delegate;
import hikogui_widgets_widget;

export namespace hi { inline namespace v1 {

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
class tab_widget final : public widget {
public:
    using super = widget;
    using delegate_type = tab_delegate;

    std::shared_ptr<delegate_type> delegate;

    ~tab_widget()
    {
        hi_assert_not_null(delegate);
        delegate->deinit(*this);
    }

    /** Construct a tab widget with a delegate.
     *
     * @param parent The owner of this widget.
     * @param delegate The delegate that will control this widget.
     */
    tab_widget(not_null<widget_intf const *> parent, std::shared_ptr<delegate_type> delegate) noexcept : super(parent), delegate(std::move(delegate))
    {
        hi_axiom(loop::main().on_thread());

        hi_assert_not_null(this->delegate);
        _delegate_cbt = this->delegate->subscribe([&] {
            ++global_counter<"tab_widget:delegate:constrain">;
            process_event({gui_event_type::window_reconstrain});
        });

        this->delegate->init(*this);
    }

    /** Construct a tab widget with an observer value.
     *
     * @param parent The owner of this widget.
     * @param value The value or observer value to monitor for which child widget
     *              to display.
     */
    tab_widget(not_null<widget_intf const *> parent, incompatible_with<std::shared_ptr<delegate_type>> auto&& value) noexcept
        requires requires { make_default_tab_delegate(hi_forward(value)); }
        : tab_widget(parent, make_default_tab_delegate(hi_forward(value)))
    {
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

        auto tmp = std::make_unique<WidgetType>(this, std::forward<Args>(args)...);
        auto& ref = *tmp;

        hi_assert_not_null(delegate);
        delegate->add_tab(*this, static_cast<std::size_t>(key), size(_children));
        _children.push_back(std::move(tmp));
        ++global_counter<"tab_widget:emplace:constrain">;
        process_event({gui_event_type::window_reconstrain});
        return ref;
    }

    /// @privatesection
    [[nodiscard]] generator<widget_intf &> children(bool include_invisible) noexcept override
    {
        for (hilet& child : _children) {
            co_yield *child;
        }
    }

    [[nodiscard]] box_constraints update_constraints() noexcept override
    {
        _layout = {};

        auto& selected_child_ = selected_child();

        if (_previous_selected_child != &selected_child_) {
            _previous_selected_child = &selected_child_;
            hi_log_info("tab_widget::update_constraints() selected tab changed");
            process_event({gui_event_type::window_resize});
        }

        for (hilet& child : _children) {
            child->mode = child.get() == &selected_child_ ? widget_mode::enabled : widget_mode::invisible;
        }

        return selected_child_.update_constraints();
    }
    void set_layout(widget_layout const& context) noexcept override
    {
        _layout = context;

        for (hilet& child : _children) {
            if (*child->mode > widget_mode::invisible) {
                child->set_layout(context);
            }
        }
    }
    void draw(draw_context const& context) noexcept override
    {
        if (*mode > widget_mode::invisible) {
            for (hilet& child : _children) {
                child->draw(context);
            }
        }
    }
    [[nodiscard]] hitbox hitbox_test(point2 position) const noexcept override
    {
        hi_axiom(loop::main().on_thread());

        if (*mode >= widget_mode::partial) {
            auto r = hitbox{};
            for (hilet& child : _children) {
                r = child->hitbox_test_from_parent(position, r);
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
        return selected_child().find_next_widget(current_widget, group, direction);
    }
    /// @endprivatesection
private:
    widget const *_previous_selected_child = nullptr;
    std::vector<std::unique_ptr<widget>> _children;
    callback<void()> _delegate_cbt;

    using const_iterator = decltype(_children)::const_iterator;

    [[nodiscard]] const_iterator find_selected_child() const noexcept
    {
        hi_axiom(loop::main().on_thread());
        hi_assert_not_null(delegate);

        auto index = delegate->index(const_cast<tab_widget&>(*this));
        if (index >= 0 and index < ssize(_children)) {
            return _children.begin() + index;
        }

        return _children.end();
    }
    [[nodiscard]] widget& selected_child() const noexcept
    {
        hi_axiom(loop::main().on_thread());
        hi_assert(not _children.empty());

        auto i = find_selected_child();
        if (i != _children.cend()) {
            return **i;
        } else {
            return *_children.front();
        }
    }
};

}} // namespace hi::v1
