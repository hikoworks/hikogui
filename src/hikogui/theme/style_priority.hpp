

#pragma once

#include "style_importance.hpp"
#include "style_specificity.hpp"
#include "../macros.hpp"
#include <cstdint>
#include <compare>

hi_export_module(hikogui.theme : style_priority);

hi_export namespace hi::inline v1 {
class style_priority {
public:
    constexpr style_priority() noexcept = default;
    constexpr style_priority(style_priority const&) noexcept = default;
    constexpr style_priority(style_priority&&) noexcept = default;
    constexpr style_priority& operator=(style_priority const&) noexcept = default;
    constexpr style_priority& operator=(style_priority&&) noexcept = default;
    constexpr style_priority(style_importance importance, style_specificity specificity) noexcept :
        _importance(std::to_underlying(importance)), _specificity(std::to_underlying(specificity))
    {
    }

    [[nodiscard]] constexpr friend bool operator==(style_priority const& lhs, style_priority const& rhs) noexcept = default;

    [[nodiscard]] constexpr friend auto operator<=>(style_priority const& lhs, style_priority const& rhs) noexcept
    {
        // Only check importance if the specificity.
        if (auto r = lhs._importance <=> rhs._importance; r != 0) {
            return r;
        }
        return lhs._specificity <=> rhs._specificity;
    }

    constexpr void set_specificity(style_specificity specificity) noexcept
    {
        _specificity = std::to_underlying(specificity);
    }

    [[nodiscard]] constexpr style_specificity specificity() const noexcept
    {
        return static_cast<style_specificity>(_specificity);
    }

    constexpr void set_importance(style_importance importance) noexcept
    {
        _importance = std::to_underlying(importance);
    }

    [[nodiscard]] constexpr style_importance importance() const noexcept
    {
        return static_cast<style_importance>(_importance);
    }

private:
    uint16_t _importance : 3 = 0;
    uint16_t _specificity : 10 = 0;
};
}
