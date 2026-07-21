#pragma once

#include <array>
#include <atomic>

namespace lm
{
template <typename T, size_t N> class CircularQueue
{
public:
    bool push(const T& value) noexcept
    {
        auto current_tail = tail.value.load(std::memory_order_acquire);
        auto current_head = head.value.load(std::memory_order_acquire);

        auto next_tail = (current_tail + 1) % N;
        if (next_tail == current_head)
        {
            // Queue is full
            return false;
        }

        queue[current_tail] = value;
        tail.value.store(next_tail, std::memory_order_release);

        return true;
    }

    const T* tryPop() noexcept
    {
        auto current_tail = tail.value.load(std::memory_order_acquire);
        auto current_head = head.value.load(std::memory_order_acquire);
        if (current_head == current_tail)
        {
            // Queue is empty
            return nullptr;
        }

        const auto* value = &queue[current_head];
        head.value.store((current_head + 1) % N, std::memory_order_release);

        return value;
    }

private:
    struct alignas(std::hardware_destructive_interference_size) PaddedAtomic
    {
        std::atomic<size_t> value = 0;
    };

    PaddedAtomic head = {0};
    PaddedAtomic tail = {0};

    std::array<T, N> queue;
};
} // namespace lm