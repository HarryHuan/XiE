// YaoArray — 羲引擎动态数组容器（Yao 层 / 羲爻）
// 仿 UE TArray，Yao 前缀表示底层内核层类型
#pragma once

#include "CoreTypes.h"
#include <utility>    // std::move, std::forward
#include <initializer_list>

namespace Xi::Yao
{

/// @brief 动态数组模板 — 引擎最常用的容器类型
/// @tparam T 元素类型（必须可移动 / 可拷贝）
template<typename T>
class YaoArray
{
public:
    // ── 类型别名 ────────────────────────────────
    using ElementType    = T;
    using Iterator       = T*;
    using ConstIterator  = const T*;

    // ── 构造 / 析构 ─────────────────────────────
    /// @brief 默认构造（空数组）
    YaoArray() = default;

    /// @brief 用 initializer_list 初始化（支持 YaoArray<int> arr = {1,2,3}）
    YaoArray(std::initializer_list<T> InitList)
    {
        ResizeToFit(static_cast<int32>(InitList.size()));
        for (const T& Item : InitList)
        {
            new (m_Data + m_Count) T(Item);  // placement new
            ++m_Count;
        }
    }

    /// @brief 预分配 count 个默认元素
    explicit YaoArray(int32 InCount)
    {
        ResizeToFit(InCount);
        for (int32 i = 0; i < InCount; ++i)
        {
            new (m_Data + i) T();
        }
        m_Count = InCount;
    }

    /// @brief 析构所有元素并释放内存
    ~YaoArray()
    {
        Clear();
        FreeData();
    }

    // ── 拷贝 ─────────────────────────────────────
    YaoArray(const YaoArray& Other)
    {
        CopyFrom(Other);
    }

    YaoArray& operator=(const YaoArray& Other)
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
    YaoArray(YaoArray&& Other) noexcept
        : m_Data(Other.m_Data), m_Count(Other.m_Count), m_Capacity(Other.m_Capacity)
    {
        Other.m_Data     = nullptr;
        Other.m_Count    = 0;
        Other.m_Capacity = 0;
    }

    YaoArray& operator=(YaoArray&& Other) noexcept
    {
        if (this != &Other)
        {
            Clear();
            FreeData();
            m_Data     = Other.m_Data;
            m_Count    = Other.m_Count;
            m_Capacity = Other.m_Capacity;
            Other.m_Data     = nullptr;
            Other.m_Count    = 0;
            Other.m_Capacity = 0;
        }
        return *this;
    }

    // ── 元素访问 ─────────────────────────────────
    /// @brief 下标访问（带边界检查）
    T& operator[](int32 Index)
    {
        xiCheck(Index >= 0 && Index < m_Count);
        return m_Data[Index];
    }

    const T& operator[](int32 Index) const
    {
        xiCheck(Index >= 0 && Index < m_Count);
        return m_Data[Index];
    }

    /// @brief 获取第一个元素
    T& First()
    {
        xiCheck(m_Count > 0);
        return m_Data[0];
    }

    const T& First() const
    {
        xiCheck(m_Count > 0);
        return m_Data[0];
    }

    /// @brief 获取最后一个元素
    T& Last()
    {
        xiCheck(m_Count > 0);
        return m_Data[m_Count - 1];
    }

    const T& Last() const
    {
        xiCheck(m_Count > 0);
        return m_Data[m_Count - 1];
    }

    /// @brief 获取原始数据指针
    T* GetData()             { return m_Data; }
    const T* GetData() const { return m_Data; }

    // ── 容量查询 ─────────────────────────────────
    /// @brief 当前元素数量
    int32 Num() const { return m_Count; }

    /// @brief 是否为空
    bool  IsEmpty() const { return m_Count == 0; }

    /// @brief 当前分配的容量（元素个数）
    int32 GetCapacity() const { return m_Capacity; }

    // ── 修改操作 ─────────────────────────────────
    /// @brief 尾部添加元素（拷贝版本），返回新增元素引用
    T& Add(const T& Item)
    {
        EnsureCapacity(m_Count + 1);
        new (m_Data + m_Count) T(Item);
        ++m_Count;
        return m_Data[m_Count - 1];
    }

    /// @brief 尾部添加元素（移动版本），返回新增元素引用
    T& Add(T&& Item)
    {
        EnsureCapacity(m_Count + 1);
        new (m_Data + m_Count) T(std::move(Item));
        ++m_Count;
        return m_Data[m_Count - 1];
    }

