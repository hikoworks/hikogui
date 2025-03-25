

#pragma once

#include "../dispatch/dispatch.hpp"
#include "../observer/observer.hpp"
#include "../GUI/GUI.hpp"
#include "../utility/utility.hpp"

hi_export_module(hikogui.widgets.toggle_delegate);

hi_export namespace hi {
inline namespace v1 {

/** A button delegate controls the state of a button widget.
 * @ingroup widget_delegates
 */
class toggle_delegate {
public:
    virtual ~toggle_delegate() = default;

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

/** A default toggle button delegate.
 *
 * The default toggle button delegate manages the state of a button widget using
 * observer values.
 *
 * @ingroup widget_delegates
 * @tparam T The type of the observer value.
 */
template<std::equality_comparable T>
class default_toggle_delegate : public toggle_delegate {
public:
    using value_type = T;
    constexpr static bool can_make_defaults =
        std::is_same_v<value_type, bool> or std::is_integral_v<value_type> or std::is_enum_v<value_type>;

    observer<value_type> value;
    observer<value_type> on_value;
    observer<value_type> off_value;

    /** Construct a delegate.
     *
     * @param value A value or observer-value used as a representation of the state.
     * @param on_value The value or observer-value that mean 'on'.
     * @param off_value The value or observer-value that mean 'off'.
     */
    template<
        forward_of<observer<value_type>> Value,
        forward_of<observer<value_type>> OnValue,
        forward_of<observer<value_type>> OffValue>
    default_toggle_delegate(Value&& value, OnValue&& on_value, OffValue&& off_value) :
        value(std::forward<Value>(value)), on_value(std::forward<OnValue>(on_value)), off_value(std::forward<OffValue>(off_value))
    {
        // clang-format off
        _value_cbt = this->value.subscribe([&](auto...){ this->_notifier(); });
        _on_value_cbt = this->on_value.subscribe([&](auto...){ this->_notifier(); });
        _off_value_cbt = this->off_value.subscribe([&](auto...){ this->_notifier(); });
        // clang-format on
    }

    /** Construct a delegate.
     *
     * @param value A value or observer-value used as a representation of the state.
     * @param on_value The value or observer-value that mean 'on'.
     */
    template<forward_of<observer<value_type>> Value, forward_of<observer<value_type>> OnValue>
    default_toggle_delegate(
        Value&& value,
        OnValue&& on_value) requires can_make_defaults
        : default_toggle_delegate(std::forward<Value>(value), std::forward<OnValue>(on_value), value_type{})
    {
    }

    /** Construct a delegate.
     *
     * @param value A value or observer-value used as a representation of the state.
     */
    template<forward_of<observer<value_type>> Value>
    default_toggle_delegate(Value&& value) requires can_make_defaults
        : default_toggle_delegate(std::forward<Value>(value), value_type{1}, value_type{})
    {
    }

    /// @privatesection
    [[nodiscard]] widget_value state(widget_intf const* sender) const override
    {
        if (*value == *on_value) {
            return widget_value::on;
        } else if (*value == *off_value) {
            return widget_value::off;
        } else {
            return widget_value::other;
        }
    }

    void activate(widget_intf const* sender) override
    {
        if (*value == *off_value) {
            value = *on_value;
        } else {
            value = *off_value;
        }
    }
    /// @endprivatesection
private:
    callback<void(value_type)> _value_cbt;
    callback<void(value_type)> _on_value_cbt;
    callback<void(value_type)> _off_value_cbt;
};

template<typename Value>
default_toggle_delegate(Value&&) -> default_toggle_delegate<observer_decay_t<Value>>;

template<typename Value, typename OnValue>
default_toggle_delegate(Value&&, OnValue&&) -> default_toggle_delegate<observer_decay_t<Value>>;

template<typename Value, typename OnValue, typename OffValue>
default_toggle_delegate(Value&&, OnValue&&, OffValue&&) -> default_toggle_delegate<observer_decay_t<Value>>;

} // namespace v1
}
