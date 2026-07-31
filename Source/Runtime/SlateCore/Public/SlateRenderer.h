// YanSlateRenderer — Slate 渲染器（Yan 层 / 羲砚）
// 将 YanDrawElementList 转换为 RHI 顶点/索引缓冲区并提交绘制
#pragma once

#include "SWidget.h"
#include "SlateTypes.h"
#include "TSharedPtr.h"
#include "TArray.h"

// RHI 依赖
#include "RHIDevice.h"

namespace Xi::Yan
{

/// @brief Slate 渲染器 — 将绘制元素列表通过 RHI 渲染到屏幕
class YanSlateRenderer
{
public:
    /// @brief 构造渲染器
    /// @param InRHI       RHI 设备指针（当前为 YaoOpenGLRHI）
    /// @param InViewWidth  视口宽度（像素）
    /// @param InViewHeight 视口高度（像素）
    YanSlateRenderer(YaoRHIDevice* InRHI, int32 InViewWidth, int32 InViewHeight)
        : m_RHI(InRHI)
        , m_ViewWidth(InViewWidth)
        , m_ViewHeight(InViewHeight)
    {
        // 编译 Slate 专用着色器（屏幕空间 → NDC 变换）
        m_Shader = InRHI->CreateShader(SlateVertexShader, SlatePixelShader);
    }

    /// @brief 渲染一帧的绘制元素列表
    void Render(const YanDrawElementList& DrawElements)
    {
        if (!m_Shader.IsValid() || DrawElements.Num() == 0) return;

        m_Shader->Bind();
        m_Shader->SetUniformMat4("ProjectionMatrix", BuildOrthoMatrix());

        // 逐个元素绘制（暂不做合批优化，阶段 1 先跑通流程）
        const YaoArray<YanDrawElement>& Elems = DrawElements.GetElements();
        for (int32 i = 0; i < Elems.Num(); ++i)
        {
            DrawBox(Elems[i]);
        }
    }

private:
    // ── Slate 专用着色器 ─────────────────────────
    /// @brief 顶点着色器：屏幕坐标 → NDC
    static constexpr const char* SlateVertexShader = R"(
#version 450 core
layout(location = 0) in vec2 InPosition;
layout(location = 1) in vec2 InUV;
layout(location = 2) in vec4 InColor;

out vec4 FragColor;

uniform mat4 ProjectionMatrix;

void main()
{
    gl_Position = ProjectionMatrix * vec4(InPosition, 0.0, 1.0);
    FragColor = InColor;
}
)";