    /// @brief 原地构造元素，返回引用
    template<typename... Args>
    T& Emplace(Args&&... args)
    {
        EnsureCapacity(m_Count + 1);
        new (m_Data + m_Count) T(std::forward<Args>(args)...);
        ++m_Count;
        return m_Data[m_Count - 1];
    }

    /// @brief 移除末尾元素
    void Pop()
    {
        xiCheck(m_Count > 0);
        --m_Count;
        m_Data[m_Count].~T();
    }

    /// @brief 按索引移除元素（保持顺序，O(n)）
    void RemoveAt(int32 Index)
    {
        xiCheck(Index >= 0 && Index < m_Count);
        m_Data[Index].~T();
        // 后续元素前移
        for (int32 i = Index; i < m_Count - 1; ++i)
        {
            new (m_Data + i) T(std::move(m_Data[i + 1]));
            m_Data[i + 1].~T();
        }
        --m_Count;
    }

    /// @brief 按索引移除元素（不保持顺序，O(1)）
    void RemoveAtSwap(int32 Index)
    {
        xiCheck(Index >= 0 && Index < m_Count);
        if (Index != m_Count - 1)
        {
            m_Data[Index].~T();
            new (m_Data + Index) T(std::move(m_Data[m_Count - 1]));
        }
        m_Data[m_Count - 1].~T();
        --m_Count;
    }

    /// @brief 清空所有元素（保留已分配容量）
    void Clear()
    {
        for (int32 i = 0; i < m_Count; ++i)
        {
            m_Data[i].~T();
        }
        m_Count = 0;
    }

    /// @brief 预分配容量
    void Reserve(int32 NewCapacity)
    {
        if (NewCapacity > m_Capacity)
        {
            Reallocate(NewCapacity);
        }
    }

    /// @brief 回收多余容量，使 Capacity == Count
    void Shrink()
    {
        if (m_Count < m_Capacity)
        {
            Reallocate(m_Count);
        }
    }

    // ── 迭代器 ───────────────────────────────────
    Iterator       begin()       { return m_Data; }
    Iterator       end()         { return m_Data + m_Count; }
    ConstIterator  begin() const { return m_Data; }
    ConstIterator  end()   const { return m_Data + m_Count; }

    // ── 查找 ─────────────────────────────────────
    /// @brief 线性查找元素，返回索引；未找到返回 -1
    int32 Find(const T& Item) const
    {
        for (int32 i = 0; i < m_Count; ++i)
        {
            if (m_Data[i] == Item)
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
    /// @brief 确保有足够容量容纳 Needed 个元素
    void EnsureCapacity(int32 Needed)
    {
        if (Needed > m_Capacity)
        {
            // 扩容策略：至少翻倍，满足需求
            int32 NewCapacity = (m_Capacity > 0) ? m_Capacity * 2 : 4;
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
        if (Needed > m_Capacity || Needed < m_Capacity / 2)
        {
            Reallocate(Needed);
        }
    }

    /// @brief 重新分配内存，移动旧元素到新内存
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
        int32 ElementsToMove = (m_Count < NewCapacity) ? m_Count : NewCapacity;
        for (int32 i = 0; i < ElementsToMove; ++i)
        {
            new (NewData + i) T(std::move(m_Data[i]));
        }

        // 析构旧元素并释放旧内存
        for (int32 i = 0; i < m_Count; ++i)
        {
            m_Data[i].~T();
        }
        ::operator delete(m_Data);

        m_Data     = NewData;
        m_Count    = ElementsToMove;
        m_Capacity = NewCapacity;
    }

    /// @brief 释放所有内存
    void FreeData()
    {
        ::operator delete(m_Data);
        m_Data     = nullptr;
        m_Capacity = 0;
    }

    /// @brief 从另一个 YaoArray 拷贝
    void CopyFrom(const YaoArray& Other)
    {
        m_Count    = 0;
        m_Capacity = 0;
        m_Data     = nullptr;
        EnsureCapacity(Other.m_Count);
        for (int32 i = 0; i < Other.m_Count; ++i)
        {
            new (m_Data + i) T(Other.m_Data[i]);
        }
        m_Count = Other.m_Count;
    }

    // ── 成员变量 ─────────────────────────────────
    T*    m_Data     = nullptr;  // 元素内存块
    int32 m_Count    = 0;       // 当前元素数
    int32 m_Capacity = 0;       // 已分配容量
};

} // namespace Xi::Yao
