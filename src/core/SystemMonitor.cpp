#include "core/SystemMonitor.hpp"
#include "collectors/CpuCollectorFactory.hpp"
#include "collectors/MemoryCollectorFactory.hpp"
#include "collectors/DiskCollectorFactory.hpp"

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
#endif
}

SystemMonitor::~SystemMonitor() {
    stop();
}

void SystemMonitor::start() {
    if (isRunning) return; // Already running

    isRunning = true;
    workerThread = std::thread(&SystemMonitor::collectionLoop, this);
}

void SystemMonitor::stop() {
    if (!isRunning) return; // Not running

    isRunning = false;
    if (workerThread.joinable()) {
        workerThread.join();
    }
}

void SystemMonitor::collectionLoop(){
    while (isRunning) {
        CpuData cpu = cpuCollector ? cpuCollector->collectCpuData() : CpuData{};
        RamData ram = ramCollector ? ramCollector->collectMemoryData() : RamData{};
        GpuData gpu = gpuCollector ? gpuCollector->collectGpuData() : GpuData{};
        DiskData disk = diskCollector ? diskCollector->collectDiskData() : DiskData{};
        
        {
            std::lock_guard<std::mutex> lock(dataMutex);
            {
                std::lock_guard<std::mutex> lock(dataMutex);
                currentSnapshot.cpuData = cpu;
                currentSnapshot.ramData = ram;
                currentSnapshot.gpuData = gpu;
                currentSnapshot.diskData = disk;
            }
            currentSnapshot = {cpu, ram, gpu, disk};
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(sampleIntervalMs.load())); 

        if (loggingEnabled.load() && csvLogger) {
            csvLogger->logSnapshot(currentSnapshot);
        }

    }

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