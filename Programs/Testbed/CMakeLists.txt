# Testbed — 阶段 0 验证程序（三角形绘制）
add_executable(XiETestbed)

target_sources(XiETestbed PRIVATE
    Main.cpp
)

target_link_libraries(XiETestbed PRIVATE
    XiECore
    XiERHI
)

target_compile_features(XiETestbed PRIVATE cxx_std_20)
