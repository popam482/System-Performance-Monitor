# pragma once
#include <string>
#include <fstream>
#include "CpuData.hpp"

class CsvExport {
    private:
        std::string filename;
        std::ofstream file;
    public:
        CsvExport(const std::string &filename);
        ~CsvExport();
        void exportData(const CpuData &cpuData);
};