#pragma once

#ifdef _WIN32

#include "../IMemoryCollector.hpp"
#include <windows.h>
#include <string>

class WindowsMemoryCollector : public IMemoryCollector {
public:
    WindowsMemoryCollector();
    ~WindowsMemoryCollector() override = default;

    RamData collectMemoryData() override;

private:
    void readMemoryInfo();
    
    unsigned long totalMemoryMB = 0;
    std::string ramType = "Unknown";
};

#endif // _WIN32
