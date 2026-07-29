// TArray.h — 羲引擎动态数组容器
// 仿 UE TArray，提供类似 std::vector 但 API 命名更符合 UE 风格的动态数组
#pragma once

#include "CoreTypes.h"
#include <utility>    // std::move, std::forward
#include <initializer_list>

/// @brief 动态数组模板 — 引擎最常用的容器类型
/// @tparam T 元素类型（必须可移动 / 可拷贝）
template<typename T>
class TArray
{
public:
    // ── 类型别名 ────────────────────────────────
    using ElementType    = T;
    using Iterator       = T*;
    using ConstIterator  = const T*;

    // ── 构造 / 析构 ─────────────────────────────
    TArray() = default;

    /// @brief 用 initializer_list 初始化（支持 TArray<int> arr = {1,2,3}）
    TArray(std::initializer_list<T> InitList)
    {
        ResizeToFit(static_cast<int32>(InitList.size()));
        for (const T& Item : InitList)
        {
            new (Data + Count) T(Item);  // placement new
            ++Count;
        }
    }

    /// @brief 预分配 count 个默认元素
    explicit TArray(int32 InCount)
    {
        ResizeToFit(InCount);
        for (int32 i = 0; i < InCount; ++i)
        {
            new (Data + i) T();
        }
        Count = InCount;
    }

    ~TArray()
    {
        Clear();
        FreeData();
    }

    // ── 拷贝 ─────────────────────────────────────
    TArray(const TArray& Other)
    {
        CopyFrom(Other);
    }

    TArray& operator=(const TArray& Other)
    {
        if (this != &Other)
        {
            Clear();
            FreeData();
            CopyFrom(Other);
        }
        return *this;
    }

    // ── 移动 ─────────────────────────────────────
    TArray(TArray&& Other) noexcept
        : Data(Other.Data), Count(Other.Count), Capacity(Other.Capacity)
    {
        Other.Data     = nullptr;
        Other.Count    = 0;
        Other.Capacity = 0;
    }

    TArray& operator=(TArray&& Other) noexcept
    {
        if (this != &Other)
        {
            Clear();
            FreeData();
            Data     = Other.Data;
            Count    = Other.Count;
            Capacity = Other.Capacity;
            Other.Data     = nullptr;
            Other.Count    = 0;
            Other.Capacity = 0;
        }
        return *this;
    }

    // ── 元素访问 ─────────────────────────────────
    T& operator[](int32 Index)
    {
        xiCheck(Index >= 0 && Index < Count);
        return Data[Index];
    }

    const T& operator[](int32 Index) const
    {
        xiCheck(Index >= 0 && Index < Count);
        return Data[Index];
    }

    /// @brief 获取第一个元素
    T& First()
    {
        xiCheck(Count > 0);
        return Data[0];
    }

    const T& First() const
    {
        xiCheck(Count > 0);
        return Data[0];
    }

    /// @brief 获取最后一个元素
    T& Last()
    {
        xiCheck(Count > 0);
        return Data[Count - 1];
    }

    const T& Last() const
    {
        xiCheck(Count > 0);
        return Data[Count - 1];
    }

    /// @brief 获取原始数据指针
    T* GetData()             { return Data; }
    const T* GetData() const { return Data; }

    // ── 容量查询 ─────────────────────────────────
    /// @brief 当前元素数量
    int32 Num() const { return Count; }

    /// @brief 是否为空
    bool  IsEmpty() const { return Count == 0; }

    /// @brief 当前分配的容量（元素个数）
    int32 GetCapacity() const { return Capacity; }

    /// @brief 最大可能元素数
    int32 Max() const { return INT32_MAX; }

    // ── 修改操作 ─────────────────────────────────
    /// @brief 尾部添加元素（拷贝版本）
    T& Add(const T& Item)
    {
        EnsureCapacity(Count + 1);
        new (Data + Count) T(Item);
        ++Count;
        return Data[Count - 1];
    }

    /// @brief 尾部添加元素（移动版本）
    T& Add(T&& Item)
    {
        EnsureCapacity(Count + 1);
        new (Data + Count) T(std::move(Item));
        ++Count;
        return Data[Count - 1];
    }

    /// @brief 原地构造元素，返回引用
    template<typename... Args>
    T& Emplace(Args&&... args)
    {
        EnsureCapacity(Count + 1);
        new (Data + Count) T(std::forward<Args>(args)...);
        ++Count;
        return Data[Count - 1];
    }

