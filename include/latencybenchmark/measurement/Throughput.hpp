#pragma once

#include <chrono>
#include <iostream>
#include <stdint.h>

#include "../base/Utils.hpp"

namespace lm
{

template <typename... PadArgs>
class ThroughputMeasurement : public Device, public CallbackStorage<void, uint64_t, PadArgs...>
{
public:
    ThroughputMeasurement(uint64_t reportInterval)
        : reportInterval(reportInterval == 0 ? 1 : reportInterval)
    {
        // Passive
        probe = std::make_unique<Pad<PadArgs...>>(PadType::PASSIVE);
        probe->setCallback(
            this, +[](void* delegate, CallbackStorage<bool, PadArgs...>* pad, PadArgs... args) {
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
                    self->triggerCallback(pace, std::forward<PadArgs>(args)...);

                    self->lastTime = now;
                }
                return true;
            });

        this->registerPad("probe", probe.get());
    }

private:
    CallbackStorage<void, uint64_t>* callback = nullptr;
    const uint64_t reportInterval = 1;

    std::unique_ptr<Pad<PadArgs...>> probe;

    uint64_t counter = 0;
    std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();
};

} // namespace lm
