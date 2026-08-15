#include "MemoryCollectorFactory.hpp"

#ifdef _WIN32
    #include "../include/collectors/windows/WindowsMemoryCollector.hpp"
#elif __linux__
    #include "../include/collectors/linux/LinuxMemoryCollector.hpp"
#elif __APPLE__
    #include "../include/collectors/mac/MacMemoryCollector.hpp"
#endif

std::unique_ptr<IMemoryCollector> MemoryCollectorFactory::createMemoryCollector() {
    #ifdef _WIN32
        return std::make_unique<WindowsMemoryCollector>();
    #elif __linux__
        return std::make_unique<LinuxMemoryCollector>();
    #elif __APPLE__
        return std::make_unique<MacMemoryCollector>();
    #else
        #error "Unsupported platform"
    #endif
}
