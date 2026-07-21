#pragma once

#include <future>
#include <generator>
#include <random>
#include <thread>

#include <latencybenchmark/base/Device.hpp>
#include <latencybenchmark/base/Task.hpp>
#include <latencybenchmark/base/Utils.hpp>
#include <orderbook/common/Log.hpp>
#include <orderbook/common/SpscMessageBus.hpp>
#include <orderbook/exchange/OrderMessage.hpp>

namespace lm
{

class OrderGeneratorDevice : public Device, public Task<OrderGeneratorDevice>
{
public:
    struct RandConfig
    {
        int seed;
        int low;
        int high;
    };

    OrderGeneratorDevice(int sideSeed, const RandConfig& quantityConfig, const RandConfig& priceConfig,
                         std::chrono::nanoseconds interval, std::atomic_bool& keepRunning,
                         size_t maxProcessingCount = std::numeric_limits<size_t>::max())
        : Device(),
          Task<OrderGeneratorDevice>(2, keepRunning),
          sideSeed(sideSeed),
          quantityConfig(quantityConfig),
          priceConfig(priceConfig),
          interval(interval),
          maxProcessingCount(maxProcessingCount)
    {
        output = std::make_unique<Pad<const OrderMessage&>>(PadType::ACTIVE);
        registerPad("output", output.get());
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

    void run(std::stop_token stoken, std::atomic_bool& keepRunning)
    {
        auto last = std::chrono::steady_clock::now();
        size_t counter = maxProcessingCount;
        auto generator = generate();
        for (auto msg : generator)
        {
            if (stoken.stop_requested() || !keepRunning.load(std::memory_order_relaxed) || counter == 0)
                break;

            if (!output->trigger(msg))
            {
                LM_LIMIT_LOG_WARN(std::chrono::seconds(5), "Order message bus is full");
            }

            counter--;

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
    }

private:
    int sideSeed;
    RandConfig quantityConfig;
    RandConfig priceConfig;
    std::chrono::nanoseconds interval;
    const size_t maxProcessingCount;

    std::unique_ptr<Pad<const OrderMessage&>> output;
};

} // namespace lm