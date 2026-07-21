#pragma once

#include <chrono>
#include <stdint.h>

#include "../base/Device.hpp"
#include "../base/Utils.hpp"

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

    uint64_t getPercentile(unsigned nth)
    {
        return calcPercentile(processingTimeNs, nth);
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

} // namespace lm