cmake_minimum_required(VERSION 3.10)
project(Kirin9000S_Spoofer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(kirin9000s_spoofer SHARED main.cpp)

target_link_libraries(kirin9000s_spoofer log)
target_include_directories(kirin9000s_spoofer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
