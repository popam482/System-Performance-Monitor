#pragma once
#include "../models/RamData.hpp"

class IMemoryCollector{
    public:
        virtual ~IMemoryCollector() = default;
        virtual RamData collectMemoryData() = 0;
};