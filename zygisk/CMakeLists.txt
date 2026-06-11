cmake_minimum_required(VERSION 3.10)
project(Kirin9000S_Spoofer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(kirin9000s_spoofer SHARED main.cpp)

# 链接日志库
target_link_libraries(kirin9000s_spoofer log)

# 包含当前目录（用于找到 zygisk.hpp）
target_include_directories(kirin9000s_spoofer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
