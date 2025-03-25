#include "../macros.hpp"
#include "stop_and_progress.hpp"
#include <string>
#include <memory>
#include <mutex>

#pragma once

hi_export_module(hikogui.dispatch : stop_and_progress_source);

hi_export namespace hi::inline v1 {

class stop_and_progress_source {
public:
    constexpr stop_and_progress_source() noexcept = default;

    ~stop_and_progress_source() noexcept
    {
        _pimpl = nullptr;
    }

    stop_and_progress_source(stop_and_progress_source const &other) noexcept :
        _pimpl(other._pimpl)
    {
    }

    stop_and_progress_source(stop_and_progress_source &&other) noexcept :
        _pimpl(std::exchange(other._pimpl, nullptr))
    {
    }

    stop_and_progress_source &operator=(stop_and_progress_source const &other) noexcept
    {
        if (this != &other) {
            _pimpl = other._pimpl;
        }
        return *this;
    }

    stop_and_progress_source &operator=(stop_and_progress_source &&other) noexcept
    {
        if (this != &other) {
            _pimpl = std::exchange(other._pimpl, nullptr);
        }
        return *this;
    }

    void request_stop() noexcept
    {
        if (_pimpl) {
            _pimpl->request_stop();
        }
    }

    [[nodiscard]] bool stop_requested() const noexcept
    {
        return _pimpl and _pimpl->stop_requested();
    }

    [[nodiscard]] bool stop_possible() const noexcept
    {
        return static_cast<bool>(_pimpl);
    }

    [[nodiscard]] std::pair<double, std::string> major_progress() const noexcept
    {
        if (_pimpl) {
            return _pimpl->major_progress();
        }
        return {0.0, ""};
    }

    [[nodiscard]] std::pair<double, std::string> minor_progress() const noexcept
    {
        if (_pimpl) {
            return _pimpl->minor_progress();
        }
        return {0.0, ""};
    }

private:
    std::shared_ptr<detail::stop_and_progress> _pimpl;
};

}