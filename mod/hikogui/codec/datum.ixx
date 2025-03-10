// Copyright Take Vos 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include "../macros.hpp"

#include <cstdint>
#include <concepts>
#include <bit>
#include <exception>
#include <chrono>
#include <limits>
#include <vector>
#include <map>

hi_warning_push();
// C26476: Expression/symbol '...' uses a naked union '...' with multiple type pointers: Use variant instead (type.7.).
// This implements `datum` which is similar to a std::variant.
hi_warning_ignore_msvc(26476);
// C26409: Avoid calling new and delete explicitly, use std::make_unique<T> instead (r.11).
// This implements `datum` which implements RAII for large objects.
hi_warning_ignore_msvc(26409);
// C26492: Don't use const_cast to cast away const or volatile (type.3).
// Needed until c++23 deducing this.
hi_warning_ignore_msvc(26492);

export module hikogui_codec_datum;
import hikogui_codec_base_n;
import hikogui_codec_jsonpath;
import hikogui_container;
import hikogui_numeric;
import hikogui_utility;

export namespace hi { inline namespace v1 {
namespace detail {

/** Promotion result.
 *
 * @tparam To The type to promote the values to.
 */
template<typename To>
class datum_promotion_result {
public:
    using value_type = To;
    constexpr static bool data_is_pointer = sizeof(value_type) > sizeof(void *);
    constexpr static bool data_is_scalar = not data_is_pointer;

    constexpr void clear() noexcept
        requires(data_is_scalar)
    {
    }
    constexpr void clear() noexcept
        requires(data_is_pointer)
    {
        if (_owns_lhs) {
            delete _lhs;
        }
        if (_owns_rhs) {
            delete _rhs;
        }
    }

    constexpr ~datum_promotion_result()
    {
        clear();
    }

    constexpr datum_promotion_result() noexcept = default;

    datum_promotion_result(datum_promotion_result const&) = delete;
    datum_promotion_result& operator=(datum_promotion_result const&) = delete;

    constexpr datum_promotion_result(datum_promotion_result&& other) noexcept :
        _lhs(other._lhs),
        _rhs(other._rhs),
        _is_result(other._is_result),
        _owns_lhs(std::exchange(other._owns_lhs, false)),
        _owns_rhs(std::exchange(other._owns_rhs, false))
    {
    }

    constexpr datum_promotion_result& operator=(datum_promotion_result&& other) noexcept
    {
        clear();
        _lhs = other.lhs;
        _rhs = other.rhs;
        _is_result = other._is_result;
        _owns_lhs = std::exchange(other._owns_lhs, false);
        _owns_rhs = std::exchange(other._owns_rhs, false);
        return *this;
    }

    constexpr explicit operator bool() const noexcept
    {
        return _is_result;
    }

    constexpr void set(value_type lhs, value_type rhs) noexcept
        requires(data_is_scalar)
    {
        _lhs = lhs;
        _rhs = rhs;
        _is_result = true;
    }

    constexpr void set(value_type const& lhs, value_type const& rhs) noexcept
        requires(data_is_pointer)
    {
        _lhs = &lhs;
        _rhs = &rhs;
        _is_result = true;
    }

    constexpr void set(value_type&& lhs, value_type const& rhs) noexcept
        requires(data_is_pointer)
    {
        _lhs = new value_type(std::move(lhs));
        _rhs = &rhs;
        _is_result = true;
        _owns_lhs = true;
    }

    constexpr void set(value_type const& lhs, value_type&& rhs) noexcept
        requires(data_is_pointer)
    {
        _lhs = &lhs;
        _rhs = new value_type(std::move(rhs));
        _is_result = true;
        _owns_rhs = true;
    }

    constexpr void set(value_type&& lhs, value_type&& rhs) noexcept
        requires(data_is_pointer)
    {
        _lhs = new value_type(std::move(lhs));
        _rhs = new value_type(std::move(rhs));
        _is_result = true;
        _owns_lhs = true;
        _owns_rhs = true;
    }

    [[nodiscard]] constexpr value_type const& lhs() const noexcept
        requires(data_is_pointer)
    {
        hi_axiom(_is_result);
        return *_lhs;
    }

    [[nodiscard]] constexpr value_type const& rhs() const noexcept
        requires(data_is_pointer)
    {
        hi_axiom(_is_result);
        return *_rhs;
    }

    [[nodiscard]] constexpr value_type lhs() const noexcept
        requires(data_is_scalar)
    {
        hi_axiom(_is_result);
        return _lhs;
    }

    [[nodiscard]] constexpr value_type rhs() const noexcept
        requires(data_is_scalar)
    {
        hi_axiom(_is_result);
        return _rhs;
    }

private:
    using data_type = std::conditional_t<data_is_pointer, value_type const *, value_type>;
    data_type _lhs = data_type{};
    data_type _rhs = data_type{};
    bool _is_result = false;
    bool _owns_lhs = false;
    bool _owns_rhs = false;
};

} // namespace detail

export template<typename T>
class is_datum_type : public std::false_type {};

export template<>
class is_datum_type<long long> : public std::true_type {};
export template<>
class is_datum_type<decimal> : public std::true_type {};
export template<>
class is_datum_type<double> : public std::true_type {};
export template<>
class is_datum_type<bool> : public std::true_type {};
export template<>
class is_datum_type<std::chrono::year_month_day> : public std::true_type {};
export template<>
class is_datum_type<std::string> : public std::true_type {};
export template<>
class is_datum_type<bstring> : public std::true_type {};

export template<typename T>
constexpr bool is_datum_type_v = is_datum_type<T>::value;

/** A dynamic data type.
 *
 * This class holds data of different types, useful as the data-type used for variables
 * of scripting languages, or for serializing and deserializing JSON and other object
 * storage formats.
 *
 * Not only does this datum handle the storage of data, but can also different operations
 * which are dynamically executed.
 */
export class datum {
public:
    using vector_type = std::vector<datum>;
    using map_type = std::map<datum, datum>;
    struct break_type {};
    struct continue_type {};

    /** Promote two datum-arguments to a common type.
     *
     * @tparam To Type to promote to.
     * @param lhs The left hand side.
     * @param rhs The right hand side.
     */
    template<typename To>
    [[nodiscard]] friend constexpr auto promote_if(datum const& lhs, datum const& rhs) noexcept
    {
        auto r = detail::datum_promotion_result<To>{};
        if (holds_alternative<To>(lhs) and holds_alternative<To>(rhs)) {
            r.set(get<To>(lhs), get<To>(rhs));

        } else if (holds_alternative<To>(lhs) and promotable_to<To>(rhs)) {
            r.set(get<To>(lhs), static_cast<To>(rhs));

        } else if (promotable_to<To>(lhs) and holds_alternative<To>(rhs)) {
            r.set(static_cast<To>(lhs), get<To>(rhs));
        }

        return r;
    }

    constexpr ~datum() noexcept
    {
        delete_pointer();
    }

    constexpr datum(datum const& other) noexcept : _tag(other._tag), _value(other._value)
    {
        if (other.is_pointer()) {
            copy_pointer(other);
        }
    }

    constexpr datum(datum&& other) noexcept : _tag(other._tag), _value(other._value)
    {
        other._tag = tag_type::monostate;
        other._value._long_long = 0;
    }

    constexpr datum() noexcept : _tag(tag_type::monostate), _value(0) {}
    constexpr explicit datum(std::monostate) noexcept : _tag(tag_type::monostate), _value(0) {}
    constexpr explicit datum(nullptr_t) noexcept : _tag(tag_type::null), _value(0) {}
    constexpr explicit datum(continue_type) noexcept : _tag(tag_type::flow_continue), _value(0) {}
    constexpr explicit datum(break_type) noexcept : _tag(tag_type::flow_break), _value(0) {}
    constexpr explicit datum(bool value) noexcept : _tag(tag_type::boolean), _value(value) {}
    constexpr explicit datum(std::floating_point auto value) noexcept :
        _tag(tag_type::floating_point), _value(narrow_cast<double>(value))
    {
    }

    constexpr explicit datum(numeric_integral auto value) noexcept :
        _tag(tag_type::integral), _value(narrow_cast<long long>(value))
    {
    }

    constexpr explicit datum(decimal value) noexcept : _tag(tag_type::decimal), _value(value) {}
    constexpr explicit datum(std::chrono::year_month_day value) noexcept : _tag(tag_type::year_month_day), _value(value) {}
    explicit datum(std::string value) noexcept : _tag(tag_type::string), _value(new std::string{std::move(value)}) {}
    explicit datum(std::string_view value) noexcept : _tag(tag_type::string), _value(new std::string{value}) {}
    explicit datum(char const *value) noexcept : _tag(tag_type::string), _value(new std::string{value}) {}
    explicit datum(vector_type value) noexcept : _tag(tag_type::vector), _value(new vector_type{std::move(value)}) {}
    explicit datum(map_type value) noexcept : _tag(tag_type::map), _value(new map_type{std::move(value)}) {}
    explicit datum(bstring value) noexcept : _tag(tag_type::bstring), _value(new bstring{std::move(value)}) {}

    template<typename... Args>
    [[nodiscard]] static datum make_vector(Args const&...args) noexcept
    {
        return datum{vector_type{datum{args}...}};
    }

