#include "storage/CsvExport.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

CsvExport::CsvExport(const std::string& filename) {
    openFile(filename);
}

CsvExport::~CsvExport() {
    closeFile();
}

std::string CsvExport::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuffer;

#ifdef _WIN32
    localtime_s(&tmBuffer, &timeT);
#else
    localtime_r(&timeT, &tmBuffer);
#endif

    std::ostringstream ss;
    ss << std::put_time(&tmBuffer, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

bool CsvExport::openFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(fileMutex);
    filePath = filename;

    std::ifstream checkFile(filePath);
    bool fileExists = checkFile.good();
    checkFile.close();

    fileStream.open(filePath, std::ios::out | std::ios::app);
    if (!fileStream.is_open()) return false;

    if (!fileExists) {
        writeHeader();
    } else {
        isHeaderWritten = true;
    }

    return true;
}

void CsvExport::writeHeader() {
    if (fileStream.is_open() && !isHeaderWritten) {
        fileStream << "Timestamp,CPU_Usage_%,RAM_Used_MB,RAM_Total_MB,RAM_Usage_%,GPU_Usage_%,VRAM_Used_MB,Disk_Used_GB,Disk_Total_GB\n";
        fileStream.flush();
        isHeaderWritten = true;
    }
}

void CsvExport::closeFile() {
    std::lock_guard<std::mutex> lock(fileMutex);
    if (fileStream.is_open()) {
        fileStream.close();
    }
}

void CsvExport::logSnapshot(const SystemSnapshot& snapshot) {
    std::lock_guard<std::mutex> lock(fileMutex);
    if (!fileStream.is_open()) return;

    fileStream << getCurrentTimestamp() << ","
               << std::fixed << std::setprecision(2)
               << snapshot.cpuData.metrics.usage << ","
               << snapshot.ramData.metrics.usedMemoryMB << ","
               << snapshot.ramData.info.totalMemoryBytes << ","
               << snapshot.ramData.metrics.usagePercentage << ","
               << snapshot.gpuData.metrics.usagePercentage << ","
               << snapshot.gpuData.metrics.usedVramMB << ","
               << snapshot.diskData.metrics.usedSpaceGB << ","
               << snapshot.diskData.info.totalSpaceGB << "\n";

    fileStream.flush(); 
}