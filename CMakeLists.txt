cmake_minimum_required(VERSION 3.10)
project(System_Performance_Monitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(System_Performance_Monitor 
    src/main.cpp
    src/mock/MockCpuCollector.cpp
    src/storage/CsvExport.cpp
)

target_include_directories(System_Performance_Monitor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/collectors
    ${CMAKE_CURRENT_SOURCE_DIR}/include/models
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mock
    ${CMAKE_CURRENT_SOURCE_DIR}/include/storage
)