// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

module;
#include "../macros.hpp"


export module hikogui_container_secure_vector;
import hikogui_security;

// namespace hi::inline v1 {
//
// template<typename Allocator>
// class secure_vector_base {
// public:
//     using pointer = std::allocator_traits<Allocator>::pointer;
//
// protected:
//     [[nodiscard]] constexpr pointer allocate(size_t n)
//     {
//         return std::allocator_traits<Allocator>::allocate(Allocator{}, n);
//     }
//
//     constexpr void deallocate(pointer p, size_t n)
//     {
//         return std::allocator_traits<Allocator>::deallocate(Allocator{}, p, n);
//     }
// };
//
// template<typename Allocator> requires(not std::allocator_traits<Allocator>::is_always_equal::value)
// class secure_vector_base<Allocator> {
// public:
//     using pointer = std::allocator_traits<Allocator>::pointer;
//
// protected:
//     Allocator _allocator;
//
//     [[nodiscard]] constexpr pointer allocate(size_t n)
//     {
//         return std::allocator_traits<Allocator>::allocate(_allocator, n);
//     }
//
//     constexpr void deallocate(pointer p, size_t n)
//     {
//         return std::allocator_traits<Allocator>::deallocate(_allocator, p, n);
//     }
// };
//
// /** Secure vector.
//  *
//  * The data being held by the vector will be securly cleared from memory
//  * when the vector is destructed. Useful for temporarily storing passwords and other secrets.
//  */
// template<typename T, typename Allocator = std::allocator<T>>
// class secure_vector : public secure_vector_base<Allocator> {
// public:
//     using value_type = T;
//     using allocator_type = Allocator;
//     using size_type = std::size_t;
//     using difference_type = std::ptrdiff_t;
//     using reference = value_type &;
//     using const_reference = value_type const &;
//     using pointer = std::allocator_traits<allocator_type>::pointer;
//     using const_pointer = std::allocator_traits<allocator_type>::const_pointer;
//     using iterator = value_type *;
//     using const_iterator = value_type const *;
//     using reverse_iterator = std::reverse_iterator<iterator>;
//     using const_reverse_iterator = std::reverse_iterator<const_iterator>;
//
//     constexpr static bool is_static_allocator = // std::allocator_traits<allocator_type>::is_always_equal::value;
//
//     constexpr secure_vector() noexcept : _begin(nullptr), _end(nullptr), _bound(nullptr) {}
//
//     constexpr ~secure_vector()
//     {
//         resize(0);
//         shrink_to_fit();
//         hi_assert(_begin == nullptr);
//         hi_assert(_end == nullptr);
//         hi_assert(_bound == nullptr);
//     }
//
//     [[nodiscard]] constexpr bool empty() const noexcept
//     {
//         return _begin == _end;
//     }
//
//     [[nodiscard]] constexpr size_type size() const noexcept
//     {
//         return static_cast<size_type>(_end - _begin);
//     }
//
//     [[nodiscard]] constexpr size_type max_size() const noexcept
//     {
//         return std::allocator_traits<allocator_type>::max_size();
//     }
//
//     [[nodiscard]] constexpr size_type capacity() const noexcept
//     {
//         return static_cast<size_type>(_bound - _begin);
//     }
//
//     [[nodiscard]] constexpr reference at(size_type pos)
//     {
//         auto *ptr = _begin + pos;
//         if (ptr >= _end) {
//             throw std::out_of_range();
//         }
//
//         return *ptr;
//     }
//
//     [[nodiscard]] constexpr const_reference at(size_type pos) const
//     {
//         auto *ptr = _begin + pos;
//         if (ptr >= _end) {
//             throw std::out_of_range();
//         }
//
//         return *ptr;
//     }
//
//     [[nodiscard]] constexpr reference operator[](size_type pos) noexcept
//     {
//         auto *ptr = _begin + pos;
//         hi_assert(ptr < _end);
//         return *ptr;
//     }
//
//     [[nodiscard]] constexpr const_reference operator[](size_type pos) const noexcept
//     {
//         auto *ptr = _begin + pos;
//         hi_assert(ptr < _end);
//         return *ptr;
//     }
//
//     [[nodiscard]] constexpr reference front() noexcept
//     {
//         hi_assert(not empty());
//         return *_begin;
//     }
//
//     [[nodiscard]] constexpr const_reference front() const noexcept
//     {
//         hi_assert(not empty());
//         return *_begin;
//     }
//
//     [[nodiscard]] constexpr reference back() noexcept
//     {
//         hi_assert(not empty());
//         return *(_end - 1);
//     }
//
//     [[nodiscard]] constexpr const_reference back() const noexcept
//     {
//         hi_assert(not empty());
//         return *(_end - 1);
//     }
//
//     [[nodiscard]] constexpr pointer data() noexcept
//     {
//         return _begin;
//     }
//
//     [[nodiscard]] constexpr const_pointer data() const noexcept
//     {
//         return _begin;
//     }
//
//     [[nodiscard]] constexpr iterator begin() noexcept
//     {
//         return _begin;
//     }
//
//     [[nodiscard]] constexpr const_iterator begin() const noexcept
//     {
//         return _begin;
//     }
//
//     [[nodiscard]] constexpr const_iterator cbegin() const noexcept
//     {
//         return _begin;
//     }
//
//     [[nodiscard]] constexpr iterator end() noexcept
//     {
//         return _end;
//     }
//
//     [[nodiscard]] constexpr const_iterator end() const noexcept
//     {
//         return _end;
//     }
//
//     [[nodiscard]] constexpr const_iterator cend() const noexcept
//     {
//         return _end;
//     }
//
//
//     constexpr void resize(size_type new_size)
//     {
//         return _resize(new_size);
//     }
//
//     constexpr void resize(size_type new_size, value_type const &value)
//     {
//         return _resize(new_size, value);
//     }
//
//     constexpr void clear()
//     {
//         return resize(0);
//     }
//
//     constexpr reference emplace_back(auto &&...args)
//     {
//         grow(1);
//         auto tmp = std::construct_at(_end, tt_forward(args)...);
//         ++_end;
//         return *tmp;
//     }
//
//     constexpr void push_back(value_type const &value)
//     {
//         emplace_back(value);
//     }
//
//     constexpr void push_back(value_type &&value)
//     {
//         emplace_back(std::move(value));
//     }
//
//     constexpr void pop_back()
//     {
//         secure_destroy_at(back());
//         --_end;
//     }
//
//     constexpr iterator emplace(const_iterator pos, auto &&...args)
//     {
//         hilet index = std::distance(begin(), pos);
//         hilet n_first = &emplace_back(hi_forward(args)...);
//
//         // Rotate the newly back-emplaced item to it's intended position.
//         hilet first = _begin + index;
//         if (first != n_first) {
//             std::rotate(first, n_first, _end);
//         }
//         return first;
//     }
//
//     constexpr iterator insert(const_iterator pos, value_type const &value)
//     {
//         return emplace(pos, value);
//     }
//
//     constexpr iterator insert(const_iterator pos, value_type &&value)
//     {
//         return emplace(pos, std::move(value));
//     }
//
//     constexpr void reserve(size_type new_capacity)
//     {
//         if (new_capacity <= capacity()) {
//             return;
//         }
//
//         hilet tmp = allocate(new_capacity);
//         try {
//             secure_uninitialized_move(_begin, _end, _tmp);
//             _end = tmp + size();
//             _bound = tmp + new_capacity;
//             _begin = tmp;
//         } catch (...) {
//             deallocate(tmp, new_capacity);
//             throw;
//         }
//     }
//
//     constexpr void shrink_to_fit()
//     {
//         if (empty()) {
//             if (_begin != nullptr) {
//                 deallocate(_begin, capacity());
//                 _begin = _end = _bound = nullptr;
//             }
//
//         } else {
//             hilet new_capacity = size();
//             hilet tmp = allocate(new_capacity);
//             try {
//                 secure_uninitialized_move(_begin, _end, _tmp);
//                 _end = tmp + size();
//                 _bound = tmp + new_capacity;
//                 _begin = tmp;
//             } catch (...) {
//                 deallocate(tmp, new_capacity);
//                 throw;
//             }
//         }
//     }
//
// private:
//     value_type *_begin;
//     value_type *_end;
//     value_type *_bound;
//
//     [[nodiscard]] constexpr bool full() const noexcept
//     {
//         return _end == _bound;
//     }
//
//     /** Increase the capacity.
//      *
//      * This function will use a growth factor of 1.5 for increasing
//      * the capacity if the @a count element will not fit the current allocation.
//      *
//      * Growth when pushing back a single element each time:
//      * - 0, 1, 2, 3, 4, 6, 9, 13, ...
//      */
//     constexpr void grow(size_t count) const noexcept
//     {
//         if (_end + count <= _bound) {
//             return;
//         }
//
//         hilet minimum_new_capacity = size() + count;
//
//         // Growth factor 1.5, slightly lower than the ideal golden ratio.
//         auto new_capacity = capacity();
//         new_capacity += new_capacity >> 1;
//
//         if (new_capacity < minimum_new_capacity) {
//             reserve(minimum_new_capacity);
//         } else {
//             reserve(new_capacity);
//         }
//     }
//
//     template<typename... Args>
//     constexpr void _resize(size_t new_size, Args const &... args)
//     {
//         reserve(new_size);
//
//         hilet new_end = _begin + new_size;
//
//         if (new_end > _end) {
//             // Construct the new values.
//             construct(_end, new_end, args...);
//
//         } else (new_end < _end) {
//             // Destroy the old values.
//             secure_destroy(new_end, _end);
//         }
//         _end = new_end;
//     }
// };
//
// namespace pmr {
//
// template<class T>
// using secure_vector = hi::secure_vector<T,std::pmr::polymorphic_allocator<T>>;
// }
//
// }

