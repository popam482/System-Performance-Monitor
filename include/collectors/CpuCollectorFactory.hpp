#pragma once

#include "ICpuCollector.hpp"
#include <memory>

class CpuCollectorFactory {
public:
    static std::unique_ptr<ICpuCollector> createCpuCollector();
};