// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../macros.hpp"
#include "assert.hpp"
#include "cast.hpp"
#include "terminate.hpp"
#include "exception.hpp"
#include "misc.hpp"
#include <string>
#include <string_view>
#include <format>
#include <array>
#include <ranges>
#include <compare>

hi_export_module(hikogui.utility.fixed_string);

hi_export namespace hi { inline namespace v1 {

/** A string which may be used as a none-type template parameter.
 *
 * example:
 *     ```
 *     template<hi::fixed_string Foo>
 *     class A {
 *         auto bar() {
 *             return std::string{Foo};
 *         }
 *     };
 *
 *     std::string test() {
 *         auto a = A<"hello">{};
 *         return a.bar();
 *     }
 *     ```
 */
hi_export template<size_t N>
struct fixed_string {
    using value_type = char;

    std::array<char, N> _str = {};

    constexpr fixed_string() noexcept = default;
    constexpr fixed_string(fixed_string const&) noexcept = default;
    constexpr fixed_string(fixed_string&&) noexcept = default;
    constexpr fixed_string& operator=(fixed_string const&) noexcept = default;
    constexpr fixed_string& operator=(fixed_string&&) noexcept = default;

    template<std::size_t O>
    constexpr fixed_string(char const (&str)[O]) noexcept
        requires((O - 1) == N)
    {
        for (auto i = 0_uz; i != (O - 1); ++i) {
            _str[i] = str[i];
        }
    }

    /** Create a fixed string from function returning a string-like.
     */
    template<std::invocable F>
    constexpr fixed_string(F const& f) noexcept
    {
        auto str = f();

        hi_assert(str.size() == N);
        for (auto i = 0_uz; i != str.size(); ++i) {
            _str[i] = char_cast<char>(str[i]);
        }
    }

    constexpr operator std::string_view() const noexcept
    {
        return std::string_view{_str.data(), size()};
    }

    constexpr operator std::string() const noexcept
    {
        return std::string{_str.data(), size()};
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        return N;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return N == 0;
    }

    template<size_t I>
    [[nodiscard]] constexpr friend char& get(fixed_string& a) noexcept
    {
        return std::get<I>(a._str);
    }

    template<size_t I>
    [[nodiscard]] constexpr friend char const& get(fixed_string const& a) noexcept
    {
        return std::get<I>(a._str);
    }

    [[nodiscard]] constexpr char& operator[](size_t index) noexcept
    {
#ifndef NDEBUG
        if (not(index < N)) {
            std::terminate();
        }
#endif
        return _str[index];
    }

    [[nodiscard]] constexpr char const& operator[](size_t index) const noexcept
    {
#ifndef NDEBUG
        if (not(index < N)) {
            std::terminate();
        }
#endif
        return _str[index];
    }

    [[nodiscard]] constexpr auto begin() noexcept
    {
        return _str.begin();
    }

    [[nodiscard]] constexpr auto end() noexcept
    {
        return _str.begin() + size();
    }

    [[nodiscard]] constexpr bool operator==(fixed_string const& rhs) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(fixed_string const& rhs) const noexcept = default;

    template<size_t O>
    [[nodiscard]] constexpr bool operator==(fixed_string<O> const& rhs) const noexcept
        requires(O != N)
    {
        return false;
    }

    template<size_t O>
    [[nodiscard]] constexpr auto operator<=>(fixed_string<O> const& rhs) const noexcept
        requires(O != N)
    {
        return static_cast<std::string_view>(*this) <=> static_cast<std::string_view>(rhs);
    }

    [[nodiscard]] constexpr bool operator==(std::string_view rhs) const noexcept
    {
        return static_cast<std::string_view>(*this) == rhs;
    }

    [[nodiscard]] constexpr auto operator<=>(std::string_view rhs) const noexcept
    {
        return static_cast<std::string_view>(*this) <=> rhs;
    }

    [[nodiscard]] constexpr bool operator==(std::string const& rhs) const noexcept
    {
        return static_cast<std::string_view>(*this) == rhs;
    }

    [[nodiscard]] constexpr auto operator<=>(std::string const& rhs) const noexcept
    {
        return static_cast<std::string_view>(*this) <=> rhs;
    }

    [[nodiscard]] constexpr bool operator==(char const *rhs) const noexcept
    {
        return static_cast<std::string_view>(*this) == rhs;
    }

    [[nodiscard]] constexpr auto operator<=>(char const *rhs) const noexcept
    {
        return static_cast<std::string_view>(*this) <=> rhs;
    }

    template<size_t O>
    [[nodiscard]] constexpr bool operator==(char const (&rhs)[O]) const noexcept
    {
        return *this == fixed_string<O - 1>(rhs);
    }

    template<size_t O>
    [[nodiscard]] constexpr auto operator<=>(char const (&rhs)[O]) const noexcept
    {
        return *this <=> fixed_string<O - 1>(rhs);
    }

    /** Append two strings.
     */
    template<size_t R>
    [[nodiscard]] constexpr auto operator+(fixed_string<R> const& rhs) const noexcept
    {
        auto r = fixed_string<N + R>{};
        auto dst_i = 0_uz;
        for (auto src_i = 0_uz; src_i != N; ++src_i, ++dst_i) {
            r[dst_i] = (*this)[src_i];
        }
        for (auto src_i = 0_uz; src_i != R; ++src_i, ++dst_i) {
            r[dst_i] = rhs[src_i];
        }

        return r;
    }

    template<size_t R>
    [[nodiscard]] constexpr auto operator+(char const (&rhs)[R]) const noexcept
    {
        return *this + fixed_string<R - 1>(rhs);
    }

    /** Join two strings with a slash '/'.
     *
     * If one or both of the operands is empty, no '/' is added.
     */
    template<size_t R>
    [[nodiscard]] constexpr auto operator/(fixed_string<R> const& rhs) const noexcept
    {
        constexpr auto has_dot = N != 0 and R != 0 ? 1_uz : 0_uz;
        auto r = fixed_string<N + R + has_dot>{};

        auto dst_i = 0_uz;
        for (auto src_i = 0_uz; src_i != N; ++src_i, ++dst_i) {
            r[dst_i] = (*this)[src_i];
        }

        if (has_dot) {
            r[dst_i++] = '/';
        }

        for (auto src_i = 0_uz; src_i != R; ++src_i, ++dst_i) {
            r[dst_i] = rhs[src_i];
        }

        return r;
    }

    template<size_t R>
    [[nodiscard]] constexpr auto operator/(char const (&rhs)[R]) const noexcept
    {
        return *this / fixed_string<R - 1>(rhs);
    }
};

hi_export template<fixed_string Tag>
[[nodiscard]] consteval uint32_t fourcc() noexcept
{
    static_assert(Tag.size() == 4, "fourcc must get a 4 character fixed_string");

    return (static_cast<uint32_t>(get<0>(Tag)) << 24) | (static_cast<uint32_t>(get<1>(Tag)) << 16) |
        (static_cast<uint32_t>(get<2>(Tag)) << 8) | static_cast<uint32_t>(get<3>(Tag));
}

[[nodiscard]] constexpr uint32_t fourcc(fixed_string<4> const& tag) noexcept
{
    return (static_cast<uint32_t>(get<0>(tag)) << 24) | (static_cast<uint32_t>(get<1>(tag)) << 16) |
        (static_cast<uint32_t>(get<2>(tag)) << 8) | static_cast<uint32_t>(get<3>(tag));
}

hi_export template<fixed_string Tag>
consteval uint32_t operator"" _fcc()
{
    return fourcc<Tag>();
}

hi_export template<std::size_t N>
fixed_string(char const (&str)[N]) -> fixed_string<N - 1>;

hi_export template<std::invocable F>
fixed_string(F const& f) -> fixed_string<F{}().size()>;

// clang-format off
#define hi_to_fixed_string(x) ::hi::fixed_string{[]{ return x; }}
// clang-format on

}} // namespace hi::v1

// XXX #617 MSVC bug does not handle partial specialization in modules, outside a namespace.
namespace std {
hi_export template<std::size_t N>
struct formatter<hi::fixed_string<N>, char> : formatter<std::string_view, char> {
    constexpr auto format(hi::fixed_string<N> const& t, auto& fc) const
    {
        return std::formatter<std::string_view, char>::format(static_cast<std::string_view>(t), fc);
    }
};
}
