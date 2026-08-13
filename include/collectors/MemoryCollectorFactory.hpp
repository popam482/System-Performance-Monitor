#pragma once

#include "IMemoryCollector.hpp"
#include <memory>

class MemoryCollectorFactory {
public:
    static std::unique_ptr<IMemoryCollector> createMemoryCollector();
};
