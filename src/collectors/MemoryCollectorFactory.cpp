#include "MemoryCollectorFactory.hpp"

#ifdef _WIN32
    #include "collectors/windows/WindowsMemoryCollector.hpp"
#endif

std::unique_ptr<IMemoryCollector> MemoryCollectorFactory::createMemoryCollector() {
    #ifdef _WIN32
        return std::make_unique<WindowsMemoryCollector>();
    #else
        return nullptr; // Not implemented on this platform yet.
    #endif
}
