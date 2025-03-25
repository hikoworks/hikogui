// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file image/pixmap.hpp Defines the pixmap type.
 * @ingroup image
 */

#pragma once

#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <cstddef>
#include <memory>
#include <span>

hi_export_module(hikogui.image.pixmap);

hi_warning_push();
// C26439: This kind of function should not throw. Declare it 'noexcept' (f.6)
// move assignment can throw because allocation may be needed due to proper allocator implementation.
hi_warning_ignore_msvc(26439);
// C26459: You called an STL function 'std::unitialized_move' with a raw pointer... (stl.1)
// Writing iterators instead of using raw pointers will require a lot of code without any added safety.
hi_warning_ignore_msvc(26459);

hi_export namespace hi {
inline namespace v1 {
template<typename T>
class pixmap_span;

/** A 2D pixel-based image.
 *
 * @ingroup image
 * @tparam T The pixel format.
 * @tparam Allocator The allocator to use for allocating the array of pixels.
 */
template<typename T, typename Allocator = std::allocator<T>>
class pixmap {
public:
    /** The pixel format type.
     */
    using value_type = T;

    /** The allocator to use for allocating the array.
     */
    using allocator_type = Allocator;

    /** The size type.
     */
    using size_type = size_t;
    using pointer = value_type*;
    using const_pointer = value_type const*;
    using reference = value_type&;
    using const_reference = value_type const&;
    using interator = pointer;
    using const_iterator = const_pointer;

    /** The type for a row of pixels.
     */
    using row_type = std::span<value_type>;

    /** The type for a row of pixels.
     */
    using const_row_type = std::span<value_type const>;

    template<typename Pixmap>
    struct row_iterator {
        Pixmap* _ptr;
        size_t _y;

        // clang-format off
        constexpr row_iterator(row_iterator const &) noexcept = default;
        constexpr row_iterator(row_iterator &&) noexcept = default;
        constexpr row_iterator &operator=(row_iterator const &) noexcept = default;
        constexpr row_iterator &operator=(row_iterator &&) noexcept = default;
        [[nodiscard]] constexpr row_iterator(Pixmap *ptr, size_t y) noexcept : _ptr(ptr), _y(y) {}
        [[nodiscard]] constexpr friend bool operator==(row_iterator const &, row_iterator const &) noexcept = default;
        constexpr row_iterator &operator++() noexcept { ++_y; return *this; }
        constexpr row_iterator &operator++(int) noexcept { auto tmp = *this; ++_y; return tmp; }
        constexpr row_iterator &operator--() noexcept { --_y; return *this; }
        constexpr row_iterator &operator--(int) noexcept { auto tmp = *this; --_y; return tmp; }
        [[nodiscard]] constexpr auto operator*() const noexcept { return (*_ptr)[_y]; }
        // clang-format on
    };

    template<typename Pixmap>
    struct row_range {
        Pixmap* _ptr;

        // clang-format off
        constexpr row_range(row_range const &) noexcept = default;
        constexpr row_range(row_range &&) noexcept = default;
        constexpr row_range &operator=(row_range const &) noexcept = default;
        constexpr row_range &operator=(row_range &&) noexcept = default;
        [[nodiscard]] constexpr row_range(Pixmap *ptr) noexcept : _ptr(ptr) {}
        [[nodiscard]] constexpr auto begin() const noexcept { return row_iterator{_ptr, 0_uz}; }
        [[nodiscard]] constexpr auto end() const noexcept { return row_iterator{_ptr, _ptr->height()}; }
        // clang-format on
    };

    ~pixmap()
    {
        std::destroy(this->begin(), this->end());
        if (_data != nullptr) {
            std::allocator_traits<allocator_type>::deallocate(_allocator, _data, _capacity);
        }
    }

    /** Copy constructor.
     *
     * Copy the image from other; using the allocator obtained using
     * `std::allocator_traits<Allocator>::select_on_container_copy_construction(other.get_allocator())`
     *
     * The new allocation will be fit the pixels in the image exactly.
     *
     * @param other The other image to copy.
     */
    constexpr pixmap(pixmap const& other) :
        _capacity(other.size()),
        _width(other._width),
        _height(other._height),
        _scale(other._scale),
        _allocator(std::allocator_traits<allocator_type>::select_on_container_copy_construction(other._allocator))
    {
        _data = std::allocator_traits<allocator_type>::allocate(_allocator, _capacity);
        std::uninitialized_copy(other.begin(), other.end(), this->begin());
    }

    /** Move constructor.
     *
     * Move the image.
     *
     * @param other The other image to copy.
     */
    constexpr pixmap(pixmap&& other) noexcept :
        _data(std::exchange(other._data, nullptr)),
        _capacity(std::exchange(other._capacity, 0)),
        _width(std::exchange(other._width, 0)),
        _height(std::exchange(other._height, 0)),
        _scale(std::exchange(other._scale, 1)),
        _allocator(std::exchange(other._allocator, {}))
    {
    }

    /** Copy assignment.
     *
     * Copy the image.
     * Allocation uses the `std::allocator_trait<Allocator>::propagate_on_container_copy_assignment`
     *
     * @param other The other image to copy.
     */
    constexpr pixmap& operator=(pixmap const& other)
    {
        constexpr auto propogate_allocator = std::allocator_traits<allocator_type>::propagate_on_container_copy_assignment::value;

        auto const use_this_allocator = this->_allocator == other._allocator or not propogate_allocator;

        if (&other == this) {
            return *this;

        } else if (this->_capacity >= other.size() and use_this_allocator) {
            static_assert(std::is_nothrow_copy_constructible_v<value_type>);

            // Reuse allocation.
            clear();
            _width = other._width;
            _height = other._height;
            _scale = other._scale;
            std::uninitialized_copy(other.begin(), other.end(), this->begin());
            return *this;

        } else {
            auto& new_allocator = propogate_allocator ? const_cast<allocator_type&>(other._allocator) : this->_allocator;
            auto const new_capacity = other.size();

            value_type* new_data = nullptr;
            try {
                new_data = std::allocator_traits<allocator_type>::allocate(new_allocator, other.size());
                std::uninitialized_copy(other.begin(), other.end(), new_data);
            } catch (...) {
                std::allocator_traits<allocator_type>::deallocate(_allocator, new_data, new_capacity);
                throw;
            }

            try {
                clear();
                shrink_to_fit();
            } catch (...) {
                std::destroy_n(new_data, new_capacity);
                std::allocator_traits<allocator_type>::deallocate(_allocator, new_data, new_capacity);
                throw;
            }

            _data = new_data;
            _capacity = new_capacity;
            _width = other._width;
            _height = other._height;
            _scale = other._scale;
            _allocator = new_allocator;
            return *this;
        }
    }

    /** Move assignment.
     *
     * Move the image.
     * Allocation uses the `std::allocator_trait<Allocator>::propagate_on_container_move_assignment`
     *
     * @param other The other image to copy.
     */
    constexpr pixmap& operator=(pixmap&& other)
    {
        constexpr auto propogate_allocator = std::allocator_traits<allocator_type>::propagate_on_container_move_assignment::value;

        if (&other == this) {
            return *this;

        } else if (_allocator == other._allocator or propogate_allocator) {
            clear();
            shrink_to_fit();

            _data = std::exchange(other._data, nullptr);
            _capacity = std::exchange(other._capacity, 0);
            _width = std::exchange(other._width, 0);
            _height = std::exchange(other._height, 0);
            _scale = std::exchange(other._scale, 1);
            _allocator = other._allocator;
            return *this;

        } else if (_capacity >= other.size()) {
            // Reuse allocation.
            clear();
            _width = other._width;
            _height = other._height;
            _scale = other._scale;

            std::uninitialized_move(other.begin(), other.end(), this->begin());

            // Clear, but leave the allocation intact, so that it can be reused.
            other.clear();
            return *this;

        } else {
            auto const new_capacity = other.size();
            value_type* new_data = nullptr;
            try {
                new_data = std::allocator_traits<allocator_type>::allocate(_allocator, new_capacity);
                std::uninitialized_move(other.begin(), other.end(), new_data);
            } catch (...) {
                std::allocator_traits<allocator_type>::deallocate(_allocator, new_data, new_capacity);
                throw;
            }

            try {
                clear();
                shrink_to_fit();
            } catch (...) {
                std::destroy_n(new_data, new_capacity);
                std::allocator_traits<allocator_type>::deallocate(_allocator, new_data, new_capacity);
                throw;
            }

            _data = new_data;
            _capacity = new_capacity;
            _width = other._width;
            _height = other._height;
            _scale = other._scale;
            _data = std::allocator_traits<allocator_type>::allocate(_allocator, _capacity);

            // Clear, but leave the allocation intact, so that it can be reused.
            other.clear();
            return *this;
        }
    }

    [[nodiscard]] constexpr pixmap() noexcept = default;

    /** Create an pixmap of width and height.
     */
    [[nodiscard]] constexpr pixmap(
        size_type width,
        size_type height,
        size_type scale = 1,
        allocator_type allocator = allocator_type{}) :
        _data(std::allocator_traits<allocator_type>::allocate(allocator, width * height)),
        _capacity(width * height),
        _width(width),
        _height(height),
        _scale(scale),
        _allocator(allocator)
    {
        std::uninitialized_value_construct(begin(), end());
    }

    template<std::convertible_to<value_type> O>
    [[nodiscard]] constexpr pixmap(
        O* hi_restrict data,
        size_type width,
        size_type height,
        size_type stride,
        size_type scale = 1,
        allocator_type allocator = allocator_type{}) :
        _data(std::allocator_traits<allocator_type>::allocate(allocator, width * height)),
        _capacity(width * height),
        _width(width),
        _height(height),
        _scale(scale),
        _allocator(allocator)
    {
        if (width == stride) {
            try {
                std::uninitialized_copy(data, data + width * height, begin());
            } catch (...) {
                std::allocator_traits<allocator_type>::deallocate(_allocator, _data, _capacity);
                throw;
            }

        } else {
            auto src = data;
            auto dst = begin();
            auto dst_end = end();

            try {
                while (dst != dst_end) {
                    std::uninitialized_copy(src, src + width, dst);
                    dst += width;
                    src += stride;
                }
            } catch (...) {
                std::destroy(begin(), dst);
                std::allocator_traits<allocator_type>::deallocate(_allocator, _data, _capacity);
                throw;
            }
        }
    }

    template<std::convertible_to<value_type> O>
    [[nodiscard]] constexpr pixmap(
        O* hi_restrict data,
        size_type width,
        size_type height,
        allocator_type allocator = allocator_type{}) noexcept :
        pixmap(data, width, height, width, 1, allocator)
    {
    }

    template<std::convertible_to<value_type> O>
    [[nodiscard]] constexpr explicit pixmap(pixmap<O> const& other, allocator_type allocator = allocator_type{}) :
        pixmap(other.data(), other.width(), other.height(), other.width(), other.scale(), allocator)
    {
    }

    template<std::convertible_to<value_type> O>
    [[nodiscard]] constexpr explicit pixmap(pixmap_span<O> const& other, allocator_type allocator = allocator_type{}) :
        pixmap(other.data(), other.width(), other.height(), other.stride(), other.scale(), allocator)
    {
    }

    template<std::same_as<value_type const> O>
    [[nodiscard]] constexpr operator pixmap_span<O>() const noexcept
    {
        return pixmap_span<O>{_data, _width, _height, _width, _scale};
    }

    [[nodiscard]] constexpr friend bool operator==(pixmap const& lhs, pixmap const& rhs) noexcept
    {
        if (lhs._width != rhs._width or lhs._height != rhs._height) {
            return false;
        }
        return std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept
    {
        return _allocator;
    }

    [[nodiscard]] constexpr size_type width() const noexcept
    {
        return _width;
    }

    [[nodiscard]] constexpr size_type height() const noexcept
    {
        return _height;
    }

    pixmap& set_scale(size_type scale) noexcept
    {
        _scale = scale;
        return *this;
    }

    [[nodiscard]] constexpr size_type scale() const noexcept
    {
        return _scale;
    }

    /** The number of pixels (width * height) in this image.
     */
    [[nodiscard]] constexpr size_type size() const noexcept
    {
        return _width * _height;
    }

    /** The number of pixels of capacity allocated.
     */
    [[nodiscard]] constexpr size_type capacity() const noexcept
    {
        return _capacity;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return _width == 0 and _height == 0;
    }

    [[nodiscard]] constexpr pointer data() noexcept
    {
        return _data;
    }

    [[nodiscard]] constexpr const_pointer data() const noexcept
    {
        return _data;
    }

    [[nodiscard]] constexpr interator begin() noexcept
    {
        return _data;
    }

    [[nodiscard]] constexpr const_iterator begin() const noexcept
    {
        return _data;
    }

    [[nodiscard]] constexpr const_iterator cbegin() const noexcept
    {
        return _data;
    }

    [[nodiscard]] constexpr interator end() noexcept
    {
        return _data + size();
    }

    [[nodiscard]] constexpr const_iterator end() const noexcept
    {
        return _data + size();
    }

    [[nodiscard]] constexpr const_iterator cend() const noexcept
    {
        return _data + size();
    }

    constexpr reference operator()(size_type x, size_type y) noexcept
    {
        hi_axiom(x < _width);
        hi_axiom(y < _height);
        return _data[y * _width + x];
    }

    constexpr const_reference operator()(size_type x, size_type y) const noexcept
    {
        hi_axiom(x < _width);
        hi_axiom(y < _height);
        return _data[y * _width + x];
    }

    [[nodiscard]] constexpr row_type operator[](size_type y) noexcept
    {
        hi_axiom(y < _height);
        return {_data + y * _width, _width};
    }

    [[nodiscard]] constexpr const_row_type operator[](size_type y) const noexcept
    {
        hi_axiom(y < height());
        return {_data + y * _width, _width};
    }

    [[nodiscard]] constexpr auto rows() noexcept
    {
        return row_range{this};
    }

    [[nodiscard]] constexpr auto rows() const noexcept
    {
        return row_range{this};
    }

    [[nodiscard]] constexpr pixmap
    subimage(size_type x, size_type y, size_type new_width, size_type new_height, allocator_type allocator) const noexcept
    {
        hi_axiom(x + new_width <= _width);
        hi_axiom(y + new_height <= _height);

        auto const p = _data + y * _width + x;
        return {p, new_width, new_height, _width, _scale, allocator};
    }

    [[nodiscard]] constexpr pixmap subimage(size_type x, size_type y, size_type new_width, size_type new_height) const noexcept
    {
        return subimage(x, y, new_width, new_height, _allocator);
    }

    constexpr void clear() noexcept
    {
        std::destroy(begin(), end());
        _width = 0;
        _height = 0;
        _scale = 1;
    }

    constexpr void shrink_to_fit()
    {
        if (empty()) {
            if (_data != nullptr) {
                std::allocator_traits<allocator_type>::deallocate(_allocator, _data, _capacity);
                _data = nullptr;
                _capacity = 0;
            }
            return;
        }

        auto const new_capacity = size();
        value_type* new_data = nullptr;
        try {
            new_data = std::allocator_traits<allocator_type>::allocate(_allocator, new_capacity);
            std::uninitialized_move(begin(), end(), new_data);
        } catch (...) {
            std::allocator_traits<allocator_type>::deallocate(_allocator, new_data, new_capacity);
            throw;
        }

        std::destroy(begin(), end());
        auto const old_capacity = std::exchange(_capacity, new_capacity);
        auto const old_data = std::exchange(_data, new_data);
        std::allocator_traits<allocator_type>::deallocate(_allocator, old_data, old_capacity);
    }

    constexpr friend void fill(pixmap& dst, value_type value = value_type{}) noexcept
    {
        std::fill(dst.begin(), dst.end(), value);
    }

private:
    value_type* _data = nullptr;
    size_type _capacity = 0;
    size_type _width = 0;
    size_type _height = 0;
    size_type _scale = 1;
    [[no_unique_address]] allocator_type _allocator = {};
};

template<typename T>
pixmap(pixmap_span<T> const& other) -> pixmap<std::remove_const_t<T>, std::allocator<std::remove_const_t<T>>>;

template<typename T, typename Allocator>
pixmap(pixmap_span<T> const& other, Allocator allocator) -> pixmap<std::remove_const_t<T>, Allocator>;

} // namespace v1
} // namespace hi::v1

hi_warning_pop();
