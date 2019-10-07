# CMakeList.txt: Kaleidoscope 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required(VERSION 3.8)

include_directories(/usr/include/llvm-6.0)
include_directories(/usr/include/llvm-c-6.0)

link_directories("/usr/lib/llvm-6.0/lib/")
link_libraries(LLVM)

# 将源代码添加到此项目的可执行文件。
add_executable(Kaleidoscope "Kaleidoscope.cpp")

# TODO: 如有需要，请添加测试并安装目标。
