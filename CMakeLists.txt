cmake_minimum_required(VERSION 3.10)
project(lang-detector C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

add_executable(lang-detector
    src/main.c
    src/lang_scan.c
    src/os_info.c
)

target_include_directories(lang-detector PRIVATE include)
