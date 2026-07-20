#pragma once

#include "Pad.hpp"
#include <any>
#include <variant>

namespace lm
{
class Device
{
public:
    template <typename U> using PadPtrVariant = typename Pad<U>::PadPtrVariant;

    template <typename P> void registerPad(const std::string& name, P pad)
    {
        using NoRefType = std::remove_cvref_t<P>;
        using RawType = std::remove_pointer_t<NoRefType>;

        pads.insert({name, PadPtrVariant<typename RawType::U>(pad)});
    }

    template <typename U> PadPtrVariant<U> getPad(const std::string& name)
    {
        if (pads.find(name) != pads.end())
            return std::any_cast<PadPtrVariant<U>>(pads[name]);

        return {};
    }

private:
    std::unordered_map<std::string, std::any> pads;
};

template <typename T>
bool connect(Device& srcDevice, const std::string& srcPadName, Device& dstDevice, const std::string& dstPadName)
{
    auto src = srcDevice.getPad<T>(srcPadName);
    auto dst = dstDevice.getPad<T>(dstPadName);

    return connect<T>(src, dst);
}

}; // namespace lm