// YanLayout — Slate 布局控件（Yan 层 / 羲砚）
// YanVerticalBox / YanHorizontalBox / YanOverlay / YanBorder
#pragma once

#include "YanPanel.h"

namespace Xi::Yan
{

// ═══════════════════════════════════════════════
// 垂直盒子 — 子控件从上到下排列
// ═══════════════════════════════════════════════

/// @brief 垂直布局控件（仿 UE SVerticalBox）
class YanVerticalBox : public YanPanel
{
protected:
    virtual YanVec2 ComputeDesiredSize() const override
    {
        float MaxWidth   = 0.0f;
        float TotalHeight = 0.0f;
        for (int32 i = 0; i < m_Slots.Num(); ++i)
        {
            if (!m_Slots[i].m_Child.IsValid()) continue;
            YanVec2 ChildSize = m_Slots[i].m_Child->GetDesiredSize();
            TotalHeight += ChildSize.m_Y + m_Slots[i].m_Padding.GetVerticalTotal();
            float W = ChildSize.m_X + m_Slots[i].m_Padding.GetHorizontalTotal();
            if (W > MaxWidth) MaxWidth = W;
        }
        return { MaxWidth, TotalHeight };
    }

public:
    virtual void ArrangeChildren(const YanGeometry& AllottedGeometry) override
    {
        // 第一遍：计算固定/自动高度槽位消耗的总高度（含 padding）
        float UsedHeight = 0.0f;
        int32 FillCount  = 0;
        for (int32 i = 0; i < m_Slots.Num(); ++i)
        {
            const YanSlot& Slot = m_Slots[i];
            if (!Slot.m_Child.IsValid()) continue;
            if (Slot.m_FixedHeight >= 0.0f)
            {
                UsedHeight += Slot.m_FixedHeight + Slot.m_Padding.GetVerticalTotal();
            }
            else if (Slot.m_bAutoHeight)
            {
                UsedHeight += Slot.m_Child->GetDesiredSize().m_Y + Slot.m_Padding.GetVerticalTotal();
            }
            else
            {
                ++FillCount;
            }
        }

        // 剩余空间平分给 fill 槽位
        float Remaining = AllottedGeometry.m_Size.m_Y - UsedHeight;
        if (Remaining < 0.0f) Remaining = 0.0f;
        float FillHeight = (FillCount > 0) ? Remaining / FillCount : 0.0f;

        // 第二遍：逐槽位分配几何
        float YOffset = 0.0f;
        for (int32 i = 0; i < m_Slots.Num(); ++i)
        {
            YanSlot& Slot = m_Slots[i];
            if (!Slot.m_Child.IsValid()) continue;

            float PadL = Slot.m_Padding.m_Left;
            float PadT = Slot.m_Padding.m_Top;
            float PadH = Slot.m_Padding.GetHorizontalTotal();
            float PadV = Slot.m_Padding.GetVerticalTotal();

            // 尺寸策略：固定高度 > 自动高度 > 平分剩余空间
            float ChildH;
            if (Slot.m_FixedHeight >= 0.0f)
            {
                ChildH = Slot.m_FixedHeight;
            }
            else if (Slot.m_bAutoHeight)
            {
                ChildH = Slot.m_Child->GetDesiredSize().m_Y;
            }
            else
            {
                ChildH = FillHeight - PadV;
                if (ChildH < 0.0f) ChildH = 0.0f;
            }

            float ChildW = AllottedGeometry.m_Size.m_X - PadH;
            if (ChildW < 0.0f) ChildW = 0.0f;

            // 计算并存储子几何（OnPaint 使用）
            Slot.m_ArrangedGeometry = AllottedGeometry.MakeChild(
                { PadL, YOffset + PadT },
                { ChildW, ChildH }
            );

            Slot.m_Child->ArrangeChildren(Slot.m_ArrangedGeometry);
            YOffset += ChildH + PadV;
        }
    }
};

// ═══════════════════════════════════════════════
// 水平盒子 — 子控件从左到右排列
// ═══════════════════════════════════════════════

/// @brief 水平布局控件（仿 UE SHorizontalBox）
class YanHorizontalBox : public YanPanel
{
protected:
    virtual YanVec2 ComputeDesiredSize() const override
    {
        float TotalWidth = 0.0f;
        float MaxHeight  = 0.0f;
        for (int32 i = 0; i < m_Slots.Num(); ++i)
        {
            if (!m_Slots[i].m_Child.IsValid()) continue;
            YanVec2 ChildSize = m_Slots[i].m_Child->GetDesiredSize();
            TotalWidth += ChildSize.m_X + m_Slots[i].m_Padding.GetHorizontalTotal();
            float H = ChildSize.m_Y + m_Slots[i].m_Padding.GetVerticalTotal();
            if (H > MaxHeight) MaxHeight = H;
        }
        return { TotalWidth, MaxHeight };
    }

public:
    virtual void ArrangeChildren(const YanGeometry& AllottedGeometry) override
    {
        float XOffset = 0.0f;
        for (int32 i = 0; i < m_Slots.Num(); ++i)
        {
            YanSlot& Slot = m_Slots[i];
            if (!Slot.m_Child.IsValid()) continue;

            float PadL = Slot.m_Padding.m_Left;
            float PadT = Slot.m_Padding.m_Top;
            float PadH = Slot.m_Padding.GetHorizontalTotal();
            float PadV = Slot.m_Padding.GetVerticalTotal();

            float ChildW = Slot.m_Child->GetDesiredSize().m_X;
            float ChildH = AllottedGeometry.m_Size.m_Y - PadV;
            if (ChildH < 0.0f) ChildH = 0.0f;

            Slot.m_ArrangedGeometry = AllottedGeometry.MakeChild(
                { XOffset + PadL, PadT },
                { ChildW, ChildH }
            );

            Slot.m_Child->ArrangeChildren(Slot.m_ArrangedGeometry);
            XOffset += ChildW + PadH;
        }
    }
};

// ═══════════════════════════════════════════════
// 叠加层 — 子控件层叠放置
// ═══════════════════════════════════════════════

/// @brief 叠加布局控件（仿 UE SOverlay）
class YanOverlay : public YanPanel
{
public:
    virtual void ArrangeChildren(const YanGeometry& AllottedGeometry) override
    {
        for (int32 i = 0; i < m_Slots.Num(); ++i)
        {
            YanSlot& Slot = m_Slots[i];
            if (!Slot.m_Child.IsValid()) continue;

            Slot.m_ArrangedGeometry = AllottedGeometry.MakeChild(
                { Slot.m_Padding.m_Left, Slot.m_Padding.m_Top },
                AllottedGeometry.m_Size
            );

            Slot.m_Child->ArrangeChildren(Slot.m_ArrangedGeometry);
        }
    }
};

// ═══════════════════════════════════════════════
// 边框 — 带背景色 + 可选子控件的装饰控件
// ═══════════════════════════════════════════════

/// @brief 边框/背景控件（仿 UE SBorder）
class YanBorder : public YanCompoundWidget
{
public:
    /// @brief 设置背景颜色
    void SetBackground(const YanColor& InColor) { m_Background = InColor; }

