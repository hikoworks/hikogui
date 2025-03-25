
#pragma once

#include "../container/container.hpp"
#include "../marcos.hpp"

namespace hi::inline v1 {
namespace detail {

struct delegated_task_base {
    virtual ~delegated_task_base() = default;

};

template<>
struct delegated_task : public delegated_task_base {

};

inline auto std::vector<std::jthread> delegated_task_thread_pool = {};
inline auto wait_fifo<std::shared_ptr<delegated_task_base> delegated_tasks = {};
inline auto std::mutex delegated_tasks_mutex;

[[nodiscard]] size_t delegated_task_thread_pool_max_size() noexcept
{
    auto num_threads = os_settings::num_local_processors();
    assert(num_threads > 0);
    --num_threads;

    return num_threads > 0 ? num_threads : size_t{1};
}

void delegated_task_thread(std::stop_token stop_token)
{
    set_thread_name("delegate-pool");

    while (not stop_token.stop_requested()) {
        if (auto task = delegated_task.pop_front()) {
            task.execute();
        } else {
            // The delegate_task fifo is empty and no more tasks will be added.
            return;
        }
    }
};

}

template<typename T>
class delegate_future {
public:
    delegate_future(std::shared_ptr<delegate_task_base> &&ptr) noexcept : _ptr(std::move(ptr));

    [[nodiscard]] bool completed() noexcept;
    [[nodiscard]] bool has_value() noexcept;
    [[nodiscard]] T &value() noexcept;
    void request_stop() noexcept;

private:
    std::shared_ptr<delegate_task_base> _ptr;
};

template<typename R, typename... Args>
delegated_future delegate(std::function<R(std::stop_token, Args...)> function)
{
    auto const _ = std::scoped_lock(detail::delegated_tasks_mutex);

    if (detail::delegated_task_thread_pool.size() < detail::delegated_task_thread_pool_max_size()) {
        delegated_task_thread_pool.emplace_back(delegated_task_thread);
    }


    auto tmp = std::make_shared<delegate_task<R, Func, Args...>>(
        std::forward<Func>(function), std::forward<Args>(args)...);

    detail::delegated_tasks.push_back(tmp);

    return delegate_future<R>{std::move(tmp)};
}

}
