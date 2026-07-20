#pragma once

#include "base/Device.hpp"
#include "base/Pad.hpp"
#include "common/SpscMessageBus.hpp"

namespace lm
{

template <typename T> class MessageBusDevice : public Device
{
public:
    MessageBusDevice()
    {
        // Passive
        input = std::make_unique<Pad<const T&>>(PadType::PASSIVE);
        input->setCallback(
            this, +[](void* delegate, CallbackStorage<bool, const T&>* pad, const T& msg) {
                return static_cast<MessageBusDevice<T>*>(delegate)->bus.send(msg);
            });

        this->registerPad("input", input.get());

        // Passive
        output = std::make_unique<Pad<T&>>(PadType::PASSIVE);
        output->setCallback(
            this, +[](void* delegate, CallbackStorage<bool, T&>* pad, T& msg) {
                return static_cast<MessageBusDevice<T>*>(delegate)->bus.tryRead(msg);
            });

        this->registerPad("output", output.get());
    }

private:
    SpscMessageBus<T> bus;

    std::unique_ptr<Pad<const T&>> input;
    std::unique_ptr<Pad<T&>> output;
};

} // namespace lm