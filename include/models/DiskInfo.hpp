#pragma once
#include <string>
#include <optional>

struct DiskInfo {
    std::string driveLetter; 
    std::optional<double> totalSpaceGB;
};
