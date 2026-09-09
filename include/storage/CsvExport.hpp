#pragma once

#include "models/SystemSnapshot.hpp"
#include <string>
#include <fstream>
#include <mutex>

class CsvExport {
private:
    std::string filePath;
    std::ofstream fileStream;
    std::mutex fileMutex;
    bool isHeaderWritten = false;

    std::string getCurrentTimestamp();
    void writeHeader();

public:
    CsvExport(const std::string& filename = "system_metrics.csv");
    ~CsvExport();

    bool openFile(const std::string& filename);
    void closeFile();
    void logSnapshot(const SystemSnapshot& snapshot);
};