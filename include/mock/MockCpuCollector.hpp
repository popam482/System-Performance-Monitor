#pragma once 
#include "CpuData.hpp"
#include "ICpuCollector.hpp"
#include <ctime>

class MockCpuCollector : public ICpuCollector {
    private:
        CpuData mockData;
    public:
        MockCpuCollector();
        CpuData collectCpuData() override;
        
};