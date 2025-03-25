// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "observed.hpp"
#include "../utility/utility.hpp"
#include "../dispatch/dispatch.hpp"
#include "../concurrency/thread.hpp" // XXX #616
#include "../macros.hpp"
#include <concepts>
#include <utility>
#include <format>
#include <new>
#include <algorithm>
#include <memory>

hi_export_module(hikogui.observer : observer_intf);

hi_export namespace hi { inline namespace v1 {

/** A observer pointing to the whole or part of a observed_base.
 *
 * A observer will point to a observed_base that was created, or possibly
 * an anonymous observed_base, which is created when a observer is created
 * as empty.
 *
 * @tparam T The type of observer.
 */
template<typename T>
class observer {
public:
    using value_type = T;
    using notifier_type = notifier<void(value_type)>;
    using callback_type = notifier_type::callback_type;
    using awaiter_type = notifier_type::awaiter_type;
    using path_type = observable_msg::path_type;

    /** A proxy object of the observer.
     *
     * The proxy is a RAII object that makes sure that listeners will
     * get notified if the value was modified.
     */
    class proxy_type {
    public:
        using reference = value_type &;
        using const_reference = value_type const &;
        using pointer = value_type *;
        using const_pointer = value_type const *;

        /** Commits and destruct the proxy object.
         *
         * If `commit()` or `abort()` are called or the proxy object
         * is empty then the destructor does not commit the changes.
         */
        ~proxy_type() noexcept
        {
            if (_observer != nullptr) {
                hi_axiom_not_null(_ptr);
                if (_original_value) {
                    if constexpr (requires(value_type const& a, value_type const& b) { a != b; }) {
                        if (*_original_value != *_ptr) {
                            _observer->notify();
                        }
                    } else {
                        _observer->notify();
                    }
                }
            }
        }

        proxy_type(proxy_type const&) = delete;
        proxy_type& operator=(proxy_type const&) = delete;

        proxy_type(proxy_type&& other) noexcept :
            _observer(std::exchange(other._observer, nullptr)),
            _ptr(std::exchange(other._ptr, nullptr)),
            _original_value(std::exchange(other._original_value, std::nullopt))
        {
        }

        proxy_type& operator=(proxy_type&& other) noexcept
        {
            _observer->notify();
            _observer = std::exchange(other._observer, nullptr);
            _ptr = std::exchange(other._ptr, nullptr);
            _original_value = std::exchange(other._original_value, std::nullopt);
            return *this;
        }

        /** Create a proxy object.
         *
         * @param observer a pointer to the observer.
         * @param ptr a pointer to the sub-object of the shared_state that the observer
         *            is pointing to.
         */
        proxy_type(observer *observer, value_type *ptr) noexcept :
            _observer(observer), _ptr(ptr), _original_value(std::nullopt)
        {
            hi_axiom_not_null(_observer);
            hi_axiom_not_null(_ptr);
        }

        /** Create a proxy object.
         *
         * @param observer a pointer to the observer.
         * @param ptr a pointer to the sub-object of the shared_state that the observer
         *            is pointing to.
         */
        proxy_type() noexcept :
            _observer(nullptr), _ptr(nullptr), _original_value(std::nullopt)
        {
        }

        /** Dereference the value.
         *
         * This function allows reads and modification to the value
         *
         * @note It is undefined behavior to call this function after calling `commit()` or `abort()`
         */
        reference operator*() noexcept
        {
            start_write();
            return *_ptr;
        }

        /** Dereference the value.
         *
         * This function allows reads and modification to the value
         *
         * @note It is undefined behavior to call this function after calling `commit()` or `abort()`
         */
        const_reference operator*() const noexcept
        {
            hi_axiom_not_null(_ptr);
            return *_ptr;
        }

        /** Pointer dereference the value.
         *
         * This function allows reads and modification to the value, including
         * calling member functions on the value.
         *
         * @note It is undefined behavior to call this function after calling `commit()` or `abort()`
         */
        pointer operator->() noexcept
        {
            start_write();
            return _ptr;
        }

        /** Pointer dereference the value.
         *
         * This function allows reads and modification to the value, including
         * calling member functions on the value.
         *
         * @note It is undefined behavior to call this function after calling `commit()` or `abort()`
         */
        const_pointer operator->() const noexcept
        {
            hi_axiom_not_null(_ptr);
            return _ptr;
        }

        /** Pointer dereference the value.
         *
         * This function allows reads and modification to the value, including
         * calling member functions on the value.
         *
         * @note It is undefined behavior to call this function after calling `commit()` or `abort()`
         */
        pointer operator&() noexcept
        {
            start_write();
            return _ptr;
        }

        /** Pointer dereference the value.
         *
         * This function allows reads and modification to the value, including
         * calling member functions on the value.
         *
         * @note It is undefined behavior to call this function after calling `commit()` or `abort()`
         */
        const_pointer operator&() const noexcept
        {
            hi_axiom_not_null(_ptr);
            return _ptr;
        }

        // clang-format off

        // prefix operators
#define X(op) \
        template<typename Rhs> \
        decltype(auto) operator op() noexcept \
            requires requires(value_type& a) { op a; } \
        { \
            start_write(); \
            return op (*_ptr); \
        }
    
        X(++)
        X(--)
#undef X
    
        // suffix operators
#define X(op) \
        template<typename Rhs> \
        auto operator op(int) noexcept \
            requires requires(value_type& a) { a op; } \
        { \
            start_write(); \
            return (*_ptr) op; \
        }
    
        X(++)
        X(--)
#undef X
    
        // inplace operators
#define X(op) \
        template<typename Rhs> \
        decltype(auto) operator op(Rhs const& rhs) noexcept \
            requires requires(value_type& a, Rhs const& b) { a op b; } \
        { \
            start_write(); \
            return (*_ptr) op rhs; \
        }
    
        X(+=)
        X(-=)
        X(*=)
        X(/=)
        X(%=)
        X(&=)
        X(|=)
        X(^=)
        X(<<=)
        X(>>=)
#undef X
    
        // mono operators
#define X(op) \
        template<typename Rhs> \
        auto operator op() const noexcept \
            requires requires(value_type const& a) { op a; } \
        { \
            hi_axiom_not_null(_ptr); \
            return op (*_ptr); \
        }
    
        X(-)
        X(+)
        X(~)
        X(!)
#undef X
    
        // binary operators
#define X(op) \
        template<typename Rhs> \
        auto operator op(Rhs const& rhs) const noexcept \
            requires requires(value_type const& a, Rhs const& b) { a op b; } \
        { \
            hi_axiom_not_null(_ptr); \
            return (*_ptr) op rhs; \
        }
    
        X(==)
        X(<=>)
        X(+)
        X(-)
        X(*)
        X(/)
        X(%)
        X(&)
        X(|)
        X(^)
        X(<<)
        X(>>)
#undef X
    
        // call operator
        template<typename... Args>
        auto operator()(Args &&... args) const noexcept
            requires requires(value_type const & a, Args &&...args) { a(std::forward<Args>(args)...); }
        {
            hi_axiom_not_null(_ptr);
            return (*_ptr)(std::forward<Args>(args)...);
        }
    
        template<typename... Args>
        decltype(auto) operator()(Args &&... args) noexcept
            requires requires(value_type & a, Args &&...args) { a(std::forward<Args>(args)...); }
        {
            start_write();
            return (*_ptr)(std::forward<Args>(args)...);
        }
        
        // index operator
        // XXX c++23
        template<typename Arg>
        auto operator[](Arg && arg) const noexcept
            requires requires(value_type const & a, Arg &&arg) { a[std::forward<Arg>(arg)]; }
        {
            hi_axiom_not_null(_ptr);
            return (*_ptr)[std::forward<Arg>(arg)];
        }

        // clang-format on

    private:
        observer *_observer;
        value_type *_ptr;
        std::optional<value_type> _original_value;

        void start_write() noexcept
        {
            if (not _original_value) {
                hi_axiom_not_null(_ptr);
                _original_value = *_ptr;
            }
        }

        friend class observer;
    };

    using const_reference = value_type const &;
    using pointer = proxy_type;
    using const_pointer = value_type const *;

    constexpr ~observer() = default;

    /** Create an observer from an observed_base.
     *
     * @param observed_base The `observed_base` which will be observed_base by this observer.
     */
    template<forward_of<std::shared_ptr<hi::observed_base>> Observed>
    observer(Observed&& observed_base) noexcept :
        observer(group_ptr<hi::observed_base>{std::forward<Observed>(observed_base)}, path_type{}, [](void *base) {
            return base;
        })
    {
    }

    /** Create a observer linked to an anonymous default initialized observed_base-value.
     *
     * @note marked 'explicit' so that accidental assignment with {} is not allowed.
     */
    constexpr explicit observer() noexcept : observer(std::make_shared<observed<value_type>>()) {}

    /** Create a observer linked to an anonymous observed_base-value.
     */
    template<std::convertible_to<value_type> Value>
    constexpr observer(Value && value) noexcept :
        observer(std::make_shared<observed<value_type>>(std::forward<Value>(value)))
    {
    }

    /** Copy construct.
     *
     * @note callback subscriptions are not copied.
     * @param other The other observer.
     */
    constexpr observer(observer const& other) noexcept : observer(other._observed, other._path, other._convert) {}

    /** Move construct.
     *
     * @note callback subscriptions are not copied.
     * @param other The other observer.
     */
    constexpr observer(observer&& other) noexcept :
        observer(std::move(other._observed), std::move(other._path), std::move(other._convert))
    {
        other.reset();
    }

    /** Copy assign.
     *
     * @note callback subscriptions remain unchanged and are not copied.
     * @param other The other observer.
     * @return this
     */
    constexpr observer& operator=(observer const& other) noexcept
    {
        _observed = other._observed;
        _path = other._path;
        _convert = other._convert;

        // Rewire the callback subscriptions and notify listeners to this observer.
        update_state_callback();
        _observed->notify_group_ptr(observable_msg{_observed->get(), _path});
        return *this;
    }

    /** Move assign.
     *
     * @note Callback subscriptions remain unchanged and are not moved.
     * @note The other shared observer will be attached to the anonymous state.
     * @param other The other observer.
     * @return this
     */
    constexpr observer& operator=(observer&& other) noexcept
    {
        _observed = std::move(other._observed);
        _path = std::move(other._path);
        _convert = std::move(other._convert);
        other.reset();

        // Rewire the callback subscriptions and notify listeners to this observer.
        update_state_callback();
        _observed->notify_group_ptr(observable_msg{_observed->get(), _path});
        return *this;
    }

    /** Reset the observer.
     *
     * This will link the observer with an anonymous observed_base with a default initialized value.
     */
    void reset() noexcept
    {
        _observed = std::make_shared<observed<value_type>>();
        _path = {};
        _convert = [](void *base) {
            return base;
        };
        update_state_callback();
    }

    /** Read the observed_base value.
     *
     * @return A const-proxy object used to access the data being observed_base.
     */
    [[nodiscard]] const_pointer get() const noexcept
    {
        return convert(_observed->get());
    }

    /** Make a copy of the observed_base value for modification.
     *
     * @return A proxy object used to modify the data being observed_base.
     */
    [[nodiscard]] pointer get() noexcept
    {
        return proxy_type{this, convert(_observed->get())};
    }

    /** Subscribe a callback to this observer.
     *
     * @param flags The callback flags on how to call the function.
     * @param function The function used as callback in the form `void()`
     * @return A callback-token used to extend the lifetime of the callback function.
     */
    template<forward_of<void(value_type)> Func>
    [[nodiscard]] callback<void(value_type)> subscribe(Func &&func, callback_flags flags = callback_flags::synchronous) noexcept
    {
        return _notifier.subscribe(std::forward<Func>(func), flags);
    }

    awaiter_type operator co_await() const noexcept
    {
        return _notifier.operator co_await();
    }

    /** Create a sub-observer by indexing into the value.
     *
     * @param index The index into the value being observed_base.
     * @return A new sub-observer which monitors the selected sub-value.
     */
    [[nodiscard]] auto sub(auto const& index) const noexcept
        requires(requires() { std::declval<value_type>()[index]; })
    {
        using result_type = std::decay_t<decltype(std::declval<value_type>()[index])>;

        auto new_path = _path;
        new_path.push_back(std::format("[{}]", index));
        return observer<result_type>{
            _observed, std::move(new_path), [convert_copy = this->_convert, index](void *base) -> void * {
                return std::addressof((*std::launder(static_cast<value_type *>(convert_copy(base))))[index]);
            }};
    }

    /** Create a sub-observer by selecting a member-variable of the value.
     *
     * @note This function requires the `hi::selector` type-trait to be implemented for `value_type`.
     * @tparam Name The name of the member-variable of value.
     * @return A new sub-observer which monitors the member of value.
     */
    template<fixed_string Name>
    [[nodiscard]] auto sub() const noexcept
    {
        using result_type = std::decay_t<decltype(selector<value_type>{}.template get<Name>(std::declval<value_type&>()))>;

        auto new_path = _path;
        new_path.push_back(std::string{Name});
        // clang-format off
        return observer<result_type>(
            _observed,
            std::move(new_path),
            [convert_copy = this->_convert](void *base) -> void * {
                return std::addressof(selector<value_type>{}.template get<Name>(
                    *std::launder(static_cast<value_type *>(convert_copy(base)))));
            });
        // clang-format on
    }

    //
    // Convenient functions / operators working on temporary proxies.
    //

    // clang-format off

    /** Assign a new value to the observed_base value.
     */
    template<typename Rhs>
    observer& operator=(Rhs&& rhs) noexcept
        requires requires (value_type &a, Rhs &&b) { a = std::forward<Rhs>(b); }
    {
        *get() = std::forward<Rhs>(rhs);
        return *this;
    }

    /** Get the value being observed_base.
     */
    const_reference operator*() const noexcept
    {
        // This returns a copy of the dereferenced value of the proxy.
        // The proxy's lifetime will be extended for the copy to be made.
        return *get();
    }

    const_pointer operator&() const noexcept
    {
        return get();
    }

    /** Constant pointer-to-member of the value being observed_base.
     */
    const_pointer operator->() const noexcept
    {
        return get();
    }

    /** Constant pointer-to-member of the value being observed_base.
     */
    pointer operator->() noexcept
    {
        return get();
    }

    // prefix operators
#define X(op) \
    template<typename Rhs> \
    observer& operator op() noexcept \
        requires requires(value_type& a) { op a; } \
    { \
        op *get(); \
        return *this; \
    }

    X(++)
    X(--)
#undef X

    // suffix operators
#define X(op) \
    template<typename Rhs> \
    auto operator op(int) noexcept \
        requires requires(value_type& a) { a op; } \
    { \
        return *get() op; \
    }

    X(++)
    X(--)
#undef X

    // inplace operators
#define X(op) \
    template<typename Rhs> \
    observer& operator op(Rhs const& rhs) noexcept \
        requires requires(value_type& a, Rhs const& b) { a op b; } \
    { \
        *get() op rhs; \
        return *this; \
    }

    X(+=)
    X(-=)
    X(*=)
    X(/=)
    X(%=)
    X(&=)
    X(|=)
    X(^=)
    X(<<=)
    X(>>=)
#undef X

    // mono operators
#define X(op) \
    template<typename Rhs> \
    auto operator op() const noexcept \
        requires requires(value_type const& a) { op a; } \
    { \
        return op *get(); \
    }

    X(-)
    X(+)
    X(~)
    X(!)
#undef X

    // binary operators
#define X(op) \
    template<typename Rhs> \
    auto operator op(Rhs const& rhs) const noexcept \
        requires requires(value_type const& a, Rhs const& b) { a op b; } \
    { \
        return *get() op rhs; \
    }

    X(==)
    X(<=>)
    X(+)
    X(-)
    X(*)
    X(/)
    X(%)
    X(&)
    X(|)
    X(^)
    X(<<)
    X(>>)
#undef X

    // call operator
    template<typename... Args>
    auto operator()(Args &&... args) const noexcept
        requires requires(value_type const & a, Args &&...args) { a(std::forward<Args>(args)...); }
    {
        return (*get())(std::forward<Args>(args)...);
    }

    // index operator
    // XXX c++23
    template<typename Arg>
    auto operator[](Arg && arg) const noexcept
        requires requires(value_type const & a, Arg &&arg) { a[std::forward<Arg>(arg)]; }
    {
        return (*get())[std::forward<Arg>(arg)];
    }

    // clang-format on

private:
    using observed_type = group_ptr<observed_base>;
    observed_type _observed = {};
    path_type _path = {};
    std::function<void *(void *)> _convert = {};
    notifier_type _notifier;
#ifndef NDEBUG
    value_type _debug_value;
#endif

    /** Construct an observer from an observed_base.
     */
    template<forward_of<observed_type> ObservedBase, forward_of<path_type> Path, forward_of<void *(void *)> Converter>
    observer(
        ObservedBase&& observed_base,
        Path&& path,
        Converter&& converter) noexcept :
        _observed(std::forward<ObservedBase>(observed_base)), _path(std::forward<Path>(path)), _convert(std::forward<Converter>(converter)), _notifier()
    {
        update_state_callback();
    }

    void notify() const noexcept
    {
        _observed->notify_group_ptr(observable_msg{_observed->get(), _path});
    }

    value_type *convert(void *base) const noexcept
    {
        return std::launder(static_cast<value_type *>(_convert(base)));
    }

    value_type const *convert(void const *base) const noexcept
    {
        return std::launder(static_cast<value_type const *>(_convert(const_cast<void *>(base))));
    }

    void update_state_callback() noexcept
    {
        _observed.subscribe([this](observable_msg const& msg) {
            auto const [msg_it, this_it] = std::mismatch(msg.path.cbegin(), msg.path.cend(), _path.cbegin(), _path.cend());
            // If the message's path is fully within the this' path, then this is a sub-path.
            // If this' path is fully within the message's path, then this is along the path.
            if (msg_it == msg.path.cend() or this_it == _path.cend()) {
#ifndef NDEBUG
                _debug_value = *convert(msg.ptr);
#endif
                _notifier(*convert(msg.ptr));
            }
        });

#ifndef NDEBUG
        _debug_value = *convert(_observed->get());
#endif
    }

    // It is possible to make sub-observables.
    template<typename>
    friend class observer;
};

/** A type-trait for observer arguments.
 *
 * returns `type` with the following forms of @a T:
 *  - type
 *  - type &
 *  - type const &
 *  - type &&
 *  - observer<type>
 *  - observer<type> &
 *  - observer<type> const &
 *  - observer<type> &&
 */
template<typename T>
struct observer_decay {
    using type = std::remove_cvref_t<T>;
};

// clang-format off
template<std::equality_comparable T> struct observer_decay<observer<T>> { using type = T; };
template<std::equality_comparable T> struct observer_decay<observer<T> &> { using type = T; };
template<std::equality_comparable T> struct observer_decay<observer<T> const &> { using type = T; };
template<std::equality_comparable T> struct observer_decay<observer<T> &&> { using type = T; };

// clang-format on

template<typename T>
using observer_decay_t = observer_decay<T>::type;

template<typename Context, typename Expected>
struct is_forward_of<Context, observer<Expected>> :
    std::conditional_t<std::is_convertible_v<Context, observer<Expected>>, std::true_type, std::false_type> {};


template<typename Context, typename Expected>
concept forward_observer = forward_of<Context, observer<observer_decay_t<Expected>>>;

}} // namespace hi::inline v1
