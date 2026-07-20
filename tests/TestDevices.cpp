#include <future>
#include <gtest/gtest.h>
#include <thread>

#include "base/Device.hpp"
#include "base/Pad.hpp"
#include "devices/MessageBusDevice.hpp"

using namespace lm;

struct TestDevice1 : public Device
{
    TestDevice1()
    {
        // Active, src
        output = std::make_unique<Pad<const uint8_t&>>(PadType::ACTIVE);
        registerPad("output", output.get());
    }

    void start()
    {
        for (auto i = 0uz; i < 10uz; i++)
        {
            output->trigger(i);
        }
    }

    std::unique_ptr<Pad<const uint8_t&>> output;
};

struct TestDevice2 : public Device
{
    TestDevice2()
    {
        // Active, polling
        input = std::make_unique<Pad<uint8_t&>>(PadType::ACTIVE);
        registerPad("input", input.get());
    }

    std::string receive()
    {
        std::stringstream ss;
        size_t count = 0;
        while (count++ < 10)
        {
            uint8_t data = 0;
            if (input->trigger(data))
            {
                ss << int(data);
            }
        }

        return ss.str();
    }

    std::unique_ptr<Pad<uint8_t&>> input;
};

TEST(TestDevices, testPad)
{
    TestDevice1 device1;
    TestDevice2 device2;

    MessageBusDevice<uint8_t> bus;

    ASSERT_TRUE(connect<uint8_t>(device1.getPad<uint8_t>("output"), bus.getPad<uint8_t>("input")));
    ASSERT_TRUE(connect<uint8_t>(device2.getPad<uint8_t>("input"), bus.getPad<uint8_t>("output")));

    std::string received;
    std::promise<void> completed;
    auto future = completed.get_future();

    auto thread = std::jthread([&device2, &received, &completed]() {
        received = device2.receive();
        completed.set_value();
    });

    device1.start();
    future.wait();

    ASSERT_EQ("0123456789", received);
}