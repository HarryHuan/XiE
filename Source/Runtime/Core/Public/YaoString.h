// YaoString — 羲引擎字符串类（Yao 层 / 羲爻）
// UTF-8 编码的可变字符串，支持格式化、拼接、查找
#pragma once

#include "CoreTypes.h"
#include <cstring>     // strlen, strcmp, memcpy
#include <cstdio>      // vsnprintf
#include <cstdarg>     // va_list
#include <ostream>     // std::ostream（调试输出）

namespace Xi::Yao
{

/// @brief 可变字符串类 — 引擎的主要文本容器
class YaoString
{
public:
    // ── 构造 / 析构 ─────────────────────────────
    /// @brief 默认构造（空字符串）
    YaoString() = default;

    /// @brief 从 C 字符串构造（拷贝）
    YaoString(const char* InStr)
    {
        if (InStr)
        {
            int32 Len = static_cast<int32>(strlen(InStr));
            Assign(InStr, Len);
        }
    }

    /// @brief 从指定长度的 C 字符串构造
    YaoString(const char* InStr, int32 InLen)
    {
        if (InStr && InLen > 0)
        {
            Assign(InStr, InLen);
        }
    }

    /// @brief 析构
    ~YaoString()
    {
        delete[] m_Data;
    }

    // ── 拷贝 ─────────────────────────────────────
    YaoString(const YaoString& Other)
    {
        if (Other.m_Length > 0)
        {
            Assign(Other.m_Data, Other.m_Length);
        }
    }

    YaoString& operator=(const YaoString& Other)
    {
        if (this != &Other)
        {
            delete[] m_Data;
            m_Data   = nullptr;
            m_Length = 0;
            if (Other.m_Length > 0)
            {
                Assign(Other.m_Data, Other.m_Length);
            }
        }
        return *this;
    }

    // ── 移动 ─────────────────────────────────────
    YaoString(YaoString&& Other) noexcept
        : m_Data(Other.m_Data), m_Length(Other.m_Length)
    {
        Other.m_Data   = nullptr;
        Other.m_Length = 0;
    }

    YaoString& operator=(YaoString&& Other) noexcept
    {
        if (this != &Other)
        {
            delete[] m_Data;
            m_Data   = Other.m_Data;
            m_Length = Other.m_Length;
            Other.m_Data   = nullptr;
            Other.m_Length = 0;
        }
        return *this;
    }

    // ── 访问 ─────────────────────────────────────
    /// @brief 返回 C 字符串（以 null 结尾，空字符串返回 ""）
    const char* CStr() const
    {
        if (!m_Data)
        {
            const_cast<YaoString*>(this)->Assign("", 0);
        }
        return m_Data;
    }

    /// @brief 返回字符串长度（不含 null 终结符）
    int32 Len() const { return m_Length; }

    /// @brief 是否为空
    bool  IsEmpty() const { return m_Length == 0; }

    /// @brief 下标访问（带边界检查）
    char  operator[](int32 Index) const
    {
        xiCheck(Index >= 0 && Index < m_Length);
        return m_Data[Index];
    }

    char& operator[](int32 Index)
    {
        xiCheck(Index >= 0 && Index < m_Length);
        return m_Data[Index];
    }

    // ── 比较 ─────────────────────────────────────
    bool operator==(const YaoString& Other) const
    {
        if (m_Length != Other.m_Length) return false;
        if (m_Length == 0) return true;
        return strcmp(CStr(), Other.CStr()) == 0;
    }

    bool operator!=(const YaoString& Other) const
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
    YaoString operator+(const YaoString& Other) const
    {
        YaoString Result;
        Result.m_Length = m_Length + Other.m_Length;
        Result.m_Data = new char[Result.m_Length + 1];
        if (m_Length > 0)       memcpy(Result.m_Data, m_Data, m_Length);
        if (Other.m_Length > 0) memcpy(Result.m_Data + m_Length, Other.m_Data, Other.m_Length);
        Result.m_Data[Result.m_Length] = '\0';
        return Result;
    }

    YaoString operator+(const char* Other) const
    {
        return *this + YaoString(Other);
    }

    YaoString& operator+=(const YaoString& Other)
    {
        return (*this = *this + Other);
    }

