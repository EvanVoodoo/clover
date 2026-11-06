#pragma once
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>

#ifndef TIMING_ENABLED
#define TIMING_ENABLED 1   // set to 0 to compile every macro out
#endif

class TimingRegistry
{
public:
    static TimingRegistry& Get()
    {
        static TimingRegistry instance;
        return instance;
    }

    void Start(const char* name)
    {
        Entry& e = mEntries[name];        // map lookup happens BEFORE the clock read
        e.running = true;
        e.start = Clock::now();
    }

    void End(const char* name)
    {
        const auto now = Clock::now();    // clock read happens BEFORE the map lookup
        Entry& e = mEntries[name];
        if (!e.running) return;           // ignore End without a matching Start
        e.running = false;
        e.totalMicros += std::chrono::duration<double, std::micro>(now - e.start).count();
        ++e.samples;
    }

    void Dump(const char* name, const std::string& label, const char* path = "timing.csv")
    {
        Entry& e = mEntries[name];
        if (e.samples == 0) return;

        const bool newFile = !std::filesystem::exists(path);
        std::ofstream file(path, std::ios::app);
        if (newFile) file << "label,avg_us,samples\n";
        file << label << "," << (e.totalMicros / e.samples) << "," << e.samples << "\n";

        e.totalMicros = 0.0;              // reset so the next configuration starts clean
        e.samples = 0;
    }

private:
    using Clock = std::chrono::high_resolution_clock;

    struct Entry
    {
        Clock::time_point start{};
        double totalMicros = 0.0;
        unsigned long long samples = 0;
        bool running = false;
    };

    std::unordered_map<std::string, Entry> mEntries;
};

#if TIMING_ENABLED
#define TIMING_START(name)        TimingRegistry::Get().Start(name)
#define TIMING_END(name)          TimingRegistry::Get().End(name)
#define TIMING_DUMP(name, label)  TimingRegistry::Get().Dump(name, label)
#else
#define TIMING_START(name)        ((void)0)
#define TIMING_END(name)          ((void)0)
#define TIMING_DUMP(name, label)  ((void)0)
#endif