    template<typename Key, typename Value, typename... Args>
    static void populate_map(map_type& r, Key const& key, Value const& value, Args const&...args) noexcept
    {
        r.insert(std::pair<datum, datum>{datum{key}, datum{value}});
        if constexpr (sizeof...(Args) > 0) {
            populate_map(r, args...);
        }
    }

    template<typename... Args>
    [[nodiscard]] static datum make_map(Args const&...args) noexcept
    {
        static_assert(sizeof...(Args) % 2 == 0, "Expect key value pairs for the arguments of make_map()");

        auto r = map_type{};
        if constexpr (sizeof...(Args) > 0) {
            populate_map(r, args...);
        }
        return datum{std::move(r)};
    }

    [[nodiscard]] static datum make_break() noexcept
    {
        return datum{break_type{}};
    }

    [[nodiscard]] static datum make_continue() noexcept
    {
        return datum{continue_type{}};
    }

    constexpr datum& operator=(datum const& other) noexcept
    {
        hi_return_on_self_assignment(other);

        delete_pointer();
        _tag = other._tag;
        _value = other._value;
        if (other.is_pointer()) {
            copy_pointer(other);
        }
        return *this;
    }

    constexpr datum& operator=(datum&& other) noexcept
    {
        std::swap(_tag, other._tag);
        std::swap(_value, other._value);
        return *this;
    }

    constexpr datum& operator=(std::floating_point auto value) noexcept(sizeof(value) <= 4)
    {
        delete_pointer();
        _tag = tag_type::floating_point;
        _value = static_cast<double>(value);
        return *this;
    }

    constexpr datum& operator=(numeric_integral auto value) noexcept(sizeof(value) <= 4)
    {
        delete_pointer();
        _tag = tag_type::integral;
        _value = static_cast<long long>(value);
        return *this;
    }

    constexpr datum& operator=(decimal value)
    {
        delete_pointer();
        _tag = tag_type::decimal;
        _value = value;
        return *this;
    }
    constexpr datum& operator=(bool value) noexcept
    {
        delete_pointer();
        _tag = tag_type::boolean;
        _value = value;
        return *this;
    }

    constexpr datum& operator=(std::chrono::year_month_day value) noexcept
    {
        delete_pointer();
        _tag = tag_type::year_month_day;
        _value = value;
        return *this;
    }

    constexpr datum& operator=(std::monostate) noexcept
    {
        delete_pointer();
        _tag = tag_type::monostate;
        _value = 0;
        return *this;
    }

    constexpr datum& operator=(nullptr_t) noexcept
    {
        delete_pointer();
        _tag = tag_type::null;
        _value = 0;
        return *this;
    }

    datum& operator=(std::string value) noexcept
    {
        delete_pointer();
        _tag = tag_type::string;
        _value = new std::string{std::move(value)};
        return *this;
    }

    datum& operator=(char const *value) noexcept
    {
        delete_pointer();
        _tag = tag_type::string;
        _value = new std::string{value};
        return *this;
    }

    datum& operator=(std::string_view value) noexcept
    {
        delete_pointer();
        _tag = tag_type::string;
        _value = new std::string{value};
        return *this;
    }

    datum& operator=(vector_type value) noexcept
    {
        delete_pointer();
        _tag = tag_type::vector;
        _value = new vector_type{std::move(value)};
        return *this;
    }

    datum& operator=(map_type value) noexcept
    {
        delete_pointer();
        _tag = tag_type::map;
        _value = new map_type{std::move(value)};
        return *this;
    }

    datum& operator=(bstring value) noexcept
    {
        delete_pointer();
        _tag = tag_type::bstring;
        _value = new bstring{std::move(value)};
        return *this;
    }

    constexpr explicit operator bool() const noexcept
    {
        switch (_tag) {
        case tag_type::floating_point:
            return to_bool(get<double>(*this));
        case tag_type::decimal:
            return to_bool(get<decimal>(*this));
        case tag_type::boolean:
            return get<bool>(*this);
        case tag_type::integral:
            return to_bool(get<long long>(*this));
        case tag_type::year_month_day:
            return true;
        case tag_type::string:
            return not get<std::string>(*this).empty();
        case tag_type::vector:
            return not get<vector_type>(*this).empty();
        case tag_type::map:
            return not get<map_type>(*this).empty();
        case tag_type::bstring:
            return not get<bstring>(*this).empty();
        default:
            return false;
        }
    }

    [[nodiscard]] constexpr bool empty() const
    {
        switch (_tag) {
        case tag_type::string:
            return get<std::string>(*this).empty();
        case tag_type::vector:
            return get<vector_type>(*this).empty();
        case tag_type::map:
            return get<map_type>(*this).empty();
        case tag_type::bstring:
            return get<bstring>(*this).empty();
        default:
            throw std::domain_error(std::format("Type {} can not be checked for empty", repr(*this)));
        }
    }

    template<std::floating_point T>
    constexpr explicit operator T() const
    {
        switch (_tag) {
        case tag_type::floating_point:
            return static_cast<T>(get<double>(*this));
        case tag_type::integral:
            return static_cast<T>(get<long long>(*this));
        case tag_type::decimal:
            return static_cast<T>(get<decimal>(*this));
        case tag_type::boolean:
            return static_cast<T>(get<bool>(*this));
        default:
            throw std::domain_error(std::format("Can't convert {} to floating point", repr(*this)));
        }
    }

    constexpr explicit operator decimal() const
    {
        switch (_tag) {
        case tag_type::floating_point:
            return decimal(get<double>(*this));
        case tag_type::integral:
            return decimal(get<long long>(*this));
        case tag_type::decimal:
            return get<decimal>(*this);
        case tag_type::boolean:
            return decimal(get<bool>(*this));
        default:
            throw std::domain_error(std::format("Can't convert {} to floating point", repr(*this)));
        }
    }

    template<numeric_integral T>
    constexpr explicit operator T() const
    {
        if (auto f = get_if<double>(*this)) {
            errno = 0;
            hilet r = std::round(*f);
            if (errno == EDOM or errno == ERANGE or r < narrow_cast<double>(std::numeric_limits<T>::min()) or
                r > narrow_cast<double>(std::numeric_limits<T>::max())) {
                throw std::overflow_error("double to integral");
            }
            return round_cast<T>(r);

        } else if (auto i = get_if<long long>(*this)) {
            if (*i < std::numeric_limits<T>::min() or *i > std::numeric_limits<T>::max()) {
                throw std::overflow_error("long long to integral");
            }
            return narrow_cast<T>(*i);

        } else if (auto d = get_if<decimal>(*this)) {
            hilet r = static_cast<long long>(*d);
            if (r < std::numeric_limits<T>::min() or r > std::numeric_limits<T>::max()) {
                throw std::overflow_error("decimal to integral");
            }
            return narrow_cast<T>(r);

        } else if (auto b = get_if<bool>(*this)) {
            return narrow_cast<T>(*b);

        } else {
            throw std::domain_error(std::format("Can't convert {} to an integral", repr(*this)));
        }
    }

    constexpr explicit operator std::chrono::year_month_day() const
    {
        if (auto ymd = get_if<std::chrono::year_month_day>(*this)) {
            return *ymd;
        } else {
            throw std::domain_error(std::format("Can't convert {} to an std::chrono::year_month_day", repr(*this)));
        }
    }

    explicit operator std::string() const noexcept
    {
        switch (_tag) {
        case tag_type::monostate:
            return "undefined";
        case tag_type::floating_point:
            return hi::to_string(_value._double);
        case tag_type::decimal:
            return to_string(_value._decimal);
        case tag_type::integral:
            return to_string(_value._long_long);
        case tag_type::boolean:
            return _value._bool ? "true" : "false";
        case tag_type::year_month_day:
            return std::format("{:%Y-%m-%d}", _value._year_month_day);
        case tag_type::null:
            return "null";
        case tag_type::flow_break:
            return "break";
        case tag_type::flow_continue:
            return "continue";
        case tag_type::string:
            return *_value._string;
        case tag_type::vector:
            {
                auto r = std::string{"["};
                for (hilet& item : *_value._vector) {
                    r += repr(item);
                    r += ',';
                }
                r += ']';
                return r;
            };
        case tag_type::map:
            {
                auto r = std::string{"{"};
                for (hilet& item : *_value._map) {
                    r += repr(item.first);
                    r += ':';
                    r += repr(item.second);
                    r += ',';
                }
                r += '}';
                return r;
            };
        case tag_type::bstring:
            return base64::encode(*_value._bstring);
        default:
            hi_no_default();
        }
    }

    explicit operator std::string_view() const
    {
        if (auto s = get_if<std::string>(*this)) {
            return std::string_view{*s};
        } else {
            throw std::domain_error(std::format("Can't convert {} to an std::string_view", repr(*this)));
        }
    }

    explicit operator vector_type() const
    {
        if (auto v = get_if<vector_type>(*this)) {
            return *v;
        } else {
            throw std::domain_error(std::format("Can't convert {} to an vector", repr(*this)));
        }
    }

    explicit operator map_type() const
    {
        if (auto m = get_if<map_type>(*this)) {
            return *m;
        } else {
            throw std::domain_error(std::format("Can't convert {} to an map", repr(*this)));
        }
    }

    explicit operator bstring() const
    {
        // XXX should be able to base-64 decode a std::string.
        if (_tag != tag_type::bstring) {
            throw std::domain_error(std::format("Can't convert {} to an bstring", repr(*this)));
        }
        return get<bstring>(*this);
    }