    YaoString& operator+=(const char* Other)
    {
        return (*this = *this + YaoString(Other));
    }

    // ── 查找 ─────────────────────────────────────
    /// @brief 查找子串，返回索引；未找到返回 -1
    int32 Find(const YaoString& SubStr, int32 StartPos = 0) const
    {
        if (SubStr.m_Length == 0) return (StartPos <= m_Length) ? StartPos : -1;
        if (StartPos + SubStr.m_Length > m_Length) return -1;

        for (int32 i = StartPos; i <= m_Length - SubStr.m_Length; ++i)
        {
            if (strncmp(m_Data + i, SubStr.m_Data, SubStr.m_Length) == 0)
            {
                return i;
            }
        }
        return -1;
    }

    /// @brief 是否以指定前缀开头
    bool StartsWith(const YaoString& Prefix) const
    {
        if (Prefix.m_Length > m_Length) return false;
        return strncmp(m_Data, Prefix.m_Data, Prefix.m_Length) == 0;
    }

    /// @brief 是否以指定后缀结尾
    bool EndsWith(const YaoString& Suffix) const
    {
        if (Suffix.m_Length > m_Length) return false;
        return strncmp(m_Data + m_Length - Suffix.m_Length, Suffix.m_Data, Suffix.m_Length) == 0;
    }

    // ── 修改 ─────────────────────────────────────
    /// @brief 清空字符串
    void Clear()
    {
        delete[] m_Data;
        m_Data = nullptr;
        m_Length = 0;
    }

    // ── 格式化 ───────────────────────────────────
    /// @brief 格式化为字符串（仿 UE FString::Printf）
    static YaoString Printf(const char* Format, ...)
    {
        va_list Args;
        va_start(Args, Format);
        YaoString Result = VPrintf(Format, Args);
        va_end(Args);
        return Result;
    }

    /// @brief va_list 版本的 Printf
    static YaoString VPrintf(const char* Format, va_list Args)
    {
        // 先计算所需缓冲区长度
        va_list ArgsCopy;
        va_copy(ArgsCopy, Args);
        int32 Needed = vsnprintf(nullptr, 0, Format, ArgsCopy);
        va_end(ArgsCopy);

        if (Needed < 0) return YaoString();

        YaoString Result;
        Result.m_Data   = new char[Needed + 1];
        Result.m_Length = Needed;
        vsnprintf(Result.m_Data, Needed + 1, Format, Args);
        Result.m_Data[Needed] = '\0';
        return Result;
    }

private:
    /// @brief 内部分配赋值（将 InLen 个字符从 InStr 拷贝到 m_Data）
    void Assign(const char* InStr, int32 InLen)
    {
        m_Length = InLen;
        m_Data   = new char[m_Length + 1];
        if (InLen > 0)
        {
            memcpy(m_Data, InStr, InLen);
        }
        m_Data[m_Length] = '\0';
    }

    // ── 成员变量 ─────────────────────────────────
    char* m_Data   = nullptr;  // 字符串缓冲区（以 null 结尾）
    int32 m_Length = 0;       // 字符串长度（不含 null）
};

// ── 全局拼接运算符 ──────────────────────────────
inline YaoString operator+(const char* Lhs, const YaoString& Rhs)
{
    return YaoString(Lhs) + Rhs;
}

// ── std::ostream 输出支持（调试用） ──────────────
inline std::ostream& operator<<(std::ostream& Os, const YaoString& Str)
{
    return Os << Str.CStr();
}

} // namespace Xi::Yao

// ── std::hash 特化（使 YaoString 可用于 TMap 等哈希容器） ──
namespace std
{
    template<>
    struct hash<Xi::Yao::YaoString>
    {
        size_t operator()(const Xi::Yao::YaoString& Str) const noexcept
        {
            // FNV-1a 哈希（简单快速）
            size_t Result = 14695981039346656037ULL;
            const char* Data = Str.CStr();
            int Len = Str.Len();
            for (int i = 0; i < Len; ++i)
            {
                Result ^= static_cast<size_t>(static_cast<unsigned char>(Data[i]));
                Result *= 1099511628211ULL;
            }
            return Result;
        }
    };
}
