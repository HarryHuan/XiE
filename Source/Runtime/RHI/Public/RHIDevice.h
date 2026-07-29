// RHIDevice.h — RHI 抽象设备接口
// 仿 UE IRHIDevice，定义所有 GPU 操作的抽象接口，后端（OpenGL/DX12/Vulkan）实现它们
#pragma once

#include "CoreTypes.h"
#include "TSharedPtr.h"
#include "TArray.h"
#include "RHITypes.h"

// ── 前向声明 ────────────────────────────────────
class IRHIBuffer;
class IRHIShader;
class IRHITexture;
class IRHIPipelineState;

// ── RHI 类型枚举 ────────────────────────────────
enum class ERHIType : uint8
{
    OpenGL,
    DX12,
    Vulkan,
};

/// @brief GPU 缓冲区抽象接口（顶点缓冲 / 索引缓冲）
class IRHIBuffer
{
public:
    virtual ~IRHIBuffer() = default;

    /// @brief 更新缓冲区数据（用于动态缓冲区）
    virtual void SetData(const void* Data, int32 SizeInBytes) = 0;

    /// @brief 获取缓冲区大小
    virtual int32 GetSize() const = 0;
};

/// @brief Shader 程序抽象接口（顶点+像素着色器的链接程序）
class IRHIShader
{
public:
    virtual ~IRHIShader() = default;

    /// @brief 绑定此着色器程序（使后续绘制调用使用它）
    virtual void Bind() = 0;

    /// @brief 设置 uniform 变量（float）
    virtual void SetUniform1f(const char* Name, float Value) = 0;

    /// @brief 设置 uniform 变量（4x4 矩阵）
    virtual void SetUniformMat4(const char* Name, const float* Matrix) = 0;
};

/// @brief 渲染管线状态抽象（后续阶段）
class IRHIPipelineState
{
public:
    virtual ~IRHIPipelineState() = default;
};

// ── RHI 设备（核心抽象） ─────────────────────────

/// @brief RHI 抽象设备 — 所有 GPU 操作的入口
/// 每个图形 API 后端（OpenGL / DX12 / Vulkan）实现此接口
class IRHIDevice
{
public:
    virtual ~IRHIDevice() = default;

    // ── 资源创建 ─────────────────────────────────
    /// @brief 创建顶点缓冲区
    virtual TSharedPtr<IRHIBuffer> CreateVertexBuffer(const FBufferDesc& Desc) = 0;

    /// @brief 创建索引缓冲区
    virtual TSharedPtr<IRHIBuffer> CreateIndexBuffer(const FBufferDesc& Desc) = 0;

    /// @brief 编译着色器程序（顶点着色器 + 像素着色器源码）
    virtual TSharedPtr<IRHIShader> CreateShader(
        const char* VertexSource,
        const char* PixelSource) = 0;

    // ── 渲染命令 ─────────────────────────────────
    /// @brief 设置视口
    virtual void SetViewport(const FViewport& Viewport) = 0;

    /// @brief 清除颜色缓冲区
    virtual void Clear(const FClearColor& Color) = 0;

    /// @brief 设置顶点缓冲区（绑定到输入装配器）
    virtual void SetVertexBuffer(IRHIBuffer* Buffer, const TArray<FVertexElement>& Layout) = 0;

    /// @brief 设置索引缓冲区
    virtual void SetIndexBuffer(IRHIBuffer* Buffer) = 0;

    /// @brief 绘制索引化几何体
    virtual void DrawIndexed(int32 IndexCount, int32 StartIndex = 0) = 0;

    // ── 帧管理 ───────────────────────────────────
    /// @brief 交换前后缓冲区（将渲染结果呈现到屏幕）
    virtual void Present() = 0;
};

// ── 工厂函数 ────────────────────────────────────
/// @brief 根据类型创建对应的 RHI 设备实例
/// @param Type   图形 API 类型（目前仅支持 OpenGL）
/// @param WindowHandle 原生窗口句柄（Win32 HWND）
/// @param Width  窗口宽度
/// @param Height 窗口高度
IRHIDevice* CreateRHI(ERHIType Type, void* WindowHandle, int32 Width, int32 Height);
