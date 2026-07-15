#pragma once

#include "../exchange/OrderMessage.hpp"
#include "OrderGenerator.hpp"

namespace lm
{

template <typename M, typename T> struct Benchmark
{
    Benchmark(T& processor, std::chrono::nanoseconds interval, std::atomic<bool>& keepRunning)
        : processor(processor),
          interval(interval),
          keepRunning(keepRunning)
    {
    }

    void run()
    {
        pinCurrentToCpuCore(1);

        auto order_bus = std::make_unique<SpscMessageBus<OrderMessage>>();
        OrderGenerator order_generator(42, {50, 100, 200}, {67, 100, 1000}, *order_bus, interval);

        (void)order_generator.start();

        while (!static_cast<M*>(this)->isFinished())
        {
            OrderMessage order;
            if (order_bus->tryRead(order))
            {
                static_cast<M*>(this)->beforeOrderProcessing(processor);
                processor.processOrderMessage(order);
                static_cast<M*>(this)->afterOrderProcessing(processor);
            }

            if (!keepRunning.load(std::memory_order_relaxed))
            {
                break;
            }
        }

        order_generator.stop();

        static_cast<M*>(this)->complete();
    }

private:
    T& processor;
    std::chrono::nanoseconds interval;
    std::atomic<bool>& keepRunning;
};
} // namespace lm