#pragma once
#include "../models/GpuData.hpp"

class IGpuCollector{
    public:
        virtual ~IGpuCollector() = default;
        virtual GpuData collectGpuData() = 0;
};