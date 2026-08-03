// YanPanel — Slate 面板基类 + 子控件槽位系统（Yan 层 / 羲砚）
// YanPanel 为多子控件容器基类，YanCompoundWidget 为单子控件基类
#pragma once

#include "YanWidget.h"
#include "YanSlateTypes.h"
#include "YaoSharedPtr.h"
#include "YaoArray.h"

using namespace Xi::Yao;

namespace Xi::Yan
{

// ── 前向声明 ────────────────────────────────────
class YanWidget;

// ═══════════════════════════════════════════════
// 槽位 — 声明式语法的核心
// ═══════════════════════════════════════════════

/// @brief 单个子控件槽位（仿 UE FSlotBase）
/// 存储子控件指针 + 布局参数（边距、尺寸策略）+ 布局结果（几何）
struct YanSlot
{
    YaoSharedPtr<YanWidget> m_Child;          // 子控件
    YanMargin               m_Padding;         // 内边距
    bool                    m_bAutoHeight = false;  // 自动高度（按 GetDesiredSize）
    float                   m_FixedHeight = -1.0f;  // 固定高度（<0 表示未设置）
    YanGeometry             m_ArrangedGeometry;     // 布局结果（由 ArrangeChildren 计算）

    YanSlot() = default;

    /// @brief 链式设置内边距（统一值）
    YanSlot& Padding(float InValue)
    {
        m_Padding = YanMargin::Uniform(InValue);
        return *this;
    }

    /// @brief 链式设置内边距（四个方向）
    YanSlot& Padding(float L, float T, float R, float B)
    {
        m_Padding.m_Left = L;
        m_Padding.m_Top = T;
        m_Padding.m_Right = R;
        m_Padding.m_Bottom = B;
        return *this;
    }

    /// @brief 链式设置自适应高度（按子控件理想尺寸）
    YanSlot& AutoHeight()
    {
        m_bAutoHeight = true;
        return *this;
    }

    /// @brief 链式设置固定高度（像素）
    YanSlot& FixedHeight(float InHeight)
    {
        m_FixedHeight = InHeight;
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
/// 管理多个子控件槽位
/// - 子类实现 ArrangeChildren：计算每个子控件的几何并存入槽位
/// - 基类提供通用 OnPaint：用槽位中存储的几何绘制子控件
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

    /// @brief 通用绘制：用槽位中存储的几何绘制每个子控件
    virtual int32 OnPaint(
        const YanGeometry& AllottedGeometry,
        YanDrawElementList& OutDrawElements) const override
    {
        int32 MaxLayer = 0;
        for (int32 i = 0; i < m_Slots.Num(); ++i)
        {
            const YanSlot& Slot = m_Slots[i];
            if (!Slot.m_Child.IsValid() || !Slot.m_Child->IsVisible()) continue;
            int32 Layer = Slot.m_Child->OnPaint(Slot.m_ArrangedGeometry, OutDrawElements);
            if (Layer > MaxLayer) MaxLayer = Layer;
        }
        return MaxLayer;
    }

protected:
    /// @brief 计算子控件的理想尺寸（面板子类可重写）
    virtual YanVec2 ComputeDesiredSize() const { return { 0.0f, 0.0f }; }

    virtual YanVec2 GetDesiredSize() const override
    {
        return ComputeDesiredSize();
    }

    YaoArray<YanSlot> m_Slots;  // 子控件槽位数组
};

} // namespace Xi::Yan
