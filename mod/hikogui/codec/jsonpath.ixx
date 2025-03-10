// Copyright Take Vos 2021-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include "../macros.hpp"

#include <string>
#include <variant>
#include <string_view>
#include <vector>
#include <limits>
#include <format>
#include <coroutine>

export module hikogui_codec_jsonpath;
import hikogui_coroutine;
import hikogui_parser;
import hikogui_utility;

export namespace hi { inline namespace v1 {

export class jsonpath {
public:
    struct root {
        [[nodiscard]] constexpr std::string string() const noexcept
        {
            return "$";
        }

        [[nodiscard]] constexpr bool is_singular() const noexcept
        {
            return true;
        }
    };

    struct current {
        [[nodiscard]] constexpr std::string string() const noexcept
        {
            return "@";
        }

        [[nodiscard]] constexpr bool is_singular() const noexcept
        {
            return true;
        }
    };

    struct wildcard {
        [[nodiscard]] constexpr std::string string() const noexcept
        {
            return "[*]";
        }

        [[nodiscard]] constexpr bool is_singular() const noexcept
        {
            return false;
        }
    };

    struct descend {
        [[nodiscard]] constexpr std::string string() const noexcept
        {
            return "..";
        }

        [[nodiscard]] constexpr bool is_singular() const noexcept
        {
            return false;
        }
    };

    struct names : public std::vector<std::string> {
        constexpr names(names const&) noexcept = default;
        constexpr names(names&&) noexcept = default;
        constexpr names& operator=(names const&) noexcept = default;
        constexpr names& operator=(names&&) noexcept = default;
        constexpr names() noexcept = default;

        constexpr names(std::string other) : names()
        {
            push_back(std::move(other));
        }

        [[nodiscard]] constexpr std::string string() const noexcept
        {
            auto r = std::string{"["};
            auto first = true;
            for (hilet& name : *this) {
                if (not first) {
                    r += ',';
                }

                r += '\'';
                r += name;
                r += '\'';
                first = false;
            }
            r += ']';
            return r;
        }

        [[nodiscard]] constexpr bool is_singular() const noexcept
        {
            return size() == 1;
        }
    };

    struct indices : public std::vector<ptrdiff_t> {
        constexpr indices(indices const&) noexcept = default;
        constexpr indices(indices&&) noexcept = default;
        constexpr indices& operator=(indices const&) noexcept = default;
        constexpr indices& operator=(indices&&) noexcept = default;
        constexpr indices() noexcept = default;

        [[nodiscard]] constexpr std::string string() const noexcept
        {
            auto r = std::string{"["};
            auto first = true;
            for (hilet index : *this) {
                if (not first) {
                    r += ',';
                }

                r += hi::to_string(index);
                first = false;
            }
            r += ']';
            return r;
        }

        [[nodiscard]] generator<std::size_t> filter(std::size_t size) const noexcept
        {
            hilet size_ = narrow_cast<ptrdiff_t>(size);

            for (hilet index : *this) {
                hilet index_ = index >= 0 ? index : size_ + index;
                if (index_ >= 0 and index_ < size_) {
                    co_yield narrow_cast<std::size_t>(index_);
                }
            }
        }

        [[nodiscard]] constexpr bool is_singular() const noexcept
        {
            return size() == 1;
        }
    };

    struct slice {
        ptrdiff_t first;
        ptrdiff_t last;
        ptrdiff_t step;

        constexpr slice(slice const&) noexcept = default;
        constexpr slice(slice&&) noexcept = default;
        constexpr slice& operator=(slice const&) noexcept = default;
        constexpr slice& operator=(slice&&) noexcept = default;

        constexpr slice(ptrdiff_t first, ptrdiff_t last, ptrdiff_t step) noexcept : first(first), last(last), step(step) {}

        [[nodiscard]] constexpr bool last_is_empty() const noexcept
        {
            return last == std::numeric_limits<ptrdiff_t>::min();
        }

        /** Get the start offset.
         *
         * @param size The size of the container.
         * @return The start offset of the slice.
         */
        [[nodiscard]] constexpr std::size_t begin(std::size_t size) const noexcept
        {
            hilet size_ = narrow_cast<ptrdiff_t>(size);
            hilet begin = first >= 0 ? first : size_ + first;
            return narrow_cast<std::size_t>(std::clamp(begin, 0_z, size_));
        }

