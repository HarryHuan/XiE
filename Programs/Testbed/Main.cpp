// Testbed Main.cpp — 阶段 1 验证：Slate UI → RHI → OpenGL 绘制
#include "Core.h"
#include "YaoOpenGLRHI.h"
#include "YanSlateCore.h"
#include "LogMacros.h"

using namespace Xi::Yao;
using namespace Xi::Yan;

DEFINE_LOG_CATEGORY(LogTestbed, Log);

// ═══════════════════════════════════════════════
int main()
{
    XI_LOG("═══ 羲引擎 阶段 1 验证 — Slate UI ═══");

    // 1. 创建窗口 + OpenGL
    void* hWnd      = nullptr;
    void* hDC       = nullptr;
    void* glContext = nullptr;
    if (!OpenGLPlatform_CreateWindow(hWnd, hDC, glContext, "Xi Engine - Phase 1: Slate UI", 800, 600))
    {
        XI_LOG_ERROR("Failed to create window");
        return -1;
    }
    if (!OpenGLPlatform_LoadFunctions())
    {
        XI_LOG_ERROR("Failed to load OpenGL functions");
        return -1;
    }

    // 2. 创建 RHI 和 Slate 渲染器
    YaoOpenGLRHI RHI(hWnd, hDC, glContext, 800, 600);
    YanSlateRenderer SlateRenderer(&RHI, 800, 600);

    // 3. 构建 Slate UI 控件树
    //  ┌─ YanBorder (深灰背景) ────────────────────┐
    //  │  └─ YanVerticalBox                       │
    //  │      ├─ YanBorder (红色标题栏, 高60)       │
    //  │      ├─ YanBorder (绿色内容区, 高400)      │
    //  │      └─ YanBorder (蓝色底部, 高140)        │
    //  └──────────────────────────────────────────┘

    // 标题栏
    auto TitleBar = SNew(YanBorder);
    TitleBar->SetBackground(YanColor(0.8f, 0.2f, 0.2f));  // 红色

    // 内容区
    auto ContentArea = SNew(YanBorder);
    ContentArea->SetBackground(YanColor(0.2f, 0.6f, 0.3f));  // 绿色

    // 底部
    auto BottomBar = SNew(YanBorder);
    BottomBar->SetBackground(YanColor(0.2f, 0.3f, 0.8f));  // 蓝色

    // 垂直布局
    auto VBox = SNew(YanVerticalBox);
    VBox->AddSlot().AutoHeight()[TitleBar];
    VBox->AddSlot()[ContentArea];
    VBox->AddSlot().AutoHeight()[BottomBar];

    // 根容器（深灰背景）
    auto RootBorder = SNew(YanBorder);
    RootBorder->SetBackground(YanColor(0.15f, 0.15f, 0.18f));  // 深灰
    RootBorder->GetSlot().Padding(10)[VBox];

    // 4. 主循环
    XI_LOG("Entering main loop...");
    while (RHI.IsWindowOpen() && OpenGLPlatform_PumpMessages())
    {
        // 清屏
        YaoClearColor ClearColor;
        ClearColor.m_R = 0.1f;
        ClearColor.m_G = 0.1f;
        ClearColor.m_B = 0.12f;
        ClearColor.m_A = 1.0f;
        RHI.Clear(ClearColor);

        // 视口
        YaoViewport VP;
        VP.m_Width  = 800;
        VP.m_Height = 600;
        RHI.SetViewport(VP);

        // ── Slate 渲染 ─────────────────────────
        // 1. 构建绘制元素列表
        YanDrawElementList DrawList;
        YanGeometry RootGeo({ 0.0f, 0.0f }, { 800.0f, 600.0f });
        RootBorder->ArrangeChildren(RootGeo);
        RootBorder->OnPaint(RootGeo, DrawList);

        // 2. 通过 Slate 渲染器提交到 RHI
        SlateRenderer.Render(DrawList);
        // ───────────────────────────────────────

        RHI.Present();
    }

    XI_LOG("Phase 1 verification complete.");
    OpenGLPlatform_DestroyWindow(hWnd, hDC, glContext);
    return 0;
}
