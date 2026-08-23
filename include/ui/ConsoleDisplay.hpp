#pragma once
#include "models/CpuData.hpp"
#include "models/RamData.hpp"
#include "models/GpuData.hpp"

class ConsoleDisplay {
public:
    static void renderDashboard(const CpuData& cpu, const RamData& ram, const GpuData& gpu);
    static void renderProgressBar(double percentage, int width = 30);
};