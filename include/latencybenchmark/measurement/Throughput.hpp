#pragma once

#include <chrono>
#include <iostream>
#include <stdint.h>

#include "../base/Utils.hpp"

namespace lm
{

template <typename T> class ThroughputMeasurement : public Device, public CallbackStorage<void, uint64_t, T>
{
public:
    ThroughputMeasurement(uint64_t reportInterval)
        : reportInterval(reportInterval == 0 ? 1 : reportInterval)
    {
        // Passive
        probe = std::make_unique<Pad<T>>(PadType::PASSIVE);
        probe->setCallback(
            this, +[](void* delegate, CallbackStorage<bool, T>* pad, T arg) {
                auto self = static_cast<ThroughputMeasurement*>(delegate);

                if (self->counter == 0)
                {
                    self->lastTime = std::chrono::steady_clock::now();
                }

                if (++self->counter % self->reportInterval == 0)
                {
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - self->lastTime).count();

                    unsigned pace = self->reportInterval * 1000ULL / elapsed;
                    self->triggerCallback(pace, arg);

                    self->lastTime = now;
                }
                return true;
            });

        this->registerPad("probe", probe.get());
    }

private:
    CallbackStorage<void, uint64_t>* callback = nullptr;
    const uint64_t reportInterval = 1;

    std::unique_ptr<Pad<T>> probe;

    uint64_t counter = 0;
    std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();
};

} // namespace lm
