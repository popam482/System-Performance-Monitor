#pragma once
#include <string>
#include "CpuInfo.hpp"
#include "CpuMetrics.hpp"

struct CpuData{
    CpuInfo info;
    CpuMetrics metrics;
};