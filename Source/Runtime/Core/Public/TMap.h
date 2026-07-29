// TMap.h — 羲引擎哈希映射容器
// 仿 UE TMap，开放定址法 + 线性探测的哈希表，键值对存储
#pragma once

#include "CoreTypes.h"
#include "TArray.h"
#include <utility>       // std::move, std::pair
#include <functional>    // std::hash

/// @brief 哈希映射模板 — 键值对容器
/// @tparam KeyType   键类型（需支持 operator== 和 std::hash）
/// @tparam ValueType 值类型
template<typename KeyType, typename ValueType>
class TMap
{
    // ── 内部结构 ─────────────────────────────────
    /// @brief 哈希表中的一个槽位
    struct FSlot
    {
        bool  bOccupied = false;     // 是否被占用
        bool  bTombstone = false;    // 是否为墓碑（删除后留下的标记）
        KeyType   Key;
        ValueType Value;

        FSlot() = default;
    };

public:
    // ── 类型别名 ────────────────────────────────
    using FKey   = KeyType;
    using FValue = ValueType;

    // ── 构造 / 析构 ─────────────────────────────
    TMap() = default;
    ~TMap() = default;

    TMap(const TMap& Other)
    {
        CopyFrom(Other);
    }

    TMap& operator=(const TMap& Other)
    {
        if (this != &Other)
        {
            CopyFrom(Other);
        }
        return *this;
    }

    TMap(TMap&& Other) noexcept
        : Slots(std::move(Other.Slots)), NumElements(Other.NumElements)
    {
        Other.NumElements = 0;
    }

    TMap& operator=(TMap&& Other) noexcept
    {
        if (this != &Other)
        {
            Slots = std::move(Other.Slots);
            NumElements = Other.NumElements;
            Other.NumElements = 0;
        }
        return *this;
    }

    // ── 元素访问 ─────────────────────────────────
    /// @brief 访问键对应的值（键不存在时断言失败）
    ValueType& operator[](const KeyType& Key)
    {
        int32 Index = FindSlot(Key);
        xiCheck(Index != -1);  // 键必须存在
        return Slots[Index].Value;
    }

    const ValueType& operator[](const KeyType& Key) const
    {
        int32 Index = FindSlot(Key);
        xiCheck(Index != -1);
        return Slots[Index].Value;
    }

    /// @brief 查找键，返回值的指针；未找到返回 nullptr
    ValueType* Find(const KeyType& Key)
    {
        int32 Index = FindSlot(Key);
        return (Index != -1) ? &Slots[Index].Value : nullptr;
    }

    const ValueType* Find(const KeyType& Key) const
    {
        int32 Index = FindSlot(Key);
        return (Index != -1) ? &Slots[Index].Value : nullptr;
    }

    /// @brief 查找键，通过引用返回值；未找到返回 false
    bool Find(const KeyType& Key, ValueType& OutValue) const
    {
        int32 Index = FindSlot(Key);
        if (Index != -1)
        {
            OutValue = Slots[Index].Value;
            return true;
        }
        return false;
    }

    // ── 容量查询 ─────────────────────────────────
    /// @brief 当前键值对数量
    int32 Num() const { return NumElements; }

    /// @brief 是否为空
    bool  IsEmpty() const { return NumElements == 0; }

    // ── 修改操作 ─────────────────────────────────
    /// @brief 插入或更新键值对
    void Add(const KeyType& Key, const ValueType& Value)
    {
        EnsureCapacity(NumElements + 1);
        InsertInto(Key, Value);
    }

    /// @brief 插入或更新键值对（移动版本）
    void Add(KeyType&& Key, ValueType&& Value)
    {
        EnsureCapacity(NumElements + 1);
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
            Slots[Index].Key.~KeyType();
            Slots[Index].Value.~ValueType();
            Slots[Index].bOccupied  = false;
            Slots[Index].bTombstone = true;
            --NumElements;
        }
    }

    /// @brief 清空所有键值对
    void Clear()
    {
        for (FSlot& Slot : Slots)
        {
            if (Slot.bOccupied)
            {
                Slot.Key.~KeyType();
                Slot.Value.~ValueType();
                Slot.bOccupied  = false;
                Slot.bTombstone = false;
            }
        }
        NumElements = 0;
        // 重置槽位数组为初始小容量
        Slots.Clear();
        Slots.Shrink();
    }

    // ── 查询 ─────────────────────────────────────
    /// @brief 是否包含某键
    bool Contains(const KeyType& Key) const
    {
        return FindSlot(Key) != -1;
    }

    // ── 迭代（遍历所有键值对） ───────────────────
    /// @brief 对每个键值对调用回调
    template<typename Func>
    void ForEach(Func&& Callback)
    {
        for (FSlot& Slot : Slots)
        {
            if (Slot.bOccupied)
            {
                Callback(Slot.Key, Slot.Value);
            }
        }
    }

    template<typename Func>
    void ForEach(Func&& Callback) const
    {
        for (const FSlot& Slot : Slots)
        {
            if (Slot.bOccupied)
            {
                Callback(Slot.Key, Slot.Value);
            }
        }
    }

