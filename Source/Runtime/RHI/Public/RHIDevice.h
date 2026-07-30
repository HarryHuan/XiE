// RHIDevice.h — RHI 抽象设备接口（Yao 层 / 羲爻）
// 仿 UE IRHIDevice，定义所有 GPU 操作的抽象接口，后端（OpenGL/DX12/Vulkan）实现它们
#pragma once

#include "CoreTypes.h"
#include "TSharedPtr.h"
#include "TArray.h"
#include "RHITypes.h"

namespace Xi::Yao
{

// ── 前向声明 ────────────────────────────────────
class YaoRHIBuffer;
class YaoRHIShader;
class YaoRHIPipelineState;

// ── RHI 类型枚举 ────────────────────────────────
/// @brief RHI 后端类型
enum class YaoRHIType : uint8
{
    OpenGL,
    DX12,
    Vulkan,
};

/// @brief GPU 缓冲区抽象接口（顶点缓冲 / 索引缓冲）
class YaoRHIBuffer
{
public:
    virtual ~YaoRHIBuffer() = default;

    /// @brief 获取所属 RHI 后端类型（用于下行转换前的安全检查，仿 UE）
    virtual YaoRHIType GetRHIType() const = 0;

    /// @brief 更新缓冲区数据（用于动态缓冲区）
    virtual void SetData(const void* Data, int32 SizeInBytes) = 0;

    /// @brief 获取缓冲区大小
    virtual int32 GetSize() const = 0;
};

/// @brief Shader 程序抽象接口（顶点+像素着色器的链接程序）
class YaoRHIShader
{
public:
    virtual ~YaoRHIShader() = default;

    /// @brief 获取所属 RHI 后端类型
    virtual YaoRHIType GetRHIType() const = 0;

    /// @brief 绑定此着色器程序（使后续绘制调用使用它）
    virtual void Bind() = 0;

    /// @brief 设置 uniform 变量（float）
    virtual void SetUniform1f(const char* Name, float Value) = 0;

    /// @brief 设置 uniform 变量（4x4 矩阵）
    virtual void SetUniformMat4(const char* Name, const float* Matrix) = 0;
};

/// @brief 渲染管线状态抽象（后续阶段扩展）
class YaoRHIPipelineState
{
public:
    virtual ~YaoRHIPipelineState() = default;
};

// ── RHI 设备（核心抽象） ─────────────────────────

/// @brief RHI 抽象设备 — 所有 GPU 操作的入口
/// 每个图形 API 后端（OpenGL / DX12 / Vulkan）实现此接口
class YaoRHIDevice
{
public:
    virtual ~YaoRHIDevice() = default;

    /// @brief 获取 RHI 后端类型
    virtual YaoRHIType GetRHIType() const = 0;

    // ── 资源创建 ─────────────────────────────────
    /// @brief 创建顶点缓冲区
    virtual YaoSharedPtr<YaoRHIBuffer> CreateVertexBuffer(const YaoBufferDesc& Desc) = 0;

    /// @brief 创建索引缓冲区
    virtual YaoSharedPtr<YaoRHIBuffer> CreateIndexBuffer(const YaoBufferDesc& Desc) = 0;

    /// @brief 编译着色器程序（顶点着色器 + 像素着色器源码）
    virtual YaoSharedPtr<YaoRHIShader> CreateShader(
        const char* VertexSource,
        const char* PixelSource) = 0;

    // ── 渲染命令 ─────────────────────────────────
    /// @brief 设置视口
    virtual void SetViewport(const YaoViewport& Viewport) = 0;

    /// @brief 清除颜色缓冲区
    virtual void Clear(const YaoClearColor& Color) = 0;

    /// @brief 设置顶点缓冲区（绑定到输入装配器）
    virtual void SetVertexBuffer(YaoRHIBuffer* Buffer, const YaoArray<YaoVertexElement>& Layout) = 0;

    /// @brief 设置索引缓冲区
    virtual void SetIndexBuffer(YaoRHIBuffer* Buffer) = 0;

    /// @brief 绘制索引化几何体
    virtual void DrawIndexed(int32 IndexCount, int32 StartIndex = 0) = 0;

    // ── 帧管理 ───────────────────────────────────
    /// @brief 交换前后缓冲区（将渲染结果呈现到屏幕）
    virtual void Present() = 0;
};

// ── 安全下行转换辅助（仿 UE RHI 类型检查） ──────

/// @brief 将 RHI 基类指针安全转换为派生类指针
/// 每个派生类须定义 `static constexpr YaoRHIType k_RHIType`
/// 用法：RHICast<YaoOpenGLBuffer>(buffer)
/// 类型不匹配时触发断言失败
template<typename DstType, typename SrcType>
inline DstType* RHICast(SrcType* Ptr)
{
    if (!Ptr) return nullptr;
    DstType* Result = static_cast<DstType*>(Ptr);
    xiCheckf(Result->GetRHIType() == DstType::k_RHIType,
        "RHI cast failed: expected backend type does not match");
    return Result;
}

// ── 工厂函数 ────────────────────────────────────
/// @brief 根据类型创建对应的 RHI 设备实例
/// @param Type         图形 API 类型（目前仅支持 OpenGL）
/// @param WindowHandle 原生窗口句柄（Win32 HWND）
/// @param Width        窗口宽度
/// @param Height       窗口高度
YaoRHIDevice* CreateRHI(YaoRHIType Type, void* WindowHandle, int32 Width, int32 Height);

} // namespace Xi::Yao
