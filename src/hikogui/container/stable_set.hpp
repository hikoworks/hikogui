// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <mutex>
#include <memory>
#include <atomic>
#include <map>
#include <unordered_map>
#include <functional>

hi_export_module(hikogui.container.stable_set);

hi_export namespace hi::inline v1 {

/** This is a set of object with stable indices.
 *
 * This container holds a set of unique objects, associated with a stable index.
 *
 * Currently the main use case is for `grapheme`. Where it stores multi code-point
 * graphemes into the stable_set, while holding the index in the `grapheme` object.
 *
 * Another use case is for `text_style` objects which only hold an index while
 * the `actual_text_style`  objects are stored in the stable_set.
 */
template<typename Key>
class stable_set {
public:
    using value_type = Key;
    using size_type = size_t;
    using map_type = std::conditional_t<
        std::is_default_constructible<std::hash<Key>>::value,
        std::unordered_map<Key, size_type>,
        std::map<Key, size_type>>;
    using key_type = Key;
    using difference_type = ptrdiff_t;
    using reference = value_type const&;
    using const_reference = value_type const&;
    using pointer = value_type const *;
    using const_pointer = value_type const *;

    ~stable_set() = default;
    constexpr stable_set() noexcept = default;
    stable_set(stable_set const&) = delete;
    stable_set(stable_set&&) = delete;
    stable_set& operator=(stable_set const&) = delete;
    stable_set& operator=(stable_set&&) = delete;

    [[nodiscard]] size_t size() const noexcept
    {
        auto const lock = std::scoped_lock(_mutex);
        return _vector.size();
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return size() == 0;
    }

    operator bool() const noexcept
    {
        return not empty();
    }

    /** Get a const reference to an object located at an index in the set.
     *
     * @note It is undefined behavior if the index is not in the set.
     * @param index The index in the set of an existing object.
     * @return A const reference to an existing object in the set.
     */
    [[nodiscard]] const_reference operator[](size_t index) const noexcept
    {
        auto const lock = std::scoped_lock(_mutex);
        hi_assert_bounds(index, _vector);
        return *_vector[index];
    }

    /** Insert an object into the stable-set.
     *
     * Forward the given object into the set and return the index where it was inserted.
     * If the object is already in the set then the index is returned where it was located, and
     * the temporary created object is destroyed.
     *
     * @param arg The object to add.
     * @return The index where the object was added, or where the object already was in the set.
     */
    template<typename Arg>
    [[nodiscard]] size_t insert(Arg&& arg) noexcept requires(std::is_same_v<std::decay_t<Arg>, value_type>)
    {
        auto const lock = std::scoped_lock(_mutex);

        auto const[it, is_inserted] = _map.emplace(std::forward<Arg>(arg), _vector.size());
        if (is_inserted) {
            _vector.push_back(std::addressof(it->first));
        }
        return it->second;
    }

    /** Emplace an object into the stable-set.
     *
     * Create a new object into the set and return the index where it was inserted.
     * If an equivalent object is already in the set then the index is returned where it was located, and
     * the temporary created object is destroyed.
     *
     * @param args The arguments to pass to the constructor of the value_type.
     * @return The index where the object was added, or where the object already was in the set.
     */
    template<typename... Args>
    [[nodiscard]] size_t emplace(Args&&...args) noexcept
    {
        auto const lock = std::scoped_lock(_mutex);

        auto const[it, is_inserted] = _map.emplace(value_type{std::forward<Args>(args)...}, _vector.size());
        if (is_inserted) {
            _vector.push_back(std::addressof(it->first));
        }
        return it->second;
    }

private:
    // clang-format sucks.
    using vector_type = std::vector<const_pointer>;

    vector_type _vector;
    map_type _map;
    mutable std::mutex _mutex;
};
} // namespace hi::inline v1
