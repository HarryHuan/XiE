// CoreTypes.h — 羲引擎基础类型定义（Yao 层 / 羲爻）
// 仿 UE 命名规范，定义引擎内统一的整数、浮点、字符类型别名
#pragma once

#include <cstdint>   // int32_t, uint32_t 等
#include <cstddef>   // size_t, ptrdiff_t
#include <cassert>   // assert

namespace Xi::Yao
{

// ── 平台检测 ────────────────────────────────────
#if defined(_WIN32) || defined(_WIN64)
    #define XI_PLATFORM_WINDOWS 1
#elif defined(__APPLE__)
    #define XI_PLATFORM_MAC 1
#elif defined(__linux__)
    #define XI_PLATFORM_LINUX 1
#endif

// ── 编译器检测 ──────────────────────────────────
#if defined(_MSC_VER)
    #define XI_COMPILER_MSVC 1
#elif defined(__clang__)
    #define XI_COMPILER_CLANG 1
#elif defined(__GNUC__)
    #define XI_COMPILER_GCC 1
#endif

// ── 整数类型别名（仿 UE：有符号 int8→int64, 无符号 uint8→uint64） ──
using int8   = int8_t;
using int16  = int16_t;
using int32  = int32_t;
using int64  = int64_t;
using uint8  = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

// ── 常用类型别名 ────────────────────────────────
using schar  = signed char;
using uchar  = unsigned char;

// ── 断言宏 ──────────────────────────────────────
#define xiCheck(Expr)        assert(Expr)
#define xiCheckf(Expr, Msg, ...)  assert(Expr)  // 简化版：消息暂未实现

// ── 常用常量 ────────────────────────────────────
constexpr float  XI_PI     = 3.14159265358979323846f;
constexpr double XI_PI_D   = 3.14159265358979323846;
constexpr float  XI_DEG2RAD = XI_PI / 180.0f;
constexpr float  XI_RAD2DEG = 180.0f / XI_PI;

// ── 禁用拷贝 / 移动的辅助宏 ─────────────────────
#define XI_DISABLE_COPY(ClassName)         \
    ClassName(const ClassName&) = delete;  \
    ClassName& operator=(const ClassName&) = delete

#define XI_DISABLE_MOVE(ClassName)         \
    ClassName(ClassName&&) = delete;       \
    ClassName& operator=(ClassName&&) = delete

// ── 对齐分配 ────────────────────────────────────
#define XI_ALIGNAS(N) alignas(N)

// ── 强制内联 / 从不内联 ─────────────────────────
#if XI_COMPILER_MSVC
    #define XI_FORCEINLINE __forceinline
    #define XI_NOINLINE    __declspec(noinline)
#else
    #define XI_FORCEINLINE __attribute__((always_inline)) inline
    #define XI_NOINLINE    __attribute__((noinline))
#endif

// ── 前向声明宏（减少 include 依赖） ──────────────
#define XI_FORWARD_DECLARE_CLASS(ClassName)  class ClassName
#define XI_FORWARD_DECLARE_STRUCT(StructName) struct StructName

} // namespace Xi::Yao
