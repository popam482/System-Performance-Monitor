#include "CsvExport.hpp"
#include "CpuData.hpp"
#include <ctime>

CsvExport::CsvExport(const std::string &filename) : filename(filename) {
    file.open(filename, std::ios::out);
    if (file.is_open()) {
        file << "Timestamp,CPU Name,Physical Cores,Logical Threads,Base Frequency (GHz),Max Frequency (GHz),Current Usage (%),Current Frequency (GHz)" << std::endl;
        file.flush();
    }
}

CsvExport::~CsvExport() {
    if (file.is_open()) {
        file.close();
    }
}

void CsvExport::exportData(const CpuData &cpuData) {
    if (file.is_open()) {
        std::time_t now = std::time(nullptr);
        std::tm *localTime = std::localtime(&now);
        char timeBuffer[20];
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", localTime);

        file << timeBuffer << ","
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