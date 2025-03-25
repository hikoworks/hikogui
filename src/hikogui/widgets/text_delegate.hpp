// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/text_delegate.hpp Defines delegate_delegate and some default text delegates.
 * @ingroup widget_delegates
 */

#pragma once

#include "widget_delegate.hpp"
#include "../observer/observer.hpp"
#include "../utility/utility.hpp"
#include "../concurrency/concurrency.hpp"
#include "../dispatch/dispatch.hpp"
#include "../GUI/GUI.hpp"
#include "../macros.hpp"
#include "../unicode/unicode.hpp"
#include "../l10n/l10n.hpp"
#include "../macros.hpp"
#include <string>
#include <memory>
#include <functional>

hi_export_module(hikogui.widgets.text_delegate);

hi_export namespace hi { inline namespace v1 {

/** A delegate that controls the state of a text_widget.
 *
 * @ingroup widget_delegates
 */
class text_delegate : public virtual widget_delegate {
public:
    [[nodiscard]] virtual bool empty_text(widget_intf const* sender) const = 0;

    /** Read text as a string of graphemes.
     */
    [[nodiscard]] virtual gstring get_text(widget_intf const* sender) const = 0;

    [[nodiscard]] virtual bool mutable_text(widget_intf const* sender) const
    {
        return false;
    }

    /** Write text from a string of graphemes.
     */
    virtual void set_text(widget_intf const* sender, gstring const& text)
    {
        std::unreachable();
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

/** A default text delegate.
 *
 * @ingroup widget_delegates
 * @tparam T The type of the observer value.
 */
template<typename...>
class default_text_delegate;

template<>
class default_text_delegate<> : public text_delegate {
public:
    /** Construct a delegate.
     */
    default_text_delegate() = default;

    /// @privatesection
    [[nodiscard]] bool empty_text(widget_intf const* sender) const override
    {
        return true;
    }

    [[nodiscard]] gstring get_text(widget_intf const* sender) const override
    {
        return {};
    }
    /// @endprivatesection
};

/** A default text delegate specialization for `std::string`.
 *
 * @ingroup widget_delegates
 */
template<>
class default_text_delegate<char const *> : public text_delegate {
public:
    using value_type = char const *;

    /** Construct a delegate.
     *
     * @param value A value or observer-value used as a representation of the state.
     */
    template<forward_of<observer<value_type>> Value>
    explicit default_text_delegate(Value&& value) : _value(std::forward<Value>(value))
    {
        _value_cbt = _value.subscribe([&](auto...) {
            this->_notifier();
        });
    }

    /// @privatesection
    [[nodiscard]] bool empty_text(widget_intf const* sender) const override
    {
        return _value == nullptr or _value[0] == '\0';
    }

    [[nodiscard]] gstring get_text(widget_intf const* sender) const override
    {
        return to_gstring(std::string{*_value});
    }
    /// @endprivatesection
private:
    observer<value_type> _value;
    callback<void(value_type)> _value_cbt;
};

/** A default text delegate specialization for `std::string`.
 *
 * @ingroup widget_delegates
 */
template<>
class default_text_delegate<std::string> : public text_delegate {
public:
    using value_type = std::string;

    /** Construct a delegate.
     *
     * @param value A value or observer-value used as a representation of the state.
     */
    template<forward_of<observer<value_type>> Value>
    explicit default_text_delegate(Value&& value) : _value(std::forward<Value>(value))
    {
        _value_cbt = _value.subscribe([&](auto...) {
            this->_notifier();
        });
    }

    /// @privatesection
    [[nodiscard]] bool empty_text(widget_intf const* sender) const override
    {
        return _value->empty();
    }

    [[nodiscard]] gstring get_text(widget_intf const* sender) const override
    {
        return to_gstring(*_value);
    }

    [[nodiscard]] bool mutable_text(widget_intf const* sender) const override
    {
        return true;
    }

    void set_text(widget_intf const* sender, gstring const& text) override
    {
        _value = to_string(text);
    }
    /// @endprivatesection
private:
    observer<value_type> _value;
    callback<void(value_type)> _value_cbt;
};

/** A default text delegate specialization for `gstring`.
 *
 * @ingroup widget_delegates
 */
template<>
class default_text_delegate<gstring> : public text_delegate {
public:
    using value_type = gstring;

    /** Construct a delegate.
     *
     * @param value A value or observer-value used as a representation of the state.
     */
    template<forward_of<observer<value_type>> Value>
    explicit default_text_delegate(Value&& value) : _value(std::forward<Value>(value))
    {
        _value_cbt = _value.subscribe([&](auto...) {
            this->_notifier();
        });
    }

    /// @privatesection
    [[nodiscard]] bool empty_text(widget_intf const* sender) const override
    {
        return _value->empty();
    }

    [[nodiscard]] gstring get_text(widget_intf const* sender) const override
    {
        return *_value;
    }

    [[nodiscard]] bool mutable_text(widget_intf const* sender) const override
    {
        return true;
    }

    void set_text(widget_intf const* sender, gstring const& text) override
    {
        _value = text;
    }
    /// @endprivatesection
private:
    observer<value_type> _value;
    callback<void(value_type)> _value_cbt;
};

/** A default text delegate specialization for `translate`.
 *
 * @ingroup widget_delegates
 */
template<>
class default_text_delegate<txt> : public text_delegate {
public:
    using value_type = txt;

    /** Construct a delegate.
     *
     * @param value A value or observer-value used as a representation of the state.
     */
    template<forward_of<observer<value_type>> Value>
    explicit default_text_delegate(Value&& value) : _value(std::forward<Value>(value))
    {
        _value_cbt = _value.subscribe([&](auto...) {
            this->_notifier();
        });
    }

    /// @privatesection
    [[nodiscard]] bool empty_text(widget_intf const* sender) const override
    {
        return _value->empty();
    }

    [[nodiscard]] gstring get_text(widget_intf const* sender) const override
    {
        return _value->translate();
    }
    /// @endprivatesection
private:
    observer<value_type> _value;
    callback<void(value_type)> _value_cbt;
};

default_text_delegate() -> default_text_delegate<>;
default_text_delegate(char const*) -> default_text_delegate<char const*>;
default_text_delegate(std::string const&) -> default_text_delegate<std::string>;
default_text_delegate(std::string&&) -> default_text_delegate<std::string>;
default_text_delegate(gstring const&) -> default_text_delegate<gstring>;
default_text_delegate(gstring&&) -> default_text_delegate<gstring>;
default_text_delegate(txt const&) -> default_text_delegate<txt>;
default_text_delegate(txt&&) -> default_text_delegate<txt>;
default_text_delegate(hi::observer<char const*> const&) -> default_text_delegate<char const*>;
default_text_delegate(hi::observer<char const*>&&) -> default_text_delegate<char const*>;
default_text_delegate(hi::observer<std::string> const&) -> default_text_delegate<std::string>;
default_text_delegate(hi::observer<std::string>&&) -> default_text_delegate<std::string>;
default_text_delegate(hi::observer<hi::gstring> const&) -> default_text_delegate<gstring>;
default_text_delegate(hi::observer<hi::gstring>&&) -> default_text_delegate<gstring>;
default_text_delegate(hi::observer<hi::txt> const&) -> default_text_delegate<txt>;
default_text_delegate(hi::observer<hi::txt>&&) -> default_text_delegate<txt>;

}} // namespace hi::v1
