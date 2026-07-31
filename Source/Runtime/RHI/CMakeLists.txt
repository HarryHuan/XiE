# RHI 模块 — 渲染硬件接口抽象层
add_library(XiERHI STATIC)

# ── 源文件 ──────────────────────────────────────
target_sources(XiERHI PRIVATE
    OpenGL/YaoOpenGLRHI.cpp
)

# ── 公开头文件 ──────────────────────────────────
target_include_directories(XiERHI PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Public
    ${CMAKE_CURRENT_SOURCE_DIR}/OpenGL
)

# ── 链接依赖 ────────────────────────────────────
target_link_libraries(XiERHI PUBLIC
    XiECore
    # Windows: 需要 opengl32.lib（wgl 函数）
    opengl32
)

target_compile_features(XiERHI PUBLIC cxx_std_20)