    virtual void ArrangeChildren(const YanGeometry& AllottedGeometry) override
    {
        if (!m_Slot.m_Child.IsValid()) return;

        m_Slot.m_ArrangedGeometry = AllottedGeometry.MakeChild(
            { m_Slot.m_Padding.m_Left, m_Slot.m_Padding.m_Top },
            AllottedGeometry.m_Size
        );
        m_Slot.m_Child->ArrangeChildren(m_Slot.m_ArrangedGeometry);
    }

    virtual int32 OnPaint(
        const YanGeometry& AllottedGeometry,
        YanDrawElementList& OutDrawElements) const override
    {
        int32 LayerId = 0;

        // 先绘制背景色块（使用本控件的实际几何）
        OutDrawElements.AddBox(
            AllottedGeometry.m_Position,
            AllottedGeometry.m_Size,
            m_Background,
            LayerId
        );

        // 再绘制子控件（使用布局计算出的子几何）
        if (m_Slot.m_Child.IsValid() && m_Slot.m_Child->IsVisible())
        {
            int32 ChildLayer = m_Slot.m_Child->OnPaint(m_Slot.m_ArrangedGeometry, OutDrawElements);
            if (ChildLayer > LayerId) LayerId = ChildLayer;
        }

        return LayerId;
    }

private:
    YanColor m_Background = YanColor::White();
};

} // namespace Xi::Yan
