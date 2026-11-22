#pragma once

#include "models/SystemSnapshot.hpp"
#include "collectors/ICpuCollector.hpp"
#include "collectors/IMemoryCollector.hpp"
#include "collectors/IGpuCollector.hpp"
#include "collectors/IDiskCollector.hpp"

#include "core/ISnapshotSink.hpp"

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

    std::unique_ptr<ISnapshotSink> snapshotSink;
    std::atomic<bool> loggingEnabled{false};

public:
    // Ownership is transferred to the monitor. Null collectors represent
    // unavailable devices/platform backends; a null sink disables logging.
    SystemMonitor(std::unique_ptr<ICpuCollector> cpu,
                  std::unique_ptr<IMemoryCollector> memory,
                  std::unique_ptr<IGpuCollector> gpu,
                  std::unique_ptr<IDiskCollector> disk,
                  std::unique_ptr<ISnapshotSink> sink = nullptr);
    ~SystemMonitor();

    void start();
    void stop();
    void setIntervalMs(int ms);
    int getIntervalMs() const;

    void enableLogging(bool enable);
    bool isLoggingEnabled() const;

    SystemSnapshot getLatestSnapshot() const;

    // Collect on the calling thread (used by the live dashboard).
    SystemSnapshot sampleOnce();
};
