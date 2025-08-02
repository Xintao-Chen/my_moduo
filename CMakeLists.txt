cmake_minimum_required(VERSION 3.10)    # 指定 CMake 最低版本
project(MyLoggerTest)                   # 工程名字

set(CMAKE_CXX_STANDARD 11)              # 使用 C++11

include_directories(include)            # 包含头文件路径

# 添加要编译的源文件
add_executable(logtest
    main.cc
    src/Logger.cc
    src/Timestamp.cc
)