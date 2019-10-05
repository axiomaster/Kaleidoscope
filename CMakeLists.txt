# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required(VERSION 3.8)

SET(CMAKE_C_COMPILER "/usr/local/clang-9.0.0/bin/clang")
SET(CMAKE_CXX_COMPILER "/usr/local/clang-9.0.0/bin/clang++")
SET(CMAKE_C_FLAGS "-Wall -std=c11")

project("Kaleidoscope")

# 包含子项目。
add_subdirectory("Kaleidoscope")
