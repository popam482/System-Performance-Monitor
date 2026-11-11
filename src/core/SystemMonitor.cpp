#include "core/SystemMonitor.hpp"
#include "collectors/CpuCollectorFactory.hpp"
#include "collectors/MemoryCollectorFactory.hpp"
#include "collectors/DiskCollectorFactory.hpp"

#include <chrono>
#include <iostream>

SystemMonitor::SystemMonitor() {
    std::cout << "[DBG] creating CPU collector...\n" << std::flush;
    cpuCollector = CpuCollectorFactory::createCpuCollector();
    std::cout << "[DBG] creating RAM collector...\n" << std::flush;
    ramCollector = MemoryCollectorFactory::createMemoryCollector();
    std::cout << "[DBG] creating Disk collector...\n" << std::flush;
    diskCollector = DiskCollectorFactory::createDiskCollector();
    // GPU disabled for now — DXGI/PDH GPU counters can hang on some setups.
    gpuCollector = nullptr;
    std::cout << "[DBG] collectors ready (GPU disabled)\n" << std::flush;
}

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
    std::cout << "[DBG] collect CPU...\n" << std::flush;
    CpuData cpu = cpuCollector ? cpuCollector->collectCpuData() : CpuData{};

    std::cout << "[DBG] collect RAM...\n" << std::flush;
    RamData ram = ramCollector ? ramCollector->collectMemoryData() : RamData{};

    std::cout << "[DBG] collect GPU (skipped)...\n" << std::flush;
    GpuData gpu{}; // intentionally empty while GPU collector is disabled

    std::cout << "[DBG] collect Disk...\n" << std::flush;
    DiskData disk = diskCollector ? diskCollector->collectDiskData() : DiskData{};

    SystemSnapshot snapshot{cpu, ram, gpu, disk};

    {
        std::lock_guard<std::mutex> lock(dataMutex);
        currentSnapshot = snapshot;
    }

    if (loggingEnabled.load() && csvLogger) {
        csvLogger->logSnapshot(snapshot);
    }

    std::cout << "[DBG] sampleOnce done\n" << std::flush;
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

void SystemMonitor::enableLogging(bool enable, const std::string& filename) {
    if (enable) {
        if (!csvLogger) {
            csvLogger = std::make_unique<CsvExport>(filename);
        }
        loggingEnabled = true;
    } else {
        loggingEnabled = false;
    }
}

bool SystemMonitor::isLoggingEnabled() const {
    return loggingEnabled.load();
}