private:
    // ── 内部方法 ─────────────────────────────────
    /// @brief 哈希槽位数（至少为 8）
    int32 SlotCount() const
    {
        int32 N = Slots.Num();
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
        TArray<FSlot> OldSlots = std::move(Slots);

        // 分配新槽位（默认构造的空槽位）
        Slots = TArray<FSlot>();
        Slots.Reserve(NewSlotCount);
        for (int32 i = 0; i < NewSlotCount; ++i)
        {
            Slots.Emplace();  // 默认构造 FSlot
        }

        // 重新插入所有有效元素
        for (FSlot& OldSlot : OldSlots)
        {
            if (OldSlot.bOccupied)
            {
                InsertInto(std::move(OldSlot.Key), std::move(OldSlot.Value));
                // 旧槽位析构
                OldSlot.Key.~KeyType();
                OldSlot.Value.~ValueType();
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
            const FSlot& Slot = Slots[Index];

            if (Slot.bTombstone)
            {
                // 墓碑槽位：继续探测（后面可能还有相同键的元素）
                continue;
            }

            if (!Slot.bOccupied)
            {
                // 空槽位：键不存在
                return -1;
            }

            if (Slot.Key == Key)
            {
                // 找到了！
                return Index;
            }
        }

        return -1;  // 表满了（不应该出现，因为负载因子 < 0.7）
    }

    /// @brief 在现有槽位数组中插入键值对（假定已确保容量且键不存在或容量足够）
    void InsertInto(const KeyType& Key, const ValueType& Value)
    {
        int32 N = SlotCount();
        size_t Hash  = std::hash<KeyType>{}(Key);
        int32  Start = static_cast<int32>(Hash % N);

        // 找到第一个空槽位（可复用墓碑）
        for (int32 Probe = 0; Probe < N; ++Probe)
        {
            int32 Index = (Start + Probe) % N;
            FSlot& Slot = Slots[Index];

            if (Slot.bOccupied && Slot.Key == Key)
            {
                // 键已存在，更新值
                Slot.Value = Value;
                return;
            }

            if (!Slot.bOccupied)
            {
                // 找到空槽位，插入
                new (&Slot.Key) KeyType(Key);
                new (&Slot.Value) ValueType(Value);
                Slot.bOccupied  = true;
                Slot.bTombstone = false;
                ++NumElements;
                return;
            }
        }
    }

    /// @brief 插入键值对（移动版本）
    void InsertInto(KeyType&& Key, ValueType&& Value)
    {
        int32 N = SlotCount();
        size_t Hash  = std::hash<KeyType>{}(Key);
        int32  Start = static_cast<int32>(Hash % N);

        for (int32 Probe = 0; Probe < N; ++Probe)
        {
            int32 Index = (Start + Probe) % N;
            FSlot& Slot = Slots[Index];

            if (Slot.bOccupied && Slot.Key == Key)
            {
                Slot.Value = std::move(Value);
                return;
            }

            if (!Slot.bOccupied)
            {
                new (&Slot.Key) KeyType(std::move(Key));
                new (&Slot.Value) ValueType(std::move(Value));
                Slot.bOccupied  = true;
                Slot.bTombstone = false;
                ++NumElements;
                return;
            }
        }
    }

    /// @brief 从另一个 TMap 拷贝
    void CopyFrom(const TMap& Other)
    {
        Clear();
        // 确保容量一致
        int32 OtherSlotCount = Other.SlotCount();
        if (OtherSlotCount > 0)
        {
            Slots.Reserve(OtherSlotCount);
            for (int32 i = 0; i < OtherSlotCount; ++i)
            {
                Slots.Emplace();  // 空槽位
            }
            // 拷贝每个元素
            for (int32 i = 0; i < OtherSlotCount; ++i)
            {
                const FSlot& OtherSlot = Other.Slots[i];
                if (OtherSlot.bOccupied)
                {
                    InsertInto(OtherSlot.Key, OtherSlot.Value);
                }
            }
        }
    }

    // ── 成员变量 ─────────────────────────────────
    TArray<FSlot> Slots;        // 哈希槽位数组
    int32         NumElements = 0;  // 当前键值对数量
};
