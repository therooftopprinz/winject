#ifndef __BFC_METRIC_HPP__
#define __BFC_METRIC_HPP__

#include <atomic>
#include <memory>
#include <thread>
#include <mutex>
#include <fstream>
#include <filesystem>

#include "IMetric.hpp"

namespace bfc
{

class Metric : public IMetric
{
public:
    void store(double val) override
    {
        value = val;
    }
    double load() override
    {
        return value;
    }
    double fetch_add(double val) override
    {
        auto old = value.load();

        if (old == old+val)
        {
            return old;
        }

        while(value.compare_exchange_strong(old, old+val));
        return old;
    }
    double fetch_sub(double val) override
    {
        auto old = value.load();

        if (old == old-val)
        {
            return old;
        }

        while(value.compare_exchange_strong(old, old-val));
        return old;
    }
private:
    std::atomic<double> value = 0;
};

class Monitor : public IMonitor
{
public:
    Monitor() = delete;

    Monitor(size_t interval_ms=100, std::string path="metrics")
        : interval_ms(interval_ms)
        , path(path)

    {
        std::ofstream(path+".csv", std::fstream::out | std::fstream::trunc);

        monitor_thread = std::thread([this](){
                run_monitor();
            });
    }

    ~Monitor()
    {
        monitor_running = false;
        if (monitor_thread.joinable())
        {
            monitor_thread.join();
        }
    }

    IMetric& getMetric(const std::string& fullName) override
    {
        std::unique_lock lg(this_mutex);

        if (metrics.count(fullName))
        {
            return *metrics.at(fullName);
        }

        auto res = metrics.emplace(fullName, std::make_unique<Metric>());
        return *res.first->second.get();
    }

private:
    void run_monitor()
    {
        monitor_running = true;
        while (monitor_running)
        {
            auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch())
                .count();
            auto diff = now - last_update;
            if (diff > interval_ms)
            {
                last_update = now;
                collect();
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));   
        }
    }

    void collect()
    {
        std::ofstream fout(path+"_", std::fstream::out | std::fstream::trunc);
        std::ofstream fout2(path+".csv", std::fstream::out | std::fstream::app);
        std::unique_lock lg(this_mutex);
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        fout2 << now << ",";
        for (auto& [key, val] : metrics)
        {
            fout.precision(20);
            fout << key << " " << val->load() << "\n";
            fout2 << val->load() << ",";
        }
        fout2 << "\n";

        std::filesystem::rename(path+"_",path);
    }

    size_t interval_ms = 1000;
    std::string path = "metrics";
    int64_t last_update = 0;
    std::atomic_bool monitor_running = false;
    std::thread monitor_thread;
    std::mutex this_mutex;
    std::map<std::string, std::unique_ptr<IMetric>> metrics;
};

} // namespace bfc

#endif // __BFC_IMETRIC_HPP__