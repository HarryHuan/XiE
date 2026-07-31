// YaoName — 羲引擎驻留名称表（Yao 层 / 羲爻）
// 将字符串驻留到全局名称表，使名称比较变为 O(1) 的整数比较
#pragma once

#include "CoreTypes.h"
#include "YaoString.h"
#include "YaoMap.h"
#include "YaoArray.h"

namespace Xi::Yao
{

/// @brief 名称表条目 — 存储驻留字符串及其索引
struct YaoNameEntry
{
    YaoString m_String;   // 实际字符串
    int32     m_Index;    // 在名称表中的索引（也是 YaoName 内部存储的值）
};

/// @brief 全局名称表（仿 UE FName::GetNameTable）
/// 单例模式，存储所有驻留字符串，支持通过字符串查找或注册新名称
class YaoNameTable
{
public:
    /// @brief 获取全局单例
    static YaoNameTable& Get()
    {
        static YaoNameTable s_Instance;
        return s_Instance;
    }

    /// @brief 根据字符串注册名称（若已存在则返回已有索引）
    int32 Register(const YaoString& InString)
    {
        // 已存在则直接返回索引
        int32* Existing = m_NameMap.Find(InString);
        if (Existing)
        {
            return *Existing;
        }

        // 新名称：添加到表
        int32 NewIndex = m_Entries.Num();
        YaoNameEntry Entry;
        Entry.m_String = InString;
        Entry.m_Index  = NewIndex;
        m_Entries.Add(std::move(Entry));
        m_NameMap.Add(InString, NewIndex);
        return NewIndex;
    }

    /// @brief 根据索引获取名称字符串
    const YaoString& GetString(int32 Index) const
    {
        xiCheck(Index >= 0 && Index < m_Entries.Num());
        return m_Entries[Index].m_String;
    }

private:
    YaoNameTable() = default;

    YaoArray<YaoNameEntry> m_Entries;        // 名称条目数组（索引即位置）
    YaoMap<YaoString, int32> m_NameMap;      // 字符串 → 索引 快速查找
};

/// @brief 名称类 — 驻留字符串的轻量句柄
/// 拷贝和比较都很廉价（只是 int32 的拷贝/比较）
class YaoName
{
public:
    // ── 构造 ─────────────────────────────────────
    /// @brief 默认构造（空名称，索引为 0，对应 "None"）
    YaoName()
        : m_NameIndex(0)
    {
        // 确保空字符串 "None" 在名称表中（仅第一次注册）
        static bool s_bNoneRegistered = false;
        if (!s_bNoneRegistered)
        {
            YaoNameTable::Get().Register("None");
            s_bNoneRegistered = true;
        }
    }

    /// @brief 从 C 字符串构造（自动注册到名称表）
    YaoName(const char* InStr)
    {
        m_NameIndex = YaoNameTable::Get().Register(YaoString(InStr));
    }

    /// @brief 从 YaoString 构造
    YaoName(const YaoString& InStr)
    {
        m_NameIndex = YaoNameTable::Get().Register(InStr);
    }

    /// @brief 从已有索引构造（内部使用，不检查有效性）
    explicit YaoName(int32 InIndex)
        : m_NameIndex(InIndex)
    {
    }

    // ── 访问 ─────────────────────────────────────
    /// @brief 获取名称的字符串表示
    const YaoString& ToString() const
    {
        return YaoNameTable::Get().GetString(m_NameIndex);
    }

    /// @brief 获取内部索引
    int32 GetIndex() const { return m_NameIndex; }

    /// @brief 是否为默认空名称
    bool IsNone() const { return m_NameIndex == 0; }

    // ── 比较 ─────────────────────────────────────
    bool operator==(const YaoName& Other) const
    {
        return m_NameIndex == Other.m_NameIndex;
    }

    bool operator!=(const YaoName& Other) const
    {
        return m_NameIndex != Other.m_NameIndex;
    }

    // ── 哈希支持 ─────────────────────────────────
    struct HashFunc
    {
        size_t operator()(const YaoName& Name) const
        {
            return static_cast<size_t>(Name.m_NameIndex);
        }
    };

private:
    int32 m_NameIndex;  // 名称表中的索引
};

} // namespace Xi::Yao
