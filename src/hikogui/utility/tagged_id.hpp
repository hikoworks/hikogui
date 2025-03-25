// Copyright Take Vos 2020-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../macros.hpp"
#include "terminate.hpp"
#include <gsl/gsl>
#include <limits>
#include <typeinfo>
#include <typeindex>
#include <compare>
#include <concepts>
#include <bit>
#include <optional>
#include <string>
#include <ostream>
#include <cstddef>
#include <utility>

hi_export_module(hikogui.utility.tagged_id);

hi_export namespace hi::inline v1 {

namespace detail {

template<typename Derived, std::unsigned_integral T, T Empty>
std::atomic<T> tagged_id_counter = Empty == 0 ? T{1} : T{0};

}

/** A tagged identifier.
 *
 * @tparam Derived The derived class.
 * @tparam T The unsigned integer type used as the underlying type.
 * @tparam Empty The underlying value the means empty. Often either zero or
 *               std::numeric_limits<T>::max().
 */
template<typename Derived, std::unsigned_integral T, T Empty>
class tagged_id {
public:
    using derived_type = Derived;
    using value_type = T;
    constexpr static value_type empty_value = Empty;

    constexpr tagged_id() noexcept = default;
    constexpr tagged_id(tagged_id const& other) noexcept = default;
    constexpr tagged_id(tagged_id&& other) noexcept = default;
    constexpr tagged_id& operator=(tagged_id const& other) noexcept = default;
    constexpr tagged_id& operator=(tagged_id&& other) noexcept = default;
    constexpr tagged_id(std::nullopt_t) noexcept : _v(empty_value) {}

    constexpr tagged_id(value_type rhs) : _v(rhs) {
        if (rhs == empty_value) {
            throw std::domain_error("The given identifier was the empty-value");
        }
    }

    template<std::integral RHS>
    constexpr tagged_id(RHS rhs) : _v(gsl::narrow<value_type>(rhs)) {
        if (rhs == empty_value) {
            throw std::domain_error("The given identifier was the empty-value");
        }
    }

    /** Make a new unique identifier.
     */
    [[nodiscard]] static tagged_id make()
    {
        return detail::tagged_id_counter<derived_type, value_type, empty_value>.fetch_add(1, std::memory_order::relaxed);
    }

    constexpr tagged_id& operator=(value_type rhs)
    {
        if (rhs == empty_value) {
            throw std::overflow_error("The given identifier was the empty-value");
        }

        _v = rhs;
        return *this;
    }

    constexpr tagged_id& operator=(std::nullopt_t) noexcept
    {
        _v = empty_value;
        return *this;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return _v == empty_value;
    }

    constexpr explicit operator bool() const noexcept
    {
        return not empty();
    }

    [[nodiscard]] constexpr value_type value() const
    {
        if (_v == empty_value) {
            throw std::overflow_error("Dereferencing an empty identifier");
        }
        return _v;
    }

    constexpr explicit operator value_type() const
    {
        return value();
    }

    [[nodiscard]] constexpr value_type operator*() const
    {
        return value();
    }

    [[nodiscard]] constexpr friend auto operator<=>(tagged_id const&, tagged_id const &) noexcept = default;
    [[nodiscard]] constexpr friend bool operator==(tagged_id const&, tagged_id const &) noexcept = default;
private:
    value_type _v = empty_value;
};

} // namespace hi::inline v1

hi_export template<typename Derived, std::unsigned_integral T, T Empty>
struct std::hash<hi::tagged_id<Derived, T, Empty>> {
    [[nodiscard]] constexpr std::size_t operator()(hi::tagged_id<Derived, T, Empty> const& rhs) const noexcept
    {
        return std::hash<T>{}(rhs.value());
    }
};
