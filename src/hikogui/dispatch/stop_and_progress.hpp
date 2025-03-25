#pragma once

#include <mutex>
#include <string>
#include "../macros.hpp"
#include "notifier.hpp"

hi_export_module(hikogui.dispatch : stop_and_progress);

hi_export namespace hi::inline v1 {
namespace detail {

class stop_and_progress {
public:
    using callback_type = notifier<void()>::callback_type;

    stop_and_progress() noexcept = default;

    [[nodiscard]] size_t token_count() const noexcept
    {
        auto const lock = std::scoped_lock(_mutex);

        return _token_count;
    }

    void increment_token_count() noexcept
    {
        auto const lock = std::scoped_lock(_mutex);

        ++_token_count;
    }

    void decrement_token_count() noexcept
    {
        auto const lock = std::scoped_lock(_mutex);

        assert(_token_count > 0);
        if (--_token_count == 0) {
            _notify_sources();
        }
    }

    [[nodiscard]] bool stop_requested() const noexcept
    {
        auto const lock = std::scoped_lock(_mutex);

        return _stop_requested;
    }

    void request_stop() noexcept
    {
        auto const lock = std::scoped_lock(_mutex);

        if (not std::exchange(_stop_requested, true)) {
            _notify_tokens();
        }
    }

    [[nodiscard]] std::pair<double, std::string> major_progress() const noexcept
    {
        auto const lock = std::scoped_lock(_mutex);

        return {_major_progress, _major_message};
    }

    [[nodiscard]] std::pair<double, std::string> minor_progress() const noexcept
    {
        auto const lock = std::scoped_lock(_mutex);

        return {_minor_progress, _minor_message};
    }

    void set_progress(
        double major_progress,
        std::string_view major_message,
        double minor_progress,
        std::string_view minor_message) noexcept
    {
        auto const lock = std::scoped_lock(_mutex);

        _major_progress = major_progress;
        _major_message = major_message;
        _minor_progress = minor_progress;
        _minor_message = minor_message;
        _notify_sources();
    }

    void set_major_progress(double progress) noexcept
    {
        auto const lock = std::scoped_lock(_mutex);

        _major_progress = progress;
        _notify_sources();
    }

    void set_major_progress(double progress, std::string_view message) noexcept
    {
        auto const lock = std::scoped_lock(_mutex);

        _major_progress = progress;
        _major_message = message;
        _notify_sources();
    }

    void set_minor_progress(double progress) noexcept
    {
        auto const lock = std::scoped_lock(_mutex);

        _minor_progress = progress;
        _notify_sources();
    }

    void set_minor_progress(double progress, std::string_view message) noexcept
    {
        auto const lock = std::scoped_lock(_mutex);

        _minor_progress = progress;
        _minor_message = message;
        _notify_sources();
    }

    template<typename F>
    [[nodiscard]] callback_type subscribe_sources(F&& f) noexcept
    {
        return _notify_sources.subscribe(std::forward<F>(f));
    }

    template<typename F>
    [[nodiscard]] callback_type subscribe_tokens(F&& f) noexcept
    {
        return _notify_tokens.subscribe(std::forward<F>(f));
    }

private:
    mutable std::mutex _mutex;

    size_t _token_count = 0;

    bool _stop_requested = false;

    double _major_progress = 0.0;
    std::string _major_message = {};
    double _minor_progress = 0.0;
    std::string _minor_message = {};

    notifier<void()> _notify_sources;
    notifier<void()> _notify_tokens;

    friend class stop_and_progress_source;
    friend class stop_and_progress_token;
    friend class stop_and_progress_callback;
};

} // namespace detail
}
