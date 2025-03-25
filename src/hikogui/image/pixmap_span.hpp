// Copyright Take Vos 2023.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

/** @file image/pixmap_span.hpp Defines the pixmap_span type.
 * @ingroup image
 */

#pragma once

#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <cstddef>
#include <span>
#include <memory>

hi_export_module(hikogui.image.pixmap_span);

hi_warning_push();
// C26459: You called an STL function 'std::copy' with a raw pointer paramter... (stl.1)
// Using iterators adds a lot of code without any extra safety.
hi_warning_ignore_msvc(26459);

hi_export namespace hi { inline namespace v1 {
template<typename T, typename Allocator>
class pixmap;

/** A non-owning 2D pixel-based image.
 *
 * @ingroup image
 * @tparam T The pixel format.
 */
template<typename T>
class pixmap_span {
public:
    using value_type = T;
    using reference = value_type&;
    using const_reference = value_type const&;
    using pointer = value_type *;
    using const_pointer = value_type const *;
    using row_type = std::span<value_type>;
    using const_row_type = std::span<value_type const>;
    using nc_value_type = std::remove_const_t<value_type>;
    using size_type = size_t;

    template<typename PixmapView>
    struct row_iterator {
        PixmapView *_ptr;
        size_t _y;

        // clang-format off
        constexpr row_iterator(row_iterator const &) noexcept = default;
        constexpr row_iterator(row_iterator &&) noexcept = default;
        constexpr row_iterator &operator=(row_iterator const &) noexcept = default;
        constexpr row_iterator &operator=(row_iterator &&) noexcept = default;
        [[nodiscard]] constexpr row_iterator(PixmapView *ptr, size_t y) noexcept : _ptr(ptr), _y(y) {}
        [[nodiscard]] constexpr friend bool operator==(row_iterator const &, row_iterator const &) noexcept = default;
        constexpr row_iterator &operator++() noexcept { ++_y; return *this; }
        constexpr row_iterator &operator++(int) noexcept { auto tmp = *this; ++_y; return tmp; }
        constexpr row_iterator &operator--() noexcept { --_y; return *this; }
        constexpr row_iterator &operator--(int) noexcept { auto tmp = *this; --_y; return tmp; }
        [[nodiscard]] constexpr auto operator*() const noexcept { return (*_ptr)[_y]; }
        // clang-format on
    };

    template<typename PixmapView>
    struct row_range {
        PixmapView *_ptr;

        // clang-format off
        constexpr row_range(row_range const &) noexcept = default;
        constexpr row_range(row_range &&) noexcept = default;
        constexpr row_range &operator=(row_range const &) noexcept = default;
        constexpr row_range &operator=(row_range &&) noexcept = default;
        [[nodiscard]] constexpr row_range(PixmapView *ptr) noexcept : _ptr(ptr) {}
        [[nodiscard]] constexpr auto begin() const noexcept { return row_iterator{_ptr, 0_uz}; }
        [[nodiscard]] constexpr auto end() const noexcept { return row_iterator{_ptr, _ptr->height()}; }
        // clang-format on
    };

    ~pixmap_span() = default;
    constexpr pixmap_span(pixmap_span const&) noexcept = default;
    constexpr pixmap_span(pixmap_span&&) noexcept = default;
    constexpr pixmap_span& operator=(pixmap_span const&) noexcept = default;
    constexpr pixmap_span& operator=(pixmap_span&&) noexcept = default;
    [[nodiscard]] constexpr pixmap_span() noexcept = default;

    [[nodiscard]] constexpr pixmap_span(value_type *data, size_type width, size_type height, size_type stride, size_type scale = 1) noexcept :
        _data(data), _width(width), _height(height), _stride(stride), _scale(1)
    {
    }

    [[nodiscard]] constexpr pixmap_span(value_type *data, size_type width, size_type height) noexcept :
        pixmap_span(data, width, height, width, 1)
    {
    }

    template<std::same_as<std::remove_const_t<value_type>> O, typename Allocator>
    [[nodiscard]] constexpr pixmap_span(pixmap<O, Allocator> const& other) noexcept :
        pixmap_span(other.data(), other.width(), other.height(), other.width(), other.scale())
    {
    }

    template<std::same_as<std::remove_const_t<value_type>> O, typename Allocator>
    [[nodiscard]] constexpr pixmap_span(pixmap<O, Allocator>& other) noexcept :
        pixmap_span(other.data(), other.width(), other.height(), other.width(), other.scale())
    {
    }

    template<std::same_as<std::remove_const_t<value_type>> O, typename Allocator>
    [[nodiscard]] constexpr pixmap_span(pixmap<O, Allocator>&& other) = delete;

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return _width == 0 and _height == 0;
    }

    [[nodiscard]] constexpr size_type width() const noexcept
    {
        return _width;
    }

    [[nodiscard]] constexpr size_type height() const noexcept
    {
        return _height;
    }

    [[nodiscard]] constexpr size_type stride() const noexcept
    {
        return _stride;
    }

    constexpr pixmap_span& set_scale(size_type scale) noexcept
    {
        _scale = scale;
        return *this;
    }

    [[nodiscard]] constexpr size_type scale() const noexcept
    {
        return _scale;
    }

    [[nodiscard]] constexpr size_type size() const noexcept
    {
        return _width * _height;
    }

    [[nodiscard]] constexpr size_type capacity() const noexcept
    {
        return _stride * _height;
    }

    [[nodiscard]] constexpr pointer data() noexcept
    {
        return _data;
    }

    [[nodiscard]] constexpr const_pointer data() const noexcept
    {
        return _data;
    }

    constexpr reference operator()(size_type x, size_type y) noexcept
    {
        hi_axiom(x < _width);
        hi_axiom(y < _height);
        return _data[y * _stride + x];
    }

    constexpr const_reference operator()(size_type x, size_type y) const noexcept
    {
        hi_axiom(x < _width);
        hi_axiom(y < _height);
        return _data[y * _stride + x];
    }

    [[nodiscard]] constexpr row_type operator[](size_type y) noexcept
    {
        hi_axiom(y < _height);
        return {_data + y * _stride, _width};
    }

    [[nodiscard]] constexpr const_row_type operator[](size_type y) const noexcept
    {
        hi_axiom(y < _height);
        return {_data + y * _stride, _width};
    }

    [[nodiscard]] constexpr auto rows() noexcept
    {
        return row_range{this};
    }

    [[nodiscard]] constexpr auto rows() const noexcept
    {
        return row_range{this};
    }

    [[nodiscard]] constexpr pixmap_span subimage(size_type x, size_type y, size_type new_width, size_type new_height) noexcept
    {
        return {_data + y * _stride + x, new_width, new_height, _stride, _scale};
    }

    [[nodiscard]] constexpr pixmap_span<value_type const>
    subimage(size_type x, size_type y, size_type new_width, size_type new_height) const noexcept
    {
        return {_data + y * _stride + x, new_width, new_height, _stride, _scale};
    }

    constexpr friend void copy(pixmap_span src, pixmap_span<std::remove_const_t<value_type>> dst) noexcept
    {
        hi_axiom(src.width() == dst.width());
        hi_axiom(src.height() == dst.height());

        if (src.width() == src.stride() and dst.width() == dst.stride()) {
            std::copy(src.data(), src.data() + src.width() * src.height(), dst.data());
        } else {
            for (auto y = 0_uz; y != src.height(); ++y) {
                auto const src_line = src[y];
                auto const dst_line = dst[y];
                std::copy(src_line.begin(), src_line.end(), dst_line.begin());
            }
        }
    }

    constexpr friend void fill(pixmap_span dst, value_type value = value_type{}) noexcept
    {
        if (dst._width == dst._stride) {
            std::fill_n(dst._data, dst._width * dst._height, value);
        } else {
            for (auto line: dst.rows()) {
                std::fill(line.begin(), line.end(), value);
            }
        }
    }

private:
    value_type *_data = nullptr;
    size_type _width = 0;
    size_type _height = 0;
    size_type _stride = 0;
    size_type _scale = 1;
};

template<typename T, typename Allocator>
pixmap_span(pixmap<T, Allocator> const& other) -> pixmap_span<T const>;

template<typename T, typename Allocator>
pixmap_span(pixmap<T, Allocator>& other) -> pixmap_span<T>;

}} // namespace hi::v1

hi_warning_pop();
