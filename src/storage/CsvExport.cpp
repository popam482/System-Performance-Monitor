#include "CsvExport.hpp"
#include <ctime>
#include <iomanip>
#include <sstream>

CsvExport::CsvExport(const std::string &filename) : filename(filename) {
    file.open(filename, std::ios::out);
    headerWritten = false;
}

CsvExport::~CsvExport() {
    if (file.is_open()) {
        file.close();
    }
}

void CsvExport::exportCpuData(const CpuData &cpuData) {
    if (file.is_open()) {
        if (!headerWritten) {
            file << "Timestamp,Type,CPU Name,Physical Cores,Logical Threads,"
                 << "Base Frequency (GHz),Max Frequency (GHz),Current Usage (%),Current Frequency (GHz)" 
                 << std::endl;
            headerWritten = true;
        }

        std::time_t now = std::time(nullptr);
        std::tm *localTime = std::localtime(&now);
        char timeBuffer[20];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", localTime);

        file << timeBuffer << ",CPU,"
             << cpuData.info.cpuName << ","
             << cpuData.info.physicalCores << ","
             << cpuData.info.logicalThreads << ","
             << cpuData.info.baseFrequencyGHz << ","
             << cpuData.info.maxFrequencyGHz << ","
             << cpuData.metrics.usage << ","
             << cpuData.metrics.currentFrequency << std::endl;
        file.flush();
    }
}

void CsvExport::exportMemoryData(const RamData &ramData) {
    if (file.is_open()) {
        if (!headerWritten) {
            file << "Timestamp,Type,Total Memory (MB),RAM Type,Used Memory (MB),"
                 << "Available Memory (MB),Usage Percentage (%)" 
                 << std::endl;
            headerWritten = true;
        }

        std::time_t now = std::time(nullptr);
        std::tm *localTime = std::localtime(&now);
        char timeBuffer[20];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", localTime);

        file << timeBuffer << ",RAM,"
             << (ramData.info.totalMemoryBytes / (1024 * 1024)) << ","
             << ramData.info.ramType << ","
             << ramData.metrics.usedMemoryMB << ","
             << ramData.metrics.availableMemoryMB << ","
             << ramData.metrics.usagePercentage << std::endl;
        file.flush();
    }
}

void CsvExport::exportData(const CpuData &cpuData, const RamData &ramData) {
    if (file.is_open()) {
        if (!headerWritten) {
            file << "Timestamp,Type,CPU Name,Physical Cores,Logical Threads,"
                 << "Base Frequency (GHz),Max Frequency (GHz),CPU Usage (%),"
                 << "Current Frequency (GHz),Total Memory (MB),RAM Type,"
                 << "Used Memory (MB),Available Memory (MB),RAM Usage (%)" 
                 << std::endl;
            headerWritten = true;
        }

        std::time_t now = std::time(nullptr);
        std::tm *localTime = std::localtime(&now);
        char timeBuffer[20];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", localTime);

        file << timeBuffer << ",SYSTEM,"
             << cpuData.info.cpuName << ","
             << cpuData.info.physicalCores << ","
             << cpuData.info.logicalThreads << ","
             << cpuData.info.baseFrequencyGHz << ","
             << cpuData.info.maxFrequencyGHz << ","
             << cpuData.metrics.usage << ","
             << cpuData.metrics.currentFrequency << ","
             << (ramData.info.totalMemoryBytes / (1024 * 1024)) << ","
             << ramData.info.ramType << ","
             << ramData.metrics.usedMemoryMB << ","
             << ramData.metrics.availableMemoryMB << ","
             << ramData.metrics.usagePercentage << std::endl;
        file.flush();
    }
}
