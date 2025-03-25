
#pragma once

#include "../dispatch/dispatch.hpp"
#include "../observer/observer.hpp"
#include "../GUI/GUI.hpp"
#include "../utility/utility.hpp"

hi_export_module(hikogui.widgets.radio_delegate);

hi_export namespace hi { inline namespace v1 {

/** A radio delegate controls the state of a radio button widget.
 * @ingroup widget_delegates
 */
class radio_delegate {
public:
    virtual ~radio_delegate() = default;

    virtual void init(widget_intf const* sender) {}

    virtual void deinit(widget_intf const* sender) {}

    /** Called when the button is pressed by the user.
     */
    virtual void activate(widget_intf const* sender) {}

    /** Used by the widget to check the state of the button.
     */
    [[nodiscard]] virtual widget_value state(widget_intf const* sender) const
    {
        return widget_value::off;
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

/** A default radio button delegate.
 *
 * The default radio button delegate manages the state of a radio widget using
 * observer values.
 *
 * @ingroup widget_delegates
 * @tparam T The type of the observer value.
 */
template<std::equality_comparable T>
class default_radio_delegate : public radio_delegate {
public:
    using value_type = T;

    observer<value_type> value;
    observer<value_type> on_value;

    /** Construct a delegate.
     *
     * @param value A value or observer-value used as a representation of the state.
     * @param on_value The value or observer-value that mean 'on'.
     */
    template<forward_of<observer<value_type>> Value, forward_of<observer<value_type>> OnValue>
    default_radio_delegate(
        Value&& value,
        OnValue&& on_value) :
        value(std::forward<Value>(value)), on_value(std::forward<OnValue>(on_value))
    {
        // clang-format off
        _value_cbt = this->value.subscribe([&](auto...){ this->_notifier(); });
        _on_value_cbt = this->on_value.subscribe([&](auto...){ this->_notifier(); });
        // clang-format on
    }

    /// @privatesection
    [[nodiscard]] widget_value state(widget_intf const* sender) const override
    {
        if (*value == *on_value) {
            return widget_value::on;
        } else {
            return widget_value::off;
        }
    }

    void activate(widget_intf const* sender) override
    {
        value = *on_value;
    }
    /// @endprivatesection
private:
    callback<void(value_type)> _value_cbt;
    callback<void(value_type)> _on_value_cbt;
};

template<typename Value, typename OnValue>
default_radio_delegate(Value&&, OnValue&&) -> default_radio_delegate<observer_decay_t<Value>>;

}}
