

#pragma once

#include "style_specificity.hpp"
#include "style_path.hpp"
#include "../macros.hpp"
#include <concepts>
#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
#include <format>
#include <cstddef>

hi_export_module(hikogui.theme : style_selector);

hi_export namespace hi::inline v1 {
/**
 * @brief Represents a style selector segment used for selecting elements in a GUI theme.
 *
 */
struct style_selector_segment {
    std::string name;
    std::string id;
    std::vector<std::string> classes;
    std::vector<std::string> pseudo_classes;

    /** The next child must follow directly after this segment.
     *
     * By default this is true, as the last segment must have set this to true.
     */
    bool child_combinator = true;

    constexpr style_selector_segment() noexcept = default;
    constexpr style_selector_segment(style_selector_segment const&) = default;
    constexpr style_selector_segment(style_selector_segment&&) = default;
    constexpr style_selector_segment& operator=(style_selector_segment const&) = default;
    constexpr style_selector_segment& operator=(style_selector_segment&&) = default;

    constexpr style_selector_segment(
        std::string name,
        std::string id = {},
        std::vector<std::string> classes = {},
        std::vector<std::string> pseudo_classes = {},
        bool child_combinator = true) :
        name(std::move(name)),
        id(std::move(id)),
        classes(std::move(classes)),
        pseudo_classes(pseudo_classes),
        child_combinator(child_combinator)
    {
        if (this->name == "*") {
            this->name.clear();
        }
        std::sort(this->classes.begin(), this->classes.end());
        std::sort(this->pseudo_classes.begin(), this->pseudo_classes.end());
    }

    [[nodiscard]] constexpr static style_selector_segment from_element(std::string name) noexcept
    {
        return style_selector_segment(std::move(name));
    }

    [[nodiscard]] constexpr static style_selector_segment from_id(std::string id) noexcept
    {
        return style_selector_segment({}, std::move(id));
    }

    template<std::convertible_to<std::string>... ClassNames>
    [[nodiscard]] constexpr static style_selector_segment from_class(ClassNames&&... class_names) noexcept
    {
        return style_selector_segment({}, {}, {std::forward<ClassNames>(class_names)...});
    }

    template<std::convertible_to<std::string>... PseudoClassNames>
    [[nodiscard]] constexpr static style_selector_segment from_pseudo_class(PseudoClassNames&&... pseudo_class_names) noexcept
    {
        return style_selector_segment({}, {}, {}, {std::forward<PseudoClassNames>(pseudo_class_names)...});
    }

    /**
     * Determines if a style selector segment matches a style path segment.
     *
     * @param lhs The style selector segment to compare.
     * @param rhs The style path segment to compare.
     * @return True if the style selector segment matches the style path segment, false otherwise.
     */
    [[nodiscard]] constexpr friend bool matches(style_selector_segment const& lhs, style_path_segment const& rhs) noexcept
    {
        if (not lhs.name.empty() and lhs.name != rhs.name) {
            return false;
        }

        if (not lhs.id.empty() and lhs.id != rhs.id) {
            return false;
        }

        if (lhs.classes.size() > rhs.classes.size()) {
            return false;
        }

        if (not std::includes(rhs.classes.begin(), rhs.classes.end(), lhs.classes.begin(), lhs.classes.end())) {
            return false;
        }

        return true;
    }
};

/**
 * @brief Represents a style selector used for selecting elements in a GUI theme.
 *
 * The `style_selector` class is derived from `std::vector<style_selector_segment>`,
 * which allows it to store a sequence of style selector segments. It also provides
 * a method to calculate the specificity of the selector based on CSS2.2 rules.
 *
 * @note The specificity of a selector is used for calculating priorities when
 * applying styles to elements.
 */
class style_selector : public std::vector<style_selector_segment> {
public:
    using super = std::vector<style_selector_segment>;
    using super::super;

    /**
     * @brief Calculates the specificity of the style selector.
     *
     * The specificity of a selector is calculated based on CSS2.2 rules. It takes
     * into account the presence of IDs, class names, pseudo classes, and element names
     * in the selector segments. The calculated specificity value is between 0 and 999.
     *
     * @note Based on CSS2.2: 6.4.3 Calculating a selector's specificity.
     *
     * @return The specificity value of the style selector.
     */
    [[nodiscard]] constexpr style_specificity specificity() const noexcept
    {
        auto b = size_t{};
        auto c = size_t{};
        auto d = size_t{};

        for (auto const& segment : *this) {
            b += static_cast<size_t>(not segment.id.empty());
            c += static_cast<size_t>(not segment.classes.empty());
            c += static_cast<size_t>(not segment.pseudo_classes.empty());
            d += static_cast<size_t>(not segment.name.empty());
        }

        b = std::min(b, size_t{9});
        c = std::min(c, size_t{9});
        d = std::min(d, size_t{9});
        return static_cast<style_specificity>(b * 100 + c * 10 + d);
    }
};

/**
 * @brief Checks if a style selector matches a style path.
 *
 * This function compares two ranges of iterators, `lhs_it_first` to `lhs_it_last` and `rhs_it_first` to `rhs_it_last`,
 * and determines if the style selector matches the style path. The function returns true if the style selector matches
 * the style path, and false otherwise.
 *
 * @param lhs_it_first The iterator pointing to the first element of the style selector range.
 * @param lhs_it_last The iterator pointing to the last element of the style selector range.
 * @param rhs_it_first The iterator pointing to the first element of the style path range.
 * @param rhs_it_last The iterator pointing to the last element of the style path range.
 * @return true if the style selector matches the style path, false otherwise.
 */
[[nodiscard]] constexpr bool matches(
    style_selector::const_iterator lhs_it_first,
    style_selector::const_iterator lhs_it_last,
    style_path::const_iterator rhs_it_first,
    style_path::const_iterator rhs_it_last) noexcept
{
    if (std::distance(lhs_it_first, lhs_it_last) > std::distance(rhs_it_first, rhs_it_last)) {
        return false;
    }

    while (lhs_it_first != lhs_it_last and rhs_it_first != rhs_it_last) {
        --lhs_it_last;
        --rhs_it_last;

        if (not matches(*lhs_it_last, *rhs_it_last)) {
            if (lhs_it_last->child_combinator or rhs_it_first == rhs_it_last) {
                return false;
            }
            // Try to match the current selector segment again, but with the next segment in the path.
            return matches(lhs_it_first, lhs_it_last + 1, rhs_it_first, rhs_it_last);
        }
    }

    return lhs_it_last == lhs_it_first;
}

/**
 * @brief Checks if a style selector matches a style path.
 *
 * This function compares a style selector with a style path to determine if they match.
 * It uses the `matches` function to perform the comparison.
 *
 * @param lhs The style selector to compare.
 * @param rhs The style path to compare.
 * @return `true` if the style selector matches the style path, `false` otherwise.
 */
[[nodiscard]] constexpr bool matches(style_selector const& selector, style_path const& path) noexcept
{
    return matches(selector.begin(), selector.end(), path.begin(), path.end());
}

/**
 * Determines whether the given style selector matches the provided pseudo classes.
 *
 * @note This function only checks the pseudo classes of the last segment in the style selector.
 * @param selector The style selector to match against.
 * @param pseudo_classes The list of pseudo classes to match.
 * @return `true` if the style selector matches the pseudo classes, `false` otherwise.
 */
[[nodiscard]] constexpr bool matches(style_selector const& selector, std::vector<std::string> const& pseudo_classes) noexcept
{
    if (selector.empty()) {
        return true;
    }

    auto const& last = selector.back();
    return std::includes(pseudo_classes.begin(), pseudo_classes.end(), last.pseudo_classes.begin(), last.pseudo_classes.end());
}

/**
 * Determines whether a given style selector matches a style path and a list of pseudo classes.
 *
 * @note This function only checks the pseudo classes of the last segment in the style selector.
 * @param selector The style selector to match.
 * @param path The style path to match against.
 * @param pseudo_classes The list of pseudo classes to match against.
 * @return `true` if the style selector matches the style path and pseudo classes, `false` otherwise.
 */
[[nodiscard]] constexpr bool
matches(style_selector const& selector, style_path const& path, std::vector<std::string> const& pseudo_classes) noexcept
{
    return matches(selector, pseudo_classes) and matches(selector, path);
}


/**
 * @brief Creates a CSS nth-child pseudo-class selector.
 * 
 * This function generates a CSS nth-child pseudo-class selector based on the given parameters.
 * The nth-child pseudo-class matches elements based on their position among a group of siblings.
 * 
 * @param n The modulus value for the nth-child pseudo-class. Must be greater than or equal to 2.
 * @param i The index of the element among the siblings. 0-indexed.
 * @return The generated nth-child pseudo-class selector.
 * 
 * @note The generated nth-child pseudo-class is 1-indexed. But the parameter @a i is 0-indexed.
 * @note The generated selector will be in the format "nth-child(an+b)".
 *       If i is equal to n, the selector will be "nth-child(an)".
 *       Otherwise, the selector will be "nth-child(an+b)".
 * 
 * @assert n must be greater than or equal to 2.
 */
[[nodiscard]] constexpr std::string make_nth_child_pseudo_class(size_t n, size_t i) noexcept
{
    assert(n >= 2);
    i %= n;

    // nth-child is 1-indexed.
    ++i;
    if (i == n) {
        return std::format("nth-child({}n)", n);
    } else {
        return std::format("nth-child({}n+{})", n, i);
    }
}

/**
 * @brief Creates a pseudo-class selector for the nth depth.
 * 
 * This function creates a pseudo-class selector for the nth depth, which is
 * used in CSS to select elements based on their position within a hierarchy.
 * 
 * @param n The modulus value for the nth-depth pseudo-class. Must be greater than or equal to 2.
 * @param i The current depth. 0-indexed.
 * @return The pseudo-class selector for the nth depth.
 */
[[nodiscard]] constexpr std::string make_nth_depth_pseudo_class(size_t n, size_t i) noexcept
{
    assert(n >= 2);
    i %= n;

    // nth-depth is 1-indexed.
    ++i;
    if (i == n) {
        return std::format("nth-depth({}n)", n);
    } else {
        return std::format("nth-depth({}n+{})", n, i);
    }
}

} // namespace hi::inline v1