        /** Get the one-step beyond last offset.
         * This will calculate the last offset of an integer number of steps
         * starting from begin(). This allows the end() to be equality compared inside
         * a for loop, even when the step is negative.
         *
         * @param size The size of the container.
         * @return The one-step beyond last offset of the slice.
         */
        [[nodiscard]] constexpr std::size_t end(std::size_t size) const noexcept
        {
            if (last_is_empty()) {
                return size;

            } else {
                hilet size_ = narrow_cast<ptrdiff_t>(size);
                hilet end = last >= 0 ? last : size_ + last;
                hilet last_ = std::clamp(end, 0_z, size_);

                hilet first_ = begin(size);
                hilet distance = last_ - first_;
                hilet steps = distance / step;
                return narrow_cast<std::size_t>(first_ + steps * step);
            }
        }

        [[nodiscard]] constexpr std::string string() const noexcept
        {
            if (last_is_empty()) {
                return std::format("[{}:e:{}]", first, step);
            } else {
                return std::format("[{}:{}:{}]", first, last, step);
            }
        }

        [[nodiscard]] constexpr bool is_singular() const noexcept
        {
            return false;
        }
    };

    using node = std::variant<root, current, wildcard, descend, names, indices, slice>;
    using container_type = std::vector<node>;
    using value_type = typename container_type::value_type;
    using iterator = typename container_type::iterator;
    using const_iterator = typename container_type::const_iterator;

    template<std::input_iterator It, std::sentinel_for<It> ItEnd>
    [[nodiscard]] constexpr jsonpath(It it, ItEnd last) : _nodes()
    {
        auto lexer_it = lexer<lexer_config::json_style()>.parse(it, last);
        auto token_it = make_lookahead_iterator<4>(lexer_it);

        while (token_it != std::default_sentinel) {
            if (*token_it == '.') {
                _nodes.emplace_back(parse_child_operator(++token_it, std::default_sentinel));

            } else if (*token_it == '[') {
                _nodes.emplace_back(parse_indexing_operator(++token_it, std::default_sentinel));

            } else if (*token_it == '$') {
                hi_check(_nodes.empty(), "Root node '$' not at start of path.");
                _nodes.emplace_back(root{});
                ++token_it;

            } else if (*token_it == '@') {
                hi_check(_nodes.empty(), "Current node '@' not at start of path.");
                _nodes.emplace_back(current{});
                ++token_it;

            } else if (*token_it == token::id) {
                hi_check(_nodes.empty(), "Unexpected child name {}.", *token_it);
                _nodes.emplace_back(names{static_cast<std::string>(*token_it)});
                ++token_it;

            } else {
                throw parse_error(std::format("Unexpected token {}.", *token_it));
            }
        }
    }

    [[nodiscard]] constexpr jsonpath(std::string_view rhs) : jsonpath(rhs.begin(), rhs.end()) {}

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return _nodes.empty();
    }

    /** The json-path will result in zero or one match.
     */
    [[nodiscard]] constexpr bool is_singular() const noexcept
    {
        auto r = true;
        for (hilet& node : _nodes) {
            r &= std::visit(
                [](hilet& node_) {
                    return node_.is_singular();
                },
                node);
        }
        return r;
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        return _nodes.size();
    }

    [[nodiscard]] constexpr iterator begin() noexcept
    {
        return _nodes.begin();
    }

    [[nodiscard]] constexpr const_iterator begin() const noexcept
    {
        return _nodes.begin();
    }

    [[nodiscard]] constexpr const_iterator cbegin() const noexcept
    {
        return _nodes.cbegin();
    }

    [[nodiscard]] constexpr iterator end() noexcept
    {
        return _nodes.end();
    }

    [[nodiscard]] constexpr const_iterator end() const noexcept
    {
        return _nodes.end();
    }

    [[nodiscard]] constexpr const_iterator cend() const noexcept
    {
        return _nodes.cend();
    }

