#include <thread>
#include <chrono>
#include <memory>
#include "collectors/CpuCollectorFactory.hpp"
#include "collectors/MemoryCollectorFactory.hpp"
#include "collectors/windows/WindowsGpuCollector.hpp"
#include "ui/ConsoleDisplay.hpp"

int main() {
    auto cpuCollector = CpuCollectorFactory::createCpuCollector();
    auto ramCollector = MemoryCollectorFactory::createMemoryCollector();
    auto gpuCollector = std::make_unique<WindowsGpuCollector>();

    while (true) {
        CpuData cpu = cpuCollector->collectCpuData();
        RamData ram = ramCollector->collectMemoryData();
        GpuData gpu = gpuCollector->collectGpuData();

        ConsoleDisplay::renderDashboard(cpu, ram, gpu);

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}