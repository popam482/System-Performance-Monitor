#include "collectors/DiskCollectorFactory.hpp"

#ifdef _WIN32
#include "collectors/windows/WindowsDiskCollector.hpp"
#elif defined(__linux__)
#include "collectors/linux/LinuxDiskCollector.hpp"
#endif

std::unique_ptr<IDiskCollector> DiskCollectorFactory::createDiskCollector() {
#ifdef _WIN32
    return std::make_unique<WindowsDiskCollector>("C:\\");
#elif defined(__linux__)
    return std::make_unique<LinuxDiskCollector>("/");
#else
    return nullptr;
#endif
}
