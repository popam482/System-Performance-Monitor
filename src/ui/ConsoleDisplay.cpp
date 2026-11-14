#include "ui/ConsoleDisplay.hpp"
#include <iomanip>
#include <iostream>
#include <optional>

namespace {
template <typename T>
void show(const std::optional<T>& value) {
    if (value) std::cout << *value;
    else std::cout << "N/A";
}
template <typename T>
void showScaled(const std::optional<T>& value, double divisor) {
    if (value) std::cout << static_cast<double>(*value) / divisor;
    else std::cout << "N/A";
}
void showUsage(const std::optional<double>& value) {
    if (value) ConsoleDisplay::renderProgressBar(*value);
    else std::cout << "N/A";
}
}

void ConsoleDisplay::renderProgressBar(double percentage, int width) {
    if (percentage < 0.0) percentage = 0.0;
    if (percentage > 100.0) percentage = 100.0;
    const int pos = static_cast<int>((percentage / 100.0) * width);
    std::cout << "[";
    for (int i = 0; i < width; ++i) {
        if (i < pos) std::cout << '=';
        else if (i == pos) std::cout << '>';
        else std::cout << ' ';
    }
    std::cout << "] " << std::fixed << std::setprecision(1) << percentage << "%";
}
void ConsoleDisplay::renderDashboard(const CpuData& cpu, const RamData& ram, const GpuData& gpu, const DiskData& disk) {
    std::cout << "\n=======================================================================\n"
              << "                   SYSTEM PERFORMANCE MONITOR (LIVE)                  \n"
              << "=======================================================================\n\n";
    std::cout << "[ CPU ] " << (cpu.info.cpuName.empty() ? "N/A" : cpu.info.cpuName) << "\n";
    std::cout << " Cores/Threads : "; show(cpu.info.physicalCores);
    std::cout << " / "; show(cpu.info.logicalThreads);
    std::cout << std::fixed << std::setprecision(2) << "\n Base / OS Max : ";
    show(cpu.info.baseFrequencyGHz); std::cout << " / "; show(cpu.info.maxFrequencyGHz);
    std::cout << " GHz\n OS Mean Clock : "; show(cpu.metrics.currentFrequency);
    std::cout << " GHz\n Usage         : "; showUsage(cpu.metrics.usage);
    std::cout << "\n\n[ RAM ]\n Type          : "; show(ram.info.ramType);
    std::cout << std::fixed << std::setprecision(2) << "\n Total / Used  : ";
    showScaled(ram.info.totalMemoryBytes, 1024.0 * 1024.0 * 1024.0);
    std::cout << " GiB / "; showScaled(ram.metrics.usedMemoryMB, 1024.0);
    std::cout << " GiB\n Usage         : "; showUsage(ram.metrics.usagePercentage);
    std::cout << "\n\n[ GPU ] " << (gpu.info.gpuName.empty() ? "N/A" : gpu.info.gpuName);
    std::cout << "\n Driver        : "; show(gpu.info.driverVersion);
    std::cout << std::fixed << std::setprecision(2) << "\n VRAM Total/Used: ";
    showScaled(gpu.info.totalVramMB, 1024.0);
    std::cout << " GiB / "; showScaled(gpu.metrics.usedVramMB, 1024.0);
    std::cout << " GiB\n 3D Engine     : "; showUsage(gpu.metrics.usagePercentage);
    std::cout << "\n Temperature   : "; show(gpu.metrics.temperatureC);
    std::cout << " C\n\n[ Disk ] " << (disk.info.driveLetter.empty() ? "N/A" : disk.info.driveLetter);
    std::cout << std::fixed << std::setprecision(2) << "\n Total / Used  : ";
    show(disk.info.totalSpaceGB); std::cout << " GiB / "; show(disk.metrics.usedSpaceGB);
    std::cout << " GiB\n Usage         : "; showUsage(disk.metrics.usagePercentage);
    std::cout << "\n\n=======================================================================\n"
              << " Q + Enter: menu | X + Enter: exit | Ctrl+C: exit\n" << std::flush;
}
