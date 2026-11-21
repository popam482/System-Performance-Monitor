#include "MemoryCollectorFactory.hpp"

#ifdef _WIN32
    #include "collectors/windows/WindowsMemoryCollector.hpp"
#elif defined(__linux__)
    #include "collectors/linux/LinuxMemoryCollector.hpp"
#endif

std::unique_ptr<IMemoryCollector> MemoryCollectorFactory::createMemoryCollector() {
    #ifdef _WIN32
        return std::make_unique<WindowsMemoryCollector>();
    #elif defined(__linux__)
        return std::make_unique<LinuxMemoryCollector>();
    #else
        return nullptr; // Not implemented on this platform yet.
    #endif
}
