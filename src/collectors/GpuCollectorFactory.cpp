#include "GpuCollectorFactory.hpp"

#ifdef _WIN32
    #include "windows/WindowsGpuCollector.hpp"
#elif __linux__
    #include "linux/LinuxGpuCollector.hpp"
#elif __APPLE__
    #include "mac/MacGpuCollector.hpp"
#endif

std::unique_ptr<IGpuCollector> GpuCollectorFactory::createGpuCollector() {
    #ifdef _WIN32
        return std::make_unique<WindowsGpuCollector>();
    #elif __linux__
        return std::make_unique<LinuxGpuCollector>();
    #elif __APPLE__
        return std::make_unique<MacGpuCollector>();
    #else
        #error "Unsupported platform"
    #endif
}
