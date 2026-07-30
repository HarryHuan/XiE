// YaoMap — 羲引擎哈希映射容器（Yao 层 / 羲爻）
// 开放定址法 + 线性探测的哈希表，键值对存储
#pragma once

#include "CoreTypes.h"
#include "TArray.h"
#include <utility>       // std::move, std::pair
#include <functional>    // std::hash

namespace Xi::Yao
{

/// @brief 哈希映射模板 — 键值对容器
/// @tparam KeyType   键类型（需支持 operator== 和 std::hash）
/// @tparam ValueType 值类型
template<typename KeyType, typename ValueType>
class YaoMap
{
    // ── 内部结构：哈希表中的一个槽位 ─────────────
    struct YaoSlot
    {
        bool      m_bOccupied  = false;   // 是否被占用
        bool      m_bTombstone = false;   // 是否为墓碑（删除后留下的标记）
        KeyType   m_Key;
        ValueType m_Value;

        YaoSlot() = default;
    };

public:
    // ── 类型别名 ────────────────────────────────
    using FKey   = KeyType;
    using FValue = ValueType;

    // ── 构造 / 析构 ─────────────────────────────
    YaoMap() = default;
    ~YaoMap() = default;

    YaoMap(const YaoMap& Other)
    {
        CopyFrom(Other);
    }

    YaoMap& operator=(const YaoMap& Other)
    {
        if (this != &Other)
        {
            CopyFrom(Other);
        }
        return *this;
    }

    YaoMap(YaoMap&& Other) noexcept
        : m_Slots(std::move(Other.m_Slots)), m_NumElements(Other.m_NumElements)
    {
        Other.m_NumElements = 0;
    }

    YaoMap& operator=(YaoMap&& Other) noexcept
    {
        if (this != &Other)
        {
            m_Slots = std::move(Other.m_Slots);
            m_NumElements = Other.m_NumElements;
            Other.m_NumElements = 0;
        }
        return *this;
    }

    // ── 元素访问 ─────────────────────────────────
    /// @brief 访问键对应的值（键不存在时断言失败）
    ValueType& operator[](const KeyType& Key)
    {
        int32 Index = FindSlot(Key);
        xiCheck(Index != -1);
        return m_Slots[Index].m_Value;
    }

    const ValueType& operator[](const KeyType& Key) const
    {
        int32 Index = FindSlot(Key);
        xiCheck(Index != -1);
        return m_Slots[Index].m_Value;
    }

    /// @brief 查找键，返回值的指针；未找到返回 nullptr
    ValueType* Find(const KeyType& Key)
    {
        int32 Index = FindSlot(Key);
        return (Index != -1) ? &m_Slots[Index].m_Value : nullptr;
    }

    const ValueType* Find(const KeyType& Key) const
    {
        int32 Index = FindSlot(Key);
        return (Index != -1) ? &m_Slots[Index].m_Value : nullptr;
    }

    /// @brief 查找键，通过引用返回值；未找到返回 false
    bool Find(const KeyType& Key, ValueType& OutValue) const
    {
        int32 Index = FindSlot(Key);
        if (Index != -1)
        {
            OutValue = m_Slots[Index].m_Value;
            return true;
        }
        return false;
    }

    // ── 容量查询 ─────────────────────────────────
    /// @brief 当前键值对数量
    int32 Num() const { return m_NumElements; }

    /// @brief 是否为空
    bool  IsEmpty() const { return m_NumElements == 0; }

    // ── 修改操作 ─────────────────────────────────
    /// @brief 插入或更新键值对（拷贝版本）
    void Add(const KeyType& Key, const ValueType& Value)
    {
        EnsureCapacity(m_NumElements + 1);
        InsertInto(Key, Value);
    }

    /// @brief 插入或更新键值对（移动版本）
    void Add(KeyType&& Key, ValueType&& Value)
    {
        EnsureCapacity(m_NumElements + 1);
        InsertInto(std::move(Key), std::move(Value));
    }

