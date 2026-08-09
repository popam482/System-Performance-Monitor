#pragma once
#include "CpuData.hpp"

class ICpuCollector{
    public:
        virtual ~ICpuCollector() = default;
        virtual CpuData collectCpuData() = 0;
};