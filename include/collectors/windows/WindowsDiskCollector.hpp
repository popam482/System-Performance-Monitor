#pragma once
#ifdef _WIN32

#include "collectors/IDiskCollector.hpp"
#include <string>

class WindowsDiskCollector : public IDiskCollector {
private:
    std::string drivePath;

public:
    WindowsDiskCollector(std::string drive = "C:\\");
    ~WindowsDiskCollector() override = default;

    DiskData collectDiskData() override;
};

#endif