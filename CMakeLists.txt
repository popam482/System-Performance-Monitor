cmake_minimum_required(VERSION 3.10)
project(System_Performance_Monitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

target_include_directories(System_Performance_Monitor PRIVATE include)


add_executable(System_Performance_Monitor 
    src/main.cpp
    src/mock/MockCpuCollector.cpp
    src/storage/CsvExport.cpp
)