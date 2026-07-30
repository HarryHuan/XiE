// OpenGLRHI.cpp — OpenGL RHI 后端实现（Yao 层 / 羲爻）
// Win32 窗口创建 + wgl OpenGL 上下文 + YaoRHI 接口的 OpenGL 实现
#include "OpenGLRHI.h"
#include "LogMacros.h"

// Windows 头（需要放在最前面以定义 HWND/HDC/HGLRC）
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdlib>

namespace Xi::Yao
{

// ── 全局 OpenGL 函数表（g_ 前缀 = 全局变量） ────
YaoGLFuncs g_GL;

// 传统 OpenGL 1.0/1.1 函数从 opengl32.dll 直接导入
// 注意：必须在函数使用前声明
extern "C" {
    __declspec(dllimport) void __stdcall glViewport(GLint, GLint, GLsizei, GLsizei);
    __declspec(dllimport) void __stdcall glClearColor(GLfloat, GLfloat, GLfloat, GLfloat);
    __declspec(dllimport) void __stdcall glClear(GLbitfield);
    __declspec(dllimport) void __stdcall glDrawElements(GLenum, GLsizei, GLenum, const void*);
}

// ═══════════════════════════════════════════════
// Win32 窗口 + wgl OpenGL 上下文创建
// ═══════════════════════════════════════════════

/// @brief Win32 窗口过程回调
static LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
        case WM_CLOSE:
            DestroyWindow(hWnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
            return 0;
    }
    return DefWindowProc(hWnd, Msg, wParam, lParam);
}

bool OpenGLPlatform_CreateWindow(
    void*& OutHwnd,
    void*& OutHDC,
    void*& OutGLContext,
    const char* Title,
    int32 Width,
    int32 Height)
{
    // 1. 注册窗口类（ANSI 版本，标题用纯英文 ASCII）
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    WNDCLASSEXA wc = {};
    wc.cbSize        = sizeof(WNDCLASSEXA);
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "XiEngineWindow";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassExA(&wc))
    {
        XI_LOG_ERROR("Failed to register window class");
        return false;
    }

    // 2. 计算窗口大小（包含边框）
    RECT Rect = { 0, 0, Width, Height };
    AdjustWindowRect(&Rect, WS_OVERLAPPEDWINDOW, FALSE);
    int32 WinW = Rect.right - Rect.left;
    int32 WinH = Rect.bottom - Rect.top;

    // 3. 创建窗口
    HWND hWnd = CreateWindowExA(
        0, "XiEngineWindow", Title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WinW, WinH,
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        XI_LOG_ERROR("Failed to create window");
        return false;
    }

    // 4. 获取设备上下文
    HDC hDC = GetDC(hWnd);
    if (!hDC)
    {
        XI_LOG_ERROR("Failed to get device context");
        DestroyWindow(hWnd);
        return false;
    }

    // 5. 设置像素格式（OpenGL 核心配置）
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize      = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion   = 1;
    pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int32 PixelFormat = ChoosePixelFormat(hDC, &pfd);
    if (!PixelFormat)
    {
        XI_LOG_ERROR("Failed to choose pixel format");
        ReleaseDC(hWnd, hDC);
        DestroyWindow(hWnd);
        return false;
    }

    if (!SetPixelFormat(hDC, PixelFormat, &pfd))
    {
        XI_LOG_ERROR("Failed to set pixel format");
        ReleaseDC(hWnd, hDC);
        DestroyWindow(hWnd);
        return false;
    }

    // 6. 创建临时 OpenGL 上下文（用于加载 wgl 扩展）
    HGLRC TempContext = wglCreateContext(hDC);
    if (!TempContext)
    {
        XI_LOG_ERROR("Failed to create temp OpenGL context");
        ReleaseDC(hWnd, hDC);
        DestroyWindow(hWnd);
        return false;
    }
    wglMakeCurrent(hDC, TempContext);

    // 7. 加载 wglCreateContextAttribsARB（用于创建现代 OpenGL 上下文）
    using PFN_wglCreateContextAttribsARB = HGLRC (*)(HDC, HGLRC, const int*);
    auto wglCreateContextAttribsARB =
        (PFN_wglCreateContextAttribsARB)wglGetProcAddress("wglCreateContextAttribsARB");

    if (wglCreateContextAttribsARB)
    {
        // 请求 OpenGL 4.5 核心配置上下文
        const int ContextAttribs[] = {
            0x2091, 4,              // WGL_CONTEXT_MAJOR_VERSION_ARB = 4
            0x2092, 5,              // WGL_CONTEXT_MINOR_VERSION_ARB = 5
            0x9126, 0x00000001,     // WGL_CONTEXT_PROFILE_MASK_ARB = WGL_CONTEXT_CORE_PROFILE_BIT_ARB
            0, 0
        };

        HGLRC CoreContext = wglCreateContextAttribsARB(hDC, nullptr, ContextAttribs);
        if (CoreContext)
        {
            // 用核心上下文替换临时上下文
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(TempContext);
            wglMakeCurrent(hDC, CoreContext);
            TempContext = CoreContext;
            XI_LOG("OpenGL 4.5 Core Profile context created");
        }
        else
        {
            XI_LOG_WARNING("Failed to create OpenGL 4.5 core context, falling back to legacy");
        }
    }
    else
    {
        XI_LOG_WARNING("wglCreateContextAttribsARB not available, using legacy OpenGL context");
    }

    XI_LOG("OpenGL context created successfully");

    OutHwnd      = hWnd;
    OutHDC       = hDC;
    OutGLContext = TempContext;

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    return true;
}

