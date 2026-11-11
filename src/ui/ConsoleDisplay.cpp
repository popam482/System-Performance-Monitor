#include "ui/ConsoleDisplay.hpp"
#include <iostream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {
void clearScreen() {
#ifdef _WIN32
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    if (console == INVALID_HANDLE_VALUE) {
        return;
    }

    CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
    if (!GetConsoleScreenBufferInfo(console, &bufferInfo)) {
        return;
    }

    const DWORD cellCount =
        static_cast<DWORD>(bufferInfo.dwSize.X) * static_cast<DWORD>(bufferInfo.dwSize.Y);
    DWORD written = 0;
    COORD home = {0, 0};

    FillConsoleOutputCharacterA(console, ' ', cellCount, home, &written);
    FillConsoleOutputAttribute(console, bufferInfo.wAttributes, cellCount, home, &written);
    SetConsoleCursorPosition(console, home);
#else
    std::cout << "\033[2J\033[H";
#endif
}
} // namespace

void ConsoleDisplay::renderProgressBar(double percentage, int width) {
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
    clearScreen();

    std::cout << "=======================================================================\n";
    std::cout << "                   SYSTEM PERFORMANCE MONITOR (LIVE)                  \n";
    std::cout << "=======================================================================\n\n";

    // 1.CPU section
    std::cout << "[ CPU ] " << cpu.info.cpuName << "\n";
    std::cout << " Cores/Threads : " << cpu.info.physicalCores << " Cores / " << cpu.info.logicalThreads << " Threads\n";
    std::cout << " Usage         : ";
    renderProgressBar(cpu.metrics.usage);
    std::cout << "\n\n";

    // 2.RAM section
    std::cout << "[ RAM ] Memory Status\n";
    std::cout << " Total / Used  : " << std::fixed << std::setprecision(2)
              << (ram.info.totalMemoryBytes / 1024.0 / 1024.0 / 1024.0) << " GB / "
              << (ram.metrics.usedMemoryMB / 1024.0) << " GB\n";
    std::cout << " Usage         : ";
    renderProgressBar(ram.metrics.usagePercentage);
    std::cout << "\n\n";

    // 3.GPU section
    std::cout << "[ GPU ] " << gpu.info.gpuName << "\n";
    std::cout << " VRAM Total/Used: " << std::fixed << std::setprecision(2)
              << (gpu.info.totalVramMB / 1024.0) << " GB / "
              << (gpu.metrics.usedVramMB / 1024.0) << " GB\n";
    std::cout << " Engine Usage  : ";
    renderProgressBar(gpu.metrics.usagePercentage);
    std::cout << "\n\n";

    // 4.Disk section
    std::cout << "[ Disk ] " << disk.info.driveLetter << "\n";
    std::cout << " Total / Used  : " << std::fixed << std::setprecision(2)
              << disk.info.totalSpaceGB << " GB / "
              << disk.metrics.usedSpaceGB << " GB\n";
    std::cout << " Usage         : ";
    renderProgressBar(disk.metrics.usagePercentage);
    std::cout << "\n\n";

    std::cout << "=======================================================================\n";
    std::cout << " Press Q to return to menu\n";
    std::cout.flush();
}
