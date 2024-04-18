cmake_minimum_required(VERSION 3.15)
project(DistributedDataScience VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /O2")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi /DDEBUG")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files - all components
file(GLOB_RECURSE UTILS_SOURCES "src/utils/*.cpp")
file(GLOB_RECURSE STORAGE_SOURCES "src/storage/*.cpp")
file(GLOB_RECURSE WEB_SOURCES "src/web/*.cpp")
file(GLOB_RECURSE DATABASE_SOURCES "src/database/*.cpp")
file(GLOB_RECURSE ALGORITHMS_SOURCES "src/algorithms/*.cpp")
file(GLOB_RECURSE MONITORING_SOURCES "src/monitoring/*.cpp")
file(GLOB_RECURSE CONFIG_SOURCES "src/config/*.cpp")

# Main executable
add_executable(dds_demo
    ${UTILS_SOURCES}
    ${STORAGE_SOURCES}
    ${WEB_SOURCES}
    ${DATABASE_SOURCES}
    ${ALGORITHMS_SOURCES}
    ${MONITORING_SOURCES}
    ${CONFIG_SOURCES}
    examples/example_usage.cpp
)

# Print configuration summary
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Building minimal version with basic utilities only") 