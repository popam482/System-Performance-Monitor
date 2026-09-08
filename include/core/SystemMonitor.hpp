#pragma once

#include "models/SystemSnapshot.hpp"
#include "collectors/ICpuCollector.hpp"
#include "collectors/IMemoryCollector.hpp"
#include "collectors/IGpuCollector.hpp"
#include "collectors/IDiskCollector.hpp"

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

public:
    SystemMonitor();
    ~SystemMonitor();

    void start();
    void stop();

    SystemSnapshot getLatestSnapshot() const;
};