#include <argparse/argparse.hpp>
#include <atomic>
#include <csignal>
#include <spdlog/spdlog.h>

#include <latencybenchmark/measurement/Percentile.hpp>
#include <latencybenchmark/measurement/Throughput.hpp>
#include <orderbook/common/SpscMessageBus.hpp>
#include <orderbook/exchange/ExchangeOrderBook.hpp>

#include "devices/ExchangeOrderBookDevice.hpp"
#include "devices/MessageBusDevice.hpp"
#include "devices/OrderGeneratorDevice.hpp"

using namespace lm;

std::atomic<bool> keepRunning(true);

void signalHandler(int signal)
{
    if (signal == SIGINT)
    {
        keepRunning = false;
    }
}

void runPercentileBenchmark(std::atomic_bool& keepRunning)
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

    generator_future.wait();
    order_book->stop();
    orderbook_future.wait();

    for (auto nth : {99, 90, 50, 30})
    {
        SPDLOG_INFO("processed time {}th percentile: {}", nth, percentile_measurement->getPercentile(nth));
    }
}

void runThroughputBenchmark(std::atomic_bool& keepRunning)
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

    auto throughput_measurement = std::make_unique<ThroughputMeasurement<size_t>>(1'000'000ULL);
    throughput_measurement->setCallback(
        nullptr, +[](void* delegate, CallbackStorage<void, uint64_t, size_t>* measurement, uint64_t pace,
                     size_t arg) { std::cout << "> " << pace << " orders/second, sitting orders: " << arg << "\n"; });

    if (!connect<size_t>(*order_book, "after_processing", *throughput_measurement, "probe"))
    {
        SPDLOG_ERROR("Failed to connect order book to measurement probe");
        return;
    }

    auto orderbook_future = order_book->start();
    auto generator_future = order_generator->start();

    generator_future.wait();
    order_book->stop();
    orderbook_future.wait();
}

int main(int argc, char* argv[])
{
    argparse::ArgumentParser program("latency_benchmark");

    program.add_argument("-m", "--method")
        .default_value(std::string("all"))
        .choices("all", "percentile", "throughput")
        .help("all, percentile, throughput");

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& err)
    {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    std::signal(SIGINT, signalHandler);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%t] [%l] %v");
    SPDLOG_INFO("Running benchmark. Press Ctrl+C to exit...");

    auto method = program.get<std::string>("--method");

    if (method == std::string("percentile") || method == std::string("all"))
    {
        runPercentileBenchmark(keepRunning);
    }

    if (method == std::string("throughput") || method == std::string("all"))
    {
        runThroughputBenchmark(keepRunning);
    }

    SPDLOG_INFO("Finished benchmark.");
}
