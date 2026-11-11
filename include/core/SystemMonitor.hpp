#pragma once

#include "models/SystemSnapshot.hpp"
#include "collectors/ICpuCollector.hpp"
#include "collectors/IMemoryCollector.hpp"
#include "collectors/IGpuCollector.hpp"
#include "collectors/IDiskCollector.hpp"

#include "storage/CsvExport.hpp"

#include <thread>
#include <mutex>
#include <atomic>
#include <memory>

class SystemMonitor {
private:
    std::unique_ptr<ICpuCollector> cpuCollector;
    std::unique_ptr<IMemoryCollector> ramCollector;
    std::unique_ptr<IGpuCollector> gpuCollector;
    std::unique_ptr<IDiskCollector> diskCollector;

    SystemSnapshot currentSnapshot;
    mutable std::mutex dataMutex; // protects access to currentSnapshot
    std::atomic<bool> isRunning{false};

    std::thread workerThread;

    void collectionLoop(); // runs on the secondary thread

    std::atomic<int> sampleIntervalMs{1000};// default 1 second

    std::unique_ptr<CsvExport> csvLogger;
    std::atomic<bool> loggingEnabled{false};

public:
    SystemMonitor();
    ~SystemMonitor();

    void start();
    void stop();
    void setIntervalMs(int ms);
    int getIntervalMs() const;

    void enableLogging(bool enable, const std::string& filename = "system_metrics.csv");
    bool isLoggingEnabled() const;

    SystemSnapshot getLatestSnapshot() const;

    // Collect on the calling thread (used by the live dashboard).
    SystemSnapshot sampleOnce();
};