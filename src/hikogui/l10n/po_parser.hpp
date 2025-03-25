// Copyright Take Vos 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "po_translations.hpp"
#include "../i18n/i18n.hpp"
#include "../file/file.hpp"
#include "../parser/parser.hpp"
#include "../macros.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <ranges>

hi_export_module(hikogui.l10n.po_parser);

hi_export namespace hi {
inline namespace v1 {

namespace detail {

template<std::input_iterator It, std::sentinel_for<It> ItEnd>
[[nodiscard]] constexpr std::tuple<std::string, size_t, std::string> parse_po_line(It& it, ItEnd last, std::string_view path)
{
    hi_assert(it != last);

    auto name = std::string{};
    if ((*it == token::id)) {
        name = static_cast<std::string>(*it++);
    } else {
        throw parse_error(
            std::format("{}: Expecting a keyword at start of each line, got {}", token_location(it, last, path), *it));
    }

    auto index = 0_uz;
    if ((*it == '[')) {
        ++it;

        if (it != last and *it == token::integer) {
            index = static_cast<size_t>(*it++);
        } else {
            throw parse_error(std::format(
                "{}: Expecting an integer literal as an index for {}, got {}", token_location(it, last, path), name, *it));
        }

        if (it != last and *it == ']') {
            ++it;
        } else {
            throw parse_error(std::format(
                "{}: The index on {} must terminate with a bracket ']', got {}", token_location(it, last, path), name, *it));
        }
    }

    auto value = std::string{};
    if (it != last and (*it == token::sstr or *it == token::dstr)) {
        value = static_cast<std::string>(*it++);
    } else {
        throw parse_error(
            std::format("{}: Expecting a string value after {}, got {}", token_location(it, last, path), name, *it));
    }

    while (it != last and (*it == token::sstr or *it == token::dstr)) {
        // Concatenating string literals.
        value += static_cast<std::string>(*it++);
    }

    return {name, index, value};
}

template<std::input_iterator It, std::sentinel_for<It> ItEnd>
[[nodiscard]] constexpr std::optional<po_translation> parse_po_translation(It& it, ItEnd last, std::string_view path)
{
    po_translation r;

    while (it != last) {
        if (r.msgstr.empty()) {
            // If there have been no "msgstr" keywords, then capture information in the translation.
            auto [name, index, value] = parse_po_line(it, last, path);

            if (name == "msgctxt") {
                r.msgctxt = value;

            } else if (name == "msgid") {
                r.msgid = value;

            } else if (name == "msgid_plural") {
                r.msgid_plural = value;

            } else if (name == "msgstr") {
                if (index >= r.msgstr.size()) {
                    r.msgstr.resize(index + 1);
                }
                r.msgstr[index] = value;

            } else {
                throw parse_error(
                    std::format("{}: Line starts with unexpected keyword {}", token_location(it, last, path), name));
            }

        } else if ((*it == token::id) and (*it == "msgstr")) {
            // After the first "msgstr" keyword there may be others, but another keyword will start a new translation.
            auto [name, index, value] = parse_po_line(it, last, path);

            if (index >= r.msgstr.size()) {
                r.msgstr.resize(index + 1);
            }
            r.msgstr[index] = value;

        } else {
            // The current keyword is not a msgstr, so return the translation captured.
            return r;
        }
    }

    return std::nullopt;
}

constexpr void parse_po_header(po_translations& r, std::string_view header)
{
    using namespace std::literals;

    for (auto const line : split_string_view(header, "\\n"sv)) {
        if (line.empty()) {
            // Skip empty header lines.
            continue;
        }

        auto split_line = split_string_view(line, ":");
        if (split_line.size() < 2) {
            throw parse_error(std::format("Unknown header '{}'", std::string_view{line}));
        }

        auto const name = to_lower(strip(split_line.front()));
        split_line.erase(split_line.begin());
        auto const value = strip(join(split_line, ":"));

        if (name == "language") {
            r.language = language_tag{value};
        }
    }
}

} // namespace detail

template<std::input_iterator It, std::sentinel_for<It> ItEnd>
[[nodiscard]] constexpr po_translations parse_po(It it, ItEnd last, std::string_view path)
{
    po_translations r;

    auto token_it = lexer<lexer_config::sh_style()>.parse(it, last);

    while (token_it != std::default_sentinel) {
        if (auto result = detail::parse_po_translation(token_it, std::default_sentinel, path)) {
            if (not result->msgid.empty()) {
                r.translations.push_back(*result);

            } else if (result->msgstr.size() == 1) {
                // If a translation has an empty msgid, then the msgstr contain headers.
                detail::parse_po_header(r, result->msgstr.front());

            } else {
                throw parse_error(std::format("{}: Unknown .po syntax.", token_location(token_it, path)));
            }
        }
    }

    return r;
}

[[nodiscard]] constexpr po_translations parse_po(std::string_view text, std::string_view path)
{
    return parse_po(text.begin(), text.end(), path);
}

[[nodiscard]] inline po_translations parse_po(std::filesystem::path const& path)
{
    return parse_po(as_string_view(file_view{path}), path.string());
}

} // namespace v1
} // namespace hi::inline v1
