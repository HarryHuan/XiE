# SlateCore 模块 — Slate UI 核心框架
add_library(XiESlateCore STATIC)

target_sources(XiESlateCore PRIVATE
    Private/SlateCoreModule.cpp
)

target_include_directories(XiESlateCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Public
)

target_link_libraries(XiESlateCore PUBLIC
    XiECore
    XiERHI
)

target_compile_features(XiESlateCore PUBLIC cxx_std_20)
