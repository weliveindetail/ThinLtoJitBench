cmake_minimum_required(VERSION 3.4.3)
project(ThinLtoJitBench)

message("")
message("Global")

message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "C++ flags Debug: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "C++ flags Release: ${CMAKE_CXX_FLAGS_RELEASE}")

# Add LLVM libraries
find_package(LLVM REQUIRED PATHS ${LLVM_DIR})
message(STATUS "LLVM_DIR: ${LLVM_DIR}")
message(STATUS "LLVM_PACKAGE_VERSION: ${LLVM_PACKAGE_VERSION}")

message(STATUS "LLVM Source Directory: ${LLVM_BUILD_MAIN_SRC_DIR}")
message(STATUS "LLVM Build Directory: ${LLVM_BUILD_BINARY_DIR}")

# Create target
add_executable(ThinLtoJitBench
    FrontloadBench.cpp
    common.h
    main.cpp)

target_compile_options(ThinLtoJitBench PUBLIC -std=c++14 -fno-exceptions -fno-rtti)

target_include_directories(ThinLtoJitBench PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LLVM_INCLUDE_DIRS}
)

# LLVM definitions
separate_arguments(LLVM_DEFINITIONS)
target_compile_definitions(ThinLtoJitBench PRIVATE
    ${LLVM_DEFINITIONS} -DBITCODE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/bitcode")

# JitFromScratch dependencies
llvm_map_components_to_libnames(LLVM_LIBS
    core
    executionengine
    native
    object
    orcjit
    runtimedyld
    support
)

target_link_libraries(ThinLtoJitBench PRIVATE
    ${LLVM_LIBS} benchmark)

# Add Google Benchmark
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Don't include tests for benchmark itself" FORCE)

cmake_policy(PUSH)
add_subdirectory(benchmark)
cmake_policy(POP)

message("")
message("ThinLtoJitBench")

get_target_property(Prop ThinLtoJitBench INCLUDE_DIRECTORIES)
if (NOT Prop STREQUAL "Prop-NOTFOUND")
    message(STATUS "Include directories: ${Prop}")
endif()

get_target_property(Prop ThinLtoJitBench LINK_LIBRARIES)
if (NOT Prop STREQUAL "Prop-NOTFOUND")
    message(STATUS "Link libraries: ${Prop}")
endif()

get_target_property(Prop ThinLtoJitBench COMPILE_OPTIONS)
if (NOT Prop STREQUAL "Prop-NOTFOUND")
    message(STATUS "Compile options: ${Prop}")
endif()

get_target_property(Prop ThinLtoJitBench COMPILE_DEFINITIONS)
if (NOT Prop STREQUAL "Prop-NOTFOUND")
    message(STATUS "Compile definitions: ${Prop}")
endif()

message("")
message("Global")
