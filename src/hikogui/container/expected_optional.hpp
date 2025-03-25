

#pragma once

#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <expected>
#include <optional>
#include <variant>

hi_export_module(hikogui.parser : expected_optional);

hi_export namespace hi {
inline namespace v1 {

template<typename T, typename E>
class expected_optional {
public:
    using value_type = T;
    using error_type = E;

    constexpr expected_optional(expected_optional const&) noexcept = default;
    constexpr expected_optional(expected_optional&&) noexcept = default;
    constexpr expected_optional& operator=(expected_optional const&) noexcept = default;
    constexpr expected_optional& operator=(expected_optional&&) noexcept = default;

    constexpr expected_optional() noexcept = default;

    constexpr expected_optional(std::nullopt_t) noexcept : _v{std::in_place_index<0>, std::monostate{}} {}

    template<typename Arg = T>
    constexpr explicit(not std::is_convertible_v<Arg, T>) expected_optional(Arg&& arg) noexcept : _v{std::in_place_index<1>, std::forward<Arg>(arg)}
    {
    }

    template<typename Arg>
    constexpr explicit(not std::is_convertible_v<const Arg&, E>) expected_optional(std::unexpected<Arg> const& arg) noexcept :
        _v{std::in_place_index<2>, arg.error()}
    {
    }

    constexpr expected_optional &operator=(std::nullopt_t) noexcept
    {
        _v.template emplace<0>();
        return *this;
    }

    template<typename Arg = T>
    constexpr expected_optional &operator=(Arg &&arg) noexcept
    {
        _v.template emplace<1>(std::forward<Arg>(arg));
        return *this;
    }

    template<typename Arg = E>
    constexpr expected_optional &operator=(std::unexpected<Arg> const &arg) noexcept
    {
        _v.template emplace<2>(arg);
        return *this;
    }

    template<typename Arg = E>
    constexpr expected_optional &operator=(std::unexpected<Arg> &&arg) noexcept
    {
        _v.template emplace<2>(std::move(arg));
        return *this;
    }

    template<typename... Args>
    constexpr value_type &emplace(Args &&... args) noexcept
    {
        return _v.template emplace<1>(std::forward<Args>(args)...);
    }

    [[nodiscard]] constexpr bool has_value() const noexcept
    {
        return _v.index() == 1;
    }

    constexpr explicit operator bool() const noexcept
    {
        return has_value();
    }

    [[nodiscard]] constexpr bool has_error() const noexcept
    {
        return _v.index() == 2;
    }

    [[nodiscard]] constexpr value_type const& operator*() const noexcept
    {
        hi_axiom(_v.index() == 1);
        return std::get<1>(_v);
    }

    [[nodiscard]] constexpr value_type& operator*() noexcept
    {
        hi_axiom(_v.index() == 1);
        return std::get<1>(_v);
    }

    [[nodiscard]] constexpr value_type const* operator->() const noexcept
    {
        hi_axiom(_v.index() == 1);
        return std::addressof(std::get<1>(_v));
    }

    [[nodiscard]] constexpr value_type* operator->() noexcept
    {
        hi_axiom(_v.index() == 1);
        return std::addressof(std::get<1>(_v));
    }

    [[nodiscard]] constexpr error_type const& error() const noexcept
    {
        hi_axiom(_v.index() == 2);
        return std::get<2>(_v);
    }

    [[nodiscard]] constexpr error_type& error() noexcept
    {
        hi_axiom(_v.index() == 2);
        return std::get<2>(_v);
    }

private:
    std::variant<std::monostate, T, E> _v;
};

} // namespace v1
}
