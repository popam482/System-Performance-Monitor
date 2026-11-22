cmake_minimum_required(VERSION 3.10)
project(System_Performance_Monitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Threads REQUIRED)

if(WIN32)
    set(PLATFORM_SOURCES
        src/collectors/windows/WindowsCpuCollector.cpp
        src/collectors/windows/WindowsMemoryCollector.cpp
        src/collectors/windows/SmbiosMemory.cpp
        src/collectors/windows/WindowsGpuCollector.cpp
        src/collectors/windows/WindowsDiskCollector.cpp
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PLATFORM_SOURCES
        src/collectors/linux/LinuxCpuCollector.cpp
        src/collectors/linux/LinuxMemoryCollector.cpp
        src/collectors/linux/LinuxDiskCollector.cpp
    )
else()
    message(FATAL_ERROR "Supported platforms are Windows and Linux.")
endif()

add_executable(System_Performance_Monitor
    src/main.cpp
    src/ui/ConsoleInput.cpp
    src/ui/ConsoleDisplay.cpp
    src/collectors/CpuCollectorFactory.cpp
    src/collectors/MemoryCollectorFactory.cpp
    src/collectors/GpuCollectorFactory.cpp
    src/collectors/DiskCollectorFactory.cpp
    src/core/SystemMonitor.cpp
    src/storage/CsvExport.cpp
    ${PLATFORM_SOURCES}
)

target_include_directories(System_Performance_Monitor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/collectors
)
target_link_libraries(System_Performance_Monitor PRIVATE Threads::Threads)
if(WIN32)
    target_link_libraries(System_Performance_Monitor PRIVATE pdh wbemuuid ole32 oleaut32 dxgi powrprof)
endif()
