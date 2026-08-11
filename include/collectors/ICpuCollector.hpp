#pragma once
#include "../models/CpuData.hpp"

class ICpuCollector{
    public:
        virtual ~ICpuCollector() = default;
        virtual CpuData collectCpuData() = 0;
};