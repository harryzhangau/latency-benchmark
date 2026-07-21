#pragma once

#include <atomic>

#include "CircularQueue.hpp"

namespace lm
{

template <typename T, size_t N = 10'1000> class SpscMessageBus
{
public:
    bool send(const T& message) noexcept
    {
        return queue.push(message);
    }

    bool tryRead(T& message) noexcept
    {
        auto result = queue.tryPop();
        if (result != nullptr)
        {
            message = *result;
            return true;
        }

        return false;
    }

private:
    CircularQueue<T, N> queue;
};
} // namespace lm