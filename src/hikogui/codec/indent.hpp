// Copyright Take Vos 2020, 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <string>
#include <cstddef>

hi_export_module(hikogui.codec.indent);

hi_export namespace hi { inline namespace v1 {

/** Indentation for writing out text files.
 * This type is used to pass indentation information recursively to
 * add indentation to text files, like for example json.
 */
hi_export class indent {
public:
    constexpr ~indent() noexcept = default;
    constexpr indent() noexcept = default;
    constexpr indent(indent const& other) noexcept = default;
    constexpr indent(indent&& other) noexcept = default;
    constexpr indent& operator=(indent const& other) noexcept = default;
    constexpr indent& operator=(indent&& other) noexcept = default;

    /** Constructor
     * This constructor will start indentation at depth 0.
     *
     * @param spaces Number of spaces per indentation.
     * @param space Character used for indentation.
     */
    [[nodiscard]] constexpr indent(int spaces, char space = ' ') noexcept : _space(space), _spaces(spaces), _depth(0) {}

    /** String conversion operator.
     */
    [[nodiscard]] operator std::string() const noexcept
    {
        return std::string(narrow_cast<std::size_t>(_depth) * narrow_cast<std::size_t>(_spaces), _space);
    }

    /** Increase the depth of this indentation.
     */
    constexpr indent &operator+=(int rhs) noexcept
    {
        _depth += rhs;
        return *this;
    }

    /** Increment the depth of this indentation.
     */
    constexpr indent &operator++() noexcept
    {
        ++_depth;
        return *this;
    }

    /** Get an indentation at increased depth.
     */
    [[nodiscard]] constexpr friend indent operator+(indent lhs, int rhs) noexcept
    {
        lhs += rhs;
        return lhs;
    }

private:
    char _space = ' ';
    int _spaces = 4;
    int _depth = 0;
};

}} // namespace hi::inline v1
