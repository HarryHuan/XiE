// YanWidget — Slate 控件基类（Yan 层 / 羲砚）
// 所有 UI 控件的根节点，提供几何、可见性、绘制接口
#pragma once

#include "YanSlateTypes.h"
#include "YaoArray.h"

using namespace Xi::Yao;

namespace Xi::Yan
{

// ── 前向声明 ────────────────────────────────────
class YanDrawElementList;

/// @brief 控件基类 — 所有 Slate 控件的祖先
class YanWidget
{
public:
    virtual ~YanWidget() = default;

    // ── 几何与布局 ─────────────────────────────
    /// @brief 获取控件的理想尺寸（不依赖父控件约束）
    virtual YanVec2 GetDesiredSize() const { return { 0.0f, 0.0f }; }

    /// @brief 排列子控件（容器控件需重写此方法）
    virtual void ArrangeChildren(const YanGeometry& AllottedGeometry) {}

    // ── 绘制 ─────────────────────────────────────
    /// @brief 绘制此控件及其子控件（核心渲染入口）
    /// @param AllottedGeometry  父控件分配给此控件的几何区域
    /// @param OutDrawElements   绘制元素输出列表
    /// @return 实际使用的绘制层级
    virtual int32 OnPaint(
        const YanGeometry& AllottedGeometry,
        YanDrawElementList& OutDrawElements) const = 0;

    // ── 可见性 ───────────────────────────────────
    void SetVisibility(bool bVisible) { m_bIsVisible = bVisible; }
    bool IsVisible() const { return m_bIsVisible; }

    // ── 着色 ─────────────────────────────────────
    void    SetTint(const YanColor& InTint) { m_Tint = InTint; }
    YanColor GetTint() const { return m_Tint; }

protected:
    YanWidget() = default;

    bool     m_bIsVisible = true;
    YanColor m_Tint       = YanColor::White();
};

/// @brief 绘制元素列表 — 一帧中所有待渲染元素的集合
class YanDrawElementList
{
public:
    /// @brief 添加一个矩形绘制元素
    void AddBox(
        const YanVec2& Position,
        const YanVec2& Size,
        const YanColor& Tint,
        int32 LayerId = 0)
    {
        YanDrawElement Elem;
        Elem.m_Type     = YanDrawElementType::Box;
        Elem.m_Position = Position;
        Elem.m_Size     = Size;
        Elem.m_Tint     = Tint;
        Elem.m_LayerId  = LayerId;
        m_Elements.Add(Elem);
    }

    const YaoArray<YanDrawElement>& GetElements() const { return m_Elements; }
    int32 Num() const { return m_Elements.Num(); }
    void  Clear() { m_Elements.Clear(); }

private:
    YaoArray<YanDrawElement> m_Elements;
};

} // namespace Xi::Yan
