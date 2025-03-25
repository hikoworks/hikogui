// Copyright Take Vos 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../algorithm/algorithm.hpp"
#include "../concurrency/concurrency.hpp"
#include "../macros.hpp"
#include <vector>
#include <algorithm>
#include <chrono>
#include <functional>

hi_export_module(hikogui.dispatch : function_predicate);

hi_export namespace hi::inline v1 {

class function_predicate {
public:
    template<forward_of<void()> Func>
    callback<void()> add(std::function<bool()> predicate, Func &&func) noexcept
    {
        auto token = callback<void()>{std::forward<Func>(func)};
        _functions.emplace_back(std::move(predicate), token);
        return token;
    }

    void run_all() noexcept
    {
        auto callbacks = _callbacks_to_run();
        for (auto &callback : callbacks) {
            callback();
        }

        _remove_expired();
    }

private:
    struct item_type {
        std::function<bool()> predicate;
        weak_callback<void()> callback;
    };

    std::vector<item_type> _functions;

    [[nodiscard]] std::vector<callback<void()>> _callbacks_to_run() noexcept
    {
        std::vector<callback<void()>> r;

        auto it = remove_transform_if(_functions.begin(), _functions.end(), std::back_inserter(r), [](auto const &x) -> std::optional<callback<void()>> {
            if (auto callback = x.callback.lock()) {
                if (x.predicate()) {
                    return callback;
                }
            }
            return std::nullopt;
        });
        
        _functions.erase(it, _functions.cend());

        return r;
    }

    void _remove_expired() noexcept
    {
        auto const it = std::remove_if(_functions.begin(), _functions.end(), [](auto const& x) {
            return x.callback.expired();
        });
        _functions.erase(it, _functions.cend());
    }
};

}
