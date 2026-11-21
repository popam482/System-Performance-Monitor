#pragma once

#include "core/ISnapshotSink.hpp"
#include <string>
#include <fstream>
#include <mutex>

class CsvExport : public ISnapshotSink {
private:
    std::string filePath;
    std::ofstream fileStream;
    std::mutex fileMutex;
    bool isHeaderWritten = false;
    bool openOnFirstWrite = false;

    std::string getCurrentTimestamp();
    void writeHeader();
    bool openFileUnlocked(const std::string& filename);

public:
    enum class OpenMode { Immediate, OnFirstWrite };
    explicit CsvExport(const std::string& filename = "system_metrics.csv",
                       OpenMode mode = OpenMode::Immediate);
    ~CsvExport() override;

    bool openFile(const std::string& filename);
    void closeFile();
    void logSnapshot(const SystemSnapshot& snapshot);
    void consume(const SystemSnapshot& snapshot) override { logSnapshot(snapshot); }
};