    [[nodiscard]] constexpr char const *type_name() const noexcept
    {
        switch (_tag) {
        case tag_type::floating_point:
            return "float";
        case tag_type::decimal:
            return "decimal";
        case tag_type::integral:
            return "int";
        case tag_type::boolean:
            return "bool";
        case tag_type::year_month_day:
            return "date";
        case tag_type::string:
            return "string";
        case tag_type::vector:
            return "vector";
        case tag_type::map:
            return "map";
        case tag_type::bstring:
            return "bytes";
        default:
            hi_no_default();
        }
    }

    /** Check if the datum has an undefined value.
     */
    [[nodiscard]] constexpr bool is_undefined() const noexcept
    {
        return _tag == tag_type::monostate;
    }

    /** Check if the result of a expression was a break flow control statement.
     * @see skeleton_node
     */
    [[nodiscard]] constexpr bool is_break() const noexcept
    {
        return _tag == tag_type::flow_break;
    }

    /** Check if the result of a expression was a continue flow control statement.
     * @see skeleton_node
     */
    [[nodiscard]] constexpr bool is_continue() const noexcept
    {
        return _tag == tag_type::flow_continue;
    }

    [[nodiscard]] std::size_t hash() const noexcept
    {
        switch (_tag) {
        case tag_type::floating_point:
            return std::hash<double>{}(_value._double);
        case tag_type::decimal:
            return std::hash<decimal>{}(_value._decimal);
        case tag_type::integral:
            return std::hash<long long>{}(_value._long_long);
        case tag_type::boolean:
            return std::hash<bool>{}(_value._bool);
        case tag_type::year_month_day:
            {
                uint32_t r = 0;
                r |= narrow_cast<uint32_t>(static_cast<int>(_value._year_month_day.year())) << 16;
                r |= narrow_cast<uint32_t>(static_cast<unsigned>(_value._year_month_day.month())) << 8;
                r |= narrow_cast<uint32_t>(static_cast<unsigned>(_value._year_month_day.day()));
                return std::hash<uint32_t>{}(r);
            }
        case tag_type::string:
            return std::hash<std::string>{}(*_value._string);
        case tag_type::vector:
            {
                std::size_t r = 0;
                for (hilet& v : *_value._vector) {
                    r = hash_mix(r, v.hash());
                }
                return r;
            }
        case tag_type::map:
            {
                std::size_t r = 0;
                for (hilet& kv : *_value._map) {
                    r = hash_mix(r, kv.first.hash(), kv.second.hash());
                }
                return r;
            }
        case tag_type::bstring:
            return std::hash<bstring>{}(*_value._bstring);
        default:
            hi_no_default();
        }
    }

    [[nodiscard]] constexpr std::size_t size() const
    {
        if (hilet *s = get_if<std::string>(*this)) {
            return s->size();
        } else if (hilet *v = get_if<vector_type>(*this)) {
            return v->size();
        } else if (hilet *m = get_if<map_type>(*this)) {
            return m->size();
        } else if (hilet *b = get_if<bstring>(*this)) {
            return b->size();
        } else {
            throw std::domain_error(std::format("Can not evaluate {}.size()", repr(*this)));
        }
    }

    [[nodiscard]] constexpr friend std::size_t size(datum const& rhs)
    {
        return rhs.size();
    }

    [[nodiscard]] constexpr datum const& back() const
    {
        if (hilet *v = get_if<vector_type>(*this)) {
            if (v->empty()) {
                throw std::domain_error(std::format("Empty vector {}.back()", repr(*this)));
            }
            return v->back();
        } else {
            throw std::domain_error(std::format("Can not evaluate {}.back()", repr(*this)));
        }
    }

    [[nodiscard]] constexpr datum& back()
    {
        if (auto *v = get_if<vector_type>(*this)) {
            if (v->empty()) {
                throw std::domain_error(std::format("Empty vector {}.back()", repr(*this)));
            }
            return v->back();
        } else {
            throw std::domain_error(std::format("Can not evaluate {}.back()", repr(*this)));
        }
    }

    [[nodiscard]] constexpr datum const& front() const
    {
        if (hilet *v = get_if<vector_type>(*this)) {
            if (v->empty()) {
                throw std::domain_error(std::format("Empty vector {}.front()", repr(*this)));
            }
            return v->front();
        } else {
            throw std::domain_error(std::format("Can not evaluate {}.front()", repr(*this)));
        }
    }

    [[nodiscard]] constexpr datum& front()
    {
        if (auto *v = get_if<vector_type>(*this)) {
            if (v->empty()) {
                throw std::domain_error(std::format("Empty vector {}.front()", repr(*this)));
            }
            return v->front();
        } else {
            throw std::domain_error(std::format("Can not evaluate {}.front()", repr(*this)));
        }
    }

    [[nodiscard]] constexpr auto cbegin() const
    {
        if (hilet *v = get_if<vector_type>(*this)) {
            return v->cbegin();
        } else {
            throw std::domain_error(std::format("Can not evaluate {}.cbegin()", repr(*this)));
        }
    }

    [[nodiscard]] constexpr auto begin() const
    {
        if (hilet *v = get_if<vector_type>(*this)) {
            return v->begin();
        } else {
            throw std::domain_error(std::format("Can not evaluate {}.begin()", repr(*this)));
        }
    }

    [[nodiscard]] constexpr auto begin()
    {
        if (hilet *v = get_if<vector_type>(*this)) {
            return v->begin();
        } else {
            throw std::domain_error(std::format("Can not evaluate {}.begin()", repr(*this)));
        }
    }

    [[nodiscard]] constexpr auto cend() const
    {
        if (hilet *v = get_if<vector_type>(*this)) {
            return v->cend();
        } else {
            throw std::domain_error(std::format("Can not evaluate {}.cend()", repr(*this)));
        }
    }

    [[nodiscard]] constexpr auto end() const
    {
        if (hilet *v = get_if<vector_type>(*this)) {
            return v->end();
        } else {
            throw std::domain_error(std::format("Can not evaluate {}.end()", repr(*this)));
        }
    }

    [[nodiscard]] constexpr auto end()
    {
        if (hilet *v = get_if<vector_type>(*this)) {
            return v->end();
        } else {
            throw std::domain_error(std::format("Can not evaluate {}.end()", repr(*this)));
        }
    }

    /** Get the sorted list of keys of a map.
     */
    [[nodiscard]] vector_type keys() const
    {
        if (hilet *m = get_if<map_type>(*this)) {
            auto r = vector_type{};
            r.reserve(m->size());
            for (hilet& kv : *m) {
                r.push_back(kv.first);
            }
            return r;
        } else {
            throw std::domain_error(std::format("Can not evaluate {}.keys()", repr(*this)));
        }
    }

    /** Get the list of values of a map.
     */
    [[nodiscard]] vector_type values() const
    {
        if (hilet *m = get_if<map_type>(*this)) {
            auto r = vector_type{};
            r.reserve(m->size());
            for (hilet& kv : *m) {
                r.push_back(kv.second);
            }
            return r;
        } else {
            throw std::domain_error(std::format("Can not evaluate {}.values()", repr(*this)));
        }
    }

    /** Get key value pairs of items of a map sorted by the key.
     */
    [[nodiscard]] vector_type items() const
    {
        if (hilet *m = get_if<map_type>(*this)) {
            auto r = vector_type{};
            r.reserve(m->size());

            for (hilet& item : *m) {
                r.push_back(make_vector(item.first, item.second));
            }
            return r;
        } else {
            throw std::domain_error(std::format("Can not evaluate {}.items()", repr(*this)));
        }
    }

    constexpr void push_back(datum const& rhs)
    {
        if (auto *v = get_if<vector_type>(*this)) {
            return v->push_back(rhs);
        } else {
            throw std::domain_error(std::format("Can not evaluate {}.push_back({})", repr(*this), repr(rhs)));
        }
    }

    constexpr void push_back(datum&& rhs)
    {
        if (auto *v = get_if<vector_type>(*this)) {
            return v->push_back(std::move(rhs));
        } else {
            throw std::domain_error(std::format("Can not evaluate {}.push_back({})", repr(*this), repr(rhs)));
        }
    }

    template<typename Arg>
    constexpr void push_back(Arg&& arg)
    {
        push_back(datum{std::forward<Arg>(arg)});
    }

    constexpr void pop_back()
    {
        if (auto *v = get_if<vector_type>(*this)) {
            if (v->empty()) {
                throw std::domain_error(std::format("Empty vector {}.pop_back()", repr(*this)));
            }
            return v->pop_back();
        } else {
            throw std::domain_error(std::format("Can not evaluate {}.pop_back()", repr(*this)));
        }
    }

    [[nodiscard]] constexpr bool contains(datum const& rhs) const
    {
        if (auto *m = get_if<map_type>(*this)) {
            return m->contains(rhs);
        } else {
            throw std::domain_error(std::format("Can not evaluate {}.contains({})", repr(*this), repr(rhs)));
        }
    }

    template<typename Arg>
    [[nodiscard]] constexpr bool contains(Arg const& arg) const
    {
        return contains(datum{arg});
    }

    [[nodiscard]] std::vector<datum *> find(jsonpath const& path) noexcept
    {
        auto r = std::vector<datum *>{};
        find(path.cbegin(), path.cend(), r);
        return r;
    }

