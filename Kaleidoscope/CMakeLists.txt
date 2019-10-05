# CMakeList.txt: Kaleidoscope 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required(VERSION 3.8)

SET(CMAKE_C_COMPILER "/usr/local/clang-9.0.0/bin/clang")
SET(CMAKE_CXX_COMPILER "/usr/local/clang-9.0.0/bin/clang++")
SET(CMAKE_C_FLAGS "-Wall -std=c11")

include_directories(/usr/local/clang-9.0.0/include/)


# 将源代码添加到此项目的可执行文件。
add_executable(Kaleidoscope "Kaleidoscope.cpp")

# TODO: 如有需要，请添加测试并安装目标。
