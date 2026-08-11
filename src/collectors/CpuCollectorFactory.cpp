#include "CpuCollectorFactory.hpp"

#ifdef _WIN32
    #include "windows/WindowsCpuCollector.hpp"
#elif __linux__
    #include "linux/LinuxCpuCollector.hpp"
#elif __APPLE__
    #include "mac/MacCpuCollector.hpp"
#endif

std::unique_ptr<ICpuCollector> CpuCollectorFactory::createCpuCollector() {
    #ifdef _WIN32
        return std::make_unique<WindowsCpuCollector>();
    #elif __linux__
        return std::make_unique<LinuxCpuCollector>();
    #elif __APPLE__
        return std::make_unique<MacCpuCollector>();
    #else
        #error "Unsupported platform"
    #endif
}
