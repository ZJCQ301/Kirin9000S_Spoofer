cmake_minimum_required(VERSION 3.10)
project(Kirin9000S_Spoofer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(kirin9000s_spoofer SHARED main.cpp)

# 链接 Android 日志库（修复 __android_log_print 未定义错误）
target_link_libraries(kirin9000s_spoofer log)

# 当前目录就是头文件搜索路径
target_include_directories(kirin9000s_spoofer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
