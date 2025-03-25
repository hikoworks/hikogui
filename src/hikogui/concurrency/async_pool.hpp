

#pragma once

#include "thread.hpp"
#include "../container/container.hpp"
#include "../macros.hpp"
#include <future>

hi_export_module(hikogui.concurrency : async_pool);

hi_export namespace hi::inline v1 {
namespace detail {
/** A pool of threads which will execute given tasks.
 *
 */
class async_pool {
public:
    ~async_pool()
    {
        _stop_source.request_stop();
        _condition.notify_all();

        while (true) {
            auto thread = [this] {
                auto const _ = std::scoped_lock(_mutex);
                if (_threads.empty()) {
                    return std::jthread();
                }

                auto thread = std::move(_threads.back());
                _threads.pop_back();
                return thread;
            }();

            if (not thread.joinable()) {
                break;
            }

            thread.join();
        }
    }

    async_pool() = default;

    /** Asynchronously execute a function on the pool.
     *
     * @tparam Func The type of the function to execute.
     * @tparam Args The types of the arguments to pass to the function.
     * @param func The function to execute.
     * @param args The arguments to pass to the function.
     * @return A future that will be ready when the function has been executed.
     */
    template<typename Func, typename... Args>
    std::future<std::invoke_result_t<Func, Args...>> async(Func&& func, Args&&... args)
    {
        using invoke_result = std::invoke_result_t<Func, Args...>;

        // std::function can not be used with move-only types, so we use a
        // shared_ptr to a promise instead.
        auto promise = std::make_shared<std::promise<invoke_result>>();
        auto future = promise->get_future();

        {
            auto const _ = std::scoped_lock(_mutex);
            _fifo.push_back(
                [promise = std::move(promise), func = std::forward<Func>(func), ... args = std::forward<Args>(args)]() mutable {
                    if constexpr (std::is_void_v<invoke_result>) {
                        func(args...);
                        promise->set_value();
                    } else {
                        promise->set_value(func(args...));
                    }
                });
        }
        _condition.notify_one();

        initialize_thread_pool();
        return future;
    }

private:
    mutable std::mutex _mutex;
    mutable std::condition_variable _condition;
    fifo<std::function<void()>> _fifo;
    std::vector<std::jthread> _threads;
    std::stop_source _stop_source;

    void worker_thread(std::stop_token stop_token, size_t worker_index)
    {
        set_thread_name(std::format("async_worker{}", worker_index));

        auto lock = std::unique_lock(_mutex);
        while (not _fifo.empty() or not stop_token.stop_requested()) {
            _condition.wait(lock, [&] {
                return not _fifo.empty() or stop_token.stop_requested();
            });

            if (not _fifo.empty()) {
                auto task = std::move(_fifo.front());
                _fifo.pop_front();
                lock.unlock();

                // Don't hold the lock while we are executing the task.
                // This allows other worker threads to pick up others tasks.
                task();

                lock.lock();
            }
        }
    }

    void initialize_thread_pool()
    {
        static auto const num_concurrent_threads = [] {
            size_t r = std::thread::hardware_concurrency();
            if (r == 0) {
                r = 1;
            } else if (r > 1) {
                r -= 1;
            }
            return r;
        }();

        auto const _ = std::scoped_lock(_mutex);
        auto const num_tasks = _fifo.size();
        auto const num_concurrent_tasks = std::min(num_tasks, num_concurrent_threads);
        if (num_concurrent_tasks <= _threads.size()) {
            return;
        }

        _threads.emplace_back(
            [this](std::stop_token stop_token, size_t worker_index) {
                this->worker_thread(stop_token, worker_index);
            },
            _stop_source.get_token(),
            _threads.size());
    }
};

inline async_pool global_async_pool;

} // namespace detail

template<typename Func, typename... Args>
std::future<std::invoke_result_t<Func, Args...>> async_on_pool(Func&& func, Args&&... args)
{
    return detail::global_async_pool.async(std::forward<Func>(func), std::forward<Args>(args)...);
}
}
