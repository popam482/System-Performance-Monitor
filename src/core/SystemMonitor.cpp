#include "core/SystemMonitor.hpp"

#include <chrono>
#include <utility>

SystemMonitor::SystemMonitor(std::unique_ptr<ICpuCollector> cpu,
                             std::unique_ptr<IMemoryCollector> memory,
                             std::unique_ptr<IGpuCollector> gpu,
                             std::unique_ptr<IDiskCollector> disk,
                             std::unique_ptr<ISnapshotSink> sink)
    : cpuCollector(std::move(cpu)), ramCollector(std::move(memory)),
      gpuCollector(std::move(gpu)), diskCollector(std::move(disk)),
      snapshotSink(std::move(sink)) {}

SystemMonitor::~SystemMonitor() {
    stop();
}

void SystemMonitor::start() {
    if (isRunning) return;
    isRunning = true;
    workerThread = std::thread(&SystemMonitor::collectionLoop, this);
}

void SystemMonitor::stop() {
    if (!isRunning) return;
    isRunning = false;
    if (workerThread.joinable()) {
        workerThread.join();
    }
}

SystemSnapshot SystemMonitor::sampleOnce() {
    CpuData cpu = cpuCollector ? cpuCollector->collectCpuData() : CpuData{};
    RamData ram = ramCollector ? ramCollector->collectMemoryData() : RamData{};
    GpuData gpu = gpuCollector ? gpuCollector->collectGpuData() : GpuData{};
    DiskData disk = diskCollector ? diskCollector->collectDiskData() : DiskData{};

    SystemSnapshot snapshot{cpu, ram, gpu, disk};

    {
        std::lock_guard<std::mutex> lock(dataMutex);
        currentSnapshot = snapshot;
    }

    if (loggingEnabled.load() && snapshotSink) {
        snapshotSink->consume(snapshot);
    }

    return snapshot;
}

void SystemMonitor::collectionLoop() {
    while (isRunning) {
        sampleOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(sampleIntervalMs.load()));
    }
}

SystemSnapshot SystemMonitor::getLatestSnapshot() const {
    std::lock_guard<std::mutex> lock(dataMutex);
    return currentSnapshot;
}

void SystemMonitor::setIntervalMs(int ms) {
    if (ms > 0) {
        sampleIntervalMs = ms;
    }
}

int SystemMonitor::getIntervalMs() const {
    return sampleIntervalMs.load();
}

void SystemMonitor::enableLogging(bool enable) {
    loggingEnabled = enable && snapshotSink != nullptr;
}

bool SystemMonitor::isLoggingEnabled() const {
    return loggingEnabled.load();
}
