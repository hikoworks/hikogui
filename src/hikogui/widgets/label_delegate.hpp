

#pragma once

#include "text_delegate.hpp"
#include "icon_delegate.hpp"

namespace hi::inline v1 {

class label_delegate : public virtual text_delegate, public virtual icon_delegate {
};

template<typename... Args>
class default_label_delegate;

template<>
class default_label_delegate<> : public virtual label_delegate {
public:
    [[nodiscard]] bool empty_text(widget_intf const* sender) const override
    {
        return true;
    }

    [[nodiscard]] bool empty_icon(widget_intf const* sender) const override
    {
        return true;
    }

    [[nodiscard]] gstring get_text(widget_intf const* sender) const override
    {
        return {};
    }

    [[nodiscard]] hi::icon get_icon(widget_intf const* sender) const override
    {
        return {};
    }
};

template<>
class default_label_delegate<hi::label> : public label_delegate {
public:
    /** Construct a delegate.
     *
     * @param value A value or observer-value used as a representation of the state.
     */
    template<forward_of<observer<hi::label>> Value>
    explicit default_label_delegate(Value&& value) : _value(std::forward<Value>(value))
    {
        _value_cbt = this->_value.subscribe([&](auto...) {
            this->_notifier();
        });
    }

    /// @privatesection
    [[nodiscard]] bool empty_text(widget_intf const* sender) const override
    {
        return _value->text.empty();
    }

    [[nodiscard]] bool empty_icon(widget_intf const* sender) const override
    {
        return _value->icon.empty();
    }

    [[nodiscard]] gstring get_text(widget_intf const* sender) const override
    {
        return _value->text.translate();
    }

    [[nodiscard]] hi::icon get_icon(widget_intf const* sender) const override
    {
        return _value->icon;
    }
    /// @endprivatesection

private:
    observer<hi::label> _value;
    callback<void(hi::label)> _value_cbt;
};

template<>
class default_label_delegate<hi::icon> : public label_delegate {
public:
    /** Construct a delegate.
     *
     * @param value A value or observer-value used as a representation of the state.
     */
    template<forward_of<observer<hi::icon>> Value>
    explicit default_label_delegate(Value&& value) : _value(std::forward<Value>(value))
    {
        _value_cbt = this->_value.subscribe([&](auto...) {
            this->_notifier();
        });
    }

    /// @privatesection
    [[nodiscard]] bool empty_text(widget_intf const* sender) const override
    {
        return true;
    }

    [[nodiscard]] bool empty_icon(widget_intf const* sender) const override
    {
        return _value->empty();
    }

    [[nodiscard]] gstring get_text(widget_intf const* sender) const override
    {
        return {};
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

template<>
class default_label_delegate<hi::txt> : public label_delegate {
public:
    /** Construct a delegate.
     *
     * @param value A value or observer-value used as a representation of the state.
     */
    template<forward_of<observer<hi::txt>> Value>
    explicit default_label_delegate(Value&& value) : _value(std::forward<Value>(value))
    {
        _value_cbt = this->_value.subscribe([&](auto...) {
            this->_notifier();
        });
    }

    /// @privatesection
    [[nodiscard]] bool empty_text(widget_intf const* sender) const override
    {
        return _value->empty();
    }

    [[nodiscard]] bool empty_icon(widget_intf const* sender) const override
    {
        return true;
    }

    [[nodiscard]] gstring get_text(widget_intf const* sender) const override
    {
        return _value->translate();
    }

    [[nodiscard]] hi::icon get_icon(widget_intf const* sender) const override
    {
        return {};
    }
    /// @endprivatesection

private:
    observer<hi::txt> _value;
    callback<void(hi::txt)> _value_cbt;
};


default_label_delegate() -> default_label_delegate<>;
default_label_delegate(hi::label const&) -> default_label_delegate<hi::label>;
default_label_delegate(hi::label&&) -> default_label_delegate<hi::label>;
default_label_delegate(hi::icon const&) -> default_label_delegate<hi::icon>;
default_label_delegate(hi::icon&&) -> default_label_delegate<hi::icon>;
default_label_delegate(hi::txt const&) -> default_label_delegate<hi::txt>;
default_label_delegate(hi::txt&&) -> default_label_delegate<hi::txt>;
default_label_delegate(hi::observer<hi::label> const&) -> default_label_delegate<hi::label>;
default_label_delegate(hi::observer<hi::label>&&) -> default_label_delegate<hi::label>;
default_label_delegate(hi::observer<hi::icon> const&) -> default_label_delegate<hi::icon>;
default_label_delegate(hi::observer<hi::icon>&&) -> default_label_delegate<hi::icon>;
default_label_delegate(hi::observer<hi::txt> const&) -> default_label_delegate<hi::txt>;
default_label_delegate(hi::observer<hi::txt>&&) -> default_label_delegate<hi::txt>;

}
