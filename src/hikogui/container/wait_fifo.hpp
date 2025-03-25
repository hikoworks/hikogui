
#pragma once

hi_export_module(hikogui.container : wait_fifo);

hi_export namespace hi::inline v1 {


template<typename T, typename Allocator = std::allocator<T>>
class wait_fifo {
public:
    using value_type = T;
    using allocator_type = Allocator;

    constexpr wait_fifo() = defaul;

    template<typename... Args>
    value_type &emplace_back(Args&&... args)
    {
        auto const _ = std::scoped_lock(_mutex);
        assert(not _stop_requested);

        auto& ref = _fifo.emplace_back(std::forward<Args>(args)...);
        _block_counter.fetch_add(1, std::memory_order::release);
        _block_counter.notify_one();
        return ref;
    }

    void push_back(value_type const& value)
    {
        emplace_back(value);
    }

    void push_back(value_type&& value)
    {
        emplace_back(std::move(value));
    }

    void request_stop()
    {
        auto const _ = std::scoped_lock(_mutex);
        _stop_requested = true;
        _block_counter.fetch_add(1, std::memory_order::relaxed);
        _block_counter.notify_one();
    }

    /** Pop a value from the fifo.
     *
     * This function blocks until a value is available.
     *
     * @return The value moved from the fifo.
     * @retval std::nullopt There are no more values in the fifo and
     *         request_stop() was called.
     */
    [[nodiscard]] std::optional<value_type> pop_front()
    {
        while (true) {
            _mutex.lock();
            if (not _fifo.empty()) {
                auto value = std::move(_fifo.front());
                _fifo.pop_front();
                return {std::move(value)};

            } else if (_stop_requested) {
                // Even if stop requested all values must be popped first.
                return std::nullopt;
            }

            // A push_back() can't happen right now as the fifo is locked.
            // Read the counter to check if a push_back() happened after unlocking.
            auto const counter = block_counter.load(std::memory_order::relaxed);
            _mutex.unlock();

            // Block without holding the mutex.
            // If the counter was changed then a push_back() happened.
            block_counter.wait(counter, std::memory_order::relaxed);
        }
    }

private:
    bool _stop_requested = false;
    fifo<T, Allocator> _fifo;
    std::mutex _mutex;
    std::atomic<atomic_unsigned_lock_free> _block_counter;
}


}

