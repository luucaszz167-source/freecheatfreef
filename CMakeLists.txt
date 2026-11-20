# language: cmake, file: CMakeLists.txt, runtime: Android NDK 26+, target: arm64-v8a
cmake_minimum_required(VERSION 3.22.1)
project("ff_native")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ff_native SHARED
    ff_android_cheat.cpp
    jni_entry.cpp
)

find_library(log-lib log)

target_link_libraries(ff_native
    ${log-lib}
    m
    pthread
)
