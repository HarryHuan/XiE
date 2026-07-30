// RHITypes.h — RHI 共享类型定义（Yao 层 / 羲爻）
// 顶点格式、图元类型、缓冲区描述等，供 RHI 接口和上层消费者共用
#pragma once

#include "CoreTypes.h"
#include "FString.h"

namespace Xi::Yao
{

// ── 图元类型 ────────────────────────────────────
/// @brief GPU 图元拓扑类型
enum class YaoPrimitiveType : uint8
{
    Triangles,      // 三角形列表
    TriangleStrip,  // 三角形带
    Lines,          // 线段列表
    Points,         // 点列表
};

// ── 顶点元素格式 ────────────────────────────────
/// @brief 顶点属性的数据类型
enum class YaoVertexElementType : uint8
{
    Float,    // 32 位浮点
    Float2,   // 2D 向量
    Float3,   // 3D 向量
    Float4,   // 4D 向量 / 颜色
    UByte4,   // 4 字节（常用于颜色 RGBA）
};

/// @brief 获取顶点元素类型的字节大小
inline int32 GetVertexElementSize(YaoVertexElementType Type)
{
    switch (Type)
    {
        case YaoVertexElementType::Float:  return 4;
        case YaoVertexElementType::Float2: return 8;
        case YaoVertexElementType::Float3: return 12;
        case YaoVertexElementType::Float4: return 16;
        case YaoVertexElementType::UByte4: return 4;
        default: return 0;
    }
}

// ── 顶点属性描述 ────────────────────────────────
/// @brief 描述顶点缓冲区中一个属性的布局
struct YaoVertexElement
{
    YaoString            m_Name;        // 属性名（如 "POSITION", "COLOR"）
    YaoVertexElementType m_Type;        // 数据类型
    int32                m_Offset;      // 在顶点结构中的字节偏移
    int32                m_Stride;      // 顶点步长（每个顶点的字节数）
    bool                 m_bNormalized = false;  // 是否归一化（用于颜色等整数类型）
};

// ── 缓冲区描述 ──────────────────────────────────
/// @brief 描述 GPU 缓冲区的创建参数
struct YaoBufferDesc
{
    int32       m_SizeInBytes  = 0;        // 缓冲区大小（字节）
    bool        m_bIsDynamic   = false;    // 是否频繁更新（影响底层分配策略）
    const void* m_InitialData  = nullptr;  // 初始数据（可为 nullptr）
};

// ── 纹理描述 ──────────────────────────────────
/// @brief 描述 GPU 纹理的创建参数
struct YaoTextureDesc
{
    int32        m_Width         = 0;
    int32        m_Height        = 0;
    int32        m_Channels      = 4;        // RGBA = 4
    bool         m_bGenerateMips = false;
    const uint8* m_PixelData     = nullptr;  // 初始像素数据
};

// ── 视口 ───────────────────────────────────────
/// @brief 视口描述（屏幕区域映射）
struct YaoViewport
{
    int32 m_X      = 0;
    int32 m_Y      = 0;
    int32 m_Width  = 0;
    int32 m_Height = 0;
};

// ── 清除颜色 ────────────────────────────────────
/// @brief 帧缓冲清除颜色
struct YaoClearColor
{
    float m_R = 0.0f;
    float m_G = 0.0f;
    float m_B = 0.0f;
    float m_A = 1.0f;
};

} // namespace Xi::Yao
