#include "../macros.hpp"
#include "stop_and_progress.hpp"
#include <string>
#include <memory>
#include <mutex>

#pragma once

hi_export_module(hikogui.dispatch : stop_and_progress_token);

hi_export namespace hi::inline v1 {

class stop_and_progress_token {
public:
    constexpr stop_and_progress_token() noexcept = default;

    ~stop_and_progress_token() noexcept
    {
        if (_pimpl) {
            _pimpl->decrement_token_count();
        }
        _pimpl = nullptr;
    }

    stop_and_progress_token(stop_and_progress_token const &other) noexcept :
        _pimpl(other._pimpl)
    {
        if (_pimpl) {
            _pimpl->increment_token_count();
        }
    }

    stop_and_progress_token(stop_and_progress_token &&other) noexcept :
        _pimpl(std::exchange(other._pimpl, nullptr))
    {
    }

    stop_and_progress_token &operator=(stop_and_progress_token const &other) noexcept
    {
        if (this != &other) {
            _pimpl = other._pimpl;
            if (_pimpl) {
                _pimpl->increment_token_count();
            }
        }
        return *this;
    }

    stop_and_progress_token &operator=(stop_and_progress_token &&other) noexcept
    {
        if (this != &other) {
            if (_pimpl) {
                _pimpl->decrement_token_count();
            }
            _pimpl = std::exchange(other._pimpl, nullptr);
            if (_pimpl) {
                _pimpl->increment_token_count();
            }
        }
        return *this;
    }

    [[nodiscard]] bool stop_requested() const noexcept
    {
        return _pimpl and _pimpl->stop_requested();
    }

    [[nodiscard]] bool stop_possible() const noexcept
    {
        return static_cast<bool>(_pimpl);
    }

    void set_major_progress(double progress) noexcept
    {
        if (_pimpl) {
            _pimpl->set_major_progress(progress);
        }
    }

    void set_major_progress(double progress, std::string_view message) noexcept
    {
        if (_pimpl) {
            _pimpl->set_major_progress(progress, message);
        }
    }

    void set_minor_progress(double progress) noexcept
    {
        if (_pimpl) {
            _pimpl->set_minor_progress(progress);
        }
    }
    
    void set_minor_progress(double progress, std::string_view message) noexcept
    {
        if (_pimpl) {
            _pimpl->set_minor_progress(progress, message);
        }
    }

private:
    std::shared_ptr<detail::stop_and_progress> _pimpl;
};

}
