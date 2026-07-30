// OpenGLRHI.h — OpenGL RHI 后端声明（Yao 层 / 羲爻）
// 实现 YaoRHIDevice / YaoRHIBuffer / YaoRHIShader 的 OpenGL 版本
#pragma once

#include "RHIDevice.h"
#include "CoreTypes.h"
#include "TArray.h"

namespace Xi::Yao
{

// ── OpenGL 类型别名（避免直接 include OpenGL 头） ──
using GLuint      = unsigned int;
using GLint       = int;
using GLenum      = unsigned int;
using GLbitfield  = unsigned int;
using GLsizei     = int;
using GLsizeiptr  = int64;        // 用于缓冲区大小
using GLchar      = char;
using GLboolean   = unsigned char;
using GLfloat     = float;

// ── OpenGL 常量 ─────────────────────────────────
constexpr GLenum GL_ARRAY_BUFFER          = 0x8892;
constexpr GLenum GL_ELEMENT_ARRAY_BUFFER  = 0x8893;
constexpr GLenum GL_STATIC_DRAW           = 0x88E4;
constexpr GLenum GL_DYNAMIC_DRAW          = 0x88E8;
constexpr GLenum GL_FLOAT                 = 0x1406;
constexpr GLenum GL_UNSIGNED_BYTE         = 0x1401;
constexpr GLenum GL_TRIANGLES             = 0x0004;
constexpr GLenum GL_COLOR_BUFFER_BIT      = 0x00004000;
constexpr GLenum GL_VERTEX_SHADER         = 0x8B31;
constexpr GLenum GL_FRAGMENT_SHADER       = 0x8B30;
constexpr GLenum GL_COMPILE_STATUS        = 0x8B81;
constexpr GLenum GL_LINK_STATUS           = 0x8B82;
constexpr GLenum GL_TRUE                  = 1;
constexpr GLenum GL_FALSE                 = 0;

// ── 函数指针类型定义（wglGetProcAddress 加载） ──
// 这些是 OpenGL 3.3+ 核心函数，动态加载以支持现代 OpenGL
using PFN_glGenBuffers          = void  (*)(GLsizei, GLuint*);
using PFN_glBindBuffer          = void  (*)(GLenum, GLuint);
using PFN_glBufferData          = void  (*)(GLenum, GLsizeiptr, const void*, GLenum);
using PFN_glDeleteBuffers       = void  (*)(GLsizei, const GLuint*);
using PFN_glGenVertexArrays     = void  (*)(GLsizei, GLuint*);
using PFN_glBindVertexArray     = void  (*)(GLuint);
using PFN_glDeleteVertexArrays  = void  (*)(GLsizei, const GLuint*);
using PFN_glEnableVertexAttribArray  = void (*)(GLuint);
using PFN_glVertexAttribPointer = void  (*)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
using PFN_glUseProgram          = void  (*)(GLuint);
using PFN_glCreateShader        = GLuint (*)(GLenum);
using PFN_glShaderSource        = void  (*)(GLuint, GLsizei, const GLchar**, const GLint*);
using PFN_glCompileShader       = void  (*)(GLuint);
using PFN_glGetShaderiv         = void  (*)(GLuint, GLenum, GLint*);
using PFN_glGetShaderInfoLog    = void  (*)(GLuint, GLsizei, GLsizei*, GLchar*);
using PFN_glCreateProgram       = GLuint (*)();
using PFN_glAttachShader        = void  (*)(GLuint, GLuint);
using PFN_glLinkProgram         = void  (*)(GLuint);
using PFN_glGetProgramiv        = void  (*)(GLuint, GLenum, GLint*);
using PFN_glGetProgramInfoLog   = void  (*)(GLuint, GLsizei, GLsizei*, GLchar*);
using PFN_glDeleteShader        = void  (*)(GLuint);
using PFN_glDeleteProgram       = void  (*)(GLuint);
using PFN_glGetUniformLocation  = GLint (*)(GLuint, const GLchar*);
using PFN_glUniform1f           = void  (*)(GLint, GLfloat);
using PFN_glUniformMatrix4fv   = void  (*)(GLint, GLsizei, GLboolean, const GLfloat*);
using PFN_glViewport            = void  (*)(GLint, GLint, GLsizei, GLsizei);
using PFN_glClearColor          = void  (*)(GLfloat, GLfloat, GLfloat, GLfloat);
using PFN_glClear               = void  (*)(GLenum);
using PFN_glDrawElements        = void  (*)(GLenum, GLsizei, GLenum, const void*);

/// @brief OpenGL 函数表 — 所有动态加载的 OpenGL 函数指针
struct YaoGLFuncs
{
    PFN_glGenBuffers           glGenBuffers          = nullptr;
    PFN_glBindBuffer           glBindBuffer          = nullptr;
    PFN_glBufferData           glBufferData          = nullptr;
    PFN_glDeleteBuffers        glDeleteBuffers       = nullptr;
    PFN_glGenVertexArrays      glGenVertexArrays     = nullptr;
    PFN_glBindVertexArray      glBindVertexArray     = nullptr;
    PFN_glDeleteVertexArrays   glDeleteVertexArrays  = nullptr;
    PFN_glEnableVertexAttribArray glEnableVertexAttribArray = nullptr;
    PFN_glVertexAttribPointer  glVertexAttribPointer = nullptr;
    PFN_glUseProgram           glUseProgram          = nullptr;
    PFN_glCreateShader         glCreateShader        = nullptr;
    PFN_glShaderSource         glShaderSource        = nullptr;
    PFN_glCompileShader        glCompileShader       = nullptr;
    PFN_glGetShaderiv          glGetShaderiv         = nullptr;
    PFN_glGetShaderInfoLog     glGetShaderInfoLog    = nullptr;
    PFN_glCreateProgram        glCreateProgram       = nullptr;
    PFN_glAttachShader         glAttachShader        = nullptr;
    PFN_glLinkProgram          glLinkProgram         = nullptr;
    PFN_glGetProgramiv         glGetProgramiv        = nullptr;
    PFN_glGetProgramInfoLog    glGetProgramInfoLog   = nullptr;
    PFN_glDeleteShader         glDeleteShader        = nullptr;
    PFN_glDeleteProgram        glDeleteProgram       = nullptr;
    PFN_glGetUniformLocation   glGetUniformLocation  = nullptr;
    PFN_glUniform1f            glUniform1f           = nullptr;
    PFN_glUniformMatrix4fv     glUniformMatrix4fv    = nullptr;
    PFN_glViewport             glViewport            = nullptr;
    PFN_glClearColor           glClearColor          = nullptr;
    PFN_glClear                glClear               = nullptr;
    PFN_glDrawElements         glDrawElements        = nullptr;
};

/// @brief 全局 OpenGL 函数表（g_ 前缀 = 全局变量）
extern YaoGLFuncs g_GL;

// ── OpenGL 平台层（Win32 窗口 + wgl 上下文） ─────────

/// @brief 创建 Win32 窗口并初始化 OpenGL 渲染上下文
/// @return 是否成功
bool OpenGLPlatform_CreateWindow(
    void*& OutHwnd,
    void*& OutHDC,
    void*& OutGLContext,
    const char* Title,
    int32 Width,
    int32 Height);

/// @brief 加载所有 OpenGL 函数指针
bool OpenGLPlatform_LoadFunctions();

/// @brief 销毁窗口和 OpenGL 上下文
void OpenGLPlatform_DestroyWindow(void* InHwnd, void* InHDC, void* InGLContext);

/// @brief 处理 Windows 消息泵（返回 false 表示窗口已关闭）
bool OpenGLPlatform_PumpMessages();

// ── OpenGL RHI 实现类 ──────────────────────────

/// @brief OpenGL 顶点/索引缓冲区实现
class YaoOpenGLBuffer : public YaoRHIBuffer
{
public:
    YaoOpenGLBuffer(const YaoBufferDesc& Desc, GLenum InTarget);
    virtual ~YaoOpenGLBuffer();