    [[nodiscard]] std::vector<datum const *> find(jsonpath const& path) const noexcept
    {
        auto tmp = std::vector<datum *>{};
        const_cast<datum *>(this)->find(path.cbegin(), path.cend(), tmp);
        auto r = std::vector<datum const *>{};
        std::copy(tmp.begin(), tmp.end(), std::back_inserter(r));
        return r;
    }

    /** Remove the object by path.
     *
     * This function will remove the object pointed to with path.
     * Any resulting empty maps and arrays will also be removed.
     *
     * @param path A json path to remove.
     * @return true if one or more objects where removed.
     */
    [[nodiscard]] bool remove(jsonpath const& path) noexcept
    {
        return to_bool(remove(path.cbegin(), path.cend()));
    }

    /** Find a object by path.
     *
     * @param path The json path to use to find an object. Path must be singular.
     * @return A pointer to the object found, or nullptr.
     */
    [[nodiscard]] datum *find_one(jsonpath const& path) noexcept
    {
        hi_axiom(path.is_singular());
        return find_one(path.cbegin(), path.cend(), false);
    }

    /** Find a object by path potentially creating intermediate objects.
     *
     * @param path The json path to use to find an object. Path must be singular.
     * @return A pointer to the object found, or nullptr.
     */
    [[nodiscard]] datum *find_one_or_create(jsonpath const& path) noexcept
    {
        hi_axiom(path.is_singular());
        return find_one(path.cbegin(), path.cend(), true);
    }

    /** Find a object by path.
     *
     * @param path The json path to use to find an object. Path must be singular.
     * @return A pointer to the object found, or nullptr.
     */
    [[nodiscard]] datum const *find_one(jsonpath const& path) const noexcept
    {
        hi_axiom(path.is_singular());
        return const_cast<datum *>(this)->find_one(path.cbegin(), path.cend(), false);
    }

    [[nodiscard]] datum const& operator[](datum const& rhs) const
    {
        if (holds_alternative<vector_type>(*this) and holds_alternative<long long>(rhs)) {
            hilet& v = get<vector_type>(*this);

            auto index = get<long long>(rhs);
            if (index < 0) {
                index = ssize(v) + index;
            }
            if (index < 0 or index >= ssize(v)) {
                throw std::overflow_error(std::format("Index {} beyond bounds of vector", repr(rhs)));
            }

            return v[index];

        } else if (holds_alternative<map_type>(*this)) {
            hilet& m = get<map_type>(*this);
            hilet it = m.find(rhs);
            if (it == m.end()) {
                throw std::overflow_error(std::format("Key {} not found in map", repr(rhs)));
            }

            return it->second;

        } else {
            throw std::domain_error(std::format("Can not evaluate {}[{}]", repr(*this), repr(rhs)));
        }
    }

    [[nodiscard]] constexpr datum& operator[](datum const& rhs)
    {
        if (holds_alternative<vector_type>(*this) and holds_alternative<long long>(rhs)) {
            auto& v = get<vector_type>(*this);

            auto index = get<long long>(rhs);
            if (index < 0) {
                index = ssize(v) + index;
            }
            if (index < 0 or index >= ssize(v)) {
                throw std::overflow_error(std::format("Index {} beyond bounds of vector", repr(rhs)));
            }

            return v[index];

        } else if (holds_alternative<map_type>(*this)) {
            auto& m = get<map_type>(*this);
            return m[rhs];

        } else {
            throw std::domain_error(std::format("Can not evaluate {}[{}]", repr(*this), repr(rhs)));
        }
    }

    [[nodiscard]] constexpr datum const& operator[](auto const& rhs) const
    {
        return (*this)[datum{rhs}];
    }

    [[nodiscard]] constexpr datum& operator[](auto const& rhs)
    {
        return (*this)[datum{rhs}];
    }

    [[nodiscard]] constexpr datum& operator++()
    {
        if (holds_alternative<long long>(*this)) {
            ++_value._long_long;
            return *this;
        } else {
            throw std::domain_error(std::format("Can not evaluate ++{}", repr(*this)));
        }
    }

    [[nodiscard]] constexpr datum& operator--()
    {
        if (holds_alternative<long long>(*this)) {
            --_value._long_long;
            return *this;
        } else {
            throw std::domain_error(std::format("Can not evaluate --{}", repr(*this)));
        }
    }

    [[nodiscard]] constexpr datum operator++(int)
    {
        if (holds_alternative<long long>(*this)) {
            auto tmp = *this;
            _value._long_long++;
            return tmp;
        } else {
            throw std::domain_error(std::format("Can not evaluate {}++", repr(*this)));
        }
    }
    [[nodiscard]] constexpr datum operator--(int)
    {
        if (holds_alternative<long long>(*this)) {
            auto tmp = *this;
            _value._long_long--;
            return tmp;
        } else {
            throw std::domain_error(std::format("Can not evaluate {}--", repr(*this)));
        }
    }

    constexpr datum& operator+=(auto const& rhs)
    {
        if (holds_alternative<vector_type>(*this)) {
            push_back(rhs);
            return *this;
        } else {
            return (*this) = (*this) + rhs;
        }
    }

#define X(op, inner_op) \
    constexpr datum& operator op(auto const& rhs) \
    { \
        return (*this) = (*this)inner_op rhs; \
    }

    X(-=, -)
    X(*=, *)
    X(/=, /)
    X(%=, %)
    X(&=, &)
    X(|=, |)
    X(^=, ^)
    X(<<=, <<)
    X(>>=, >>)
#undef X

    [[nodiscard]] friend constexpr bool operator==(datum const& lhs, datum const& rhs) noexcept
    {
        if (hilet doubles = promote_if<double>(lhs, rhs)) {
            return doubles.lhs() == doubles.rhs();

        } else if (hilet decimals = promote_if<decimal>(lhs, rhs)) {
            return decimals.lhs() == decimals.rhs();

        } else if (hilet long_longs = promote_if<long long>(lhs, rhs)) {
            return long_longs.lhs() == long_longs.rhs();

        } else if (hilet bools = promote_if<bool>(lhs, rhs)) {
            return bools.lhs() == bools.rhs();

        } else if (hilet ymds = promote_if<std::chrono::year_month_day>(lhs, rhs)) {
            return ymds.lhs() == ymds.rhs();

        } else if (hilet strings = promote_if<std::string>(lhs, rhs)) {
            return strings.lhs() == strings.rhs();

        } else if (hilet vectors = promote_if<vector_type>(lhs, rhs)) {
            return vectors.lhs() == vectors.rhs();

        } else if (hilet maps = promote_if<map_type>(lhs, rhs)) {
            return maps.lhs() == maps.rhs();

        } else {
            return lhs._tag == rhs._tag;
        }
    }

    /** Compare datums.
     *
     * Compare are done in the following order:
     * - promote both arguments to `double`.
     * - promote both arguments to `decimal`.
     * - promote both arguments to `long long`.
     * - promote both arguments to `bool`.
     * - promote both arguments to `std::chrono::year_month_day`.
     * - promote both arguments to `std::string`.
     * - promote both arguments to `datum::vector_type`.
     * - promote both arguments to `datum::map_type` sorted by key.
     * - promote both arguments to `bstring`.
     * - Then compare the types them selfs, ordered in the following order:
     *    + bstring = -5,
     *    + map = -3,
     *    + vector = -2,
     *    + string = -1,
     *    + monostate = 0,
     *    + floating_point = 1,
     *    + integral = 2,
     *    + decimal = 3,
     *    + boolean = 4,
     *    + null = 5,
     *    + year_month_day = 6,
     *    + flow_continue = 7,
     *    + flow_break = 8,
     *
     */
    [[nodiscard]] friend constexpr std::partial_ordering operator<=>(datum const& lhs, datum const& rhs) noexcept
    {
        if (hilet doubles = promote_if<double>(lhs, rhs)) {
            return doubles.lhs() <=> doubles.rhs();

        } else if (hilet decimals = promote_if<decimal>(lhs, rhs)) {
            return decimals.lhs() <=> decimals.rhs();

        } else if (hilet long_longs = promote_if<long long>(lhs, rhs)) {
            return long_longs.lhs() <=> long_longs.rhs();

        } else if (hilet bools = promote_if<bool>(lhs, rhs)) {
            return bools.lhs() <=> bools.rhs();

        } else if (hilet year_month_days = promote_if<std::chrono::year_month_day>(lhs, rhs)) {
            return year_month_days.lhs() <=> year_month_days.rhs();

        } else if (hilet strings = promote_if<std::string>(lhs, rhs)) {
            return strings.lhs() <=> strings.rhs();

        } else if (hilet vectors = promote_if<vector_type>(lhs, rhs)) {
            return vectors.lhs() <=> vectors.rhs();

        } else if (hilet maps = promote_if<map_type>(lhs, rhs)) {
            return maps.lhs() <=> maps.rhs();

        } else if (hilet bstrings = promote_if<bstring>(lhs, rhs)) {
            return bstrings.lhs() <=> bstrings.rhs();

        } else {
            return lhs._tag <=> rhs._tag;
        }
    }

