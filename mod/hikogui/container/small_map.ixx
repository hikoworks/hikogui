// Copyright Take Vos 2019-2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include "../macros.hpp"

#include <array>
#include <utility>
#include <optional>
#include <type_traits>

export module hikogui_container_small_map;
import hikogui_utility;


export namespace hi::inline v1 {

template<typename K, typename V, int N>
class small_map {
public:
    using key_type = K;
    using value_type = V;
    struct item_type {
        K key;
        V value;
    };
    constexpr static int capacity = N;
    using array_type = std::array<item_type, capacity>;

private:
    typename array_type::iterator _end;
    array_type items = {};

public:
    small_map()
    {
        _end = items.begin();
    }

    small_map(small_map const &other)
    {
        hi_axiom(this != &other);
        _end = items.begin();
        for (hilet &other_item : other) {
            auto &this_item = *(_end++);
            this_item = other_item;
        }
    }

    small_map(small_map &&other)
    {
        hi_axiom(this != &other);
        using std::swap;

        _end = items.begin();
        for (auto &other_item : other) {
            auto &this_item = *(_end++);
            swap(this_item, other_item);
        }
        // All items in other are valid, no reason to set other._end.
    }

    small_map &operator=(small_map const &other)
    {
        hi_return_on_self_assignment(other);
        _end = items.begin();
        for (hilet &other_item : other) {
            auto &this_item = *(_end++);
            this_item = other_item;
        }
        return *this;
    }

    small_map &operator=(small_map &&other)
    {
        hi_return_on_self_assignment(other);
        _end = items.begin();
        for (hilet &other_item : other) {
            auto &this_item = *(_end++);
            this_item = std::move(other_item);
        }
        other._end = other.begin();
        return *this;
    }

    std::size_t size() const
    {
        return _end - items.begin();
    }

    decltype(auto) begin() const
    {
        return items.begin();
    }
    decltype(auto) begin()
    {
        return items.begin();
    }
    decltype(auto) end() const
    {
        return _end;
    }
    decltype(auto) end()
    {
        return _end;
    }

    std::optional<V> get(K const &key) const noexcept
    {
        for (auto i = begin(); i != end(); ++i) {
            if (i->key == key) {
                return i->value;
            }
        }
        return {};
    }

    V get(K const &key, V const &default_value) const noexcept
    {
        if (hilet &value = get(key)) {
            return *value;
        } else {
            return default_value;
        }
    }

    template<typename KK, typename VV>
    bool set(KK &&key, VV &&value) noexcept
    {
        auto i = begin();
        for (; i != end(); ++i) {
            if (i->key == key) {
                i->value = std::forward<VV>(value);
                return true;
            }
        }
        if (i != items.end()) {
            _end = i + 1;
            i->key = std::forward<KK>(key);
            i->value = std::forward<VV>(value);
            return true;
        }

        return false;
    }

    V increment(K const &key) noexcept
    {
        static_assert(std::is_arithmetic_v<V>, "Only increment on a arithmetic value");
        auto i = begin();
        for (; i != end(); ++i) {
            if (i->key == key) {
                return ++(i->value);
            }
        }
        if (i != items.end()) {
            _end = i + 1;
            i->key = key;
            return i->value = V{1};
        }

        return 0;
    }
};

} // namespace hi::inline v1
