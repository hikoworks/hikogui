
#pragma once

#include "../macros.hpp"
#include <mutex>

hi_export_module(hikogui.container : resource_pool);

hi_export namespace hi::inline v1 {

/** A resource pool.
 *
 * This class is a thread-safe pool of resource objects.
 * 
 * A thread would pop a resource from the pool to use the resource and then
 * push it back when it is done.
 * 
 * @tparam T The type of the resource.
 */
template<typename T>
class resource_pool {
public:
    using value_type = T;

    constexpr resource_pool() = default;
    resource_pool(resource_pool const&) = delete;
    resource_pool& operator=(resource_pool const&) = delete;

    /** Emplace a resource into the pool.
     * 
     * This function is also used to initially add resources to the pool.
     * 
     * @tparam Args The types of the arguments to pass to the constructor of the resource.
     * @param args The arguments to pass to the constructor of the resource.
     * @return A reference to the resource object in the pool.
     */
    template<typename... Args>
    value_type& emplace(Args&&... args)
    {
        _mutex.lock();
        auto& ref = _stack.emplace_back(std::forward<Args>(args)...);
        _mutex.unlock();
        _condition.notify_one();
        return ref;
    }

    /** Push a resource into the pool
     * 
     * This function is also used to initially add resources to the pool.
     * 
     * @param value The resource object to move into the pool.
     */
    void push(value_type&& value)
    {
        emplace(std::move(value));
    }

    /** Push a resource into the pool
     * 
     * This function is also used to initially add resources to the pool.
     * 
     * @param value The resource object to copy into the pool.
     */
    void push(value_type const& value)
    {
        emplace(value);
    }

    /** Try to pop a resource from the pool.
     * 
     * This function will not block if there are no resources in the pool.
     * 
     * @return The resource object moved from the pool; or std::nullopt if the pool is empty.
     */
    [[nodiscard]] std::optional<value_type> try_pop()
    {
        auto const _ = std::scoped_lock(_mutex);

        if (_stack.empty()) {
            return std::nullopt;
        } else {
            auto value = std::move(_stack.back());
            _stack.pop_back();
            return {std::move(value)};
        }
    }

    /** Pop a resource from the pool.
     * 
     * This function will block until a resource is available.
     * 
     * @return The resource object moved from the pool.
     */
    [[nodiscard]] value_type pop()
    {
        auto lock = std::unique_lock(_mutex);
        _condition.wait(lock, [&] { return not _stack.empty(); });

        auto value = std::move(_stack.back());
        _stack.pop_back();
        return value;
    }

private:
    std::vector<value_type> _stack;
    mutable std::mutex _mutex;
    mutable std::condition_variable _condition;
};

} // namespace hi::inline v1
