cmake_minimum_required(VERSION 3.10)
project(System_Performance_Monitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Detectare platformă
if(WIN32)
    set(PLATFORM_SOURCES
        src/collectors/windows/WindowsCpuCollector.cpp
    )
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_SOURCES
        src/collectors/linux/LinuxCpuCollector.cpp
    )
elseif(APPLE)
    set(PLATFORM_SOURCES
        src/collectors/mac/MacCpuCollector.cpp
    )
endif()

add_executable(System_Performance_Monitor 
    src/main.cpp
    src/collectors/CpuCollectorFactory.cpp
    src/storage/CsvExport.cpp
    src/collectors/windows/WindowsCpuCollector.cpp
    ${PLATFORM_SOURCES}
)

target_include_directories(System_Performance_Monitor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/collectors
    ${CMAKE_CURRENT_SOURCE_DIR}/include/models
    ${CMAKE_CURRENT_SOURCE_DIR}/include/storage
)

if(WIN32)
    target_link_libraries(System_Performance_Monitor PRIVATE pdh wbemuuid ole32 oleaut32)
endif()