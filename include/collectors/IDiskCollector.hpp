#pragma once
#include "models/DiskData.hpp"

class IDiskCollector {
public:
    virtual ~IDiskCollector() = default;
    virtual DiskData collectDiskData() = 0;
};