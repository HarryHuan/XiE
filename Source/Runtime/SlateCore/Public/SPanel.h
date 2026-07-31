// SPanel.h — Slate 面板基类 + 子控件槽位系统（Yan 层 / 羲砚）
// YanPanel 为多子控件容器基类，YanCompoundWidget 为单子控件基类
#pragma once

#include "SWidget.h"
#include "SlateTypes.h"
#include "TSharedPtr.h"
#include "TArray.h"

using namespace Xi::Yao;

namespace Xi::Yan
{

// ── 前向声明 ────────────────────────────────────
class YanWidget;

// ═══════════════════════════════════════════════
// 槽位 — 声明式语法的核心
// ═══════════════════════════════════════════════

/// @brief 单个子控件槽位（仿 UE FSlotBase）
/// 存储子控件指针 + 布局参数（边距、对齐、大小策略）
struct YanSlot
{
    YaoSharedPtr<YanWidget> m_Child;        // 子控件
    YanMargin               m_Padding;       // 内边距
    float                   m_SizeValue = 0.0f;  // 尺寸值（AutoHeight 时为自动）
    bool                    m_bAutoHeight = false;

    YanSlot() = default;

    /// @brief 链式设置内边距（统一值）
    YanSlot& Padding(float InValue)
    {
        m_Padding = YanMargin::Uniform(InValue);
        return *this;
    }

    /// @brief 链式设置自适应高度
    YanSlot& AutoHeight()
    {
        m_bAutoHeight = true;
        return *this;
    }

    /// @brief 链式设置子控件（支持 operator[] 语法）
    YanSlot& operator[](YaoSharedPtr<YanWidget> InChild)
    {
        m_Child = InChild;
        return *this;
    }
};

// ═══════════════════════════════════════════════
// 单子控件基类
// ═══════════════════════════════════════════════

/// @brief 单子控件基类（仿 UE SCompoundWidget）
/// 适合叶子控件（如 STextBlock、SButton）和只有一个子控件的容器
class YanCompoundWidget : public YanWidget
{
public:
    /// @brief 获取唯一的子控件槽位
    YanSlot& GetSlot() { return m_Slot; }

protected:
    YanSlot m_Slot;  // 唯一子控件槽位
};

// ═══════════════════════════════════════════════
// 多子控件面板基类
// ═══════════════════════════════════════════════

/// @brief 面板基类（仿 UE SPanel）
/// 管理多个子控件槽位，子类在 ArrangeChildren 中决定每个子控件的位置和大小
class YanPanel : public YanWidget
{
public:
    /// @brief 添加一个槽位（返回引用，支持链式调用）
    YanSlot& AddSlot()
    {
        m_Slots.Add(YanSlot());
        return m_Slots.Last();
    }

    /// @brief 获取子槽位数量
    int32 NumSlots() const { return m_Slots.Num(); }

    /// @brief 获取索引对应的槽位
    YanSlot& GetSlot(int32 Index)
    {
        xiCheck(Index >= 0 && Index < m_Slots.Num());
        return m_Slots[Index];
    }

    const YanSlot& GetSlot(int32 Index) const
    {
        xiCheck(Index >= 0 && Index < m_Slots.Num());
        return m_Slots[Index];
    }

protected:
    YaoArray<YanSlot> m_Slots;  // 子控件槽位数组
};

} // namespace Xi::Yan
