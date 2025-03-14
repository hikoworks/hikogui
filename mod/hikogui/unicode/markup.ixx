

module;

#include <string>
#include <string_view>
#include <algorithm>
#include <iterator>
#include <ranges>
#include <concepts>

export module hikogui_unicode_markup;
import hikogui_i18n;
import hikogui_unicode_grapheme;
import hikogui_unicode_gstring;
import hikogui_unicode_phrasing;
import hikogui_utility;

export namespace hi { inline namespace v1 {

/** @file unicode/markup.hpp
 *
 *   Sequence    | Description
 *  :----------- |:-------------
 *   `[[`        | Escape literal `~`
 *   `[.]`       | Reset phrasing and language to default.
 *   `[r]`       | Set phrasing to regular
 *   `[e]`       | Set phrasing to emphasis
 *   `[s]`       | Set phrasing to strong
 *   `[c]`       | Set phrasing to code
 *   `[a]`       | Set phrasing to abbreviation
 *   `[q]`       | Set phrasing to quote
 *   `[k]`       | Set phrasing to keyboard
 *   `[h]`       | Set phrasing to highlight
 *   `[m]`       | Set phrasing to math
 *   `[x]`       | Set phrasing to example
 *   `[u]`       | Set phrasing to unarticulated
 *   `[t]`       | Set phrasing to title
 *   `[S]`       | Set phrasing to success
 *   `[W]`       | Set phrasing to warning
 *   `[E]`       | Set phrasing to error
 *   `[` tag `]` | Set language to the tag.
 *
 * ```
 * This [e]is[.] [NL-nl]hallo[.] world.
 * ```
 */

/** Inplace-apply markup to a string of graphemes.
 *
 * After the markup is applied the range is either the same or shorter.
 *
 * @param first An iterator to the first grapheme.
 * @param last An iterator beyond the last grapheme.
 * @return The new end iterator after applying the markup.
 */
export template<std::input_or_output_iterator It, std::sentinel_for<It> ItEnd>
constexpr It
apply_markup(It first, ItEnd last, language_tag default_language = language_tag{"en-US"}, phrasing default_phrasing = phrasing::regular) noexcept
    requires std::same_as<typename It::value_type, grapheme>
{
    enum class state_type { idle, command };

    auto src_it = first;
    auto dst_it = first;
    auto command_start = first;

    default_language = default_language.expand();
    auto current_language = default_language;
    auto current_phrasing = default_phrasing;

    auto write_character = [&](grapheme c) {
        c.set_language_tag(current_language);
        c.set_phrasing(current_phrasing);
        *dst_it++ = c;
    };

    auto write_command = [&](gstring_view command) {
        write_character('[');
        for (auto c : command) {
            write_character(c);
        }
        write_character(']');
    };

    auto state = state_type::idle;
    while (src_it != last) {
        auto c = *src_it++;

        if (state == state_type::idle) {
            if (c == '[') {
                command_start = src_it;
                state = state_type::command;
            } else {
                write_character(c);
            }

        } else if (state == state_type::command) {
            if (c == '[') {
                // Escaped open bracket.
                write_character(c);
                state = state_type::idle;

            } else if (c == ']') {
                // End of command.
                auto command = gstring_view{command_start, src_it - 1};
                if (command.empty()) {
                    // An empty command is an error, keep the command in the text.
                    write_command(command);

                } else if (command.size() == 1) {
                    if (hilet command_g = command.front(); command_g.is_ascii()) {
                        hilet command_c = char_cast<char>(command_g.starter());
                        if (command_c == '.') {
                            current_language = default_language;
                            current_phrasing = default_phrasing;

                        } else if (auto p = to_phrasing(command_c)) {
                            current_phrasing = *p;

                        } else {
                            // Unknown command, leep the command in the text.
                            write_command(command);
                        }

                    } else {
                        // Unknown command, keep the command in the text.
                        write_command(command);
                    }

                } else {
                    try {
                        current_language = language_tag{to_string(command)}.expand();
                    } catch (...) {
                        // Leave the full command in the output.
                        write_command(command);
                    }
                }
                state = state_type::idle;
            }
        }
    }

    return dst_it;
}

/** Apply markup to a string of graphemes.
 *
 * After the markup is applied the range is either the same or shorter.
 *
 * @param str A grapheme string to apply the markup to.
 * @return A grapheme string with the markup applied.
 */
export [[nodiscard]] constexpr gstring
apply_markup(gstring str, language_tag default_language = language_tag{"en-US"}, phrasing default_phrasing = phrasing::regular) noexcept
{
    auto it = apply_markup(str.begin(), str.end(), default_language, default_phrasing);
    str.erase(it, str.end());
    return str;
}

/** Apply markup to a string of graphemes.
 *
 * After the markup is applied the range is either the same or shorter.
 *
 * @param str A UTF-8 string to apply the markup to.
 * @return A grapheme string with the markup applied.
 */
export [[nodiscard]] constexpr gstring
apply_markup(std::string_view str, language_tag default_language = language_tag{"en-US"}, phrasing default_phrasing = phrasing::regular) noexcept
{
    return apply_markup(to_gstring(str), default_language, default_phrasing);
}

}} // namespace hi::v1
