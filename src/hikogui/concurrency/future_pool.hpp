

#pragma once

#include "../macros.hpp"
#include <future>
#include <vector>

hi_export_module(hikogui.concurrency : future_pool);

hi_export namespace hi::inline v1 {

/** A pool of futures.
 * This class is used to keep track of futures that are not yet ready.
 * When a future is ready it is removed from the pool, on the next call to add().
 */
template<typename T>
class future_pool {
public:
    using future_type = std::future<T>;

    constexpr future_pool() = default;

    /** Take ownership of a future.
     * 
     * @param cb The future to take ownership of.
     */
    void add(future_type&& cb)
    {
        cleanup();
        _list.push_back(std::move(cb));
    }

    /** Wait for all futures to be ready.
     */
    void wait()
    {
        for (auto const &f : _list) {
            f.wait();
        }
        _list.clear();
    }

private:
    std::vector<future_type> _list;

    void cleanup()
    {
        std::erase_if(_list, [](auto const &f) {
            using namespace std::chrono_literals;
            
            if (not f.valid()) {
                return true;
            }
            return f.wait_for(0ms) != std::future_status::timeout;
        });
    }
};

}

