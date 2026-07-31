// SlateTypes.h — Slate UI 基础类型定义（Yan 层 / 羲砚）
// 几何、边距、颜色、绘制元素等 Slate 系统核心数据结构
#pragma once

#include "CoreTypes.h"

using namespace Xi::Yao;

namespace Xi::Yan
{

// ── 边距 ────────────────────────────────────────
/// @brief 四周边距（仿 UE FMargin），用于 Padding / Margin
struct YanMargin
{
    float m_Left   = 0.0f;    // 左边距
    float m_Top    = 0.0f;    // 上边距
    float m_Right  = 0.0f;    // 右边距
    float m_Bottom = 0.0f;    // 下边距

    /// @brief 四边等距构造
    static YanMargin Uniform(float InValue)
    {
        return { InValue, InValue, InValue, InValue };
    }

    /// @brief 水平方向总宽度
    float GetHorizontalTotal() const { return m_Left + m_Right; }

    /// @brief 垂直方向总高度
    float GetVerticalTotal()   const { return m_Top + m_Bottom; }
};

// ── 2D 向量 ─────────────────────────────────────
/// @brief 简易 2D 向量，用于控件位置和大小
struct YanVec2
{
    float m_X = 0.0f;
    float m_Y = 0.0f;

    YanVec2() = default;
    YanVec2(float InX, float InY) : m_X(InX), m_Y(InY) {}
};

// ── 几何信息 ────────────────────────────────────
/// @brief 控件的几何信息（仿 UE FGeometry）
/// 描述控件在屏幕上的位置、大小和缩放
struct YanGeometry
{
    YanVec2 m_Position;       // 绝对屏幕位置（左上角）
    YanVec2 m_Size;           // 控件大小（宽 x 高）
    float   m_Scale = 1.0f;   // 缩放因子

    YanGeometry() = default;

    /// @brief 带位置的构造
    YanGeometry(const YanVec2& InPos, const YanVec2& InSize, float InScale = 1.0f)
        : m_Position(InPos), m_Size(InSize), m_Scale(InScale)
    {
    }

    /// @brief 创建一个子几何（相对于此几何的局部坐标）
    YanGeometry MakeChild(const YanVec2& InLocalPos, const YanVec2& InChildSize) const
    {
        return YanGeometry(
            { m_Position.m_X + InLocalPos.m_X * m_Scale,
              m_Position.m_Y + InLocalPos.m_Y * m_Scale },
            { InChildSize.m_X * m_Scale, InChildSize.m_Y * m_Scale },
            m_Scale
        );
    }
};

// ── 颜色 ────────────────────────────────────────
/// @brief RGBA 颜色（0.0~1.0），仿 UE FLinearColor
struct YanColor
{
    float m_R = 1.0f;
    float m_G = 1.0f;
    float m_B = 1.0f;
    float m_A = 1.0f;

    YanColor() = default;
    YanColor(float InR, float InG, float InB, float InA = 1.0f)
        : m_R(InR), m_G(InG), m_B(InB), m_A(InA) {}

    static YanColor White()  { return { 1.0f, 1.0f, 1.0f, 1.0f }; }
    static YanColor Black()  { return { 0.0f, 0.0f, 0.0f, 1.0f }; }
    static YanColor Red()    { return { 1.0f, 0.0f, 0.0f, 1.0f }; }
    static YanColor Green()  { return { 0.0f, 1.0f, 0.0f, 1.0f }; }
    static YanColor Blue()   { return { 0.0f, 0.0f, 1.0f, 1.0f }; }
};

// ── 绘制元素类型 ────────────────────────────────
/// @brief Slate 绘制元素类型枚举
enum class YanDrawElementType : uint8
{
    Box,            // 纯色/带边框的矩形
    Text,           // 文字（后续阶段实现）
    Line,           // 线段
    Gradient,       // 渐变矩形（后续阶段实现）
};

// ── 顶点格式 ────────────────────────────────────
/// @brief Slate 渲染的顶点格式（位置 + 颜色 + UV）
struct YanSlateVertex
{
    float m_X, m_Y;         // 屏幕位置（NDC 空间）
    float m_U, m_V;         // UV 坐标
    float m_R, m_G, m_B, m_A;  // 顶点颜色
};

// ── 绘制元素 ────────────────────────────────────
/// @brief 单个绘制元素（仿 UE FSlateDrawElement）
/// 描述一个待渲染的图形（矩形、文字、线段等）
struct YanDrawElement
{
    YanDrawElementType m_Type  = YanDrawElementType::Box;  // 元素类型
    YanVec2            m_Position;    // 屏幕位置（NDC）
    YanVec2            m_Size;        // 元素大小（NDC）
    YanColor           m_Tint;        // 着色颜色
    float              m_CornerRadius = 0.0f;  // 圆角半径（0 = 直角）
    int32              m_LayerId = 0;          // 层级（用于排序）
    int32              m_BatchKey = 0;         // 批次键（相同着色器/纹理的归入同一批）
};

} // namespace Xi::Yan
