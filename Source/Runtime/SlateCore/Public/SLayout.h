// SLayout.h — Slate 布局控件（Yan 层 / 羲砚）
// YanVerticalBox / YanHorizontalBox / YanOverlay / YanBorder
#pragma once

#include "SPanel.h"

namespace Xi::Yan
{

// ═══════════════════════════════════════════════
// 垂直盒子 — 子控件从上到下排列
// ═══════════════════════════════════════════════

/// @brief 垂直布局控件（仿 UE SVerticalBox）
class YanVerticalBox : public YanPanel
{
public:
    virtual YanVec2 GetDesiredSize() const override
    {
        float MaxWidth  = 0.0f;
        float TotalHeight = 0.0f;
        for (int32 i = 0; i < m_Slots.Num(); ++i)
        {
            if (!m_Slots[i].m_Child.IsValid()) continue;
            YanVec2 ChildSize = m_Slots[i].m_Child->GetDesiredSize();
            float PadH = m_Slots[i].m_Padding.GetHorizontalTotal();
            float PadV = m_Slots[i].m_Padding.GetVerticalTotal();
            float W = ChildSize.m_X + PadH;
            TotalHeight += ChildSize.m_Y + PadV;
            if (W > MaxWidth) MaxWidth = W;
        }
        return { MaxWidth, TotalHeight };
    }

    virtual void ArrangeChildren(const YanGeometry& AllottedGeometry) override
    {
        float YOffset = 0.0f;
        for (int32 i = 0; i < m_Slots.Num(); ++i)
        {
            YanSlot& Slot = m_Slots[i];
            if (!Slot.m_Child.IsValid()) continue;

            float PadL = Slot.m_Padding.m_Left;
            float PadT = Slot.m_Padding.m_Top;
            float PadH = Slot.m_Padding.GetHorizontalTotal();
            float PadV = Slot.m_Padding.GetVerticalTotal();

            float ChildH;
            if (Slot.m_bAutoHeight)
            {
                ChildH = Slot.m_Child->GetDesiredSize().m_Y;
            }
            else
            {
                ChildH = AllottedGeometry.m_Size.m_Y - YOffset - PadV;
                if (ChildH < 0.0f) ChildH = 0.0f;
            }

            float ChildW = AllottedGeometry.m_Size.m_X - PadH;
            if (ChildW < 0.0f) ChildW = 0.0f;

            YanGeometry ChildGeo = AllottedGeometry.MakeChild(
                { PadL, YOffset + PadT },
                { ChildW, ChildH }
            );
            Slot.m_Child->ArrangeChildren(ChildGeo);
            YOffset += ChildH + PadV;
        }
    }

    virtual int32 OnPaint(
        const YanGeometry& AllottedGeometry,
        YanDrawElementList& OutDrawElements) const override
    {
        int32 MaxLayer = 0;
        for (int32 i = 0; i < m_Slots.Num(); ++i)
        {
            const YanSlot& Slot = m_Slots[i];
            if (!Slot.m_Child.IsValid() || !Slot.m_Child->IsVisible()) continue;
            int32 Layer = Slot.m_Child->OnPaint(AllottedGeometry, OutDrawElements);
            if (Layer > MaxLayer) MaxLayer = Layer;
        }
        return MaxLayer;
    }
};

// ═══════════════════════════════════════════════
// 水平盒子 — 子控件从左到右排列
// ═══════════════════════════════════════════════

/// @brief 水平布局控件（仿 UE SHorizontalBox）
class YanHorizontalBox : public YanPanel
{
public:
    virtual YanVec2 GetDesiredSize() const override
    {
        float TotalWidth  = 0.0f;
        float MaxHeight = 0.0f;
        for (int32 i = 0; i < m_Slots.Num(); ++i)
        {
            if (!m_Slots[i].m_Child.IsValid()) continue;
            YanVec2 ChildSize = m_Slots[i].m_Child->GetDesiredSize();
            float PadH = m_Slots[i].m_Padding.GetHorizontalTotal();
            float PadV = m_Slots[i].m_Padding.GetVerticalTotal();
            TotalWidth += ChildSize.m_X + PadH;
            float H = ChildSize.m_Y + PadV;
            if (H > MaxHeight) MaxHeight = H;
        }
        return { TotalWidth, MaxHeight };
    }

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

            YanGeometry ChildGeo = AllottedGeometry.MakeChild(
                { XOffset + PadL, PadT },
                { ChildW, ChildH }
            );
            Slot.m_Child->ArrangeChildren(ChildGeo);
            XOffset += ChildW + PadH;
        }
    }

    virtual int32 OnPaint(
        const YanGeometry& AllottedGeometry,
        YanDrawElementList& OutDrawElements) const override
    {
        int32 MaxLayer = 0;
        for (int32 i = 0; i < m_Slots.Num(); ++i)
        {
            const YanSlot& Slot = m_Slots[i];
            if (!Slot.m_Child.IsValid() || !Slot.m_Child->IsVisible()) continue;
            int32 Layer = Slot.m_Child->OnPaint(AllottedGeometry, OutDrawElements);
            if (Layer > MaxLayer) MaxLayer = Layer;
        }
        return MaxLayer;
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
            YanGeometry ChildGeo = AllottedGeometry.MakeChild(
                { Slot.m_Padding.m_Left, Slot.m_Padding.m_Top },
                AllottedGeometry.m_Size
            );
            Slot.m_Child->ArrangeChildren(ChildGeo);
        }
    }

    virtual int32 OnPaint(
        const YanGeometry& AllottedGeometry,
        YanDrawElementList& OutDrawElements) const override
    {
        int32 MaxLayer = 0;
        for (int32 i = 0; i < m_Slots.Num(); ++i)
        {
            const YanSlot& Slot = m_Slots[i];
            if (!Slot.m_Child.IsValid() || !Slot.m_Child->IsVisible()) continue;
            int32 Layer = Slot.m_Child->OnPaint(AllottedGeometry, OutDrawElements);
            if (Layer > MaxLayer) MaxLayer = Layer;
        }
        return MaxLayer;
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

    virtual int32 OnPaint(
        const YanGeometry& AllottedGeometry,
        YanDrawElementList& OutDrawElements) const override
    {
        int32 LayerId = 0;

        // 先绘制背景色块
        OutDrawElements.AddBox(
            AllottedGeometry.m_Position,
            AllottedGeometry.m_Size,
            m_Background,
            LayerId
        );

        // 再绘制子控件
        if (m_Slot.m_Child.IsValid() && m_Slot.m_Child->IsVisible())
        {
            YanGeometry ChildGeo = AllottedGeometry.MakeChild(
                { m_Slot.m_Padding.m_Left, m_Slot.m_Padding.m_Top },
                AllottedGeometry.m_Size
            );
            int32 ChildLayer = m_Slot.m_Child->OnPaint(ChildGeo, OutDrawElements);
            if (ChildLayer > LayerId) LayerId = ChildLayer;
        }

        return LayerId;
    }

private:
    YanColor m_Background = YanColor::White();
};

} // namespace Xi::Yan