    /// @brief 仅当键不存在时插入，返回是否插入成功
    bool TryAdd(const KeyType& Key, const ValueType& Value)
    {
        if (FindSlot(Key) != -1)
        {
            return false;
        }
        Add(Key, Value);
        return true;
    }

    /// @brief 移除键（键不存在则什么都不做）
    void Remove(const KeyType& Key)
    {
        int32 Index = FindSlot(Key);
        if (Index != -1)
        {
            // 析构数据，标记为墓碑
            m_Slots[Index].m_Key.~KeyType();
            m_Slots[Index].m_Value.~ValueType();
            m_Slots[Index].m_bOccupied  = false;
            m_Slots[Index].m_bTombstone = true;
            --m_NumElements;
        }
    }

    /// @brief 清空所有键值对，重置为最小容量
    void Clear()
    {
        for (YaoSlot& Slot : m_Slots)
        {
            if (Slot.m_bOccupied)
            {
                Slot.m_Key.~KeyType();
                Slot.m_Value.~ValueType();
                Slot.m_bOccupied  = false;
                Slot.m_bTombstone = false;
            }
        }
        m_NumElements = 0;
        m_Slots.Clear();
        m_Slots.Shrink();
    }

    // ── 查询 ─────────────────────────────────────
    /// @brief 是否包含某键
    bool Contains(const KeyType& Key) const
    {
        return FindSlot(Key) != -1;
    }

    // ── 迭代（遍历所有键值对） ───────────────────
    /// @brief 对每个键值对调用回调（可修改版本）
    template<typename Func>
    void ForEach(Func&& Callback)
    {
        for (YaoSlot& Slot : m_Slots)
        {
            if (Slot.m_bOccupied)
            {
                Callback(Slot.m_Key, Slot.m_Value);
            }
        }
    }

    /// @brief 对每个键值对调用回调（只读版本）
    template<typename Func>
    void ForEach(Func&& Callback) const
    {
        for (const YaoSlot& Slot : m_Slots)
        {
            if (Slot.m_bOccupied)
            {
                Callback(Slot.m_Key, Slot.m_Value);
            }
        }
    }

private:
    // ── 内部方法 ─────────────────────────────────
    /// @brief 哈希槽位数（至少为 8）
    int32 SlotCount() const
    {
        int32 N = m_Slots.Num();
        return (N >= 8) ? N : 0;
    }

    /// @brief 确保槽位数组能容纳 NewCount 个元素（负载因子 ≤ 0.7）
    void EnsureCapacity(int32 Needed)
    {
        int32 CurrentCapacity = SlotCount();
        // 当前槽位数可容纳的元素上限 = 槽位数 * 0.7
        int32 MaxElements = static_cast<int32>(CurrentCapacity * 0.7f);
        if (Needed <= MaxElements)
        {
            return;
        }

        // 新的槽位数：至少 8，满足负载因子后满足需求
        int32 NewSlotCount = (CurrentCapacity > 0) ? CurrentCapacity * 2 : 8;
        while (static_cast<int32>(NewSlotCount * 0.7f) < Needed)
        {
            NewSlotCount *= 2;
        }

        Rehash(NewSlotCount);
    }

    /// @brief 以新大小重建哈希表
    void Rehash(int32 NewSlotCount)
    {
        // 保存旧槽位
        YaoArray<YaoSlot> OldSlots = std::move(m_Slots);

        // 分配新槽位（默认构造的空槽位）
        m_Slots = YaoArray<YaoSlot>();
        m_Slots.Reserve(NewSlotCount);
        for (int32 i = 0; i < NewSlotCount; ++i)
        {
            m_Slots.Emplace();  // 默认构造 YaoSlot
        }

        // 重新插入所有有效元素
        for (YaoSlot& OldSlot : OldSlots)
        {
            if (OldSlot.m_bOccupied)
            {
                InsertInto(std::move(OldSlot.m_Key), std::move(OldSlot.m_Value));
                // 旧槽位析构
                OldSlot.m_Key.~KeyType();
                OldSlot.m_Value.~ValueType();
            }
        }
    }

