#pragma once

#include <chrono>
#include <spdlog/spdlog.h>
#include <stdint.h>

#include "common/Utils.hpp"
#include <spdlog/spdlog.h>

#include "devices/ExchangeOrderBookDevice.hpp"
#include "devices/MessageBusDevice.hpp"
#include "devices/OrderGeneratorDevice.hpp"

#include <iostream>

namespace lm
{
class ThroughputMeasurement : public Device
{
public:
    ThroughputMeasurement()
    {
        // Passive
        probe = std::make_unique<Pad<size_t>>(PadType::PASSIVE);
        probe->setCallback(
            this, +[](void* delegate, CallbackStorage<bool, size_t>* pad, size_t orderBookSize) {
                auto self = static_cast<ThroughputMeasurement*>(delegate);

                if (self->counter == 0)
                {
                    self->lastTime = std::chrono::steady_clock::now();
                }

                if (++self->counter % ThroughputMeasurement::REPORT_INTERVAL == 0)
                {
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - self->lastTime).count();

                    unsigned ordersPerSecond = REPORT_INTERVAL * 1000ULL / elapsed;
                    std::cout << "> " << ordersPerSecond << " orders/second, sitting orders: " << orderBookSize << "\n";

                    self->lastTime = now;
                }
                return true;
            });

        this->registerPad("probe", probe.get());
    }

private:
    std::unique_ptr<Pad<size_t>> probe;

    size_t counter = 0;
    std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();

    static constexpr size_t REPORT_INTERVAL = 1'000'000ULL;
};

struct Throughput
{
    void operator()(std::atomic_bool& keepRunning)
    {
        SPDLOG_INFO("----------- Throughput -----------");

        constexpr size_t MAX_ORDER_MESSAGES = 10'000'000;
        auto order_bus = std::make_unique<MessageBusDevice<OrderMessage>>();
        auto order_generator = std::make_unique<OrderGeneratorDevice>(
            42, OrderGeneratorDevice::RandConfig{50, 100, 200}, OrderGeneratorDevice::RandConfig{67, 100, 1000},
            std::chrono::milliseconds(0), keepRunning, MAX_ORDER_MESSAGES);
        auto order_book = std::make_unique<ExchangeOrderBookDevice>(keepRunning, MAX_ORDER_MESSAGES);

        if (!connect<OrderMessage>(*order_generator, "output", *order_bus, "input"))
        {
            SPDLOG_ERROR("Failed to connect order generator to order bus");
            return;
        }

        if (!connect<OrderMessage>(*order_book, "input", *order_bus, "output"))
        {
            SPDLOG_ERROR("Failed to connect order book to order bus");
            return;
        }

        auto throughput_measurement = std::make_unique<ThroughputMeasurement>();
        if (!connect<size_t>(*order_book, "after_processing", *throughput_measurement, "probe"))
        {
            SPDLOG_ERROR("Failed to connect order book to measurement probe");
            return;
        }

        auto orderbook_future = order_book->start();
        auto generator_future = order_generator->start();

        orderbook_future.wait();
        generator_future.wait();
    }
};

} // namespace lm