    /// @brief 像素着色器：直接输出颜色
    static constexpr const char* SlatePixelShader = R"(
#version 450 core
in vec4 FragColor;
out vec4 OutColor;

void main()
{
    OutColor = FragColor;
}
)";

    // ── 单个矩形绘制 ─────────────────────────────
    /// @brief 将 YanDrawElement 的 Box 绘制到屏幕
    void DrawBox(const YanDrawElement& Elem)
    {
        if (Elem.m_Type != YanDrawElementType::Box) return;

        // 构造四边形顶点（两个三角形 = 6 个顶点，用索引缓冲优化）
        float L = Elem.m_Position.m_X;
        float T = Elem.m_Position.m_Y;
        float R = L + Elem.m_Size.m_X;
        float B = T + Elem.m_Size.m_Y;

        YanSlateVertex Verts[4] = {
            { L, T, 0.0f, 0.0f,  Elem.m_Tint.m_R, Elem.m_Tint.m_G, Elem.m_Tint.m_B, Elem.m_Tint.m_A },  // 左上
            { R, T, 1.0f, 0.0f,  Elem.m_Tint.m_R, Elem.m_Tint.m_G, Elem.m_Tint.m_B, Elem.m_Tint.m_A },  // 右上
            { R, B, 1.0f, 1.0f,  Elem.m_Tint.m_R, Elem.m_Tint.m_G, Elem.m_Tint.m_B, Elem.m_Tint.m_A },  // 右下
            { L, B, 0.0f, 1.0f,  Elem.m_Tint.m_R, Elem.m_Tint.m_G, Elem.m_Tint.m_B, Elem.m_Tint.m_A },  // 左下
        };

        // 索引（CCW 三角形绕序）
        uint8 Indices[6] = { 0, 1, 2, 0, 2, 3 };

        // 创建临时 Vertex Buffer
        YaoBufferDesc VBDesc;
        VBDesc.m_SizeInBytes = sizeof(Verts);
        VBDesc.m_InitialData = Verts;
        auto VB = m_RHI->CreateVertexBuffer(VBDesc);

        // 创建临时 Index Buffer
        YaoBufferDesc IBDesc;
        IBDesc.m_SizeInBytes = sizeof(Indices);
        IBDesc.m_InitialData = Indices;
        auto IB = m_RHI->CreateIndexBuffer(IBDesc);

        // 定义顶点布局
        YaoArray<YaoVertexElement> Layout;
        YaoVertexElement PosElem;
        PosElem.m_Name   = "POSITION";
        PosElem.m_Type   = YaoVertexElementType::Float2;
        PosElem.m_Offset = 0;
        PosElem.m_Stride = sizeof(YanSlateVertex);
        Layout.Add(PosElem);

        YaoVertexElement UVElem;
        UVElem.m_Name   = "UV";
        UVElem.m_Type   = YaoVertexElementType::Float2;
        UVElem.m_Offset = sizeof(float) * 2;
        UVElem.m_Stride = sizeof(YanSlateVertex);
        Layout.Add(UVElem);

        YaoVertexElement ColElem;
        ColElem.m_Name   = "COLOR";
        ColElem.m_Type   = YaoVertexElementType::Float4;
        ColElem.m_Offset = sizeof(float) * 4;
        ColElem.m_Stride = sizeof(YanSlateVertex);
        Layout.Add(ColElem);

        // 绑定并绘制
        m_RHI->SetVertexBuffer(VB.Get(), Layout);
        m_RHI->SetIndexBuffer(IB.Get());
        m_RHI->DrawIndexed(6);  // 两个三角形 = 6 个索引
    }

    // ── 正交投影矩阵（屏幕像素 → NDC [-1,1]） ────
    /// @brief 构建 2D 正交投影矩阵
    float* BuildOrthoMatrix()
    {
        // 正交投影：像素坐标 (0,0) 左上 → NDC (-1,1) 左上
        // x: [0, Width] → [-1, 1]
        // y: [0, Height] → [1, -1]（Y 轴翻转）
        static float Matrix[16];
        float L = 0.0f;
        float R = static_cast<float>(m_ViewWidth);
        float T = 0.0f;
        float B = static_cast<float>(m_ViewHeight);

        // 标准正交投影矩阵（行优先填充）
        Matrix[0]  =  2.0f / (R - L);
        Matrix[1]  =  0.0f;
        Matrix[2]  =  0.0f;
        Matrix[3]  =  0.0f;

        Matrix[4]  =  0.0f;
        Matrix[5]  = -2.0f / (B - T);  // Y 轴翻转
        Matrix[6]  =  0.0f;
        Matrix[7]  =  0.0f;

        Matrix[8]  =  0.0f;
        Matrix[9]  =  0.0f;
        Matrix[10] = -1.0f;
        Matrix[11] =  0.0f;

        Matrix[12] = -(R + L) / (R - L);
        Matrix[13] = -(B + T) / (B - T);
        Matrix[14] =  0.0f;
        Matrix[15] =  1.0f;

        return Matrix;
    }

    // ── 成员变量 ─────────────────────────────────
    YaoRHIDevice* m_RHI = nullptr;          // RHI 设备
    int32         m_ViewWidth  = 0;          // 视口宽度
    int32         m_ViewHeight = 0;          // 视口高度
    YaoSharedPtr<YaoRHIShader> m_Shader;    // Slate 着色器
};

} // namespace Xi::Yan
