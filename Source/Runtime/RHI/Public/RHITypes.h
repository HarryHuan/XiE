// RHITypes.h — RHI 共享类型定义
// 顶点格式、图元类型、缓冲区描述等，供 RHI 接口和上层消费者共用
#pragma once

#include "CoreTypes.h"
#include "FString.h"

// ── 图元类型 ────────────────────────────────────
enum class EPrimitiveType : uint8
{
    Triangles,      // 三角形列表
    TriangleStrip,  // 三角形带
    Lines,          // 线段列表
    Points,         // 点列表
};

// ── 顶点元素格式 ────────────────────────────────
enum class EVertexElementType : uint8
{
    Float,    // 32 位浮点
    Float2,   // 2D 向量
    Float3,   // 3D 向量
    Float4,   // 4D 向量 / 颜色
    UByte4,   // 4 字节（常用于颜色 RGBA）
};

/// @brief 获取顶点元素类型的字节大小
inline int32 GetVertexElementSize(EVertexElementType Type)
{
    switch (Type)
    {
        case EVertexElementType::Float:  return 4;
        case EVertexElementType::Float2: return 8;
        case EVertexElementType::Float3: return 12;
        case EVertexElementType::Float4: return 16;
        case EVertexElementType::UByte4: return 4;
        default: return 0;
    }
}

// ── 顶点属性描述 ────────────────────────────────
/// @brief 描述顶点缓冲区中一个属性的布局
struct FVertexElement
{
    FString            Name;       // 属性名（如 "POSITION", "COLOR"）
    EVertexElementType Type;       // 数据类型
    int32              Offset;     // 在顶点结构中的字节偏移
    int32              Stride;     // 顶点步长（每个顶点的字节数）
    bool               bNormalized = false;  // 是否归一化（用于颜色等整数类型）
};

// ── 缓冲区描述 ──────────────────────────────────
/// @brief 描述 GPU 缓冲区的创建参数
struct FBufferDesc
{
    int32  SizeInBytes = 0;         // 缓冲区大小（字节）
    bool   bIsDynamic  = false;     // 是否频繁更新（影响底层分配策略）
    const void* InitialData = nullptr;  // 初始数据（可为 nullptr）
};

// ── 纹理描述 ──────────────────────────────────
/// @brief 描述 GPU 纹理的创建参数
struct FTextureDesc
{
    int32  Width  = 0;
    int32  Height = 0;
    int32  Channels = 4;           // RGBA = 4
    bool   bGenerateMips = false;
    const uint8* PixelData = nullptr;  // 初始像素数据
};

// ── 视口 ───────────────────────────────────────
/// @brief 视口描述（屏幕区域映射）
struct FViewport
{
    int32 X = 0;
    int32 Y = 0;
    int32 Width  = 0;
    int32 Height = 0;
};

// ── 清除颜色 ────────────────────────────────────
struct FClearColor
{
    float R = 0.0f;
    float G = 0.0f;
    float B = 0.0f;
    float A = 1.0f;
};
