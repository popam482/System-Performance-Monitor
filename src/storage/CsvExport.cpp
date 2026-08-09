#include "CsvExport.hpp"
#include "CpuData.hpp"

CsvExport::CsvExport(const std::string &filename){
    this->filename = filename;
    file.open(filename, std::ios::out);
    if(file.is_open()){
        // Write the header line
        file << "CPU Name,Physical Cores,Logical Threads,Base Frequency (GHz),Max Frequency (GHz),Current Usage (%),Current Frequency (GHz)" << std::endl;
        file.close();
    }
}

CsvExport::~CsvExport(){
    if(file.is_open()){
        file.close(); //close the file at the end of the program
    }
}

void CsvExport::exportData(const CpuData &cpuData){
    file.open(filename, std::ios::app);
    if(file.is_open()){
        file << cpuData.info.cpuName << "," << cpuData.info.physicalCores << "," << cpuData.info.logicalThreads << "," << cpuData.info.baseFrequencyGHz << "," << cpuData.info.maxFrequencyGHz << "," << cpuData.metrics.usage << "," << cpuData.metrics.currentFrequency << std::endl;
        file.close();
    }
}