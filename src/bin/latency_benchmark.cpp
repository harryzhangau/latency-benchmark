
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

template <typename... T> void runBenchmark(std::atomic_bool& keepRunnng)
{
    (T()(keepRunning), ...);
}

int main()
{
    std::signal(SIGINT, signalHandler);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%t] [%l] %v");
    SPDLOG_INFO("Running benchmark. Press Ctrl+C to exit...");

    runBenchmark<Percentile, Throughput>(keepRunning);

    SPDLOG_INFO("Finished benchmark.");
}
