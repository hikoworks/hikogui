// Copyright Take Vos 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file widgets/selection_delegate.hpp Defines delegate_delegate and some default selection delegates.
 * @ingroup widget_delegates
 */

#pragma once

#include "../l10n/l10n.hpp"
#include "../observer/observer.hpp"
#include "../utility/utility.hpp"
#include "../concurrency/concurrency.hpp"
#include "../dispatch/dispatch.hpp"
#include "../GUI/GUI.hpp"
#include "../macros.hpp"
#include "radio_delegate.hpp"
#include "radio_widget.hpp"
#include <memory>
#include <functional>
#include <vector>

hi_export_module(hikogui.widgets.selection_delegate);

hi_export namespace hi { inline namespace v1 {
class selection_widget;

/** A delegate that controls the state of a selection_widget.
 *
 * @ingroup widget_delegates
 */
class selection_delegate {
public:
    virtual ~selection_delegate() = default;

    virtual void init(widget_intf const* sender) {}
    virtual void deinit(widget_intf const* sender) {}

    /** The id of the widget that will need to get keyboard focus when the pull-down menu is opened.
     *
     * @return The id of the widget that needs keyboard focus.
     * @retval std::nullopt There are no options.
     */
    [[nodiscard]] virtual std::optional<widget_id> keyboard_focus_id(widget_intf const* sender) const
    {
        return std::nullopt;
    }

    /** The number of options in the pull-down menu.
     */
    [[nodiscard]] virtual size_t size(widget_intf const* sender) const
    {
        return 0;
    }

    [[nodiscard]] bool empty(widget_intf const* sender) const
    {
        return this->size(sender) == 0;
    }

    /** Create a new widget that represents the button in the selection menu.
     *
     * @param sender The selection widget that uses this delegate.
     * @param index The index of the option.
     * @return A new widget that represents the option at @a index. 
     */
    [[nodiscard]] virtual std::unique_ptr<widget> make_option_widget(widget_intf const* sender, size_t index) = 0;

    /** Get the label of the selected option.
     *
     * @return The label of the selected option.
     * @retval std::nullopt None of the options has been selected.
     */
    [[nodiscard]] virtual std::optional<label> selected_label(widget_intf const* sender) const
    {
        return std::nullopt;
    }

    /** Subscribe a callback for notifying the widget of a change in the value.
     */
    template<forward_of<void()> Func>
    [[nodiscard]] callback<void()> subscribe_on_value(widget_intf const* sender, Func&& func, callback_flags flags = callback_flags::synchronous)
    {
        return _value_notifier.subscribe(std::forward<Func>(func), flags);
    }

    /** Subscribe a callback for notifying the widget of a change in the options.
     */
    template<forward_of<void()> Func>
    [[nodiscard]] callback<void()> subscribe_on_options(widget_intf const* sender, Func&& func, callback_flags flags = callback_flags::synchronous)
    {
        return _options_notifier.subscribe(std::forward<Func>(func), flags);
    }

protected:
    notifier<void()> _value_notifier;
    notifier<void()> _options_notifier;
};

/** A delegate that control the state of a selection_widget.
 *
 * @ingroup widget_delegates
 * @tparam T the type used as the key for which option is selected.
 */
template<typename T>
class default_selection_delegate : public selection_delegate {
public:
    using value_type = T;
    using option_type = std::pair<value_type, label>;
    using options_type = std::vector<option_type>;

    observer<value_type> value;
    observer<options_type> options;

    /** Construct a default selection delegate.
     *
     * @param value The observer value which represents the selected option.
     * @param options An observer std::vector<std::pair<value_type,label>> of all possible options.
     */
    template<forward_of<observer<value_type>> Value, forward_of<observer<options_type>> Options>
    default_selection_delegate(Value&& value, Options&& options) :
        value(std::forward<Value>(value)), options(std::forward<Options>(options))
    {
        _option_delegate = std::make_shared<option_delegate_type>(*this);

        // clang-format off
        _value_cbt = this->value.subscribe([&](auto...){ this->_value_notifier(); });
        _options_cbt = this->options.subscribe([&](auto...){ this->_options_notifier(); });
        // clang-format on
    }

    [[nodiscard]] size_t size(widget_intf const* sender) const override
    {
        return options->size();
    }

    [[nodiscard]] std::optional<label> selected_label(widget_intf const* sender) const override
    {
        for (auto&& option : *options) {
            if (option.first == *value) {
                return option.second;
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<widget_id> keyboard_focus_id(widget_intf const* sender) const override
    {
        return _option_delegate->keyboard_focus_id();
    }

    /** Create a new widget that represents the button in the selection menu.
     *
     * @param sender The selection widget that uses this delegate.
     * @param index The index of the option.
     * @return A new widget that represents the option at @a index. 
     */
    [[nodiscard]] std::unique_ptr<widget> make_option_widget(widget_intf const* sender, size_t index) override
    {
        auto& [option_value, option_label] = options->at(index);
        return _option_delegate->make_option_widget(sender, option_value, option_label, _option_delegate);
    }

private:
    class option_delegate_type : public radio_delegate {
    public:
        option_delegate_type(default_selection_delegate &parent) : _parent(&parent)
        {
            _value_cbt = _parent->value.subscribe([&](auto...) {
                this->_notifier();
            });
        }

        void init(widget_intf const* sender) override
        {
            assert(sender != nullptr);
            hi_assert(_next_value, "The value was not set of this option widget.");

            auto const it = std::lower_bound(_senders.begin(), _senders.end(), sender->id());
            hi_assert(it == _senders.end() or it->id != sender->id(), "button was already registered with selection-delegate.");

            _senders.emplace(it, sender->id(), *_next_value);
            _next_value = std::nullopt;
        }

        void deinit(widget_intf const* sender) override
        {
            assert(sender != nullptr);

            auto const it = std::lower_bound(_senders.begin(), _senders.end(), sender->id());
            if (it != _senders.end() and it->id == sender->id()) {
                _senders.erase(it);
            }
        }

        [[nodiscard]] std::optional<widget_id> keyboard_focus_id() const
        {
            if (_senders.empty()) {
                return std::nullopt;
            }

            for (auto const& sender : _senders) {
                if (sender.value == *_parent->value) {
                    return sender.id;
                }
            }
            return _senders.front().id;
        }

        [[nodiscard]] std::unique_ptr<widget>
        make_option_widget(widget_intf const* sender, value_type const& value, label const& label, std::shared_ptr<option_delegate_type> shared_this)
        {
            using button_widget = radio_menu_button_widget;

            // Prepare the value for the next widget, so that the widget immediately can retrieve its value.
            _next_value = value;
            auto button = std::make_unique<button_widget>(std::move(shared_this));
            button->label = label;
            return button;
        }

        [[nodiscard]] widget_value state(widget_intf const* sender) const override
        {
            assert(sender != nullptr);

            auto const it = std::lower_bound(_senders.begin(), _senders.end(), sender->id());

            if (it != _senders.end() and it->id == sender->id()) {
                return *_parent->value == it->value ? widget_value::on : widget_value::off;

            } else {
                // button-button was not yet registered.
                return widget_value::off;
            }
        }

        void activate(widget_intf const* sender) override
        {
            assert(sender != nullptr);

            auto const it = std::lower_bound(_senders.begin(), _senders.end(), sender->id());

            if (it != _senders.end() and it->id == sender->id()) {
                _parent->value = it->value;
            }
        }

    private:
        struct sender_info_type {
            widget_id id = {};
            value_type value;

            [[nodiscard]] friend bool operator==(sender_info_type const& lhs, widget_id const& rhs) noexcept
            {
                return lhs.id == rhs;
            }

            [[nodiscard]] friend auto operator<=>(sender_info_type const& lhs, widget_id const& rhs) noexcept
            {
                return lhs.id <=> rhs;
            }
        };

        default_selection_delegate* _parent;
        std::vector<sender_info_type> _senders;
        std::optional<value_type> _next_value;
        callback<void(value_type)> _value_cbt;
    };

    std::shared_ptr<option_delegate_type> _option_delegate;

    callback<void(value_type)> _value_cbt;
    callback<void(options_type)> _options_cbt;
};

template<typename Value, typename Options>
default_selection_delegate(Value&&, Options&&) -> default_selection_delegate<observer_decay_t<Value>>;

}} // namespace hi::v1
