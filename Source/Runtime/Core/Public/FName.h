// FName.h — 羲引擎名称表（驻留字符串）
// 仿 UE FName，将字符串驻留到全局名称表，使名称比较变为 O(1) 的整数比较
#pragma once

#include "CoreTypes.h"
#include "FString.h"
#include "TMap.h"
#include "TArray.h"

/// @brief 名称表条目 — 存储驻留字符串及其索引
struct FNameEntry
{
    FString String;    // 实际字符串
    int32   Index;     // 在名称表中的索引（也是 FName 内部存储的值）
};

/// @brief 全局名称表（仿 UE FName::GetNameTable）
/// 单例模式，存储所有驻留字符串，支持通过字符串查找或注册新名称
class FNameTable
{
public:
    /// @brief 获取全局单例
    static FNameTable& Get()
    {
        static FNameTable Instance;
        return Instance;
    }

    /// @brief 根据字符串注册名称（若已存在则返回已有索引）
    int32 Register(const FString& InString)
    {
        // 已存在则直接返回索引
        int32* Existing = NameMap.Find(InString);
        if (Existing)
        {
            return *Existing;
        }

        // 新名称：添加到表
        int32 NewIndex = Entries.Num();
        FNameEntry Entry;
        Entry.String = InString;
        Entry.Index  = NewIndex;
        Entries.Add(std::move(Entry));
        NameMap.Add(InString, NewIndex);
        return NewIndex;
    }

    /// @brief 根据索引获取名称字符串
    const FString& GetString(int32 Index) const
    {
        xiCheck(Index >= 0 && Index < Entries.Num());
        return Entries[Index].String;
    }

private:
    FNameTable() = default;

    TArray<FNameEntry> Entries;          // 名称条目数组（索引即位置）
    TMap<FString, int32> NameMap;        // 字符串 → 索引 快速查找
};

/// @brief 名称类 — 驻留字符串的轻量句柄
/// 拷贝和比较都很廉价（只是 int32 的拷贝/比较）
class FName
{
public:
    // ── 构造 ─────────────────────────────────────
    /// @brief 默认构造（空名称，索引为 0）
    FName()
        : NameIndex(0)
    {
        // 确保空字符串 "None" 在名称表中
        static bool bNoneRegistered = false;
        if (!bNoneRegistered)
        {
            FNameTable::Get().Register("None");
            bNoneRegistered = true;
        }
    }

    /// @brief 从 C 字符串构造（自动注册到名称表）
    FName(const char* InStr)
    {
        NameIndex = FNameTable::Get().Register(FString(InStr));
    }

    /// @brief 从 FString 构造
    FName(const FString& InStr)
    {
        NameIndex = FNameTable::Get().Register(InStr);
    }

    /// @brief 从已有索引构造（内部使用，不检查有效性）
    explicit FName(int32 InIndex)
        : NameIndex(InIndex)
    {
    }

    // ── 访问 ─────────────────────────────────────
    /// @brief 获取名称的字符串表示
    const FString& ToString() const
    {
        return FNameTable::Get().GetString(NameIndex);
    }

    /// @brief 获取内部索引
    int32 GetIndex() const { return NameIndex; }

    /// @brief 是否为默认空名称
    bool IsNone() const { return NameIndex == 0; }

    // ── 比较 ─────────────────────────────────────
    bool operator==(const FName& Other) const
    {
        return NameIndex == Other.NameIndex;
    }

    bool operator!=(const FName& Other) const
    {
        return NameIndex != Other.NameIndex;
    }

    // ── 哈希支持 ─────────────────────────────────
    struct HashFunc
    {
        size_t operator()(const FName& Name) const
        {
            return static_cast<size_t>(Name.NameIndex);
        }
    };

private:
    int32 NameIndex;  // 名称表中的索引
};
