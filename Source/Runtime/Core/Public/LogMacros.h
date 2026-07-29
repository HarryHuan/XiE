// LogMacros.h — 羲引擎日志系统
// 仿 UE UE_LOG 宏，提供分类、等级的日志输出
#pragma once

#include "CoreTypes.h"
#include "FString.h"
#include <cstdio>      // vfprintf, stdout, stderr
#include <cstdarg>     // va_list
#include <ctime>       // time_t, localtime

/// @brief 日志详细等级（仿 UE ELogVerbosity）
enum class ELogVerbosity : uint8
{
    Fatal,      // 致命错误，程序将终止
    Error,      // 错误
    Warning,    // 警告
    Log,        // 普通日志
    Verbose,    // 详细信息
};

/// @brief 日志分类基类（仿 UE FLogCategoryBase）
/// 每个分类是一个编译期常量，用宏定义
struct FLogCategoryBase
{
    const char* CategoryName;
    ELogVerbosity DefaultVerbosity;

    FLogCategoryBase(const char* InName, ELogVerbosity InVerbosity)
        : CategoryName(InName), DefaultVerbosity(InVerbosity)
    {
    }
};

// ── 日志系统（静态方法集合） ─────────────────────

/// @brief 核心日志函数 — 格式化并输出一条日志
inline void XiLogInternal(
    const char* Category,
    ELogVerbosity Verbosity,
    const char* File,
    int32 Line,
    const char* Format,
    ...)
{
    // 获取时间戳
    time_t Now = time(nullptr);
    struct tm TimeInfo;
    localtime_s(&TimeInfo, &Now);  // Windows 安全版本

    // 将详细等级转为可读字符串
    const char* VerbosityStr = "LOG";
    switch (Verbosity)
    {
        case ELogVerbosity::Fatal:   VerbosityStr = "FATAL";   break;
        case ELogVerbosity::Error:   VerbosityStr = "ERROR";   break;
        case ELogVerbosity::Warning: VerbosityStr = "WARNING"; break;
        case ELogVerbosity::Verbose: VerbosityStr = "VERBOSE"; break;
        default:                     VerbosityStr = "LOG";     break;
    }

    // 打印日志头：[时间] [等级] [分类] 文件:行号
    FILE* Out = (Verbosity <= ELogVerbosity::Error) ? stderr : stdout;
    fprintf(Out, "[%02d:%02d:%02d] [%s] [%s] %s:%d: ",
        TimeInfo.tm_hour, TimeInfo.tm_min, TimeInfo.tm_sec,
        VerbosityStr, Category, File, Line);

    // 格式化消息体
    va_list Args;
    va_start(Args, Format);
    vfprintf(Out, Format, Args);
    va_end(Args);

    fprintf(Out, "\n");
    fflush(Out);

    // Fatal 级别终止程序
    if (Verbosity == ELogVerbosity::Fatal)
    {
        abort();
    }
}

// ── UE_LOG 宏 ───────────────────────────────────

/// @brief 声明一个日志分类（放在头文件中）
/// 用法：DECLARE_LOG_CATEGORY(LogTemp, Log);
#define DECLARE_LOG_CATEGORY(CategoryName, DefaultVerbosity) \
    extern FLogCategoryBase CategoryName

/// @brief 定义一个日志分类（放在一个 .cpp 文件中）
/// 用法：DEFINE_LOG_CATEGORY(LogTemp, Log);
#define DEFINE_LOG_CATEGORY(CategoryName, DefaultVerbosity) \
    FLogCategoryBase CategoryName(#CategoryName, ELogVerbosity::DefaultVerbosity)

/// @brief 输出日志的主宏
/// 用法：UE_LOG(LogTemp, Log, "Value = %d", 42);
#define UE_LOG(Category, Verbosity, Format, ...) \
    do { \
        XiLogInternal( \
            Category.CategoryName, \
            ELogVerbosity::Verbosity, \
            __FILE__, \
            __LINE__, \
            Format, \
            ##__VA_ARGS__ \
        ); \
    } while(0)

/// @brief 简洁日志宏（自动使用 Log 等级和 LogTemp 分类）
#define XI_LOG(Format, ...) \
    XiLogInternal("LogTemp", ELogVerbosity::Log, __FILE__, __LINE__, Format, ##__VA_ARGS__)

#define XI_LOG_WARNING(Format, ...) \
    XiLogInternal("LogTemp", ELogVerbosity::Warning, __FILE__, __LINE__, Format, ##__VA_ARGS__)

#define XI_LOG_ERROR(Format, ...) \
    XiLogInternal("LogTemp", ELogVerbosity::Error, __FILE__, __LINE__, Format, ##__VA_ARGS__)

// ── 条件检查宏 ──────────────────────────────────

/// @brief 条件检查，失败时输出 Fatal 日志并终止
#define xiCheckMsg(Expr, Format, ...) \
    do { \
        if (!(Expr)) { \
            XiLogInternal("Assert", ELogVerbosity::Fatal, __FILE__, __LINE__, \
                "Assertion failed: " #Expr " — " Format, ##__VA_ARGS__); \
        } \
    } while(0)
