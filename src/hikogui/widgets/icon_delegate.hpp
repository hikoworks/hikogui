

#pragma once

#include "widget_delegate.hpp"
#include "../utility/utility.hpp"
#include "../GUI/GUI.hpp"
#include "../observer/observer.hpp"
#include "../l10n/l10n.hpp"
#include "../macros.hpp"
#include <utility>

hi_export_module(hikogui.widgets : icon_delegate);

hi_export namespace hi::inline v1 {

class icon_delegate : public virtual widget_delegate {
public:
    /** Check if the icon is empty..
     */
    [[nodiscard]] virtual bool empty_icon(widget_intf const* sender) const = 0;

    /** Get the icon to display.
     */
    [[nodiscard]] virtual hi::icon get_icon(widget_intf const* sender) const = 0;

    /** Check if icon is mutable.
     */
    [[nodiscard]] virtual bool mutable_icon(widget_intf const* sender) const
    {
        return false;
    }

    /** Change the icon.
     */
    virtual void set_icon(widget_intf const* sender, hi::icon const& icon)
    {
        std::unreachable();
    }
};

template<typename... Args>
class default_icon_delegate;

template<>
class default_icon_delegate<> : public virtual icon_delegate {
public:
    default_icon_delegate() = default;

    /// @privatesection
    [[nodiscard]] virtual bool empty_icon(widget_intf const* sender) const
    {
        return true;
    }

    [[nodiscard]] virtual hi::icon get_icon(widget_intf const* sender) const
    {
        return {};
    }
    /// @endprivatesection
};

template<>
class default_icon_delegate<hi::icon> : public virtual icon_delegate {
public:
    template<forward_of<observer<hi::icon>> Value>
    default_icon_delegate(Value&& value) : _value(std::forward<Value>(value))
    {
        _value_cbt = this->_value.subscribe([this](auto...) {
            this->_notifier();
        });
    }

    /// @privatesection
    [[nodiscard]] virtual bool empty_icon(widget_intf const* sender) const override
    {
        return _value->empty();
    }

    [[nodiscard]] hi::icon get_icon(widget_intf const* sender) const override
    {
        return *_value;
    }
    /// @endprivatesection
private:
    observer<hi::icon> _value;
    callback<void(hi::icon)> _value_cbt;
};


default_icon_delegate() -> default_icon_delegate<>;

template<forward_of<observer<hi::icon>> Value>
default_icon_delegate(Value&&) -> default_icon_delegate<observer_decay_t<Value>>;

}

