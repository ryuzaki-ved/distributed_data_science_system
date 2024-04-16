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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")

# Find required packages
find_package(MPI REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem system thread program_options)
find_package(Eigen3 REQUIRED)
find_package(PkgConfig REQUIRED)

# Find Hadoop libraries
pkg_check_modules(HADOOP REQUIRED hadoop)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${MPI_CXX_INCLUDE_PATH}
    ${Boost_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
    ${HADOOP_INCLUDE_DIRS}
)

# Link directories
link_directories(
    ${MPI_CXX_LIBRARIES}
    ${Boost_LIBRARY_DIRS}
    ${HADOOP_LIBRARY_DIRS}
)

# Source files
file(GLOB_RECURSE CLIENT_SOURCES "src/client/*.cpp")
file(GLOB_RECURSE JOB_MANAGER_SOURCES "src/job_manager/*.cpp")
file(GLOB_RECURSE WORKER_SOURCES "src/worker/*.cpp")
file(GLOB_RECURSE ALGORITHMS_SOURCES "src/algorithms/*.cpp")
file(GLOB_RECURSE COMMUNICATION_SOURCES "src/communication/*.cpp")
file(GLOB_RECURSE STORAGE_SOURCES "src/storage/*.cpp")
file(GLOB_RECURSE UTILS_SOURCES "src/utils/*.cpp")

# Common library
add_library(dds_common STATIC
    ${UTILS_SOURCES}
    ${COMMUNICATION_SOURCES}
    ${STORAGE_SOURCES}
)

target_link_libraries(dds_common
    ${MPI_CXX_LIBRARIES}
    ${Boost_LIBRARIES}
    ${HADOOP_LIBRARIES}
    pthread
)

# Client executable
add_executable(dds_client ${CLIENT_SOURCES})
target_link_libraries(dds_client dds_common)

# Job Manager executable
add_executable(dds_job_manager ${JOB_MANAGER_SOURCES})
target_link_libraries(dds_job_manager dds_common)

# Worker executable
add_executable(dds_worker ${WORKER_SOURCES} ${ALGORITHMS_SOURCES})
target_link_libraries(dds_worker dds_common)

# Test executable
enable_testing()
file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")
if(TEST_SOURCES)
    add_executable(dds_tests ${TEST_SOURCES})
    target_link_libraries(dds_tests dds_common)
    add_test(NAME DDSTests COMMAND dds_tests)
endif()

# Installation
install(TARGETS dds_client dds_job_manager dds_worker
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY config/ DESTINATION config)
install(DIRECTORY scripts/ DESTINATION scripts)

# Create package
include(CPack)
set(CPACK_PACKAGE_NAME "DistributedDataScience")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Distributed Data Science Framework")
set(CPACK_PACKAGE_VENDOR "DDS Team")
set(CPACK_PACKAGE_CONTACT "dds@example.com")

# Print configuration summary
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "MPI found: ${MPI_CXX_FOUND}")
message(STATUS "Boost found: ${Boost_FOUND}")
message(STATUS "Eigen3 found: ${EIGEN3_FOUND}")
message(STATUS "Hadoop found: ${HADOOP_FOUND}") 