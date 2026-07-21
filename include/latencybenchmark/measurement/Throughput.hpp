#pragma once

#include <chrono>
#include <iostream>
#include <stdint.h>

#include "../base/Utils.hpp"

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

} // namespace lm
