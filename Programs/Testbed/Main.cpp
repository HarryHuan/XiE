// Testbed Main.cpp — 阶段 0 验证：通过 RHI → OpenGL 绘制三角形
// 验证目标：用 RHI 接口创建缓冲区、着色器，绘制一个彩色三角形到窗口
#include "Core.h"
#include "OpenGLRHI.h"
#include "LogMacros.h"

using namespace Xi::Yao;

DEFINE_LOG_CATEGORY(LogTestbed, Log);

// ── 顶点结构 ────────────────────────────────────
/// @brief 三角形顶点：位置 (float3) + 颜色 (float3)
struct FVertex
{
    float X, Y, Z;       // 位置
    float R, G, B;       // 颜色
};

// ── 着色器源码（GLSL 4.50 core） ────────────────
/// @brief 顶点着色器 — 简单的正交投影变换
static const char* VertexShaderGLSL = R"(
#version 450 core
layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec3 InColor;

out vec3 FragColor;

// 正交投影矩阵（NDC 空间：x∈[-1,1], y∈[-1,1]）
uniform mat4 ProjectionMatrix;

void main()
{
    gl_Position = ProjectionMatrix * vec4(InPosition, 1.0);
    FragColor = InColor;
}
)";

/// @brief 像素着色器 — 直接输出插值颜色
static const char* PixelShaderGLSL = R"(
#version 450 core
in vec3 FragColor;
out vec4 OutColor;

void main()
{
    OutColor = vec4(FragColor, 1.0);
}
)";

// ── 正交投影矩阵（单位矩阵，顶点直接映射到 NDC） ──
static float OrthoProjection[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
};

// ═══════════════════════════════════════════════
// 主函数
// ═══════════════════════════════════════════════
int main()
{
    XI_LOG("═══ 羲引擎 阶段 0 验证 — 三角形绘制 ═══");

    // 1. 创建窗口 + OpenGL 上下文
    void* hWnd      = nullptr;
    void* hDC       = nullptr;
    void* glContext = nullptr;

    if (!OpenGLPlatform_CreateWindow(hWnd, hDC, glContext, "Xi Engine - Phase 0: Triangle", 800, 600))
    {
        XI_LOG_ERROR("Failed to create window");
        return -1;
    }

    // 2. 加载 OpenGL 函数
    if (!OpenGLPlatform_LoadFunctions())
    {
        XI_LOG_ERROR("Failed to load OpenGL functions");
        return -1;
    }

    // 3. 创建 RHI 设备（Yao 层）
    YaoOpenGLRHI RHI(hWnd, hDC, glContext, 800, 600);

    // 4. 定义三角形顶点（位置 + 颜色）
    FVertex Vertices[] = {
        //   X      Y      Z      R      G      B
        {  0.0f,  0.8f, 0.0f,  1.0f, 0.0f, 0.0f },  // 顶部  — 红色
        { -0.8f, -0.6f, 0.0f,  0.0f, 1.0f, 0.0f },  // 左下  — 绿色
        {  0.8f, -0.6f, 0.0f,  0.0f, 0.0f, 1.0f },  // 右下  — 蓝色
    };

    // 5. 定义索引（绘制顺序）
    uint8 Indices[] = { 0, 1, 2 };

    // 6. 创建顶点缓冲区（通过 RHI 接口）
    YaoBufferDesc VBDesc;
    VBDesc.m_SizeInBytes = sizeof(Vertices);
    VBDesc.m_InitialData = Vertices;
    YaoSharedPtr<YaoRHIBuffer> VertexBuffer = RHI.CreateVertexBuffer(VBDesc);
    if (!VertexBuffer.IsValid())
    {
        XI_LOG_ERROR("Failed to create vertex buffer");
        return -1;
    }

    // 7. 创建索引缓冲区（通过 RHI 接口）
    YaoBufferDesc IBDesc;
    IBDesc.m_SizeInBytes = sizeof(Indices);
    IBDesc.m_InitialData = Indices;
    YaoSharedPtr<YaoRHIBuffer> IndexBuffer = RHI.CreateIndexBuffer(IBDesc);
    if (!IndexBuffer.IsValid())
    {
        XI_LOG_ERROR("Failed to create index buffer");
        return -1;
    }

    // 8. 编译着色器（通过 RHI 接口）
    YaoSharedPtr<YaoRHIShader> Shader = RHI.CreateShader(VertexShaderGLSL, PixelShaderGLSL);
    if (!Shader.IsValid())
    {
        XI_LOG_ERROR("Failed to create shader");
        return -1;
    }

    // 9. 定义顶点布局
    YaoArray<YaoVertexElement> VertexLayout;
    YaoVertexElement PosElem;
    PosElem.m_Name   = "POSITION";
    PosElem.m_Type   = YaoVertexElementType::Float3;
    PosElem.m_Offset = 0;
    PosElem.m_Stride = sizeof(FVertex);
    VertexLayout.Add(PosElem);

    YaoVertexElement ColElem;
    ColElem.m_Name   = "COLOR";
    ColElem.m_Type   = YaoVertexElementType::Float3;
    ColElem.m_Offset = sizeof(float) * 3;  // 跳过位置 (X,Y,Z)
    ColElem.m_Stride = sizeof(FVertex);
    VertexLayout.Add(ColElem);

    // 10. 主渲染循环
    XI_LOG("Entering main loop...");
    while (RHI.IsWindowOpen() && OpenGLPlatform_PumpMessages())
    {
        // 清屏（深灰色背景）
        YaoClearColor ClearColor;
        ClearColor.m_R = 0.1f;
        ClearColor.m_G = 0.1f;
        ClearColor.m_B = 0.15f;
        ClearColor.m_A = 1.0f;
        RHI.Clear(ClearColor);

        // 设置视口
        YaoViewport Viewport;
        Viewport.m_Width  = 800;
        Viewport.m_Height = 600;
        RHI.SetViewport(Viewport);

        // 绑定着色器并设置投影矩阵
        Shader->Bind();
        Shader->SetUniformMat4("ProjectionMatrix", OrthoProjection);

        // 绑定顶点缓冲区和布局
        RHI.SetVertexBuffer(VertexBuffer.Get(), VertexLayout);

        // 绑定索引缓冲区
        RHI.SetIndexBuffer(IndexBuffer.Get());

        // 绘制 3 个索引（一个三角形）
        RHI.DrawIndexed(3);

        // 交换前后缓冲区
        RHI.Present();
    }

    // 11. 清理
    Shader.Reset();
    VertexBuffer.Reset();
    IndexBuffer.Reset();

    XI_LOG("Phase 0 verification complete. 羲引擎正常退出。");

    OpenGLPlatform_DestroyWindow(hWnd, hDC, glContext);
    return 0;
}
