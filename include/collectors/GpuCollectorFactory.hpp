#pragma once

#include "IGpuCollector.hpp"
#include <memory>

class GpuCollectorFactory {
public:
    static std::unique_ptr<IGpuCollector> createGpuCollector();
};
