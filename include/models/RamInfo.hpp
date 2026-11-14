#pragma once
#include <string>
#include <optional>

struct RamInfo{
    std::optional<unsigned long long> totalMemoryBytes;
    std::optional<std::string> ramType;
};