    [[nodiscard]] constexpr friend std::string to_string(jsonpath const& path) noexcept
    {
        auto r = std::string{};
        for (hilet& node : path._nodes) {
            r += std::visit(
                [](hilet& node_) {
                    return node_.string();
                },
                node);
        }
        return r;
    }

private:
    std::vector<node> _nodes;

    template<std::input_iterator It, std::sentinel_for<It> ItEnd>
    [[nodiscard]] constexpr static node parse_slicing_operator(It& it, ItEnd last)
    {
        auto start = 0_z;
        auto end = std::numeric_limits<ptrdiff_t>::min();
        auto step = 1_z;

        if (it.size() >= 2 and it[0] == '-' and it[1] == token::integer) {
            auto tmp = static_cast<size_t>(it[1]);
            hi_check(can_narrow_cast<ptrdiff_t>(tmp), "Start-index out of range {}", tmp);
            start = -narrow_cast<ptrdiff_t>(tmp);
            it += 2;

        } else if (*it == token::integer) {
            auto tmp = static_cast<size_t>(*it);
            hi_check(can_narrow_cast<ptrdiff_t>(tmp), "Start-index out of range {}", tmp);
            start = narrow_cast<ptrdiff_t>(tmp);
            ++it;

        } else if (*it == ':') {
            // Use the default.

        } else {
            throw parse_error(std::format("Unexpected token while parsing start-index of the slicing operator, got {}", *it));
        }

        hi_check(it != last, "Unexpected end-of-text after the start-index of the slicing operator.");
        hi_check(*it == ':', "Expecting ':' at the start-index of the slicing operator, got {}", *it);
        ++it;

        hi_check(it != last, "Unexpected end-of-text while parsing the end-index of the slicing operator.");
        if (*it == ']') {
            ++it;
            return slice{start, end, step};

        } else if (it.size() >= 2 and it[0] == '-' and it[1] == token::integer) {
            auto tmp = static_cast<size_t>(it[1]);
            hi_check(can_narrow_cast<ptrdiff_t>(tmp), "End-index out of range {}", tmp);
            end = -narrow_cast<ptrdiff_t>(tmp);
            it += 2;

        } else if (*it == token::integer) {
            auto tmp = static_cast<size_t>(*it);
            hi_check(can_narrow_cast<ptrdiff_t>(tmp), "End-index out of range {}", tmp);
            end = narrow_cast<ptrdiff_t>(tmp);
            ++it;

        } else if (*it == ':' or *it == ']') {
            // Use the default.

        } else {
            throw parse_error(std::format("Unexpected token while parsing the end-index of the slicing operator, got {}", *it));
        }

        hi_check(it != last, "Unexpected end-of-text after the end-index of the slicing operator.");
        if (*it == ']') {
            ++it;
            return slice{start, end, step};

        } else if (it.size() >= 2 and it[0] == '-' and it[1] == token::integer) {
            auto tmp = static_cast<size_t>(it[1]);
            hi_check(can_narrow_cast<ptrdiff_t>(tmp), "Step-value out of range {}", tmp);
            step = -narrow_cast<ptrdiff_t>(tmp);
            it += 2;

        } else if (*it == token::integer) {
            auto tmp = static_cast<size_t>(*it);
            hi_check(can_narrow_cast<ptrdiff_t>(tmp), "Step-value out of range {}", tmp);
            step = narrow_cast<ptrdiff_t>(tmp);
            ++it;

        } else {
            throw parse_error(std::format("Unexpected token while parsing step-value of the slicing operator, got {}", *it));
        }

        hi_check(it != last, "Unexpected end-of-text after the step-value of the slicing operator.");
        hi_check(*it == ']', "Expecting '] after step-value of the slicing operator, got {}", *it);
        ++it;
        return slice{start, end, step};
    }

