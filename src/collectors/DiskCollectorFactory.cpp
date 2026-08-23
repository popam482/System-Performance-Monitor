#include "collectors/DiskCollectorFactory.hpp"

#ifdef _WIN32
#include "collectors/windows/WindowsDiskCollector.hpp"
#endif

std::unique_ptr<IDiskCollector> DiskCollectorFactory::createDiskCollector() {
#ifdef _WIN32
    return std::make_unique<WindowsDiskCollector>("C:\\");
#else
    return nullptr;
#endif
}