// Copyright Take Vos 2020-2021.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <type_traits>
#include <memory>
#include <initializer_list>
#include <stdexcept>
#include <array>

hi_export_module(hikogui.container.stack);

hi_export namespace hi::inline v1 {

/** A static sized stack.
 *
 * This `stack` class is meant to be used as a relative small object
 * that lives in an automatic variable. The objects in this stack should be
 * mostly trivial, as they will not be destroyed when the stack is popped or
 * cleared.
 * 
 * Because the stack can not grow or shrink, the iterators remain valid over the
 * lifetime of the stack.
 */
template<typename T, std::size_t MaxSize>
class stack {
public:
    using value_type = T;
    using array_type = std::array<value_type, MaxSize>;
    using pointer = array_type::pointer;
    using const_pointer = array_type::const_pointer;
    using reference = array_type::reference;
    using const_reference = array_type::const_reference;
    using iterator = array_type::iterator;
    using const_iterator = array_type::const_iterator;
    using size_type = array_type::size_type;
    using difference_type = array_type::difference_type;

    constexpr ~stack() noexcept = default;
    stack(stack const&) = delete;
    stack(stack&&) = delete;
    stack& operator=(stack const&) = delete;
    stack& operator=(stack&&) = delete;

    /** Construct an empty stack.
     */
    constexpr stack() noexcept
    {
        _top = _data.begin();
    }

    [[nodiscard]] constexpr const_pointer data() const noexcept
    {
        return _data.data();
    }

    [[nodiscard]] constexpr pointer data() noexcept
    {
        return _data.data();
    }

    /** Get an iterator to the first element on the stack.
     * @return An iterator to the first element on the stack.
     */
    [[nodiscard]] constexpr iterator begin() noexcept
    {
        return _data.begin();
    }

    /** Get an iterator to the first element on the stack.
     * @return An iterator to the first element on the stack.
     */
    [[nodiscard]] constexpr const_iterator begin() const noexcept
    {
        return _data.begin();
    }

    /** Get an iterator to the first element on the stack.
     * @return An iterator to the first element on the stack.
     */
    [[nodiscard]] constexpr const_iterator cbegin() const noexcept
    {
        return _data.cbegin();
    }

    /** Get an iterator to the last element on the stack.
     * @return An iterator one beyond the last element on the stack.
     */
    [[nodiscard]] constexpr iterator end() noexcept
    {
        return _top;
    }

    /** Get an iterator to the last element on the stack.
     * @return An iterator one beyond the last element on the stack.
     */
    [[nodiscard]] constexpr const_iterator end() const noexcept
    {
        return _top;
    }

    /** Get an iterator to the last element on the stack.
     * @return An iterator one beyond the last element on the stack.
     */
    [[nodiscard]] constexpr const_iterator cend() const noexcept
    {
        return _top;
    }

    /** The maximum number of elements that fit on the stack.
     * @return the maximum number of elements that fit on the stack.
     */
    [[nodiscard]] constexpr size_type max_size() const noexcept
    {
        return MaxSize;
    }

    /** The number of elements on the stack.
     * @return the number of elements on the stack.
     */
    [[nodiscard]] constexpr size_type size() const noexcept
    {
        return narrow_cast<size_type>(std::distance(begin(), end()));
    }

    /** Check if the stack is full.
     * @return True if the stack is full, empty if otherwise.
     */
    [[nodiscard]] constexpr bool full() const noexcept
    {
        return _top == _data.end();
    }

    /** Check if the stack is empty.
     * @return True if the stack is empty, empty if otherwise.
     */
    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return _top == _data.begin();
    }

    /** Get a reference to an element on the stack at an index.
     * No bounds checking is performed.
     * @return True if the stack is empty, empty if otherwise.
     */
    [[nodiscard]] constexpr reference operator[](std::size_t index) noexcept
    {
        hi_axiom_bounds(index, *this);
        return _data[index];
    }

    /** Get a reference to an element on the stack at an index.
     * No bounds checking is performed.
     * @return True if the stack is empty, empty if otherwise.
     */
    [[nodiscard]] constexpr const_reference operator[](std::size_t index) const noexcept
    {
        hi_axiom_bounds(index, *this);
        return _data[index];
    }

    /** Get a reference to an element on the stack at an index.
     * @throws std::out_of_range when the index points beyond the top of the stack.
     * @return True if the stack is empty, empty if otherwise.
     */
    [[nodiscard]] constexpr reference at(std::size_t index) noexcept
    {
        if (index >= size()) {
            throw std::out_of_range("stack::at");
        }
        return _data[index];
    }

    /** Get a reference to an element on the stack at an index.
     * @throws std::out_of_range when the index points beyond the top of the stack.
     * @return True if the stack is empty, empty if otherwise.
     */
    [[nodiscard]] constexpr const_reference at(std::size_t index) const noexcept
    {
        if (index >= size()) {
            throw std::out_of_range("stack::at");
        }
        return _data[index];
    }

    /** Get a reference to the element at the top of the stack.
     * Calling `back()` on an empty container causes undefined behavior.
     */
    [[nodiscard]] constexpr reference back() noexcept
    {
        hi_axiom(not empty());
        return *(_top - 1);
    }

    /** Get a reference to the element at the top of the stack.
     * Calling `back()` on an empty container causes undefined behavior.
     */
    [[nodiscard]] constexpr const_reference back() const noexcept
    {
        hi_axiom(not empty());
        return *(_top - 1);
    }

    /** Construct an object after the current top of the stack.
     * @tparam Args The types of each argument
     * @param args The arguments for the constructor of `value_type`.
     */
    template<typename... Args>
    constexpr void emplace_back(Args&&...args) noexcept
    {
        hi_axiom(not full());
        *(_top++) = value_type(std::forward<Args>(args)...);
    }

    /** Push a new value to after the current top of the stack.
     * @tparam Arg The type of an object that can be converted to `value_type`
     * @param arg The object to be pushed on the stack.
     */
    template<typename Arg>
    constexpr void push_back(Arg&& arg) noexcept
        requires(std::is_convertible_v<Arg, value_type>)
    {
        hi_axiom(not full());
        *(_top++) = std::forward<Arg>(arg);
    }

    /** Remove the value at the top of the stack.
     * It is undefined behaviour to call this function on an empty stack.
     */
    constexpr void pop_back() noexcept
    {
        hi_axiom(not empty());
        --_top;
    }

    /** Pop elements of the stack through the given iterator.
     * Pop elements up to and including the element at new_end.
     * @param new_end Iterator to the object to be removed.
     */
    constexpr void pop_back(const_iterator new_end) noexcept
    {
        while (_top != new_end) {
            pop_back();
        }
    }

    /** Remove all elements from the stack.
     */
    constexpr void clear() noexcept
    {
        _top = _data.begin();
    }

private:
    array_type _data;
    array_type::iterator _top;
};

} // namespace hi::inline v1