bool OpenGLPlatform_LoadFunctions()
{
    // OpenGL 1.0/1.1 传统函数：直接从 opengl32.dll 导入，取地址赋给函数表
    g_GL.glViewport   = glViewport;
    g_GL.glClearColor = glClearColor;
    g_GL.glClear      = glClear;
    g_GL.glDrawElements = glDrawElements;

    // OpenGL 1.2+ 核心函数：通过 wglGetProcAddress 动态加载
    #define LOAD_FUNC(Name) \
        g_GL.Name = (decltype(g_GL.Name))wglGetProcAddress(#Name); \
        if (!g_GL.Name) { \
            XI_LOG_ERROR("Failed to load OpenGL function: %s", #Name); \
            return false; \
        }

    LOAD_FUNC(glGenBuffers);
    LOAD_FUNC(glBindBuffer);
    LOAD_FUNC(glBufferData);
    LOAD_FUNC(glDeleteBuffers);
    LOAD_FUNC(glGenVertexArrays);
    LOAD_FUNC(glBindVertexArray);
    LOAD_FUNC(glDeleteVertexArrays);
    LOAD_FUNC(glEnableVertexAttribArray);
    LOAD_FUNC(glVertexAttribPointer);
    LOAD_FUNC(glUseProgram);
    LOAD_FUNC(glCreateShader);
    LOAD_FUNC(glShaderSource);
    LOAD_FUNC(glCompileShader);
    LOAD_FUNC(glGetShaderiv);
    LOAD_FUNC(glGetShaderInfoLog);
    LOAD_FUNC(glCreateProgram);
    LOAD_FUNC(glAttachShader);
    LOAD_FUNC(glLinkProgram);
    LOAD_FUNC(glGetProgramiv);
    LOAD_FUNC(glGetProgramInfoLog);
    LOAD_FUNC(glDeleteShader);
    LOAD_FUNC(glDeleteProgram);
    LOAD_FUNC(glGetUniformLocation);
    LOAD_FUNC(glUniform1f);
    LOAD_FUNC(glUniformMatrix4fv);

    #undef LOAD_FUNC

    XI_LOG("All OpenGL functions loaded successfully");
    return true;
}

void OpenGLPlatform_DestroyWindow(void* InHwnd, void* InHDC, void* InGLContext)
{
    HWND  hWnd  = (HWND)InHwnd;
    HDC   hDC   = (HDC)InHDC;
    HGLRC hGLRC = (HGLRC)InGLContext;

    if (hGLRC)
    {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(hGLRC);
    }
    if (hDC && hWnd)
    {
        ReleaseDC(hWnd, hDC);
    }
    if (hWnd)
    {
        DestroyWindow(hWnd);
    }
}

bool OpenGLPlatform_PumpMessages()
{
    MSG Msg;
    while (PeekMessageA(&Msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (Msg.message == WM_QUIT)
        {
            return false;
        }
        TranslateMessage(&Msg);
        DispatchMessageA(&Msg);
    }
    return true;
}

// ═══════════════════════════════════════════════
// YaoOpenGLBuffer — 实现 YaoRHIBuffer
// ═══════════════════════════════════════════════

YaoOpenGLBuffer::YaoOpenGLBuffer(const YaoBufferDesc& Desc, GLenum InTarget)
    : m_Target(InTarget)
    , m_Size(Desc.m_SizeInBytes)
{
    g_GL.glGenBuffers(1, &m_BufferID);
    g_GL.glBindBuffer(m_Target, m_BufferID);
    g_GL.glBufferData(m_Target, m_Size, Desc.m_InitialData,
        Desc.m_bIsDynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
    g_GL.glBindBuffer(m_Target, 0);  // 解绑
}

YaoOpenGLBuffer::~YaoOpenGLBuffer()
{
    if (m_BufferID)
    {
        g_GL.glDeleteBuffers(1, &m_BufferID);
    }
}

void YaoOpenGLBuffer::SetData(const void* Data, int32 SizeInBytes)
{
    if (SizeInBytes > m_Size)
    {
        m_Size = SizeInBytes;
    }
    // 重新分配缓冲区
    g_GL.glDeleteBuffers(1, &m_BufferID);
    g_GL.glGenBuffers(1, &m_BufferID);
    g_GL.glBindBuffer(m_Target, m_BufferID);
    g_GL.glBufferData(m_Target, SizeInBytes, Data, GL_DYNAMIC_DRAW);
    g_GL.glBindBuffer(m_Target, 0);
}

// ═══════════════════════════════════════════════
// YaoOpenGLShader — 实现 YaoRHIShader
// ═══════════════════════════════════════════════

YaoOpenGLShader::YaoOpenGLShader(const char* VertexSource, const char* PixelSource)
{
    GLuint VS = CompileStage(GL_VERTEX_SHADER, VertexSource);
    GLuint PS = CompileStage(GL_FRAGMENT_SHADER, PixelSource);

    if (!VS || !PS)
    {
        XI_LOG_ERROR("Shader compilation failed");
        if (VS) g_GL.glDeleteShader(VS);
        if (PS) g_GL.glDeleteShader(PS);
        return;
    }

    m_ProgramID = g_GL.glCreateProgram();
    g_GL.glAttachShader(m_ProgramID, VS);
    g_GL.glAttachShader(m_ProgramID, PS);
    g_GL.glLinkProgram(m_ProgramID);

    // 检查链接状态
    GLint LinkStatus = GL_FALSE;
    g_GL.glGetProgramiv(m_ProgramID, GL_LINK_STATUS, &LinkStatus);
    if (LinkStatus != GL_TRUE)
    {
        char InfoLog[512];
        g_GL.glGetProgramInfoLog(m_ProgramID, sizeof(InfoLog), nullptr, InfoLog);
        XI_LOG_ERROR("Shader program linking failed: %s", InfoLog);
        g_GL.glDeleteProgram(m_ProgramID);
        m_ProgramID = 0;
    }

    // 标记着色器可删除（已链接到程序中）
    g_GL.glDeleteShader(VS);
    g_GL.glDeleteShader(PS);

    XI_LOG("Shader program created (ID=%u)", m_ProgramID);
}

YaoOpenGLShader::~YaoOpenGLShader()
{
    if (m_ProgramID)
    {
        g_GL.glDeleteProgram(m_ProgramID);
    }
}

GLuint YaoOpenGLShader::CompileStage(GLenum StageType, const char* Source)
{
    GLuint Shader = g_GL.glCreateShader(StageType);
    g_GL.glShaderSource(Shader, 1, &Source, nullptr);
    g_GL.glCompileShader(Shader);

    GLint CompileStatus = GL_FALSE;
    g_GL.glGetShaderiv(Shader, GL_COMPILE_STATUS, &CompileStatus);
    if (CompileStatus != GL_TRUE)
    {
        char InfoLog[512];
        g_GL.glGetShaderInfoLog(Shader, sizeof(InfoLog), nullptr, InfoLog);
        const char* StageName = (StageType == GL_VERTEX_SHADER) ? "Vertex" : "Fragment";
        XI_LOG_ERROR("%s shader compilation failed: %s", StageName, InfoLog);
        g_GL.glDeleteShader(Shader);
        return 0;
    }

    return Shader;
}

void YaoOpenGLShader::Bind()
{
    g_GL.glUseProgram(m_ProgramID);
}

void YaoOpenGLShader::SetUniform1f(const char* Name, float Value)
{
    GLint Location = g_GL.glGetUniformLocation(m_ProgramID, Name);
    if (Location != -1)
    {
        g_GL.glUniform1f(Location, Value);
    }
}

void YaoOpenGLShader::SetUniformMat4(const char* Name, const float* Matrix)
{
    GLint Location = g_GL.glGetUniformLocation(m_ProgramID, Name);
    if (Location != -1)
    {
        g_GL.glUniformMatrix4fv(Location, 1, GL_FALSE, Matrix);
    }
}

// ═══════════════════════════════════════════════
// YaoOpenGLRHI — 实现 YaoRHIDevice
// ═══════════════════════════════════════════════

YaoOpenGLRHI::YaoOpenGLRHI(void* InHwnd, void* InHDC, void* InGLContext, int32 InWidth, int32 InHeight)
    : m_WindowHandle(InHwnd)
    , m_DeviceContext(InHDC)
    , m_GLRenderContext(InGLContext)
    , m_Width(InWidth)
    , m_Height(InHeight)
{
    // 创建默认 VAO（OpenGL 核心配置要求）
    g_GL.glGenVertexArrays(1, &m_CurrentVAO);
    g_GL.glBindVertexArray(m_CurrentVAO);

    XI_LOG("YaoOpenGLRHI initialized (%d x %d)", m_Width, m_Height);
}

YaoOpenGLRHI::~YaoOpenGLRHI()
{
    if (m_CurrentVAO)
    {
        g_GL.glDeleteVertexArrays(1, &m_CurrentVAO);
    }
}

// ── 资源创建 ─────────────────────────────────────

YaoSharedPtr<YaoRHIBuffer> YaoOpenGLRHI::CreateVertexBuffer(const YaoBufferDesc& Desc)
{
    return YaoMakeShared<YaoOpenGLBuffer>(Desc, GL_ARRAY_BUFFER);
}

YaoSharedPtr<YaoRHIBuffer> YaoOpenGLRHI::CreateIndexBuffer(const YaoBufferDesc& Desc)
{
    return YaoMakeShared<YaoOpenGLBuffer>(Desc, GL_ELEMENT_ARRAY_BUFFER);
}

YaoSharedPtr<YaoRHIShader> YaoOpenGLRHI::CreateShader(
    const char* VertexSource,
    const char* PixelSource)
{
    auto Shader = YaoMakeShared<YaoOpenGLShader>(VertexSource, PixelSource);
    if (Shader->GetProgramID() == 0)
    {
        return nullptr;  // 编译失败
    }
    return Shader;
}

// ── 渲染命令 ─────────────────────────────────────

void YaoOpenGLRHI::SetViewport(const YaoViewport& Viewport)
{
    g_GL.glViewport(Viewport.m_X, Viewport.m_Y, Viewport.m_Width, Viewport.m_Height);
}

void YaoOpenGLRHI::Clear(const YaoClearColor& Color)
{
    g_GL.glClearColor(Color.m_R, Color.m_G, Color.m_B, Color.m_A);
    g_GL.glClear(GL_COLOR_BUFFER_BIT);
}

void YaoOpenGLRHI::SetVertexBuffer(YaoRHIBuffer* Buffer, const YaoArray<YaoVertexElement>& Layout)
{
    YaoOpenGLBuffer* GLBuffer = static_cast<YaoOpenGLBuffer*>(Buffer);
    if (!GLBuffer) return;

    g_GL.glBindVertexArray(m_CurrentVAO);
    g_GL.glBindBuffer(GL_ARRAY_BUFFER, GLBuffer->GetGLBuffer());

    // 设置顶点属性布局
    for (int32 i = 0; i < Layout.Num(); ++i)
    {
        const YaoVertexElement& Elem = Layout[i];

        GLenum Type      = GL_FLOAT;
        GLboolean bNorm  = Elem.m_bNormalized ? GL_TRUE : GL_FALSE;
        int32 CompCount  = 1;

        switch (Elem.m_Type)
        {
            case YaoVertexElementType::Float:  Type = GL_FLOAT; CompCount = 1; break;
            case YaoVertexElementType::Float2: Type = GL_FLOAT; CompCount = 2; break;
            case YaoVertexElementType::Float3: Type = GL_FLOAT; CompCount = 3; break;
            case YaoVertexElementType::Float4: Type = GL_FLOAT; CompCount = 4; break;
            case YaoVertexElementType::UByte4: Type = GL_UNSIGNED_BYTE; CompCount = 4; bNorm = GL_TRUE; break;
        }

        g_GL.glEnableVertexAttribArray(i);
        g_GL.glVertexAttribPointer(
            i,                                    // 属性索引
            CompCount,                            // 分量数
            Type,                                 // 数据类型
            bNorm,                                // 是否归一化
            Elem.m_Stride,                        // 步长
            (const void*)(intptr_t)Elem.m_Offset  // 偏移
        );
    }

    m_CurrentVertexBuffer = GLBuffer->GetGLBuffer();
    m_CurrentStride       = (Layout.Num() > 0) ? Layout[0].m_Stride : 0;
}

void YaoOpenGLRHI::SetIndexBuffer(YaoRHIBuffer* Buffer)
{
    YaoOpenGLBuffer* GLBuffer = static_cast<YaoOpenGLBuffer*>(Buffer);
    if (!GLBuffer) return;

    g_GL.glBindVertexArray(m_CurrentVAO);
    g_GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GLBuffer->GetGLBuffer());
    m_CurrentIndexBuffer = GLBuffer->GetGLBuffer();
}

void YaoOpenGLRHI::DrawIndexed(int32 IndexCount, int32 StartIndex)
{
    g_GL.glDrawElements(
        GL_TRIANGLES,
        IndexCount,
        GL_UNSIGNED_BYTE,  // 假设索引类型是 uint8
        (const void*)(intptr_t)(StartIndex * sizeof(uint8))
    );
}

// ── 帧管理 ───────────────────────────────────────

void YaoOpenGLRHI::Present()
{
    SwapBuffers(reinterpret_cast<HDC>(m_DeviceContext));
}

bool YaoOpenGLRHI::IsWindowOpen() const
{
    return IsWindow((HWND)m_WindowHandle) != 0;
}

// ═══════════════════════════════════════════════
// RHI 工厂函数
// ═══════════════════════════════════════════════

YaoRHIDevice* CreateRHI(YaoRHIType Type, void* WindowHandle, int32 Width, int32 Height)
{
    if (Type == YaoRHIType::OpenGL)
    {
        // 当前由外部 OpenGLPlatform_ 创建窗口和上下文后直接构造 YaoOpenGLRHI
        return nullptr;
    }

    XI_LOG_ERROR("Unsupported RHI type");
    return nullptr;
}

} // namespace Xi::Yao
