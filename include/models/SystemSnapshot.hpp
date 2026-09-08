#include "models/CpuData.hpp"
#include "models/RamData.hpp"
#include "models/GpuData.hpp"
#include "models/DiskData.hpp"

struct SystemSnapshot {
    CpuData cpuData;
    RamData ramData;
    GpuData gpuData;
    DiskData diskData;
};
