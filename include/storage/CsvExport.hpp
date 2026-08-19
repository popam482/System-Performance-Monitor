# pragma once
#include <string>
#include <fstream>
#include "CpuData.hpp"
#include "RamData.hpp"
#include "GpuData.hpp"

class CsvExport {
    private:
        std::string filename;
        std::ofstream file;
        bool headerWritten;
    public:
        CsvExport(const std::string &filename);
        ~CsvExport();
        void exportCpuData(const CpuData &cpuData);
        void exportMemoryData(const RamData &ramData);
        void exportGpuData(const GpuData &gpuData);
        void exportData(const CpuData &cpuData, const RamData &ramData, const GpuData &gpuData);
};