    virtual void  SetData(const void* Data, int32 SizeInBytes) override;
    virtual int32 GetSize() const override { return m_Size; }

    GLuint GetGLBuffer() const { return m_BufferID; }
    GLenum GetTarget()   const { return m_Target; }

private:
    GLuint m_BufferID = 0;     // OpenGL 缓冲区对象 ID
    GLenum m_Target;            // GL_ARRAY_BUFFER 或 GL_ELEMENT_ARRAY_BUFFER
    int32  m_Size = 0;         // 缓冲区字节大小
};

/// @brief OpenGL Shader 程序实现
class YaoOpenGLShader : public YaoRHIShader
{
public:
    YaoOpenGLShader(const char* VertexSource, const char* PixelSource);
    virtual ~YaoOpenGLShader();

    virtual void Bind() override;
    virtual void SetUniform1f(const char* Name, float Value) override;
    virtual void SetUniformMat4(const char* Name, const float* Matrix) override;

    GLuint GetProgramID() const { return m_ProgramID; }

private:
    /// @brief 编译单个着色器阶段
    GLuint CompileStage(GLenum StageType, const char* Source);

    GLuint m_ProgramID = 0;    // OpenGL 着色器程序 ID
};

/// @brief OpenGL RHI 设备实现
class YaoOpenGLRHI : public YaoRHIDevice
{
public:
    YaoOpenGLRHI(void* InHwnd, void* InHDC, void* InGLContext, int32 InWidth, int32 InHeight);
    virtual ~YaoOpenGLRHI();

    // ── 资源创建 ─────────────────────────────────
    virtual YaoSharedPtr<YaoRHIBuffer> CreateVertexBuffer(const YaoBufferDesc& Desc) override;
    virtual YaoSharedPtr<YaoRHIBuffer> CreateIndexBuffer(const YaoBufferDesc& Desc) override;
    virtual YaoSharedPtr<YaoRHIShader> CreateShader(
        const char* VertexSource,
        const char* PixelSource) override;

    // ── 渲染命令 ─────────────────────────────────
    virtual void SetViewport(const YaoViewport& Viewport) override;
    virtual void Clear(const YaoClearColor& Color) override;
    virtual void SetVertexBuffer(YaoRHIBuffer* Buffer, const YaoArray<YaoVertexElement>& Layout) override;
    virtual void SetIndexBuffer(YaoRHIBuffer* Buffer) override;
    virtual void DrawIndexed(int32 IndexCount, int32 StartIndex = 0) override;

    // ── 帧管理 ───────────────────────────────────
    virtual void Present() override;

    /// @brief 获取窗口是否仍然打开
    bool IsWindowOpen() const;

private:
    void* m_WindowHandle    = nullptr;   // Win32 窗口句柄 (HWND)
    void* m_DeviceContext   = nullptr;   // 设备上下文 (HDC)
    void* m_GLRenderContext = nullptr;   // OpenGL 渲染上下文 (HGLRC)
    int32 m_Width           = 0;
    int32 m_Height          = 0;

    // 当前绑定的状态
    GLuint m_CurrentVAO          = 0;   // 当前顶点数组对象
    GLuint m_CurrentVertexBuffer = 0;
    GLuint m_CurrentIndexBuffer  = 0;
    int32  m_CurrentStride       = 0;   // 当前顶点步长
};

} // namespace Xi::Yao
