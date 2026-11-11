#include "ui/ConsoleDisplay.hpp"
#include <iostream>
#include <iomanip>

void ConsoleDisplay::renderProgressBar(double percentage, int width) {
    if (percentage < 0.0) percentage = 0.0;
    if (percentage > 100.0) percentage = 100.0;

    int pos = static_cast<int>((percentage / 100.0) * width);
    std::cout << "[";
    for (int i = 0; i < width; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << std::fixed << std::setprecision(1) << percentage << "%";
}

void ConsoleDisplay::renderDashboard(const CpuData& cpu, const RamData& ram, const GpuData& gpu, const DiskData& disk) {
    // No screen clear: system("cls") and ANSI clears can hang or blank VS Code terminals.
    std::cout << "\n=======================================================================\n";
    std::cout << "                   SYSTEM PERFORMANCE MONITOR (LIVE)                  \n";
    std::cout << "=======================================================================\n\n";

    std::cout << "[ CPU ] " << (cpu.info.cpuName.empty() ? "(reading...)" : cpu.info.cpuName) << "\n";
    std::cout << " Cores/Threads : " << cpu.info.physicalCores << " Cores / " << cpu.info.logicalThreads << " Threads\n";
    std::cout << " Usage         : ";
    renderProgressBar(cpu.metrics.usage);
    std::cout << "\n\n";

    std::cout << "[ RAM ] Memory Status\n";
    std::cout << " Total / Used  : " << std::fixed << std::setprecision(2)
              << (ram.info.totalMemoryBytes / 1024.0 / 1024.0 / 1024.0) << " GB / "
              << (ram.metrics.usedMemoryMB / 1024.0) << " GB\n";
    std::cout << " Usage         : ";
    renderProgressBar(ram.metrics.usagePercentage);
    std::cout << "\n\n";

    std::cout << "[ GPU ] " << (gpu.info.gpuName.empty() ? "(reading...)" : gpu.info.gpuName) << "\n";
    std::cout << " VRAM Total/Used: " << std::fixed << std::setprecision(2)
              << (gpu.info.totalVramMB / 1024.0) << " GB / "
              << (gpu.metrics.usedVramMB / 1024.0) << " GB\n";
    std::cout << " Engine Usage  : ";
    renderProgressBar(gpu.metrics.usagePercentage);
    std::cout << "\n\n";

    std::cout << "[ Disk ] " << (disk.info.driveLetter.empty() ? "(reading...)" : disk.info.driveLetter) << "\n";
    std::cout << " Total / Used  : " << std::fixed << std::setprecision(2)
              << disk.info.totalSpaceGB << " GB / "
              << disk.metrics.usedSpaceGB << " GB\n";
    std::cout << " Usage         : ";
    renderProgressBar(disk.metrics.usagePercentage);
    std::cout << "\n\n";

    std::cout << "=======================================================================\n";
    std::cout << " Press Ctrl + C to stop monitoring\n";
    std::cout.flush();
}
