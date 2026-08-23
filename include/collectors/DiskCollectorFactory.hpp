#pragma once
#include <memory>
#include "collectors/IDiskCollector.hpp"

class DiskCollectorFactory {
public:
    static std::unique_ptr<IDiskCollector> createDiskCollector();
};