    /** Arithmetic negation.
     *
     * A arithmetic negation happens when the operand is `double`, `decimal` or `long long`.
     *
     * @throws std::domain_error When either argument can not be promoted to `double`,
     *         `decimal` or `long long`.
     * @param lhs The left-hand-side operand of the operation.
     * @param lhs The right-hand-side operand of the operation.
     * @return The result of the operation.
     */
    [[nodiscard]] friend constexpr datum operator-(datum const& rhs)
    {
        if (hilet rhs_double = get_if<double>(rhs)) {
            return datum{-*rhs_double};

        } else if (hilet rhs_decimal = get_if<decimal>(rhs)) {
            return datum{-*rhs_decimal};

        } else if (hilet rhs_long_long = get_if<long long>(rhs)) {
            return datum{-*rhs_long_long};

        } else {
            throw std::domain_error(std::format("Can not evaluate -{}", repr(rhs)));
        }
    }

    /** Binary inversion.
     *
     * A binary inversion if the operand is `long long`.
     *
     * @throws std::domain_error When either argument can not be promoted to `long long`.
     * @param lhs The left-hand-side operand of the operation.
     * @param lhs The right-hand-side operand of the operation.
     * @return The result of the operation.
     */
    [[nodiscard]] friend constexpr datum operator~(datum const& rhs)
    {
        if (hilet rhs_long_long = get_if<long long>(rhs)) {
            return datum{~*rhs_long_long};

        } else {
            throw std::domain_error(std::format("Can not evaluate ~{}", repr(rhs)));
        }
    }

    /** add or concatenate.
     *
     * A numeric addition happens when both operands are promoted to `double`,
     * `decimal` or `long long` before the operation is executed.
     *
     * A concatenation happens when both operand are promoted to `std::string` or
     * a `std::vector<datum>`.
     *
     * @throws std::domain_error When either argument can not be promoted to `double`,
     *         `decimal` or `long long`.
     * @param lhs The left-hand-side operand of the operation.
     * @param lhs The right-hand-side operand of the operation.
     * @return The result of the operation.
     */
    [[nodiscard]] friend constexpr datum operator+(datum const& lhs, datum const& rhs)
    {
        if (hilet doubles = promote_if<double>(lhs, rhs)) {
            return datum{doubles.lhs() + doubles.rhs()};

        } else if (hilet decimals = promote_if<decimal>(lhs, rhs)) {
            return datum{decimals.lhs() + decimals.rhs()};

        } else if (hilet long_longs = promote_if<long long>(lhs, rhs)) {
            return datum{long_longs.lhs() + long_longs.rhs()};

        } else if (hilet strings = promote_if<std::string>(lhs, rhs)) {
            return datum{strings.lhs() + strings.rhs()};

        } else if (hilet vectors = promote_if<vector_type>(lhs, rhs)) {
            auto r = vectors.lhs();
            r.insert(r.end(), vectors.rhs().begin(), vectors.rhs().end());
            return datum{std::move(r)};

        } else {
            throw std::domain_error(std::format("Can not evaluate {} '+' {}", repr(lhs), repr(rhs)));
        }
    }

    /** Arithmetic subtract.
     *
     * Both operands are first promoted to `double`, `decimal` or `long long` before the
     * operation is executed.
     *
     * @throws std::domain_error When either argument can not be promoted to `double`,
     *         `decimal` or `long long`.
     * @param lhs The left-hand-side operand of the operation.
     * @param lhs The right-hand-side operand of the operation.
     * @return The result of the operation.
     */
    [[nodiscard]] friend constexpr datum operator-(datum const& lhs, datum const& rhs)
    {
        if (hilet doubles = promote_if<double>(lhs, rhs)) {
            return datum{doubles.lhs() - doubles.rhs()};

        } else if (hilet decimals = promote_if<decimal>(lhs, rhs)) {
            return datum{decimals.lhs() - decimals.rhs()};

        } else if (hilet long_longs = promote_if<long long>(lhs, rhs)) {
            return datum{long_longs.lhs() - long_longs.rhs()};

        } else {
            throw std::domain_error(std::format("Can not evaluate {} '-' {}", repr(lhs), repr(rhs)));
        }
    }

    /**
     * Arithmetic multiply.
     *
     * Both operands are first promoted to `double`, `decimal` or `long long` before the
     * operation is executed.
     *
     * @throws std::domain_error When either argument can not be promoted to `double`,
     *         `decimal` or `long long`.
     * @param lhs The left-hand-side operand of the operation.
     * @param lhs The right-hand-side operand of the operation.
     * @return The result of the operation.
     */
    [[nodiscard]] friend constexpr datum operator*(datum const& lhs, datum const& rhs)
    {
        if (hilet doubles = promote_if<double>(lhs, rhs)) {
            return datum{doubles.lhs() * doubles.rhs()};

        } else if (hilet decimals = promote_if<decimal>(lhs, rhs)) {
            return datum{decimals.lhs() * decimals.rhs()};

        } else if (hilet long_longs = promote_if<long long>(lhs, rhs)) {
            return datum{long_longs.lhs() * long_longs.rhs()};

        } else {
            throw std::domain_error(std::format("Can not evaluate {} '*' {}", repr(lhs), repr(rhs)));
        }
    }

    /** divide.
     *
     * Both operands are first promoted to `double`, `decimal` or `long long` before the
     * operation is executed.
     *
     * @throws std::domain_error When either argument can not be promoted to `long long`. Or when
     *         the right hand operand is zero.
     * @param lhs The left-hand-side operand of the operation.
     * @param lhs The right-hand-side operand of the operation.
     * @return The result of the operation.
     */
    [[nodiscard]] friend constexpr datum operator/(datum const& lhs, datum const& rhs)
    {
        if (hilet doubles = promote_if<double>(lhs, rhs)) {
            if (doubles.rhs() == 0) {
                throw std::domain_error(std::format("Divide by zero {} '/' {}", repr(lhs), repr(rhs)));
            }
            return datum{doubles.lhs() / doubles.rhs()};

        } else if (hilet decimals = promote_if<decimal>(lhs, rhs)) {
            if (decimals.rhs() == 0) {
                throw std::domain_error(std::format("Divide by zero {} '/' {}", repr(lhs), repr(rhs)));
            }
            return datum{decimals.lhs() / decimals.rhs()};

        } else if (hilet long_longs = promote_if<long long>(lhs, rhs)) {
            if (long_longs.rhs() == 0) {
                throw std::domain_error(std::format("Divide by zero {} '/' {}", repr(lhs), repr(rhs)));
            }
            return datum{long_longs.lhs() / long_longs.rhs()};

        } else {
            throw std::domain_error(std::format("Can not evaluate {} '/' {}", repr(lhs), repr(rhs)));
        }
    }

    /** Arithmetic modulo.
     *
     * Both operands are first promoted to `long long` before the
     * operation is executed.
     *
     * @throws std::domain_error When either argument can not be promoted to `long long`. Or when
     *         the right hand operand is zero.
     * @param lhs The left-hand-side operand of the operation.
     * @param lhs The right-hand-side operand of the operation.
     * @return The result of the operation.
     */
    [[nodiscard]] friend constexpr datum operator%(datum const& lhs, datum const& rhs)
    {
        if (hilet long_longs = promote_if<long long>(lhs, rhs)) {
            if (long_longs.rhs() == 0) {
                throw std::domain_error(std::format("Divide by zero {} '%' {}", repr(lhs), repr(rhs)));
            }
            return datum{long_longs.lhs() % long_longs.rhs()};

        } else {
            throw std::domain_error(std::format("Can not evaluate {} '%' {}", repr(lhs), repr(rhs)));
        }
    }

    /** Arithmetic logarithm.
     *
     * Both operands are first promoted to `double` or `long long` before the
     * operation is executed.
     *
     * @throws std::domain_error When either argument can not be promoted to `double` or `long long`.
     * @param lhs The left-hand-side operand of the operation.
     * @param lhs The right-hand-side operand of the operation.
     * @return The result of the operation.
     */
    [[nodiscard]] friend constexpr datum pow(datum const& lhs, datum const& rhs)
    {
        if (hilet doubles = promote_if<double>(lhs, rhs)) {
            return datum{pow(doubles.lhs(), doubles.rhs())};

        } else if (hilet long_longs = promote_if<long long>(lhs, rhs)) {
            return datum{pow(long_longs.lhs(), long_longs.rhs())};

        } else {
            throw std::domain_error(std::format("Can not evaluate pow({}, {})", repr(lhs), repr(rhs)));
        }
    }

    /** binary and.
     *
     * Both operands are first promoted to `long long` or `bool` before the
     * operation is executed.
     *
     * @throws std::domain_error When either argument can not be promoted to `long long` or `bool`.
     * @param lhs The left-hand-side operand of the operation.
     * @param lhs The right-hand-side operand of the operation.
     * @return The result of the operation.
     */
    [[nodiscard]] friend constexpr datum operator&(datum const& lhs, datum const& rhs)
    {
        if (hilet long_longs = promote_if<long long>(lhs, rhs)) {
            return datum{long_longs.lhs() & long_longs.rhs()};

        } else if (hilet bools = promote_if<bool>(lhs, rhs)) {
            return datum{bools.lhs() and bools.rhs()};

        } else {
            throw std::domain_error(std::format("Can not evaluate {} '&' {}", repr(lhs), repr(rhs)));
        }
    }

