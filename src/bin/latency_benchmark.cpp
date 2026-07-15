
#include <atomic>
#include <csignal>
#include <spdlog/spdlog.h>

#include "common/SpscMessageBus.hpp"
#include "exchange/ExchangeOrderBook.hpp"
#include "exchange/ItchMessage.hpp"
#include "methods/Percentile.hpp"
#include "methods/Throughput.hpp"

using namespace lm;

std::atomic<bool> keepRunning(true);

void signalHandler(int signal)
{
    if (signal == SIGINT)
    {
        keepRunning = false;
    }
}

template <template <typename> typename Method>
void benchmarkExchangeOrderBook(std::chrono::nanoseconds orderGenerationInterval)
{
    using ProcessorType = ExchangeOrderBook<SpscMessageBus<itch::ItchMessage>>;

    if (!keepRunning.load(std::memory_order_relaxed))
        return;

    auto message_bus = std::make_unique<SpscMessageBus<itch::ItchMessage>>();
    auto processor = std::make_unique<ProcessorType>(*message_bus);

    auto method = std::make_unique<Method<ProcessorType>>(*processor, orderGenerationInterval, keepRunning);

    SPDLOG_INFO("Running benchmark. Press Ctrl+C to exit...");
    method->run();
    SPDLOG_INFO("Finished benchmark.");
}

int main()
{
    std::signal(SIGINT, signalHandler);

    benchmarkExchangeOrderBook<Percentile>(std::chrono::milliseconds(1));
    benchmarkExchangeOrderBook<Throughput>(std::chrono::microseconds(1));
}
