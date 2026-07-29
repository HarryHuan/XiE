// FString.h — 羲引擎字符串类
// 仿 UE FString，UTF-8 编码的可变字符串，支持格式化、拼接、查找
#pragma once

#include "CoreTypes.h"
#include <cstring>     // strlen, strcmp, memcpy
#include <cstdio>      // vsnprintf
#include <cstdarg>     // va_list
#include <ostream>     // std::ostream（调试输出）

/// @brief 可变字符串类 — 引擎的主要文本容器
class FString
{
public:
    // ── 构造 / 析构 ─────────────────────────────
    FString() = default;

    /// @brief 从 C 字符串构造（拷贝）
    FString(const char* InStr)
    {
        if (InStr)
        {
            int32 Len = static_cast<int32>(strlen(InStr));
            Assign(InStr, Len);
        }
    }

    /// @brief 从指定长度的 C 字符串构造
    FString(const char* InStr, int32 InLen)
    {
        if (InStr && InLen > 0)
        {
            Assign(InStr, InLen);
        }
    }

    ~FString()
    {
        delete[] Data;
    }

    // ── 拷贝 ─────────────────────────────────────
    FString(const FString& Other)
    {
        if (Other.Length > 0)
        {
            Assign(Other.Data, Other.Length);
        }
    }

    FString& operator=(const FString& Other)
    {
        if (this != &Other)
        {
            delete[] Data;
            Data   = nullptr;
            Length = 0;
            if (Other.Length > 0)
            {
                Assign(Other.Data, Other.Length);
            }
        }
        return *this;
    }

    // ── 移动 ─────────────────────────────────────
    FString(FString&& Other) noexcept
        : Data(Other.Data), Length(Other.Length)
    {
        Other.Data   = nullptr;
        Other.Length = 0;
    }

    FString& operator=(FString&& Other) noexcept
    {
        if (this != &Other)
        {
            delete[] Data;
            Data   = Other.Data;
            Length = Other.Length;
            Other.Data   = nullptr;
            Other.Length = 0;
        }
        return *this;
    }

    // ── 访问 ─────────────────────────────────────
    /// @brief 返回 C 字符串（以 null 结尾）
    const char* CStr() const
    {
        // 懒初始化：空字符串也返回有效的空串
        if (!Data)
        {
            const_cast<FString*>(this)->Assign("", 0);
        }
        return Data;
    }

    /// @brief 返回字符串长度（不含 null 终结符）
    int32 Len() const { return Length; }

    /// @brief 是否为空
    bool  IsEmpty() const { return Length == 0; }

    /// @brief 下标访问（不检查边界）
    char  operator[](int32 Index) const
    {
        xiCheck(Index >= 0 && Index < Length);
        return Data[Index];
    }

    char& operator[](int32 Index)
    {
        xiCheck(Index >= 0 && Index < Length);
        return Data[Index];
    }

    // ── 比较 ─────────────────────────────────────
    bool operator==(const FString& Other) const
    {
        if (Length != Other.Length) return false;
        if (Length == 0) return true;
        return strcmp(CStr(), Other.CStr()) == 0;
    }

    bool operator!=(const FString& Other) const
    {
        return !(*this == Other);
    }

    bool operator==(const char* Other) const
    {
        return strcmp(CStr(), Other ? Other : "") == 0;
    }

    bool operator!=(const char* Other) const
    {
        return !(*this == Other);
    }

    // ── 拼接 ─────────────────────────────────────
    FString operator+(const FString& Other) const
    {
        FString Result;
        Result.Length = Length + Other.Length;
        Result.Data = new char[Result.Length + 1];
        if (Length > 0)    memcpy(Result.Data, Data, Length);
        if (Other.Length > 0) memcpy(Result.Data + Length, Other.Data, Other.Length);
        Result.Data[Result.Length] = '\0';
        return Result;
    }

    FString operator+(const char* Other) const
    {
        return *this + FString(Other);
    }

    FString& operator+=(const FString& Other)
    {
        return (*this = *this + Other);
    }

    FString& operator+=(const char* Other)
    {
        return (*this = *this + FString(Other));
    }

    // ── 查找 ─────────────────────────────────────
    /// @brief 查找子串，返回索引；未找到返回 -1
    int32 Find(const FString& SubStr, int32 StartPos = 0) const
    {
        if (SubStr.Length == 0) return (StartPos <= Length) ? StartPos : -1;
        if (StartPos + SubStr.Length > Length) return -1;

        for (int32 i = StartPos; i <= Length - SubStr.Length; ++i)
        {
            if (strncmp(Data + i, SubStr.Data, SubStr.Length) == 0)
            {
                return i;
            }
        }
        return -1;
    }

    /// @brief 是否以指定前缀开头
    bool StartsWith(const FString& Prefix) const
    {
        if (Prefix.Length > Length) return false;
        return strncmp(Data, Prefix.Data, Prefix.Length) == 0;
    }

    /// @brief 是否以指定后缀结尾
    bool EndsWith(const FString& Suffix) const
    {
        if (Suffix.Length > Length) return false;
        return strncmp(Data + Length - Suffix.Length, Suffix.Data, Suffix.Length) == 0;
    }

    // ── 修改 ─────────────────────────────────────
    /// @brief 清空字符串
    void Clear()
    {
        delete[] Data;
        Data = nullptr;
        Length = 0;
    }

    /// @brief 格式化为字符串（仿 UE FString::Printf）
    static FString Printf(const char* Format, ...)
    {
        va_list Args;
        va_start(Args, Format);
        FString Result = VPrintf(Format, Args);
        va_end(Args);
        return Result;
    }

    /// @brief va_list 版本的 Printf
    static FString VPrintf(const char* Format, va_list Args)
    {
        // 先计算所需缓冲区长度
        va_list ArgsCopy;
        va_copy(ArgsCopy, Args);
        int32 Needed = vsnprintf(nullptr, 0, Format, ArgsCopy);
        va_end(ArgsCopy);

        if (Needed < 0) return FString();

        FString Result;
        Result.Data   = new char[Needed + 1];
        Result.Length = Needed;
        vsnprintf(Result.Data, Needed + 1, Format, Args);
        Result.Data[Needed] = '\0';
        return Result;
    }

private:
    /// @brief 内部分配赋值
    void Assign(const char* InStr, int32 InLen)
    {
        Length = InLen;
        Data   = new char[Length + 1];
        if (InLen > 0)
        {
            memcpy(Data, InStr, InLen);
        }
        Data[Length] = '\0';
    }

    // ── 成员变量 ─────────────────────────────────
    char* Data   = nullptr;  // 字符串缓冲区（以 null 结尾）
    int32 Length = 0;       // 字符串长度（不含 null）
};

// ── 全局拼接运算符 ──────────────────────────────
inline FString operator+(const char* Lhs, const FString& Rhs)
{
    return FString(Lhs) + Rhs;
}

// ── std::ostream 输出支持（调试用） ──────────────
inline std::ostream& operator<<(std::ostream& Os, const FString& Str)
{
    return Os << Str.CStr();
}

// ── std::hash 特化（使 FString 可用于 TMap 等哈希容器） ──
namespace std
{
    template<>
    struct hash<FString>
    {
        size_t operator()(const FString& Str) const noexcept
        {
            // FNV-1a 哈希（简单快速）
            size_t Result = 14695981039346656037ULL;
            const char* Data = Str.CStr();
            int32 Len = Str.Len();
            for (int32 i = 0; i < Len; ++i)
            {
                Result ^= static_cast<size_t>(static_cast<unsigned char>(Data[i]));
                Result *= 1099511628211ULL;
            }
            return Result;
        }
    };
}