    /** binary or.
     *
     * Both operands are first promoted to `long long` or `bool` before the
     * operation is executed.
     *
     * @throws std::domain_error When either argument can not be promoted to `long long` or `bool`.
     * @param lhs The left-hand-side operand of the operation.
     * @param lhs The right-hand-side operand of the operation.
     * @return The result of the operation.
     */
    [[nodiscard]] friend constexpr datum operator|(datum const& lhs, datum const& rhs)
    {
        if (hilet long_longs = promote_if<long long>(lhs, rhs)) {
            return datum{long_longs.lhs() | long_longs.rhs()};

        } else if (hilet bools = promote_if<bool>(lhs, rhs)) {
            return datum{bools.lhs() or bools.rhs()};

        } else {
            throw std::domain_error(std::format("Can not evaluate {} '|' {}", repr(lhs), repr(rhs)));
        }
    }

    /** binary xor.
     *
     * Both operands are first promoted to `long long` or `bool` before the
     * operation is executed.
     *
     * @throws std::domain_error When either argument can not be promoted to `long long` or `bool`.
     * @param lhs The left-hand-side operand of the operation.
     * @param lhs The right-hand-side operand of the operation.
     * @return The result of the operation.
     */
    [[nodiscard]] friend constexpr datum operator^(datum const& lhs, datum const& rhs)
    {
        if (hilet long_longs = promote_if<long long>(lhs, rhs)) {
            return datum{long_longs.lhs() ^ long_longs.rhs()};

        } else if (hilet bools = promote_if<bool>(lhs, rhs)) {
            return datum{bools.lhs() != bools.rhs()};

        } else {
            throw std::domain_error(std::format("Can not evaluate {} '^' {}", repr(lhs), repr(rhs)));
        }
    }

    /** Shift left.
     *
     * Both operands are first promoted to `long long` before the
     * operation is executed. Shift left of negative values are
     * done modulo the size of a `long long`
     *
     * @throws std::domain_error On Negative or large shift counts, or when
     *         either argument can not be promoted to `long long`.
     * @param lhs The left-hand-side operand of the operation.
     * @param lhs The right-hand-side operand of the operation.
     * @return The result of the operation.
     */
    [[nodiscard]] friend constexpr datum operator<<(datum const& lhs, datum const& rhs)
    {
        if (hilet long_longs = promote_if<long long>(lhs, rhs)) {
            if (long_longs.rhs() < 0 or long_longs.rhs() > (sizeof(long long) * CHAR_BIT - 1)) {
                throw std::domain_error(std::format("Invalid shift count {} '<<' {}", repr(lhs), repr(rhs)));
            }
            return datum{long_longs.lhs() << long_longs.rhs()};

        } else {
            throw std::domain_error(std::format("Can not evaluate {} '<<' {}", repr(lhs), repr(rhs)));
        }
    }

    /** Shift right arithmetically.
     *
     * Both operands are first promoted to `long long` before the
     * operation is executed. Right shift will do sign extension.
     *
     * @throws std::domain_error On Negative or large shift counts, or when
     *         either argument can not be promoted to `long long`.
     * @param lhs The left-hand-side operand of the operation.
     * @param lhs The right-hand-side operand of the operation.
     * @return The result of the operation.
     */
    [[nodiscard]] friend constexpr datum operator>>(datum const& lhs, datum const& rhs)
    {
        if (hilet long_longs = promote_if<long long>(lhs, rhs)) {
            if (long_longs.rhs() < 0 or long_longs.rhs() > (sizeof(long long) * CHAR_BIT - 1)) {
                throw std::domain_error(std::format("Invalid shift count {} '>>' {}", repr(lhs), repr(rhs)));
            }
            return datum{long_longs.lhs() >> long_longs.rhs()};

        } else {
            throw std::domain_error(std::format("Can not evaluate {} '>>' {}", repr(lhs), repr(rhs)));
        }
    }

