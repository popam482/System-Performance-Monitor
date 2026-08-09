#pragma once 
#include "CpuData.hpp"
#include "ICpuCollector.hpp"

class MockCpuCollector : public ICpuCollector {
    private:
        CpuData mockData;
    public:
        MockCpuCollector();
        CpuData collectCpuData() override;
        
};