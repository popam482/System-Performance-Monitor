#include "GpuCollectorFactory.hpp"

#ifdef _WIN32
    #include "windows/WindowsGpuCollector.hpp"
#elif defined(__linux__)
    #include "linux/LinuxGpuCollector.hpp"
#endif

std::unique_ptr<IGpuCollector> GpuCollectorFactory::createGpuCollector() {
    #ifdef _WIN32
        return std::make_unique<WindowsGpuCollector>();
    #elif defined(__linux__)
        return std::make_unique<LinuxGpuCollector>();
    #else
        return nullptr; // Not implemented on this platform yet.
    #endif
}