    /// @brief 移除末尾元素
    void Pop()
    {
        xiCheck(Count > 0);
        --Count;
        Data[Count].~T();
    }

    /// @brief 按索引移除元素（保持顺序，O(n)）
    void RemoveAt(int32 Index)
    {
        xiCheck(Index >= 0 && Index < Count);
        Data[Index].~T();
        // 后续元素前移
        for (int32 i = Index; i < Count - 1; ++i)
        {
            new (Data + i) T(std::move(Data[i + 1]));
            Data[i + 1].~T();
        }
        --Count;
    }

    /// @brief 按索引移除元素（不保持顺序，O(1)）
    void RemoveAtSwap(int32 Index)
    {
        xiCheck(Index >= 0 && Index < Count);
        if (Index != Count - 1)
        {
            Data[Index].~T();
            new (Data + Index) T(std::move(Data[Count - 1]));
        }
        Data[Count - 1].~T();
        --Count;
    }

    /// @brief 清空所有元素（保持分配的容量）
    void Clear()
    {
        for (int32 i = 0; i < Count; ++i)
        {
            Data[i].~T();
        }
        Count = 0;
    }

    /// @brief 预分配容量
    void Reserve(int32 NewCapacity)
    {
        if (NewCapacity > Capacity)
        {
            Reallocate(NewCapacity);
        }
    }

    /// @brief 回收多余容量，使 Capacity == Count
    void Shrink()
    {
        if (Count < Capacity)
        {
            Reallocate(Count);
        }
    }

    // ── 迭代器 ───────────────────────────────────
    Iterator       begin()       { return Data; }
    Iterator       end()         { return Data + Count; }
    ConstIterator  begin() const { return Data; }
    ConstIterator  end()   const { return Data + Count; }

    // ── 查找 —————————————————————————————————————
    /// @brief 线性查找元素，返回索引；未找到返回 -1
    int32 Find(const T& Item) const
    {
        for (int32 i = 0; i < Count; ++i)
        {
            if (Data[i] == Item)
            {
                return i;
            }
        }
        return -1;
    }

    /// @brief 是否包含某元素
    bool Contains(const T& Item) const
    {
        return Find(Item) != -1;
    }

private:
    // ── 内部辅助方法 ─────────────────────────────
    /// @brief 确保有足够容量容纳 NewCount 个元素
    void EnsureCapacity(int32 Needed)
    {
        if (Needed > Capacity)
        {
            // 扩容策略：至少翻倍，满足需求
            int32 NewCapacity = (Capacity > 0) ? Capacity * 2 : 4;
            if (NewCapacity < Needed)
            {
                NewCapacity = Needed;
            }
            Reallocate(NewCapacity);
        }
    }

    /// @brief 调整容量以恰好容纳 Needed 个元素
    void ResizeToFit(int32 Needed)
    {
        if (Needed > Capacity || Needed < Capacity / 2)
        {
            Reallocate(Needed);
        }
    }

    /// @brief 重新分配内存，移动旧元素
    void Reallocate(int32 NewCapacity)
    {
        if (NewCapacity == 0)
        {
            FreeData();
            return;
        }

        // 分配新内存（原始字节，不构造对象）
        T* NewData = static_cast<T*>(::operator new(NewCapacity * sizeof(T)));

        // 移动已有元素到新内存
        int32 ElementsToMove = (Count < NewCapacity) ? Count : NewCapacity;
        for (int32 i = 0; i < ElementsToMove; ++i)
        {
            new (NewData + i) T(std::move(Data[i]));
        }

        // 析构旧元素并释放旧内存
        for (int32 i = 0; i < Count; ++i)
        {
            Data[i].~T();
        }
        ::operator delete(Data);

        Data     = NewData;
        Count    = ElementsToMove;
        Capacity = NewCapacity;
    }

    /// @brief 释放所有内存
    void FreeData()
    {
        ::operator delete(Data);
        Data     = nullptr;
        Capacity = 0;
    }

    /// @brief 从另一个 TArray 拷贝
    void CopyFrom(const TArray& Other)
    {
        Count    = 0;
        Capacity = 0;
        Data     = nullptr;
        EnsureCapacity(Other.Count);
        for (int32 i = 0; i < Other.Count; ++i)
        {
            new (Data + i) T(Other.Data[i]);
        }
        Count = Other.Count;
    }

    // ── 成员变量 ─────────────────────────────────
    T*    Data     = nullptr;  // 元素内存块
    int32 Count    = 0;       // 当前元素数
    int32 Capacity = 0;       // 已分配容量
};
