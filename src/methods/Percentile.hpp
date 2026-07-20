#pragma once

#include <chrono>
#include <spdlog/spdlog.h>
#include <stdint.h>

#include "common/Utils.hpp"
#include <spdlog/spdlog.h>

#include "devices/ExchangeOrderBookDevice.hpp"
#include "devices/MessageBusDevice.hpp"
#include "devices/OrderGeneratorDevice.hpp"

namespace lm
{
class PercentileMeasurement : public Device
{
public:
    PercentileMeasurement()
    {
        // Passive
        start = std::make_unique<Pad<std::monostate>>(PadType::PASSIVE);
        start->setCallback(
            this, +[](void* delegate, CallbackStorage<bool, std::monostate>* pad, std::monostate) {
                auto self = static_cast<PercentileMeasurement*>(delegate);
                self->startTime = std::chrono::steady_clock::now();
                return true;
            });

        this->registerPad("start", start.get());

        // Passive
        end = std::make_unique<Pad<size_t>>(PadType::PASSIVE);
        end->setCallback(
            this, +[](void* delegate, CallbackStorage<bool, size_t>* pad, size_t) {
                auto self = static_cast<PercentileMeasurement*>(delegate);

                auto finish_time = std::chrono::steady_clock::now();

                self->processingTimeNs[self->counter & MASK] =
                    std::chrono::duration_cast<std::chrono::nanoseconds>(finish_time - self->startTime).count();

                self->counter++;
                return true;
            });

        this->registerPad("end", end.get());
    }

    void logPercentile()
    {
        SPDLOG_INFO("processed time 99th percentile: {} ns", calcPercentile(processingTimeNs, 99));
        SPDLOG_INFO("processed time 90th percentile: {} ns", calcPercentile(processingTimeNs, 90));
        SPDLOG_INFO("processed time 50th percentile: {} ns", calcPercentile(processingTimeNs, 50));
        SPDLOG_INFO("processed time 30th percentile: {} ns", calcPercentile(processingTimeNs, 30));
    }

private:
    std::unique_ptr<Pad<std::monostate>> start;
    std::unique_ptr<Pad<size_t>> end;

    static constexpr size_t BUFFER_SIZE = 1ULL << 12;
    static constexpr size_t MASK = BUFFER_SIZE - 1;

    std::chrono::steady_clock::time_point startTime;
    std::vector<uint64_t> processingTimeNs = std::vector<uint64_t>(BUFFER_SIZE);
    size_t counter = 0;
};

struct Percentile
{
    void operator()(std::atomic_bool& keepRunning)
    {
        SPDLOG_INFO("----------- Percentile -----------");

        constexpr size_t MAX_ORDER_MESSAGES = 10'000;
        auto order_bus = std::make_unique<MessageBusDevice<OrderMessage>>();
        auto order_generator = std::make_unique<OrderGeneratorDevice>(
            42, OrderGeneratorDevice::RandConfig{50, 100, 200}, OrderGeneratorDevice::RandConfig{67, 100, 1000},
            std::chrono::milliseconds(1), keepRunning, MAX_ORDER_MESSAGES);
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

        auto percentile_measurement = std::make_unique<PercentileMeasurement>();
        if (!connect<std::monostate>(*order_book, "before_processing", *percentile_measurement, "start"))
        {
            SPDLOG_ERROR("Failed to connect order book to measurement start");
            return;
        }

        if (!connect<size_t>(*order_book, "after_processing", *percentile_measurement, "end"))
        {
            SPDLOG_ERROR("Failed to connect order book to measurement end");
            return;
        }

        auto orderbook_future = order_book->start();
        auto generator_future = order_generator->start();

        orderbook_future.wait();
        generator_future.wait();

        percentile_measurement->logPercentile();
    }
};

} // namespace lm