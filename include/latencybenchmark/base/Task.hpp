#pragma once

#include <future>
#include <thread>

namespace lm
{
template <typename Derived> class Task
{
public:
    Task(std::optional<int> coreId, std::atomic_bool& keepRunning)
        : coreId(coreId),
          keepRunning(keepRunning)
    {
    }

    std::future<void> start()
    {
        std::promise<void> complete_promise;
        std::future<void> complete_future = complete_promise.get_future();

        thread = std::jthread(
            [this](std::stop_token stoken, std::promise<void> complete_promise) {
                static_cast<Derived*>(this)->run(stoken, keepRunning);

                complete_promise.set_value();
            },
            std::move(complete_promise));

        if (coreId)
        {
            pinToCpuCore(thread, *coreId);
        }

        return complete_future;
    }

    void stop()
    {
        thread.request_stop();
        thread.join();
    }

private:
    std::optional<int> coreId;
    std::jthread thread;

    std::atomic_bool& keepRunning;
};

} // namespace lm
