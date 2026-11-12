#include "core/SystemMonitor.hpp"
#include "collectors/CpuCollectorFactory.hpp"
#include "collectors/DiskCollectorFactory.hpp"
#include "collectors/MemoryCollectorFactory.hpp"

#ifdef _WIN32
#include "collectors/windows/WindowsGpuCollector.hpp"
#endif

#include <chrono>

SystemMonitor::SystemMonitor() {
    cpuCollector = CpuCollectorFactory::createCpuCollector();
    ramCollector = MemoryCollectorFactory::createMemoryCollector();
    diskCollector = DiskCollectorFactory::createDiskCollector();
#ifdef _WIN32
    gpuCollector = std::make_unique<WindowsGpuCollector>();
#else
    gpuCollector = nullptr;
#endif
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
    CpuData cpu = cpuCollector ? cpuCollector->collectCpuData() : CpuData{};
    RamData ram = ramCollector ? ramCollector->collectMemoryData() : RamData{};
    GpuData gpu = gpuCollector ? gpuCollector->collectGpuData() : GpuData{};
    DiskData disk = diskCollector ? diskCollector->collectDiskData() : DiskData{};

    SystemSnapshot snapshot{cpu, ram, gpu, disk};

    {
        std::lock_guard<std::mutex> lock(dataMutex);
        currentSnapshot = snapshot;
    }

    if (loggingEnabled.load() && csvLogger) {
        csvLogger->logSnapshot(snapshot);
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
