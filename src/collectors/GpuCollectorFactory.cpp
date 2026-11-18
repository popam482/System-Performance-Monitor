#include "GpuCollectorFactory.hpp"

#ifdef _WIN32
    #include "windows/WindowsGpuCollector.hpp"
#endif

std::unique_ptr<IGpuCollector> GpuCollectorFactory::createGpuCollector() {
    #ifdef _WIN32
        return std::make_unique<WindowsGpuCollector>();
    #else
        return nullptr; // Not implemented on this platform yet.
    #endif
}