    template<std::input_iterator It, std::sentinel_for<It> ItEnd>
    [[nodiscard]] constexpr static node parse_integer_indexing_operator(It& it, ItEnd last)
    {
        auto r = indices();

        while (true) {
            if (it.size() >= 2 and it[0] == '-' and it[1] == token::integer) {
                auto tmp = static_cast<size_t>(it[1]);
                hi_check(can_narrow_cast<ptrdiff_t>(tmp), "Index out of range {}", tmp);
                r.push_back(-narrow_cast<ptrdiff_t>(tmp));
                it += 2;

            } else if (*it == token::integer) {
                auto tmp = static_cast<size_t>(*it);
                hi_check(can_narrow_cast<ptrdiff_t>(tmp), "Index out of range {}", tmp);
                r.push_back(narrow_cast<ptrdiff_t>(tmp));
                ++it;

            } else {
                throw parse_error(std::format("Expected an integer-index, got {}", *it));
            }

            if (it == last) {
                throw parse_error("Unexpected end-of-text while parsing the index operator '['.");

            } else if (*it == ']') {
                ++it;
                return r;

            } else if (*it == ',') {
                ++it;
                continue;

            } else {
                throw parse_error(std::format("Unexpected token after a integer-index: {}.", *it));
            }
        }
    }

    template<std::input_iterator It, std::sentinel_for<It> ItEnd>
    [[nodiscard]] constexpr static node parse_name_indexing_operator(It& it, ItEnd last)
    {
        auto r = names();

        while (true) {
            if (*it != token::id and *it != token::sstr and *it != token::dstr) {
                throw parse_error(std::format("Expected a name-index, got {}", *it));
            }

            r.push_back(static_cast<std::string>(*it++));

            if (it == last) {
                throw parse_error("Unexpected end-of-text while parsing the index operator '['.");

            } else if (*it == ']') {
                ++it;
                return r;

            } else if (*it == ',') {
                ++it;
                continue;

            } else {
                throw parse_error(std::format("Unexpected token after a name-index: {}.", *it));
            }
        }
    }

    template<std::input_iterator It, std::sentinel_for<It> ItEnd>
    [[nodiscard]] constexpr static node parse_indexing_operator(It& it, ItEnd last)
    {
        hi_check(it != last, "Unexpected end-of-text at index operator token '['.");

        if (*it == '*') {
            hi_check(it.size() >= 2 and it[1] == ']', "Expected end of wildcast-indexing operator '[*', got {}.", it[1]);
            it += 2;
            return wildcard{};

        } else if (
            *it == ':' or (it.size() >= 2 and it[0] == token::integer and it[1] == ':') or
            (it.size() >= 3 and it[0] == '-' and it[1] == token::integer and it[2] == ':')) {
            return parse_slicing_operator(it, last);

        } else if (*it == token::integer or (it.size() >= 2 and it[0] == '-' and it[1] == token::integer)) {
            return parse_integer_indexing_operator(it, last);

        } else if (*it == token::id or *it == token::sstr or *it == token::dstr) {
            return parse_name_indexing_operator(it, last);

        } else {
            throw parse_error(
                std::format("Expected a integer index or name-index after indexing operator '[', got token {}.", *it));
        }
    }

    template<std::input_iterator It, std::sentinel_for<It> ItEnd>
    [[nodiscard]] constexpr static node parse_child_operator(It& it, ItEnd last)
    {
        hi_check(it != last, "Unexpected end-of-text at child operator token '.'.");

        if (*it == '*') {
            ++it;
            return wildcard{};

        } else if (*it == '.') {
            if (it.size() >= 2 and it[1] == '[') {
                // When the descend operator '..' is followed by an indexing operator.
                // Then the full descend operator is consumed here.
                ++it;
                return descend{};

            } else {
                // The descend operator '..' is often followed by a name or '*' as-if the
                // second dot in the descend operator is a child selector. Then don't consume
                // the second dot and treat it as-if it is a child operator.
                return descend{};
            }

        } else if (*it == token::id) {
            auto name = static_cast<std::string>(*it);
            ++it;
            return names{std::move(name)};

        } else {
            throw parse_error(std::format("Expected a child name or wildcard, got token {}.", *it));
        }
    }
};

}} // namespace hi::v1

// XXX #617 MSVC bug does not handle partial specialization in modules.
export template<>
struct std::formatter<hi::jsonpath, char> : std::formatter<char const *, char> {
    auto format(hi::jsonpath const& t, auto& fc) const
    {
        return std::formatter<std::string, char>{}.format(to_string(t), fc);
    }
};
