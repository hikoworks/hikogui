
#pragma once

#include "../macros.hpp"
#include <memory_resource>

hi_export_module(hikogui.container : fifo)


hi_export namespace hi::inline v1 {

template<typename T, typename Allocator = std::allocator<T>>
class fifo {
public:
    static_assert(std::is_same_v<T, std::remove_cv_t<T>>, "T must be a non-const, non-volatile type");
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = value_type const&;
    using pointer = std::allocator_traits<Allocator>::pointer;
    using const_pointer = std::allocator_traits<Allocator>::const_pointer;
   
    constexpr fifo() noexcept = default;

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return _size == 0;
    }

    [[nodiscard]] constexpr size_type size() const noexcept
    {
        return _size;
    }

    [[nodiscard]] constexpr size_type max_size() const noexcept
    {
        return std::allocator_traits<allocator_type>::max_size(_allocator);
    }

    [[nodiscard]] constexpr size_type capacity() const noexcept
    {
        return std::distance(_begin, _end);
    }

    constexpr void reserve(size_t new_size)
    {
        if (new_size > max_size()) {
            throw std::length_error("Reservation too large");
        }

        auto const old_size = capacity();
        if (new_size <= old_size) {
            return;
        }

        // Use the growth factor of 1.5 which is preferred as it
        // is more likely to reuse previous allocations.
        auto const grow_size = old_size + (old_size >> 1);

        // If the requested size is larger, use that, since
        // we could not reuse a previous allocation.
        //
        // reallocate() may request more capacity based on the allocator's
        // ability to return a larger size.
        reallocate(grow_size > new_size ? grow_size : new_size);
    }

    constexpr void shrink_to_fit()
    {
        reallocate(size());
    }

    constexpr void clear()
    {
        std::destroy_n(_tail, std::min(wrap_size(), _size));
        if (wrap_size() < _size) {
            std::destroy_n(_begin, _size - wrap_size());
        }
        _tail = _begin;
        _size = 0;
    }

    [[nodiscard]] constexpr const_reference front() const
    {
        assert(_size != 0);
        return *_tail;
    }

    [[nodiscard]] constexpr reference front()
    {
        assert(_size != 0);
        return *_tail;
    }

    [[nodiscard]] constexpr const_reference back() const
    {
        assert(_size != 0);

        if (_size - 1 < wrap_size()) {
            return _tail[_size - 1];
        } else {
            return _begin[_size - wrap_size() - 1];
        }
    }

    [[nodiscard]] constexpr reference back()
    {
        assert(_size != 0);

        if (_size - 1 < wrap_size()) {
            return _tail[_size - 1];
        } else {
            return _begin[_size - wrap_size() - 1];
        }
    }

    [[nodiscard]] constexpr const_reference operator[](size_t i) const
    {
        assert(i < _size);

        if (i < wrap_size()) {
            return _tail[i];
        } else {
            return _begin[i - wrap_size()];
        }
    }

    [[nodiscard]] constexpr reference operator[](size_t i)
    {
        assert(i < size());

        if (i < wrap_size()) {
            return _tail[i];
        } else {
            return _begin[i - wrap_size()];
        }
    }

    constexpr void pop_front()
    {
        assert(_size != 0);

        std::destroy_at(std::addressof(front()));
        if (++_tail == _end) {
            _tail = _begin;
        }
        --_size;
    }

    constexpr void pop_back()
    {
        assert(_size != 0);

        std::destroy_at(std::addressof(back()));
        --_size;
    }

    template<typename... Args>
    constexpr reference emplace_front(Args&&... args)
    {
        reserve(size() + 1);

        assert(_tail != _end);
        if (_tail-- == _begin) {
            _tail = _end - 1;
        }

        ++_size;
        assert(_size <= capacity());
        return *std::construct_at(std::addressof(front()), std::forward<Args>(args)...);
    }

    constexpr void push_front(value_type const& value)
    {
        emplace_front(value);
    }

    constexpr void push_front(value_type&& value)
    {
        emplace_front(std::move(value));
    }

    template<typename... Args>
    constexpr value_type& emplace_back(Args&&... args)
    {
        reserve(size() + 1);

        ++_size;
        assert(_size <= capacity());
        return *std::construct_at(std::addressof(back()), std::forward<Args>(args)...);
    }

    constexpr void push_back(value_type const& value)
    {
        emplace_back(value);
    }

    constexpr void push_back(value_type&& value)
    {
        emplace_back(std::move(value));
    }

private:
    [[no_unique_address]] allocator_type _allocator;
    pointer _begin = nullptr;
    pointer _end = nullptr;
    pointer _tail = nullptr;
    size_t _size = 0;

    [[nodiscard]] constexpr size_t wrap_size() const noexcept
    {
        auto const wrap_size = std::distance(_tail, _end);
        return gsl::narrow<size_t>(wrap_size);
    }

    /** Reallocate the data in the fifo.
     *
     * This has the same exception guarantees as std::vector:
     *  - If allocation fails, then the fifo is not modified.
     *  - If the move of objects fails, then the objects in the fifo
     *    may be in a valid but unspecified state.
     */
    void reallocate(size_t new_capacity)
    {
        assert(new_capacity >= _size);

        auto const new_allocation = std::allocator_traits<allocator_type>::allocate_at_least(_allocator, new_capacity);
        auto const new_begin = new_allocation.ptr;
        auto const new_end = new_begin + new_allocation.count;
        auto const new_tail = new_begin;
        auto const new_size = _size;

        try {
            std::uninitialized_move_n(_tail, std::min(wrap_size(), _size), new_tail);
            if (wrap_size() < _size) {
                try {
                    std::uninitialized_move_n(_begin, _size - wrap_size(), new_tail + wrap_size());
                } catch (...) {
                    // Destroy objects from the previous move, so that the
                    // next exception handler can deallocate the memory, without
                    // calling destructors.
                    std::destroy_n(new_tail, wrap_size());
                    throw;
                }
            }

        } catch (...) {
            std::allocator_traits<allocator_type>::deallocate(_allocator, new_begin, std::distance(new_begin, new_end));
            throw;
        }

        // Destroy the moved from objects.
        std::destroy_n(_tail, std::min(wrap_size(), _size));
        if (wrap_size() < _size) {
            std::destroy_n(_begin, _size - wrap_size());
        }

        auto const old_begin = std::exchange(_begin, new_begin);
        auto const old_end = std::exchange(_end, new_end);
        _tail = new_tail;
        _size = new_size;
        std::allocator_traits<allocator_type>::deallocate(_allocator, old_begin, std::distance(old_begin, old_end));
    }
};

}

