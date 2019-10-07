# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required(VERSION 3.8)

project("Kaleidoscope")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE Debug)

SET(CMAKE_C_COMPILER "/usr/bin/clang")
SET(CMAKE_CXX_COMPILER "/usr/bin/clang++")

# 包含子项目。
add_subdirectory("Kaleidoscope")

