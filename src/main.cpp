#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include "collectors/ICpuCollector.hpp"
#include "mock/MockCpuCollector.hpp"
#include <storage/CsvExport.hpp>

int main(){
    std::cout << " --- System Performance Monitor ---" << std::endl;
    std::unique_ptr<ICpuCollector> cpuCollector = std::make_unique<MockCpuCollector>();
    std::unique_ptr<CsvExport> csvExport = std::make_unique<CsvExport>("cpu_data.csv");

    std::cout << "Collecting CPU data..." << std::endl;
    while(true){
        CpuData cpuData = cpuCollector->collectCpuData();
        std::cout << "CPU Name: " << cpuData.info.cpuName << std::endl;
        std::cout << "Physical Cores: " << cpuData.info.physicalCores << std::endl;
        std::cout << "Logical Threads: " << cpuData.info.logicalThreads << std::endl;
        std::cout << "Base Frequency (GHz): " << cpuData.info.baseFrequencyGHz << std::endl;
        std::cout << "Max Frequency (GHz): " << cpuData.info.maxFrequencyGHz << std::endl;
        std::cout << "Current Usage (%): " << cpuData.metrics.usage << std::endl;
        std::cout << "Current Frequency (GHz): " << cpuData.metrics.currentFrequency << std::endl;
        std::cout << "-----------------------------" << std::endl;
        csvExport->exportData(cpuCollector->collectCpuData());
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Sleep for 1 second before collecting data again
    }
    return 0;
}