    /// @brief 在现有槽位数组中查找键，返回索引（-1 表示未找到）
    int32 FindSlot(const KeyType& Key) const
    {
        int32 N = SlotCount();
        if (N == 0) return -1;

        size_t Hash   = std::hash<KeyType>{}(Key);
        int32  Start  = static_cast<int32>(Hash % N);

        // 线性探测
        for (int32 Probe = 0; Probe < N; ++Probe)
        {
            int32 Index = (Start + Probe) % N;
            const YaoSlot& Slot = m_Slots[Index];

            if (Slot.m_bTombstone)
            {
                // 墓碑槽位：继续探测（后面可能还有相同键的元素）
                continue;
            }

            if (!Slot.m_bOccupied)
            {
                // 空槽位：键不存在
                return -1;
            }

            if (Slot.m_Key == Key)
            {
                // 找到了！
                return Index;
            }
        }

        return -1;  // 表满了（不应出现，负载因子 < 0.7）
    }

    /// @brief 在现有槽位中插入键值对（拷贝版本）
    void InsertInto(const KeyType& Key, const ValueType& Value)
    {
        int32 N = SlotCount();
        size_t Hash  = std::hash<KeyType>{}(Key);
        int32  Start = static_cast<int32>(Hash % N);

        for (int32 Probe = 0; Probe < N; ++Probe)
        {
            int32 Index = (Start + Probe) % N;
            YaoSlot& Slot = m_Slots[Index];

            if (Slot.m_bOccupied && Slot.m_Key == Key)
            {
                // 键已存在，更新值
                Slot.m_Value = Value;
                return;
            }

            if (!Slot.m_bOccupied)
            {
                // 找到空槽位（含墓碑），插入
                new (&Slot.m_Key) KeyType(Key);
                new (&Slot.m_Value) ValueType(Value);
                Slot.m_bOccupied  = true;
                Slot.m_bTombstone = false;
                ++m_NumElements;
                return;
            }
        }
    }

    /// @brief 在现有槽位中插入键值对（移动版本）
    void InsertInto(KeyType&& Key, ValueType&& Value)
    {
        int32 N = SlotCount();
        size_t Hash  = std::hash<KeyType>{}(Key);
        int32  Start = static_cast<int32>(Hash % N);

        for (int32 Probe = 0; Probe < N; ++Probe)
        {
            int32 Index = (Start + Probe) % N;
            YaoSlot& Slot = m_Slots[Index];

            if (Slot.m_bOccupied && Slot.m_Key == Key)
            {
                Slot.m_Value = std::move(Value);
                return;
            }

            if (!Slot.m_bOccupied)
            {
                new (&Slot.m_Key) KeyType(std::move(Key));
                new (&Slot.m_Value) ValueType(std::move(Value));
                Slot.m_bOccupied  = true;
                Slot.m_bTombstone = false;
                ++m_NumElements;
                return;
            }
        }
    }

    /// @brief 从另一个 YaoMap 拷贝
    void CopyFrom(const YaoMap& Other)
    {
        Clear();
        int32 OtherSlotCount = Other.SlotCount();
        if (OtherSlotCount > 0)
        {
            m_Slots.Reserve(OtherSlotCount);
            for (int32 i = 0; i < OtherSlotCount; ++i)
            {
                m_Slots.Emplace();  // 空槽位
            }
            for (int32 i = 0; i < OtherSlotCount; ++i)
            {
                const YaoSlot& OtherSlot = Other.m_Slots[i];
                if (OtherSlot.m_bOccupied)
                {
                    InsertInto(OtherSlot.m_Key, OtherSlot.m_Value);
                }
            }
        }
    }

    // ── 成员变量 ─────────────────────────────────
    YaoArray<YaoSlot> m_Slots;           // 哈希槽位数组
    int32             m_NumElements = 0; // 当前键值对数量
};

} // namespace Xi::Yao
