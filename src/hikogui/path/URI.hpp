// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file file/URI.hpp Defines the URI class.
 * @ingroup file
 */

#pragma once

#include "../utility/utility.hpp"
#include "../algorithm/algorithm.hpp"
#include "../macros.hpp"
#include <string>
#include <optional>
#include <ranges>
#include <vector>
#include <format>
#include <string_view>
#include <string>
#include <compare>
#include <ostream>

hi_export_module(hikogui.path.URI);

hi_export namespace hi { inline namespace v1 {

#define HI_SUB_DELIM '!', '$', '&', '\'', '(', ')', '*', '+', ',', ';', '='
#define HI_PCHAR HI_SUB_DELIM, ':', '@'

/** A Uniform Resource Identifier.
 * @ingroup file
 *
 * This class holds the URI separated and unencoded into it components:
 *  - scheme (optional)
 *  - authority (optional)
 *    + userinfo (optional)
 *    + host
 *    + port (optional)
 *  - path
 *  - query (optional)
 *  - fragment (optional)
 *
 */
class URI {
private:
    using const_iterator = std::string_view::const_iterator;

public:
    class authority_type {
    public:
        constexpr authority_type() noexcept = default;
        constexpr authority_type(authority_type const&) noexcept = default;
        constexpr authority_type(authority_type&&) noexcept = default;
        constexpr authority_type& operator=(authority_type const&) noexcept = default;
        constexpr authority_type& operator=(authority_type&&) noexcept = default;

        constexpr authority_type(std::string_view const& rhs) noexcept
        {
            parse(rhs);
        }

        // constexpr void normalize(std::optional<std::string> const& scheme) noexcept
        //{
        //     hi_not_implemented();
        // }

        [[nodiscard]] constexpr std::optional<std::string> const& userinfo() const noexcept
        {
            return _userinfo;
        }

        constexpr authority_type& set_userinfo(std::optional<std::string> const& rhs) noexcept
        {
            _userinfo = rhs;
            return *this;
        }

        [[nodiscard]] constexpr std::string const& host() const noexcept
        {
            return _host;
        }

        constexpr authority_type& set_host(std::string const& rhs)
        {
            validate_host(rhs);
            _host = to_lower(rhs);
            return *this;
        }

        [[nodiscard]] constexpr std::optional<std::string> const& port() const noexcept
        {
            return _port;
        }

        constexpr authority_type& set_port(std::optional<std::string> const& rhs)
        {
            if (rhs) {
                validate_port(*rhs);
                _port = *rhs;
            } else {
                _port = {};
            }
            return *this;
        }

        [[nodiscard]] constexpr friend bool operator==(authority_type const&, authority_type const&) noexcept = default;
        [[nodiscard]] constexpr friend auto operator<=>(authority_type const&, authority_type const&) noexcept = default;

        [[nodiscard]] constexpr friend size_t to_string_size(authority_type const& rhs) noexcept
        {
            auto size = 0_uz;
            if (rhs._userinfo) {
                size += rhs._userinfo->size() + 1;
            }
            size += rhs._host.size();
            if (rhs._port) {
                size += rhs._port->size() + 1;
            }
            return size;
        }

        [[nodiscard]] constexpr friend std::string to_string(authority_type const& rhs) noexcept
        {
            auto r = std::string{};

            if (rhs._userinfo) {
                r += URI::encode<HI_SUB_DELIM, ':'>(*rhs._userinfo);
                r += '@';
            }

            if (rhs._host.empty() or rhs._host.front() != '[') {
                r += URI::encode<HI_SUB_DELIM>(rhs._host);
            } else {
                r += URI::encode<HI_SUB_DELIM, '[', ']', ':'>(rhs._host);
            }

            if (rhs._port) {
                r += ':';
                r += *rhs._port;
            }

            return r;
        }

    private:
        std::optional<std::string> _userinfo = {};
        std::string _host = {};
        std::optional<std::string> _port = {};

        constexpr static void validate_host(std::string_view str)
        {
            if (str.starts_with('[') and not str.ends_with(']')) {
                throw uri_error("The host-component starts with '[' has missing ']' at end");
            } else if (str.ends_with(']') and str.starts_with('[')) {
                throw uri_error("The host-component ends with ']' has missing '[' at start");
            }
        }

        constexpr static void validate_port(std::string_view str)
        {
            for (auto c : str) {
                if (not(c >= '0' and c <= '9')) {
                    throw uri_error("The port-component contains a non-digit.");
                }
            }
        }

        /** Parse a URI userinfo part.
         *
         * @param str Substring of the URI starting at the potential userinfo.
         * @return The number of characters in the userinfo.
         * @retval 0 The userinfo is an empty string.
         * @retval -1 There is no userinfo
         */
        [[nodiscard]] constexpr const_iterator parse_userinfo(const_iterator first, const_iterator last) noexcept
        {
            for (auto it = first; it != last; ++it) {
                if (auto const c = *it; c == '@') {
                    set_userinfo(URI::decode(first, it));
                    return it + 1; // Skip over '@'.
                }
            }

            // Reached at end of URI, this is not a userinfo.
            return first;
        }

        /** Parse a URI host part.
         *
         * @param str Substring of the URI starting at the potential host.
         * @return The number of characters in the host.
         * @retval 0 The host is an empty string.
         */
        [[nodiscard]] constexpr const_iterator parse_host(const_iterator first, const_iterator last) noexcept
        {
            if (first == last) {
                return first;
            }

            auto it = first;
            if (*it == '[') {
                while (it != last) {
                    if (*it == ']') {
                        set_host(URI::decode(first, it + 1));
                        return it + 1; // Skip over ']'.
                    }
                }
                // End of URI mean this is not a host, interpret as path instead.
                return first;

            } else {
                for (; it != last; ++it) {
                    if (auto const c = *it; c == ':') {
                        // End of host.
                        set_host(URI::decode(first, it));
                        return it;
                    }
                }

                // End of URI means that the host is the last part of the URI
                set_host(URI::decode(first, last));
                return last;
            }
        }

        [[nodiscard]] constexpr const_iterator parse_port(const_iterator first, const_iterator last) noexcept
        {
            set_port(std::string{first, last});
            return last;
        }

        constexpr void parse(std::string_view rhs) noexcept
        {
            auto first = rhs.cbegin();
            auto last = rhs.cend();
            auto it = parse_userinfo(first, last);
            it = parse_host(it, last);

            if (it != last and *it == ':') {
                it = parse_port(++it, last);
            }
        }
    };

    /** A path type.
     *
     * A path is a vector of path-segments:
     * - An empty path has no segments.
     * - The last segment is the filename, if it is an empty string then there is no filename.
     * - The segments names ".", ".." and "**" are always directories. If the string to be parsed
     *   ends in ".", ".." or "**" then it is treated as if the string is terminated with a "/".
     * - A segment may contain any characters including '/' and ':'.
     * - non-filename segments may be empty strings, which can be used to represent the root-directory.
     *
     *  Path string             | Segment list
     *  ----------------------- | -----------------
     *  ""                      | []
     *  "/"                     | ["", ""]
     *  "filename"              | ["filename"]
     *  "/filename"             | ["", "filename"]
     *  "dirname/"              | ["dirname", ""]
     *  "/dirname/"             | ["", "dirname", ""]
     *  "/dirname/filename"     | ["", "dirname", "filename"]
     *  "."                     | [".", ""]
     *  "/."                    | ["", ".", ""]
     *  "./"                    | [".", ""]
     *  "/./"                   | ["", ".", ""]
     *  "/./."                  | ["", ".", ".", ""]
     *  ".."                    | ["..", ""]
     *  "/.."                   | ["", "..", ""]
     *  "../"                   | ["..", ""]
     *  "/../"                  | ["", "..", ""]
     *  "/../.."                | ["", "..", ".", ""]
     */
    class path_type : public std::vector<std::string> {
    public:
        constexpr path_type() noexcept = default;
        constexpr path_type(path_type const&) noexcept = default;
        constexpr path_type(path_type&&) noexcept = default;
        constexpr path_type& operator=(path_type const&) noexcept = default;
        constexpr path_type& operator=(path_type&&) noexcept = default;

        [[nodiscard]] constexpr static std::vector<std::string> parse(std::string_view str)
        {
            auto r = std::ranges::to<std::vector>(std::views::transform(std::views::split(str, std::string_view{"/"}), [](auto&& x) {
                return URI::decode(std::string_view{std::ranges::begin(x), std::ranges::end(x)});
            }));

            if (r.size() == 1 and r.front().empty()) {
                // An empty string will evaluate to a single segment.
                r.clear();
            }
            if (not r.empty() and (r.back() == "." or r.back() == ".." or r.back() == "**")) {
                // ".", ".." and "**" are directories always terminate with a slash.
                r.emplace_back();
            }

            return r;
        }

        constexpr path_type(std::string_view str) noexcept : std::vector<std::string>(parse(str))
        {
            hi_axiom(holds_invariant());
        }

        [[nodiscard]] constexpr bool absolute() const noexcept
        {
            return size() >= 2 and front().empty();
        }

        [[nodiscard]] constexpr bool double_absolute() const noexcept
        {
            return size() >= 3 and (*this)[0].empty() and (*this)[1].empty();
        }

        [[nodiscard]] constexpr bool is_root() const noexcept
        {
            return size() == 2 and (*this)[0].empty() and (*this)[1].empty();
        }

        [[nodiscard]] constexpr std::optional<std::string> filename() const noexcept
        {
            if (empty() or back().empty()) {
                return {};
            } else {
                return back();
            }
        }

        /** Remove the filename part of the path.
         *
         * @note No change when the path does not contain a filename.
         * @return A reference to this.
         */
        constexpr path_type& remove_filename() noexcept
        {
            switch (size()) {
            case 0: // Don't remove a filename from an empty path.
                break;
            case 1: // relative filename, make the path empty.
                clear();
                break;
            default: // relative or absolute directory with optional filename. Just empty the last path segment.
                back().clear();
            }
            hi_axiom(holds_invariant());
            return *this;
        }

        [[nodiscard]] constexpr friend path_type merge(path_type base, path_type const& ref, bool base_has_authority) noexcept
        {
            if (base_has_authority and base.empty()) {
                // A URL with an authority and empty path is implicitly the root path.
                base.emplace_back();
                base.emplace_back();
            }

            if (not base.empty()) {
                // Remove the (possibly empty) filename from the base.
                base.pop_back();
            }
            base.insert(base.cend(), ref.cbegin(), ref.cend());

            if (base.size() == 1 and base.front().empty()) {
                // Empty ref-path added to root base-path, fix by appending an empty filename.
                base.emplace_back();
            }

            hi_axiom(base.holds_invariant());
            return base;
        }

        /**
         *
         *  Path              | List                            | Result Path | Result List
         *  -------------     | ---------------------------     | ----------- | -----------
         *  ".."              | ["..", ""]                      | ""          | [""]
         *  "/.."             | ["", "..", ""]                  | ""          | [""]
         *  "foo/.."          | ["foo", "..", ""]               | ""          | [""]
         *  "/foo/.."         | ["", "foo", "..", ""]           | "/"         | ["", ""]
         *  "baz/foo/.."      | ["baz", "foo", "..", ""]        | "baz/"      | ["baz", ""]
         *  "/baz/foo/.."     | ["", "baz", "foo", "..", ""]    | "/baz/"     | ["", "baz", ""]
         *                    |                                 |             |
         *  "../"             | ["..", ""]                      | ""          | [""]
         *  "/../"            | ["", "..", ""]                  | ""          | [""]
         *  "foo/../"         | ["foo", "..", ""]               | ""          | [""]
         *  "/foo/../"        | ["", "foo", "..", ""]           | "/"         | ["", ""]
         *  "baz/foo/../"     | ["baz", "foo", "..", ""]        | "baz/"      | ["baz", ""]
         *  "/baz/foo/../"    | ["", "baz", "foo", "..", ""]    | "/baz/"     | ["", "baz", ""]
         *                    |                                 |             |
         *  "../bar"          | ["..", "bar"]                   | "bar"       | ["bar"]
         *  "/../bar"         | ["", "..", "bar"]               | "bar"       | ["bar"]
         *  "foo/../bar"      | ["foo", "..", "bar"]            | "bar"       | ["bar"]
         *  "/foo/../bar"     | ["", "foo", "..", "bar"]        | "/bar"      | ["", "bar"]
         *  "baz/foo/../bar"  | ["baz", "foo", "..", "bar"]     | "baz/bar"   | ["baz", "bar"]
         *  "/baz/foo/../bar" | ["", "baz", "foo", "..", "bar"] | "/baz/bar"  | ["", "baz", "bar"]
         */
        [[nodiscard]] constexpr friend path_type remove_dot_segments(path_type path) noexcept
        {
            for (auto it = path.cbegin(); it != path.cend();) {
                if (*it == ".") {
                    //  Remove any "." from the path.
                    it = path.erase(it);

                } else if (*it == "..") {
                    if (it == path.cbegin()) {
                        // Remove the ".." at the start of a relative path.
                        it = path.erase(it);

                    } else if (it - 1 == path.cbegin() and (it - 1)->empty()) {
                        // Remove just the ".." at the start of an absolute path.
                        it = path.erase(it);

                    } else {
                        // Remove ".." and the segment in front of it.
                        it = path.erase(it - 1, it + 1);
                    }
                } else {
                    // Ignore other segments.
                    ++it;
                }
            }

            if (path.size() == 1 and path.front().empty()) {
                // After removing the ".." at the start of the path we are left with an empty segment.
                path.clear();
            }

            hi_axiom(path.holds_invariant());
            return path;
        }

        [[nodiscard]] constexpr bool holds_invariant() const noexcept
        {
            if (empty()) {
                return true;
            }

            if (back() == "." or back() == ".." or back() == "**") {
                // ".", ".." and "**" are always directories and may not be the last segment.
                return false;
            }
            return true;
        }

        [[nodiscard]] constexpr friend size_t to_string_size(path_type const& rhs) noexcept
        {
            size_t r = 0_uz;
            for (auto const& segment : rhs) {
                r += segment.size();
            }
            r += rhs.size() + 1;
            return r;
        }

        /** Convert the URI path component to a string.
         *
         * @param rhs The URI path component.
         * @param has_scheme If true than the first segment may contain a ':' without percent encoding.
         * @return The path component converted to a string.
         */
        [[nodiscard]] constexpr friend std::string to_string(path_type const& rhs, bool has_scheme = false) noexcept
        {
            auto r = std::string{};
            r.reserve(to_string_size(rhs));

            auto segment_is_empty = false;
            for (auto it = rhs.cbegin(); it != rhs.cend(); ++it) {
                segment_is_empty = it->empty();

                if (it == rhs.cbegin() and not has_scheme) {
                    r += URI::encode<HI_SUB_DELIM, '@'>(*it);
                } else {
                    r += URI::encode<HI_PCHAR>(*it);
                }
                r += '/';
            }

            if (not r.empty() and not segment_is_empty) {
                // The last path-component was a filename, remove the trailing slash '/'.
                r.pop_back();
            }
            return r;
        }
    };

    constexpr URI() noexcept = default;
    constexpr URI(URI const&) noexcept = default;
    constexpr URI(URI&&) noexcept = default;
    constexpr URI& operator=(URI const&) noexcept = default;
    constexpr URI& operator=(URI&&) noexcept = default;

    /** Construct a URI from a string.
     *
     * @note This constructor will normalize the URI
     * @param str A URI encoded as a string.
     * @throws uri_error When the URI can not be normalized due to a parse error.
     */
    constexpr explicit URI(std::string_view str)
    {
        parse(str);
    }

    /** Construct a URI from a string.
     *
     * @note This constructor will normalize the URI
     * @param str A URI encoded as a string.
     * @throws uri_error When the URI can not be normalized due to a parse error.
     */
    constexpr explicit URI(std::string const& str) : URI(std::string_view{str}) {}

    /** Construct a URI from a string.
     *
     * @note This constructor will normalize the URI
     * @param str A URI encoded as a string.
     * @throws uri_error When the URI can not be normalized due to a parse error.
     */
    constexpr explicit URI(const char *str) : URI(std::string_view{str}) {}

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return not _scheme and not _authority and _path.empty() and not _query and not _fragment;
    }

    constexpr operator bool() const noexcept
    {
        return not empty();
    }

    /** Get the scheme-component of the URI.
     *
     * @return The optional and lower-cased scheme-component.
     */
    [[nodiscard]] constexpr std::optional<std::string> const& scheme() const noexcept
    {
        return _scheme;
    }

    /** Get the scheme-component of the URI.
     *
     */
    constexpr URI& set_scheme(std::optional<std::string> const& rhs)
    {
        if (rhs) {
            validate_scheme(*rhs);
            _scheme = *rhs;
        } else {
            _scheme = {};
        }
        return *this;
    }

    /** Get the authority-component of the URI.
     *
     * @return The optional and decoded userinfo, host and port..
     */
    [[nodiscard]] constexpr std::optional<authority_type> const& authority() const noexcept
    {
        return _authority;
    }

    constexpr URI& set_authority(std::optional<authority_type> const& rhs) noexcept
    {
        _authority = rhs;
        return *this;
    }

    [[nodiscard]] constexpr path_type const& path() const noexcept
    {
        return _path;
    }

    constexpr URI& set_path(path_type const& rhs)
    {
        validate_path(rhs, to_bool(_authority));
        _path = rhs;
        return *this;
    }

    [[nodiscard]] constexpr std::optional<std::string> filename() const noexcept
    {
        return _path.filename();
    }

    /** Remove the filename part of the path.
     *
     * @note No change when the path does not contain a filename.
     * @return A reference to this.
     */
    constexpr URI& remove_filename() noexcept
    {
        _path.remove_filename();
        return *this;
    }

    /** Get the query-component of the URI.
     *
     * @return The optional and decoded query-component.
     */
    [[nodiscard]] constexpr std::optional<std::string> const& query() const noexcept
    {
        return _query;
    }

    constexpr URI& set_query(std::optional<std::string> const& rhs) noexcept
    {
        _query = rhs;
        return *this;
    }

    /** Get the fragment-component of the URI.
     *
     * @return The optional and decoded fragment-component.
     */
    [[nodiscard]] constexpr std::optional<std::string> const& fragment() const noexcept
    {
        return _fragment;
    }

    constexpr URI& set_fragment(std::optional<std::string> const& rhs) noexcept
    {
        _fragment = rhs;
        return *this;
    }

    [[nodiscard]] constexpr friend std::string to_string(URI const& rhs) noexcept
    {
        auto r = std::string{};
        r.reserve(to_string_size(rhs));

        if (rhs._scheme) {
            r += *rhs._scheme;
            r += ':';
        }

        if (rhs._authority) {
            r += '/';
            r += '/';
            r += to_string(*rhs._authority);
        }

        r += to_string(rhs._path, to_bool(rhs._scheme));

        if (rhs._query) {
            r += '?';
            r += URI::encode<HI_PCHAR, '/', '?'>(*rhs._query);
        }

        if (rhs._fragment) {
            r += '#';
            r += URI::encode<HI_PCHAR, '/', '?'>(*rhs._fragment);
        }

        return r;
    }

    friend std::ostream& operator<<(std::ostream& lhs, URI const& rhs) noexcept
    {
        return lhs << to_string(rhs);
    }

    [[nodiscard]] constexpr friend bool operator==(URI const& lhs, URI const& rhs) noexcept = default;
    [[nodiscard]] constexpr friend auto operator<=>(URI const& lhs, URI const& rhs) noexcept = default;

    [[nodiscard]] constexpr friend URI operator/(URI const& base, URI const& ref) noexcept
    {
        auto target = URI{};

        if (ref._scheme) {
            target._scheme = ref._scheme;
            target._authority = ref._authority;
            target._path = remove_dot_segments(ref._path);
            target._query = ref._query;
        } else {
            if (ref._authority) {
                target._authority = ref._authority;
                target._path = remove_dot_segments(ref._path);
                target._query = ref._query;

            } else {
                if (ref._path.empty()) {
                    target._path = base._path;
                    if (ref._query) {
                        target._query = ref._query;
                    } else {
                        target._query = base._query;
                    }
                } else {
                    if (ref._path.absolute()) {
                        target._path = remove_dot_segments(ref._path);
                    } else {
                        target._path = remove_dot_segments(merge(base._path, ref._path, to_bool(base._authority)));
                    }
                    target._query = ref._query;
                }
                target._authority = base._authority;
            }
            target._scheme = base._scheme;
        }

        target._fragment = ref._fragment;

        return target;
    }

    [[nodiscard]] constexpr friend bool operator==(URI const& lhs, std::string_view rhs) noexcept
    {
        return lhs == URI(rhs);
    }

    [[nodiscard]] constexpr friend auto operator<=>(URI const& lhs, std::string_view rhs) noexcept
    {
        return lhs == URI(rhs);
    }

    [[nodiscard]] constexpr friend URI operator/(URI const& base, std::string_view ref) noexcept
    {
        return base / URI(ref);
    }

    /** URI percent-encoding decode function.
     *
     * @param rhs A percent-encoded string.
     * @return A UTF-8 encoded string.
     */
    [[nodiscard]] constexpr static std::string decode(std::string_view rhs)
    {
        auto r = std::string{rhs};

        for (auto i = r.find('%'); i != std::string::npos; i = r.find('%', i)) {
            // This may throw a parse_error, if not hexadecimal
            if (auto c = from_string<uint8_t>(r.substr(i + 1, 2), 16)) {

                // Replace the % encoded character.
                r.replace(i, 3, 1, char_cast<char>(*c));

                // Skip over encoded-character.
                ++i;

            } else {
                throw uri_error("Invalid percent-encoded character.");
            }
        }

        return r;
    }

    /** URI percent-encoding decode function.
     *
     * @param first An iterator to the first character of a percent encoded string.
     * @param last An iterator to one beyond the last character of a percent encoded string.
     * @return A UTF-8 encoded string.
     */
    [[nodiscard]] constexpr static std::string decode(const_iterator first, const_iterator last)
    {
        return decode(std::string_view{first, last});
    }

    /** URI encode a component.
     *
     * @tparam Extras The extra characters beyond the unreserved characters to pct-encode.
     * @param first Iterator to a UTF-8 encoded string; a component or sub-component of a URI.
     * @param last Iterator pointing one beyond the UTF-8 encoded string.
     * @return A percent-encoded string.
     */
    template<char... Extras, typename It, typename ItEnd>
    [[nodiscard]] constexpr static std::string encode(It first, ItEnd last) noexcept
    {
        auto r = std::string{};
        if constexpr (requires { std::distance(first, last); }) {
            r.reserve(std::distance(first, last));
        }

        for (auto it = first; it != last; ++it) {
            auto const c = *it;
            // clang-format off
            if (((
                    (c >= 'a' and c <= 'z') or
                    (c >= 'A' and c <= 'Z') or
                    (c >= '0' and c <= '9') or
                    c == '-' or c == '.' or c == '_' or c == '~'
                ) or ... or (c == Extras))) {
                r += c;

            } else {
                r += '%';
                auto uc = char_cast<uint8_t>(c);
                if (auto nibble = narrow_cast<char>(uc >> 4); nibble <= 9) {
                    r += '0' + nibble;
                } else {
                    r += 'A' + nibble - 10;
                }
                if (auto nibble = narrow_cast<char>(uc & 0xf); nibble <= 9) {
                    r += '0' + nibble;
                } else {
                    r += 'A' + nibble - 10;
                }
            }
            // clang-format on
        }

        return r;
    }

    /** URI encode a component.
     *
     * @tparam Extras The extra characters beyond the unreserved characters to pct-encode.
     * @param range A range representing UTF-8 encoded string; a component or sub-component of a URI.
     * @return A percent-encoded string.
     */
    template<char... Extras, typename Range>
    [[nodiscard]] constexpr static std::string encode(Range&& range) noexcept
    {
        return encode<Extras...>(std::ranges::begin(range), std::ranges::end(range));
    }

private:
    std::optional<std::string> _scheme;
    std::optional<authority_type> _authority;
    path_type _path;
    std::optional<std::string> _query;
    std::optional<std::string> _fragment;

    [[nodiscard]] constexpr friend size_t to_string_size(URI const& rhs) noexcept
    {
        auto size = 0_uz;
        if (rhs._scheme) {
            size += rhs._scheme->size() + 1;
        }
        if (rhs._authority) {
            size += to_string_size(*rhs._authority) + 2;
        }
        size += to_string_size(rhs._path);

        if (rhs._query) {
            size += rhs._query->size() + 1;
        }
        if (rhs._fragment) {
            size += rhs._fragment->size() + 1;
        }

        return size;
    }

    [[nodiscard]] constexpr static bool is_scheme_start(char c) noexcept
    {
        return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z');
    }

    [[nodiscard]] constexpr static bool is_scheme(char c) noexcept
    {
        return is_scheme_start(c) or (c >= '0' and c <= '9') or c == '+' or c == '-' or c == '.';
    }

    constexpr static void validate_scheme(std::string_view str)
    {
        if (str.empty()) {
            throw uri_error("The scheme-component is not allowed to be empty (it is allowed to not exist).");
        }
        if (not is_scheme_start(str.front())) {
            throw uri_error("The scheme-component does not start with [a-zA-Z].");
        }
        for (auto c : str) {
            if (not is_scheme(c)) {
                throw uri_error("The scheme-component contains a character outside [a-zA-Z0-9.+-].");
            }
        }
    }

    constexpr static void validate_path(path_type const& path, bool has_authority)
    {
        if (has_authority) {
            if (not(path.empty() or path.absolute())) {
                throw uri_error("A path-component in a URI with an authority-component must be empty or absolute.");
            }
        } else if (path.double_absolute()) {
            throw uri_error("A path-component in a URI without an authority-component may not start with a double slash '//'.");
        }
    }

    /** Parse a URI scheme part.
     *
     * @param str Substring of the URI starting at the potential scheme.
     * @return The number of characters in the scheme.
     * @retval -1 There is no scheme.
     */
    [[nodiscard]] constexpr const_iterator parse_scheme(const_iterator first, const_iterator last)
    {
        for (auto it = first; it != last; ++it) {
            if (auto const c = *it; c == ':') {
                set_scheme(std::string{first, it});
                return it + 1; // Skip over ':'.

            } else if (c == '/' or c == '?' or c == '#') {
                // Invalid character, this is not a scheme.
                return first;
            }
        }

        // Reached at end of URI, this is not a scheme.
        return first;
    }

    [[nodiscard]] constexpr const_iterator parse_authority(const_iterator first, const_iterator last)
    {
        for (auto it = first; it != last; it++) {
            if (auto const c = *it; c == '/' or c == '?' or c == '#') {
                set_authority(authority_type{std::string_view{first, it}});
                return it;
            }
        }

        set_authority(authority_type{std::string_view{first, last}});
        return last;
    }

    [[nodiscard]] constexpr const_iterator parse_path(const_iterator first, const_iterator last)
    {
        for (auto it = first; it != last; it++) {
            if (auto const c = *it; c == '?' or c == '#') {
                set_path(path_type{std::string_view{first, it}});
                return it;
            }
        }

        set_path(path_type{std::string_view{first, last}});
        return last;
    }

    [[nodiscard]] constexpr const_iterator parse_query(const_iterator first, const_iterator last)
    {
        for (auto it = first; it != last; it++) {
            if (auto const c = *it; c == '#') {
                set_query(URI::decode(first, it));
                return it;
            }
        }

        set_query(URI::decode(first, last));
        return last;
    }

    [[nodiscard]] constexpr const_iterator parse_fragment(const_iterator first, const_iterator last)
    {
        set_fragment(URI::decode(first, last));
        return last;
    }

    constexpr void parse(std::string_view str)
    {
        auto first = str.cbegin();
        auto last = str.cend();
        auto it = parse_scheme(first, last);

        if (std::distance(it, last) >= 2 and it[0] == '/' and it[1] == '/') {
            it += 2;
            it = parse_authority(it, last);
        }

        it = parse_path(it, last);

        if (it != last and *it == '?') {
            it = parse_query(++it, last);
        }

        if (it != last and *it == '#') {
            it = parse_fragment(++it, last);
        }
    }

    friend struct std::hash<URI>;
};

#undef HI_PCHAR
#undef HI_SUB_DELIM

}} // namespace hi::v1

template<>
struct std::hash<hi::URI> {
    [[nodiscard]] size_t operator()(hi::URI const& rhs) const noexcept
    {
        return std::hash<std::string>{}(to_string(rhs));
    }
};

// XXX #617 MSVC bug does not handle partial specialization in modules.
hi_export template<>
struct std::formatter<hi::URI, char> : std::formatter<std::string, char> {
    auto format(hi::URI const& t, auto& fc) const
    {
        return std::formatter<std::string, char>::format(to_string(t), fc);
    }
};
