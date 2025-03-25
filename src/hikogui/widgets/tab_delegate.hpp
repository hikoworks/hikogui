// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/tab_delegate.hpp Defines delegate_delegate and some default tab delegates.
 * @ingroup widget_delegates
 */

#pragma once

#include "../observer/observer.hpp"
#include "../utility/utility.hpp"
#include "../concurrency/concurrency.hpp"
#include "../dispatch/dispatch.hpp"
#include "../GUI/GUI.hpp"
#include "../macros.hpp"
#include "../macros.hpp"
#include <memory>
#include <functional>

hi_export_module(hikogui.widgets.tab_delegate);

hi_export namespace hi { inline namespace v1 {

/** A delegate that controls the state of a tab_widget.
 *
 * @ingroup widget_delegates
 */
class tab_delegate {
public:
    virtual ~tab_delegate() = default;
    virtual void init(widget_intf const* sender) {}
    virtual void deinit(widget_intf const* sender) {}

    virtual void add_tab(widget_intf const* sender, std::size_t key, std::size_t index) {}

    virtual std::ptrdiff_t index(widget_intf const* sender)
    {
        return -1;
    }

    /** Subscribe a callback for notifying the widget of a data change.
     */
    template<forward_of<void()> Func>
    [[nodiscard]] callback<void()> subscribe(widget_intf const* sender, Func&& func, callback_flags flags = callback_flags::synchronous)
    {
        return _notifier.subscribe(std::forward<Func>(func), flags);
    }

protected:
    notifier<void()> _notifier;
};

/** A delegate that control the state of a tab_widget.
 *
 * @ingroup widget_delegates
 * @tparam T the type used as the key for which tab is selected.
 */
template<typename T>
class default_tab_delegate : public tab_delegate {
public:
    using value_type = T;

    observer<value_type> value;
    std::unordered_map<std::size_t, std::size_t> tab_indices;

    /** Construct a default tab delegate.
     *
     * @param value The observer value which represents the selected tab.
     */
    template<forward_of<observer<value_type>> Value>
    default_tab_delegate(Value&& value) : value(std::forward<Value>(value))
    {
        _value_cbt = this->value.subscribe([&](auto...) {
            this->_notifier();
        });
    }

    // XXX key should really be of value_type, not sure how to handle that with the tab_widget not knowing the type of key.
    void add_tab(widget_intf const* sender, std::size_t key, std::size_t index) override
    {
        hi_assert(not tab_indices.contains(key));
        tab_indices[key] = index;
    }

    [[nodiscard]] std::ptrdiff_t index(widget_intf const* sender) override
    {
        auto it = tab_indices.find(*value);
        if (it == tab_indices.end()) {
            return -1;
        } else {
            return static_cast<std::ptrdiff_t>(it->second);
        }
    }

private:
    callback<void(value_type)> _value_cbt;
};

/** Create a shared pointer to a default tab delegate.
 *
 * @ingroup widget_delegates
 * @see default_tab_delegate
 * @param value The observer value which represents the selected tab.
 * @return shared pointer to a tab delegate
 */
template<typename Value>
std::shared_ptr<tab_delegate> make_default_tab_delegate(Value&& value)
    requires requires { default_tab_delegate<observer_decay_t<Value>>{std::forward<Value>(value)}; }
{
    using value_type = observer_decay_t<Value>;
    return std::make_shared<default_tab_delegate<value_type>>(std::forward<Value>(value));
}

}} // namespace hi::v1
