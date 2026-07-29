# 羲 (Xi) — 根 CMake 工程
# 从零实现的 UE5 核心功能精简版（不含蓝图系统）
cmake_minimum_required(VERSION 3.21)
project(XiE VERSION 0.1.0 LANGUAGES CXX)

# ── C++20 标准 ──────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── MSVC 配置 ───────────────────────────────────
if(MSVC)
    # 使用 UTF-8 源码编码，中文注释不乱码
    add_compile_options(/utf-8)
    # 多线程 DLL 运行时
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# ── 输出目录 ────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ── 全局包含路径（所有模块都能看到 Source） ──────
include_directories(${CMAKE_SOURCE_DIR}/Source)

# ── 运行时模块 ──────────────────────────────────
add_subdirectory(Source/Runtime/Core)
add_subdirectory(Source/Runtime/RHI)

# ── 验证程序（阶段 0：三角形绘制） ──────────────
add_subdirectory(Programs/Testbed)
