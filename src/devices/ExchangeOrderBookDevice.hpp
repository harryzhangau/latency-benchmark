#pragma once

#include "base/Device.hpp"
#include "base/Pad.hpp"
#include "base/Task.hpp"
#include "exchange/ExchangeOrderBook.hpp"
#include "exchange/OrderMessage.hpp"

namespace lm
{

class ExchangeOrderBookDevice : public Device, public Task<ExchangeOrderBookDevice>
{
public:
    ExchangeOrderBookDevice(std::atomic_bool& keepRunning,
                            size_t maxProcessingCount = std::numeric_limits<size_t>::max())
        : Device(),
          Task<ExchangeOrderBookDevice>(1, keepRunning),
          maxProcessingCount(maxProcessingCount)
    {
        // Passive
        input = std::make_unique<Pad<OrderMessage&>>(PadType::ACTIVE);
        this->registerPad("input", input.get());

        beforeProcessing = std::make_unique<Pad<std::monostate>>(PadType::ACTIVE);
        afterProcessing = std::make_unique<Pad<size_t>>(PadType::ACTIVE);

        this->registerPad("before_processing", beforeProcessing.get());
        this->registerPad("after_processing", afterProcessing.get());
    }

    void run(std::stop_token stoken, std::atomic_bool& keepRunning)
    {
        orderBook = std::make_unique<ExchangeOrderBook>();

        size_t counter = maxProcessingCount;
        while (!stoken.stop_requested() && keepRunning.load(std::memory_order_relaxed) && counter != 0)
        {
            OrderMessage order;
            if (input->trigger(order))
            {
                (void)beforeProcessing->trigger(std::monostate());
                orderBook->processOrderMessage(order);
                (void)afterProcessing->trigger(orderBook->size());

                counter--;
            }
        }

        orderBook.reset();
    }

private:
    std::unique_ptr<ExchangeOrderBook> orderBook;

    std::unique_ptr<Pad<OrderMessage&>> input;
    std::unique_ptr<Pad<std::monostate>> beforeProcessing;
    std::unique_ptr<Pad<size_t>> afterProcessing;

    const size_t maxProcessingCount;
};

} // namespace lm