    friend std::ostream& operator<<(std::ostream& lhs, datum const& rhs)
    {
        return lhs << to_string(rhs);
    }

#define X(op) \
    [[nodiscard]] friend constexpr auto operator op(datum const& lhs, auto const& rhs) \
    { \
        return lhs op datum{rhs}; \
    } \
    [[nodiscard]] friend constexpr auto operator op(auto const& lhs, datum const& rhs) \
    { \
        return datum{lhs} op rhs; \
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
    X(^) X(<<) X(>>)
#undef X

        /** Get the string representation of the value.
         */
        [[nodiscard]] friend std::string repr(datum const& rhs) noexcept
    {
        switch (rhs._tag) {
        case tag_type::monostate:
            return "undefined";
        case tag_type::floating_point:
            return std::format("{:.1f}", rhs._value._double);
        case tag_type::decimal:
            return to_string(rhs._value._decimal);
        case tag_type::integral:
            return std::format("{}", rhs._value._long_long);
        case tag_type::boolean:
            return rhs._value._bool ? "true" : "false";
        case tag_type::year_month_day:
            return std::format("{:%Y-%m-%d}", rhs._value._year_month_day);
        case tag_type::null:
            return "null";
        case tag_type::flow_break:
            return "break";
        case tag_type::flow_continue:
            return "continue";
        case tag_type::string:
            return std::format("\"{}\"", *rhs._value._string);
        case tag_type::vector:
            {
                auto r = std::string{"["};
                for (hilet& item : *rhs._value._vector) {
                    r += repr(item);
                    r += ',';
                }
                r += ']';
                return r;
            };
        case tag_type::map:
            {
                auto r = std::string{"{"};
                for (hilet& item : *rhs._value._map) {
                    r += repr(item.first);
                    r += ':';
                    r += repr(item.second);
                    r += ',';
                }
                r += '}';
                return r;
            };
        case tag_type::bstring:
            return base64::encode(*rhs._value._bstring);
        default:
            hi_no_default();
        }
    }

    /** Get the string representation of the value.
     */
    [[nodiscard]] friend std::string to_string(datum const& rhs) noexcept
    {
        return static_cast<std::string>(rhs);
    }

    /** Check if the stored value is of a specific type.
     *
     * @tparam T Type to check.
     * @param rhs The datum to check the value-type of.
     * @return True if the value-type matches the template parameter @a T
     */
    template<typename T>
    [[nodiscard]] friend constexpr bool holds_alternative(datum const& rhs) noexcept
    {
        if constexpr (std::is_same_v<T, double>) {
            return rhs._tag == tag_type::floating_point;
        } else if constexpr (std::is_same_v<T, decimal>) {
            return rhs._tag == tag_type::decimal;
        } else if constexpr (std::is_same_v<T, long long>) {
            return rhs._tag == tag_type::integral;
        } else if constexpr (std::is_same_v<T, bool>) {
            return rhs._tag == tag_type::boolean;
        } else if constexpr (std::is_same_v<T, std::chrono::year_month_day>) {
            return rhs._tag == tag_type::year_month_day;
        } else if constexpr (std::is_same_v<T, nullptr_t>) {
            return rhs._tag == tag_type::null;
        } else if constexpr (std::is_same_v<T, std::monostate>) {
            return rhs._tag == tag_type::monostate;
        } else if constexpr (std::is_same_v<T, break_type>) {
            return rhs._tag == tag_type::flow_break;
        } else if constexpr (std::is_same_v<T, continue_type>) {
            return rhs._tag == tag_type::flow_continue;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return rhs._tag == tag_type::string;
        } else if constexpr (std::is_same_v<T, vector_type>) {
            return rhs._tag == tag_type::vector;
        } else if constexpr (std::is_same_v<T, map_type>) {
            return rhs._tag == tag_type::map;
        } else if constexpr (std::is_same_v<T, bstring>) {
            return rhs._tag == tag_type::bstring;
        } else {
            hi_static_no_default();
        }
    }

    /** Check if the type held by the datum can be promoted.
     *
     * A value can always be promoted to its own type.
     * The following promotions also exist:
     *  - `decimal` -> `double'
     *  - `long long` -> `decimal'
     *  - `bool` -> `long long'
     *
     * @tparam To Type to promote the value to.
     * @param rhs The value to promote.
     */
    template<typename To>
    [[nodiscard]] friend constexpr bool promotable_to(datum const& rhs) noexcept
    {
        if constexpr (std::is_same_v<To, double>) {
            return holds_alternative<double>(rhs) or holds_alternative<decimal>(rhs) or holds_alternative<long long>(rhs) or
                holds_alternative<bool>(rhs);
        } else if constexpr (std::is_same_v<To, decimal>) {
            return holds_alternative<decimal>(rhs) or holds_alternative<long long>(rhs) or holds_alternative<bool>(rhs);
        } else if constexpr (std::is_same_v<To, long long>) {
            return holds_alternative<long long>(rhs) or holds_alternative<bool>(rhs);
        } else {
            return holds_alternative<To>(rhs);
        }
    }

    /** Get the value of a datum.
     *
     * It is monostate behavior if the type does not match the stored value.
     *
     * @tparam T Type to check, must be one of: `bool`, `double`, `long long`, `decimal`, `std::chrono::year_month_day`
     * @param rhs The datum to get the value from.
     * @return A copy of the value in the datum.
     */
    template<typename T>
    [[nodiscard]] friend constexpr T const& get(datum const& rhs) noexcept
    {
        hi_axiom(holds_alternative<T>(rhs));
        if constexpr (std::is_same_v<T, double>) {
            return rhs._value._double;
        } else if constexpr (std::is_same_v<T, decimal>) {
            return rhs._value._decimal;
        } else if constexpr (std::is_same_v<T, long long>) {
            return rhs._value._long_long;
        } else if constexpr (std::is_same_v<T, bool>) {
            return rhs._value._bool;
        } else if constexpr (std::is_same_v<T, std::chrono::year_month_day>) {
            return rhs._value._year_month_day;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return *rhs._value._string;
        } else if constexpr (std::is_same_v<T, vector_type>) {
            return *rhs._value._vector;
        } else if constexpr (std::is_same_v<T, map_type>) {
            return *rhs._value._map;
        } else if constexpr (std::is_same_v<T, bstring>) {
            return *rhs._value._bstring;
        } else {
            hi_static_no_default();
        }
    }

    /** Get the value of a datum.
     *
     * It is undefined behavior if the type does not match the stored value.
     *
     * @tparam T Type to check, must be one of: `bool`, `double`, `long long`, `decimal`, `std::chrono::year_month_day`
     * @param rhs The datum to get the value from.
     * @return A copy of the value in the datum.
     */
    template<typename T>
    [[nodiscard]] friend constexpr T& get(datum& rhs) noexcept
    {
        hi_axiom(holds_alternative<T>(rhs));
        if constexpr (std::is_same_v<T, double>) {
            return rhs._value._double;
        } else if constexpr (std::is_same_v<T, decimal>) {
            return rhs._value._decimal;
        } else if constexpr (std::is_same_v<T, long long>) {
            return rhs._value._long_long;
        } else if constexpr (std::is_same_v<T, bool>) {
            return rhs._value._bool;
        } else if constexpr (std::is_same_v<T, std::chrono::year_month_day>) {
            return rhs._value._year_month_day;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return *rhs._value._string;
        } else if constexpr (std::is_same_v<T, vector_type>) {
            return *rhs._value._vector;
        } else if constexpr (std::is_same_v<T, map_type>) {
            return *rhs._value._map;
        } else if constexpr (std::is_same_v<T, bstring>) {
            return *rhs._value._bstring;
        } else {
            hi_static_no_default();
        }
    }

    /** Get the value of a datum.
     *
     * If the type does not matched the stored value this function will return a nullptr.
     *
     * @tparam T Type to check.
     * @param rhs The datum to get the value from.
     * @return A pointer to the value, or nullptr.
     */
    template<typename T>
    [[nodiscard]] friend constexpr T *get_if(datum& rhs) noexcept
    {
        if (holds_alternative<T>(rhs)) {
            return &get<T>(rhs);
        } else {
            return nullptr;
        }
    }

    /** Get the value of a datum.
     *
     * If the type does not matched the stored value this function will return a nullptr.
     *
     * @tparam T Type to check.
     * @param rhs The datum to get the value from.
     * @return A pointer to the value, or nullptr.
     */
    template<typename T>
    [[nodiscard]] friend constexpr T const *get_if(datum const& rhs) noexcept
    {
        if (holds_alternative<T>(rhs)) {
            return &get<T>(rhs);
        } else {
            return nullptr;
        }
    }

    /** Get the value of a datum.
     *
     * If the type does not matched the stored value this function will return a nullptr.
     *
     * @tparam T Type to check.
     * @param rhs The datum to get the value from.
     * @param path The json-path to the value to extract.
     * @return A pointer to the value, or nullptr.
     */
    template<typename T>
    [[nodiscard]] friend T *get_if(datum& rhs, jsonpath const& path) noexcept
    {
        if (auto *value = rhs.find_one(path)) {
            if (holds_alternative<T>(*value)) {
                return &get<T>(*value);
            } else {
                return nullptr;
            }
        } else {
            return nullptr;
        }
    }

    /** Get the value of a datum.
     *
     * If the type does not matched the stored value this function will return a nullptr.
     *
     * @tparam T Type to check.
     * @param rhs The datum to get the value from.
     * @param path The json-path to the value to extract.
     * @return A pointer to the value, or nullptr.
     */
    template<typename T>
    [[nodiscard]] friend T const *get_if(datum const& rhs, jsonpath const& path) noexcept
    {
        if (auto *value = const_cast<datum&>(rhs).find_one(path)) {
            if (holds_alternative<T>(*value)) {
                return &get<T>(*value);
            } else {
                return nullptr;
            }
        } else {
            return nullptr;
        }
    }

private:
    enum class tag_type : signed char {
        // scalars are detected by: `std::to_underlying(tag_type) >= 0`
        monostate = 0,
        floating_point = 1,
        integral = 2,
        decimal = 3,
        boolean = 4,
        null = 5,
        year_month_day = 6,
        flow_continue = 7,
        flow_break = 8,

        // pointers are detected by: `std::to_underlying(tag_type) < 0`.
        string = -1,
        vector = -2,
        map = -3,
        bstring = -5
    };

    tag_type _tag = tag_type::monostate;
    union value_type {
        double _double;
        long long _long_long;
        decimal _decimal;
        bool _bool;
        std::chrono::year_month_day _year_month_day;
        std::string *_string;
        vector_type *_vector;
        map_type *_map;
        bstring *_bstring;

        constexpr value_type(numeric_integral auto value) noexcept : _long_long(narrow_cast<long long>(value)) {}
        constexpr value_type(std::floating_point auto value) noexcept : _double(narrow_cast<double>(value)) {}
        constexpr value_type(decimal value) noexcept : _decimal(value) {}
        constexpr value_type(bool value) noexcept : _bool(value) {}
        constexpr value_type(std::chrono::year_month_day value) noexcept : _year_month_day(value) {}
        constexpr value_type(std::string *value) noexcept : _string(value) {}
        constexpr value_type(vector_type *value) noexcept : _vector(value) {}
        constexpr value_type(map_type *value) noexcept : _map(value) {}
        constexpr value_type(bstring *value) noexcept : _bstring(value) {}
    };

    value_type _value;

    [[nodiscard]] constexpr bool is_scalar() const noexcept
    {
        return std::to_underlying(_tag) >= 0;
    }

    [[nodiscard]] constexpr bool is_pointer() const noexcept
    {
        return std::to_underlying(_tag) < 0;
    }

    hi_no_inline void copy_pointer(datum const& other) noexcept
    {
        hi_axiom(other.is_pointer());
        switch (other._tag) {
        case tag_type::string:
            _value._string = new std::string{*other._value._string};
            return;
        case tag_type::vector:
            _value._vector = new vector_type{*other._value._vector};
            return;
        case tag_type::map:
            _value._map = new map_type{*other._value._map};
            return;
        case tag_type::bstring:
            _value._bstring = new bstring{*other._value._bstring};
            return;
        default:
            hi_no_default();
        }
    }

    hi_no_inline void _delete_pointer() noexcept
    {
        hi_axiom(is_pointer());
        switch (_tag) {
        case tag_type::string:
            delete _value._string;
            return;
        case tag_type::vector:
            delete _value._vector;
            return;
        case tag_type::map:
            delete _value._map;
            return;
        case tag_type::bstring:
            delete _value._bstring;
            return;
        default:
            hi_no_default();
        }
    }

    constexpr void delete_pointer() noexcept
    {
        if (is_pointer()) {
            _delete_pointer();
        }
    }

    void find_wildcard(jsonpath::const_iterator it, jsonpath::const_iterator it_end, std::vector<datum *>& r) noexcept
    {
        if (auto vector = get_if<datum::vector_type>(*this)) {
            for (auto& item : *vector) {
                item.find(it + 1, it_end, r);
            }

        } else if (auto map = get_if<datum::map_type>(*this)) {
            for (auto& item : *map) {
                item.second.find(it + 1, it_end, r);
            }
        }
    }

    void find_descend(jsonpath::const_iterator it, jsonpath::const_iterator it_end, std::vector<datum *>& r) noexcept
    {
        this->find(it + 1, it_end, r);

        if (auto vector = get_if<datum::vector_type>(*this)) {
            for (auto& item : *vector) {
                item.find(it, it_end, r);
            }

        } else if (auto map = get_if<datum::map_type>(*this)) {
            for (auto& item : *map) {
                item.second.find(it, it_end, r);
            }
        }
    }

    void find_indices(
        jsonpath::indices const& indices,
        jsonpath::const_iterator it,
        jsonpath::const_iterator it_end,
        std::vector<datum *>& r) noexcept
    {
        if (auto vector = get_if<datum::vector_type>(*this)) {
            for (hilet index : indices.filter(ssize(*vector))) {
                (*vector)[index].find(it + 1, it_end, r);
            }
        }
    }

    void find_names(
        jsonpath::names const& names,
        jsonpath::const_iterator it,
        jsonpath::const_iterator it_end,
        std::vector<datum *>& r) noexcept
    {
        if (auto map = get_if<datum::map_type>(*this)) {
            for (hilet& name : names) {
                hilet name_ = datum{name};
                auto jt = map->find(name_);
                if (jt != map->cend()) {
                    jt->second.find(it + 1, it_end, r);
                }
            }
        }
    }

    void find_slice(
        jsonpath::slice const& slice,
        jsonpath::const_iterator it,
        jsonpath::const_iterator it_end,
        std::vector<datum *>& r) noexcept
    {
        if (auto vector = get_if<datum::vector_type>(*this)) {
            hilet first = slice.begin(vector->size());
            hilet last = slice.end(vector->size());

            for (auto index = first; index != last; index += slice.step) {
                if (index >= 0 and index < vector->size()) {
                    (*this)[index].find(it + 1, it_end, r);
                }
            }
        }
    }

    void find(jsonpath::const_iterator it, jsonpath::const_iterator it_end, std::vector<datum *>& r) noexcept
    {
        if (it == it_end) {
            r.push_back(this);

        } else if (std::holds_alternative<jsonpath::root>(*it)) {
            find(it + 1, it_end, r);

        } else if (std::holds_alternative<jsonpath::current>(*it)) {
            find(it + 1, it_end, r);

        } else if (std::holds_alternative<jsonpath::wildcard>(*it)) {
            find_wildcard(it, it_end, r);

        } else if (std::holds_alternative<jsonpath::descend>(*it)) {
            find_descend(it, it_end, r);

        } else if (auto indices = std::get_if<jsonpath::indices>(&*it)) {
            find_indices(*indices, it, it_end, r);

        } else if (auto names = std::get_if<jsonpath::names>(&*it)) {
            find_names(*names, it, it_end, r);

        } else if (auto slice = std::get_if<jsonpath::slice>(&*it)) {
            find_slice(*slice, it, it_end, r);

        } else {
            hi_no_default();
        }
    }

    [[nodiscard]] int remove_wildcard(jsonpath::const_iterator it, jsonpath::const_iterator it_end) noexcept
    {
        int r = 0;

        if (auto vector = get_if<datum::vector_type>(*this)) {
            auto jt = vector->begin();
            while (jt != vector->end()) {
                hilet match = jt->remove(it + 1, it_end);
                r |= match ? 1 : 0;

                if (match == 2) {
                    jt = vector->erase(jt);
                } else {
                    ++jt;
                }
            }
            return vector->empty() ? 2 : r;

        } else if (auto map = get_if<datum::map_type>(*this)) {
            auto jt = map->begin();
            while (jt != map->end()) {
                hilet match = jt->second.remove(it + 1, it_end);
                r |= match ? 1 : 0;

                if (match == 2) {
                    jt = map->erase(jt);
                } else {
                    ++jt;
                }
            }
            return map->empty() ? 2 : r;

        } else {
            return 0;
        }
    }

    [[nodiscard]] int remove_descend(jsonpath::const_iterator it, jsonpath::const_iterator it_end) noexcept
    {
        int r = 0;

        {
            hilet match = this->remove(it + 1, it_end);
            if (match == 2) {
                return 2;
            }
            r |= match ? 1 : 0;
        }

        if (auto vector = get_if<datum::vector_type>(*this)) {
            auto jt = vector->begin();
            while (jt != vector->end()) {
                hilet match = jt->remove(it, it_end);
                r |= match ? 1 : 0;

                if (match == 2) {
                    jt = vector->erase(jt);
                } else {
                    ++jt;
                }
            }
            return vector->empty() ? 2 : r;

        } else if (auto map = get_if<datum::map_type>(*this)) {
            auto jt = map->begin();
            while (jt != map->end()) {
                hilet match = jt->second.remove(it, it_end);
                r |= match ? 1 : 0;

                if (match == 2) {
                    jt = map->erase(jt);
                } else {
                    ++jt;
                }
            }
            return map->empty() ? 2 : r;

        } else {
            return 0;
        }
    }

    [[nodiscard]] int
    remove_indices(jsonpath::indices const& indices, jsonpath::const_iterator it, jsonpath::const_iterator it_end) noexcept
    {
        if (auto vector = get_if<datum::vector_type>(*this)) {
            int r = 0;
            std::size_t offset = 0;

            for (hilet index : indices.filter(ssize(*vector))) {
                hilet match = (*vector)[index - offset].remove(it + 1, it_end);
                r |= match ? 1 : 0;
                if (match == 2) {
                    vector->erase(vector->begin() + (index - offset));
                    ++offset;
                }
            }

            return vector->empty() ? 2 : r;

        } else {
            return 0;
        }
    }

    [[nodiscard]] int
    remove_names(jsonpath::names const& names, jsonpath::const_iterator it, jsonpath::const_iterator it_end) noexcept
    {
        if (auto map = get_if<datum::map_type>(*this)) {
            int r = 0;

            for (hilet& name : names) {
                hilet name_ = datum{name};
                auto jt = map->find(name_);
                if (jt != map->cend()) {
                    hilet match = jt->second.remove(it + 1, it_end);
                    r |= match ? 1 : 0;
                    if (match == 2) {
                        map->erase(jt);
                    }
                }
            }

            return map->empty() ? 2 : r;

        } else {
            return 0;
        }
    }

    [[nodiscard]] int
    remove_slice(jsonpath::slice const& slice, jsonpath::const_iterator it, jsonpath::const_iterator it_end) noexcept
    {
        if (auto vector = get_if<datum::vector_type>(*this)) {
            int r = 0;

            hilet first = slice.begin(vector->size());
            hilet last = slice.end(vector->size());

            std::size_t offset = 0;
            for (auto index = first; index != last; index += slice.step) {
                if (index >= 0 and index < vector->size()) {
                    hilet match = (*this)[index - offset].remove(it + 1, it_end);
                    r |= match ? 1 : 0;

                    if (match == 2) {
                        vector->erase(vector->begin() + (index - offset));
                        ++offset;
                    }
                }
            }

            return vector->empty() ? 2 : r;

        } else {
            return 0;
        }
    }

    [[nodiscard]] int remove(jsonpath::const_iterator it, jsonpath::const_iterator it_end) noexcept
    {
        if (it == it_end) {
            // Reached end, remove matching name or index in parent.
            return 2;

        } else if (std::holds_alternative<jsonpath::root>(*it)) {
            return remove(it + 1, it_end);

        } else if (std::holds_alternative<jsonpath::current>(*it)) {
            return remove(it + 1, it_end);

        } else if (std::holds_alternative<jsonpath::wildcard>(*it)) {
            return remove_wildcard(it, it_end);

        } else if (std::holds_alternative<jsonpath::descend>(*it)) {
            return remove_descend(it, it_end);

        } else if (auto indices = std::get_if<jsonpath::indices>(&*it)) {
            return remove_indices(*indices, it, it_end);

        } else if (auto names = std::get_if<jsonpath::names>(&*it)) {
            return remove_names(*names, it, it_end);

        } else if (auto slice = std::get_if<jsonpath::slice>(&*it)) {
            return remove_slice(*slice, it, it_end);

        } else {
            hi_no_default();
        }
    }

    [[nodiscard]] datum *
    find_one_name(datum const& name, jsonpath::const_iterator it, jsonpath::const_iterator it_end, bool create) noexcept
    {
        hi_axiom(holds_alternative<std::string>(name));

        if (auto *map = get_if<map_type>(*this)) {
            auto i = map->find(name);
            if (i != map->end()) {
                return i->second.find_one(it + 1, it_end, create);

            } else if (create) {
                (*map)[name] = datum{std::monostate{}};
                return find_one_name(name, it, it_end, create);

            } else {
                return nullptr;
            }

        } else if (holds_alternative<std::monostate>(*this) and create) {
            *this = datum::make_map(name, std::monostate{});
            return find_one_name(name, it, it_end, create);

        } else {
            return nullptr;
        }
    }

    [[nodiscard]] datum *
    find_one_index(std::size_t index, jsonpath::const_iterator it, jsonpath::const_iterator it_end, bool create) noexcept
    {
        if (auto *vector = get_if<vector_type>(*this)) {
            if (index < vector->size()) {
                return (*vector)[index].find_one(it + 1, it_end, create);
            } else if (index == vector->size() and create) {
                vector->push_back(datum{std::monostate{}});
                return find_one_index(index, it, it_end, create);
            } else {
                return nullptr;
            }

        } else if (holds_alternative<std::monostate>(*this) and index == 0 and create) {
            *this = datum::make_vector(std::monostate{});
            return find_one_index(index, it, it_end, create);

        } else {
            return nullptr;
        }
    }

    [[nodiscard]] datum *find_one(jsonpath::const_iterator it, jsonpath::const_iterator it_end, bool create) noexcept
    {
        if (it == it_end) {
            return this;

        } else if (std::holds_alternative<jsonpath::root>(*it)) {
            return find_one(it + 1, it_end, create);

        } else if (std::holds_alternative<jsonpath::current>(*it)) {
            return find_one(it + 1, it_end, create);

        } else if (hilet *indices = std::get_if<jsonpath::indices>(&*it)) {
            hi_axiom(indices->size() == 1);
            return find_one_index(indices->front(), it, it_end, create);

        } else if (hilet *names = std::get_if<jsonpath::names>(&*it)) {
            hi_axiom(names->size() == 1);
            return find_one_name(datum{names->front()}, it, it_end, create);

        } else {
            hi_no_default();
        }
    }
};

}} // namespace hi::v1

export template<>
struct std::hash<hi::datum> {
    [[nodiscard]] std::size_t operator()(hi::datum const& rhs) const noexcept
    {
        return rhs.hash();
    }
};

// XXX #617 MSVC bug does not handle partial specialization in modules.
export template<>
struct std::formatter<hi::datum, char> : std::formatter<std::string, char> {
    auto format(hi::datum const& t, auto& fc) const
    {
        return std::formatter<std::string, char>{}.format(to_string(t), fc);
    }
};

hi_warning_pop();
