cmake_minimum_required(VERSION 3.0.0)
project(KMER_GWAS VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include(CTest)
enable_testing()

add_library(gwas_tools STATIC thread_pool.hpp mmap_io.cpp mmap_io.hpp)
target_link_libraries(gwas_tools Threads::Threads)

add_executable(kmer_count kmer_count.cpp)
target_link_libraries(kmer_count gwas_tools)

add_executable(matrix_merge matrix_merge.cpp)
target_link_libraries(matrix_merge gwas_tools)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
