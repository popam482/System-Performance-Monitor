#pragma once

#include "collectors/IGpuCollector.hpp"
#include <memory>

class LinuxGpuCollector : public IGpuCollector {
public:
    LinuxGpuCollector();
    ~LinuxGpuCollector() override;
    GpuData collectGpuData() override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
