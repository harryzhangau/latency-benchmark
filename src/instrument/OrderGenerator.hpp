#pragma once

#include <future>
#include <generator>
#include <random>
#include <thread>

#include "../common/Log.hpp"
#include "../common/SpscMessageBus.hpp"
#include "../common/Utils.hpp"
#include "../exchange/OrderMessage.hpp"

namespace lm
{

class OrderGenerator
{
public:
    struct RandConfig
    {
        int seed;
        int low;
        int high;
    };

    OrderGenerator(int sideSeed, const RandConfig& quantityConfig, const RandConfig& priceConfig,
                   SpscMessageBus<OrderMessage>& orderBus,
                   std::chrono::nanoseconds interval = std::chrono::nanoseconds(0))
        : sideSeed(sideSeed),
          quantityConfig(quantityConfig),
          priceConfig(priceConfig),
          orderBus(orderBus),
          interval(interval)
    {
    }

    std::generator<OrderMessage> generate()
    {
        std::mt19937 side_device(sideSeed);
        std::mt19937 quantity_device(quantityConfig.seed);
        std::mt19937 price_device(priceConfig.seed);

        std::uniform_int_distribution<int> side_distrib(0, 1);
        std::uniform_int_distribution<int> quantity_distrib(quantityConfig.low, quantityConfig.high);
        std::uniform_int_distribution<int> price_distrib(priceConfig.low, priceConfig.high);

        uint64_t id = 0;

        while (true)
        {
            while ((id / 1'000'000) % 2 == 0)
            {
                AddOrder order;
                order.id = ++id;
                order.side = side_distrib(side_device) ? AddOrder::Side::BUY : AddOrder::Side::SELL;
                order.quantity = quantity_distrib(quantity_device);
                order.price = price_distrib(price_device);

                co_yield (order);
            }

            while ((id / 1'000'000) % 2 == 1)
            {
                DeleteOrder order;
                order.id = ++id - 1'000'000;

                co_yield (order);
            }
        }
    }

    std::future<void> start()
    {
        std::promise<void> complete_promise;
        std::future<void> complete_future = complete_promise.get_future();

        running.store(true, std::memory_order_release);

        thread = std::jthread([this](std::stop_token stoken,
                                     std::promise<void> complete_promise) { run(stoken, std::move(complete_promise)); },
                              std::move(complete_promise));

        pinToCpuCore(thread, 2);

        return complete_future;
    }

    void stop()
    {
        thread.request_stop();
        thread.join();

        running.store(true, std::memory_order_release);
    }

    void run(std::stop_token stoken, std::promise<void> complete_promise)
    {
        auto last = std::chrono::steady_clock::now();

        auto generator = generate();
        for (auto msg : generator)
        {
            if (stoken.stop_requested())
                break;

            if (!orderBus.send(msg))
            {
                LM_LIMIT_LOG_WARN(std::chrono::seconds(5), "Order message bus is full");
            }

            if (interval.count() > 0)
            {
                auto now = std::chrono::steady_clock::now();
                while (now - last < interval)
                {
                    now = std::chrono::steady_clock::now();
                }

                last = now;
            }
        }

        complete_promise.set_value();
    }

private:
    int sideSeed;
    RandConfig quantityConfig;
    RandConfig priceConfig;

    std::jthread thread;
    std::atomic<bool> running{false};

    SpscMessageBus<OrderMessage>& orderBus;
    std::chrono::nanoseconds interval;
};

} // namespace lm