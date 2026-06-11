cmake_minimum_required(VERSION 3.5)
project(Kirin9000S_Spoofer)

add_library(kirin9000s_spoofer SHARED main.cpp)
target_compile_features(kirin9000s_spoofer PRIVATE cxx_std_17)
set_target_properties(kirin9000s_spoofer PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
