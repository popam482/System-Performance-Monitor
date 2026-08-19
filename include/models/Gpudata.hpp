#pragma once
#include <string>
#include "GpuMetrics.hpp"
#include "GpuInfo.hpp"

struct GpuData{
    GpuInfo info;
    GpuMetrics metrics;
};