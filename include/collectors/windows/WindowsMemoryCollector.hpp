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
    
    std::optional<std::string> ramType;
};

#endif // _WIN32
