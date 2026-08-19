#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include "collectors/ICpuCollector.hpp"
#include "collectors/IMemoryCollector.hpp"
#include "mock/MockCpuCollector.hpp"
#include "storage/CsvExport.hpp"
#include "collectors/CpuCollectorFactory.hpp"
#include "collectors/MemoryCollectorFactory.hpp"
#include "collectors/GpuCollectorFactory.hpp"

#include <ctime>
#include <iomanip>

int main(){
    std::cout << " --- System Performance Monitor ---" << std::endl;
    
    // Create collectors
    std::unique_ptr<ICpuCollector> cpuCollector = CpuCollectorFactory::createCpuCollector();
    std::unique_ptr<IMemoryCollector> memoryCollector = MemoryCollectorFactory::createMemoryCollector();
    std::unique_ptr<IGpuCollector> gpuCollector = GpuCollectorFactory::createGpuCollector();
    std::unique_ptr<CsvExport> csvExport = std::make_unique<CsvExport>("system_data.csv");

    time_t timestamp;

    std::cout << "Collecting system performance data..." << std::endl;
    while(true){
        CpuData cpuData = cpuCollector->collectCpuData();
        RamData ramData = memoryCollector->collectMemoryData();
        GpuData gpuData = gpuCollector->collectGpuData();
        time(&timestamp);
        
        // Display CPU data
        std::cout << "\n=== CPU DATA ===" << std::endl;
        std::cout << "Timestamp: " << std::ctime(&timestamp);
        std::cout << "CPU Name: " << cpuData.info.cpuName << std::endl;
        std::cout << "Physical Cores: " << cpuData.info.physicalCores << std::endl;
        std::cout << "Logical Threads: " << cpuData.info.logicalThreads << std::endl;
        std::cout << "Base Frequency (GHz): " << std::fixed << std::setprecision(2) 
                  << cpuData.info.baseFrequencyGHz << std::endl;
        std::cout << "Max Frequency (GHz): " << cpuData.info.maxFrequencyGHz << std::endl;
        std::cout << "Current Usage (%): " << cpuData.metrics.usage << std::endl;
        std::cout << "Current Frequency (GHz): " << cpuData.metrics.currentFrequency << std::endl;
        
        // Display RAM data
        std::cout << "\n=== RAM DATA ===" << std::endl;
        std::cout << "Total Memory (MB): " << (ramData.info.totalMemoryBytes / (1024 * 1024)) << std::endl;
        std::cout << "RAM Type: " << ramData.info.ramType << std::endl;
        std::cout << "Used Memory (MB): " << ramData.metrics.usedMemoryMB << std::endl;
        std::cout << "Available Memory (MB): " << ramData.metrics.availableMemoryMB << std::endl;
        std::cout << "Usage Percentage (%): " << std::fixed << std::setprecision(2) 
                  << ramData.metrics.usagePercentage << std::endl;
        std::cout << "-----------------------------" << std::endl;
        
        // Display GPU data
        std::cout << "\n=== GPU DATA ===" << std::endl;
        std::cout << "GPU Name: " << gpuData.info.gpuName << std::endl;
        std::cout << "VRAM (MB): " << gpuData.info.totalVramMB << std::endl;
        std::cout << "Usage Percentage (%): " << std::fixed << std::setprecision(2) 
                  << gpuData.metrics.usagePercentage << std::endl;
        std::cout << "-----------------------------" << std::endl;

        // Export data
        csvExport->exportData(cpuData, ramData, gpuData);
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}
