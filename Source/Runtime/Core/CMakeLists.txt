# Core 模块 — 基础类型、容器、字符串、日志
# 这是整个引擎的最底层，不允许依赖其他 XiE 模块
add_library(XiECore STATIC)

# ── 源文件 ──────────────────────────────────────
target_sources(XiECore PRIVATE
    Private/CoreModule.cpp
)

# ── 公开头文件 ──────────────────────────────────
target_include_directories(XiECore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Public
)

# ── 链接第三方 ──────────────────────────────────
target_link_libraries(XiECore PUBLIC
    # Core 不依赖任何 XiE 模块，仅链接标准库
)

# ── 编译特性 ────────────────────────────────────
target_compile_features(XiECore PUBLIC cxx_std_20)
