// Copyright Take Vos 2024.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../utility/utility.hpp"
#include "../macros.hpp"
#include <type_traits>
#include <compare>
#include <iterator>
#include <ranges>
#include <vector>

hi_warning_push();
// MSVC: warning C4146: unary minus operator applied to unsigned type, result
// still unsigned. Used to properly negate a minimum signed integer to a
// positive unsigned integer. Example:
// ```cpp
// auto const x = std::numeric_limits<int>::min();
// auto const y = -static_cast<unsigned int>(x);
// ```
hi_warning_ignore_msvc(4146);

hi_export_module(hikogui.algorithm.recursive_iterator);

hi_export namespace hi::inline v1 {

/** A sequence of ranges.
 *
 * @tparam Pair The pair type of the range. Default is std::pair<size_t,
 *         size_t>. The first element is the first index of the range, and the
 *         second element is one beyond the last index of the range.
 */
template<typename Pair = std::pair<size_t, size_t>>
class sequence : public std::vector<Pair> {
private:
    [[nodiscard]] consteval static auto pair_index_type() noexcept
    {
        auto const first = get_data_member<0>(Pair{});
        auto const second = get_data_member<1>(Pair{});

        static_assert(std::is_integral_v<decltype(first)>);
        static_assert(std::is_same_v<decltype(first), decltype(second)>);
        return first;
    }

public:
    using super = std::vector<Pair>;
    using index_type = decltype(pair_index_type());

    class index_iterator {
    public:
        constexpr index_iterator() noexcept = default;
        constexpr index_iterator(index_iterator const&) noexcept = default;
        constexpr index_iterator(index_iterator&&) noexcept = default;
        constexpr index_iterator& operator=(index_iterator const&) noexcept = default;
        constexpr index_iterator& operator=(index_iterator&&) noexcept = default;
        [[nodiscard]] constexpr friend bool operator==(index_iterator const&, index_iterator const&) noexcept = default;
        [[nodiscard]] constexpr friend std::partial_ordering operator<=>(index_iterator const& lhs, index_iterator const& rhs) noexcept
        {
            if (lhs._sequence != rhs._sequence) {
                return std::partial_ordering::unordered;
            }
            if (lhs._major_index != rhs._major_index) {
                return lhs._major_index <=> rhs._major_index;
            }
            return lhs._minor_index <=> rhs._minor_index;
        }

        constexpr index_iterator(
            sequence const* sequence,
            size_t major_index = 0,
            index_type minor_index = 0) noexcept :
            _sequence(sequence), _major_index(major_index), _minor_index(minor_index)
        {
        }

        [[nodiscard]] constexpr index_type operator*() const
        {
            assert(_sequence != nullptr);
            auto const& item = _sequence->at(_major_index);
            auto const first = get_data_member<0>(item);
            auto const last = get_data_member<1>(item);
            
            auto const r = first + _minor_index;
            assert(r < last);
            return r;
        }

        constexpr index_iterator& operator++()
        {
            return advance(1);
        }

        constexpr index_iterator operator++(int)
        {
            auto r = *this;
            advance(1);
            return r;
        }

        constexpr index_iterator& operator--()
        {
            return retreat(1);
        }

        constexpr index_iterator operator--(int)
        {
            auto r = *this;
            retreat(1);
            return r;
        }

        constexpr index_iterator& operator+=(ptrdiff_t n)
        {
            if (n >= 0) {
                return advance(static_cast<size_t>(n));
            } else {
                return retreat(-static_cast<size_t>(n));
            }
        }

        constexpr index_iterator& operator-=(ptrdiff_t n)
        {
            if (n >= 0) {
                return retreat(static_cast<size_t>(n));
            } else {
                return advance(-static_cast<size_t>(n));
            }
        }

        [[nodiscard]] constexpr index_iterator operator+(ptrdiff_t n) const
        {
            auto r = *this;
            r += n;
            return r;
        }

        [[nodiscard]] constexpr index_iterator operator-(ptrdiff_t n) const
        {
            auto r = *this;
            r -= n;
            return r;
        }

    private:
        sequence const* _sequence = nullptr;
        size_t _major_index = 0;
        index_type _minor_index = 0;

        constexpr index_iterator& advance(size_t n)
        {
            assert(_sequence != nullptr);
            while (n != 0) {
                auto const& item = _sequence->at(_major_index);
                auto const first = get_data_member<0>(item);
                auto const last = get_data_member<1>(item);

                auto const size = last - first;
                auto const remaining = size - _minor_index;
                auto const m = std::min(n, remaining);
                _minor_index += m;
                n -= m;
                if (_minor_index == size) {
                    ++_major_index;
                    _minor_index = 0;
                }
            }
            return *this;
        }

        constexpr index_iterator& retreat(size_t n)
        {
            assert(_sequence != nullptr);
            while (n != 0) {
                if (_minor_index == 0) {
                    assert(_major_index != 0);
                    --_major_index;
                    auto const &item = _sequence->at(_major_index);
                    auto const first = get_data_member<0>(item);
                    auto const last = get_data_member<1>(item);

                    _minor_index = last - first;
                }

                auto const m = std::min(n, _minor_index);
                _minor_index -= m;
                n -= m;
            }
            return *this;
        }
    };

    using super::super;

    [[nodiscard]] constexpr size_t index_size() const
    {
        return std::accumulate(this->begin(), this->end(), size_t{0}, [](size_t acc, auto const& x) {
            auto const first = get_data_member<0>(x);
            auto const last = get_data_member<1>(x);
            return acc + (last - first);
        });
    }

    [[nodiscard]] constexpr index_iterator index_begin() const noexcept
    {
        return index_iterator{this};
    }

    [[nodiscard]] constexpr index_iterator index_end() const noexcept
    {
        return index_iterator{this, this->size()};
    }
};


}

hi_warning_pop();
