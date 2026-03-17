cmake_minimum_required(VERSION 3.15)
project(congenial_waffle CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC 以 UTF-8 读取源文件，避免中文注释导致解析错误
if(MSVC)
    add_compile_options(/utf-8)
endif()

# ── 库源文件 ──────────────────────────────────────────────────────────────────
add_library(config_lib STATIC
    ConfigInstance.cpp
    ConfigManager.cpp
    xmlsetting/XmlSettings.cpp
    xmlsetting/tinyxml2.cpp
)
target_include_directories(config_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# ── 测试可执行文件 ────────────────────────────────────────────────────────────
add_executable(config_test test/config_test.cpp)
target_link_libraries(config_test PRIVATE config_lib)
