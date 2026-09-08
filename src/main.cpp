#include <thread>
#include <chrono>
#include <memory>
#include "collectors/CpuCollectorFactory.hpp"
#include "collectors/MemoryCollectorFactory.hpp"
#include "collectors/windows/WindowsGpuCollector.hpp"
#include "collectors/windows/WindowsDiskCollector.hpp"
#include "ui/ConsoleDisplay.hpp"
#include "core/SystemMonitor.hpp"


int main() {

    SystemMonitor systemMonitor;
    systemMonitor.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(300)); 

    while(true){
        SystemSnapshot snapshot = systemMonitor.getLatestSnapshot();
        ConsoleDisplay::renderDashboard(snapshot.cpuData, snapshot.ramData, snapshot.gpuData, snapshot.diskData);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Update every second
    }

   

    return 0;
}