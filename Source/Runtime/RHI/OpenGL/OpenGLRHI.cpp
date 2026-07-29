// OpenGLRHI.cpp — OpenGL RHI 后端实现
// Win32 窗口创建 + wgl OpenGL 上下文 + IRHI 接口的 OpenGL 实现
#include "OpenGLRHI.h"
#include "LogMacros.h"

// Windows 头（需要放在最前面以定义 HWND/HDC/HGLRC）
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdlib>

// ── 全局 OpenGL 函数表 ──────────────────────────
FOpenGLFuncs GL;

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
            // 窗口大小改变时，OpenGL 视口由上层设置
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
    // 1. 注册窗口类
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
        0,
        "XiEngineWindow",
        Title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WinW, WinH,
        nullptr, nullptr,
        hInstance,
        nullptr);

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
            0x2091, 4,       // WGL_CONTEXT_MAJOR_VERSION_ARB = 4
            0x2092, 5,       // WGL_CONTEXT_MINOR_VERSION_ARB = 5
            0x9126, 0x00000001,  // WGL_CONTEXT_PROFILE_MASK_ARB = WGL_CONTEXT_CORE_PROFILE_BIT_ARB
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

    // 8. 输出 OpenGL 信息
    const char* GLVersion   = (const char*)wglGetProcAddress("glGetString") ?
        "" : "";  // 需要先加载函数才能调用 glGetString
    XI_LOG("OpenGL context created successfully");

    OutHwnd      = hWnd;
    OutHDC       = hDC;
    OutGLContext = TempContext;

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    return true;
}

// 传统 OpenGL 1.0/1.1 函数从 opengl32.dll 直接导入
// wglGetProcAddress 在某些驱动上不返回这些函数
extern "C" {
    __declspec(dllimport) void __stdcall glViewport(GLint, GLint, GLsizei, GLsizei);
    __declspec(dllimport) void __stdcall glClearColor(GLfloat, GLfloat, GLfloat, GLfloat);
    __declspec(dllimport) void __stdcall glClear(GLbitfield);
    __declspec(dllimport) void __stdcall glDrawElements(GLenum, GLsizei, GLenum, const void*);
}

bool OpenGLPlatform_LoadFunctions()
{
    // OpenGL 1.0/1.1 传统函数：直接从 opengl32.dll 导入，取地址赋给函数表
    GL.glViewport   = glViewport;
    GL.glClearColor = glClearColor;
    GL.glClear      = glClear;
    GL.glDrawElements = glDrawElements;
    #define LOAD_FUNC(Name) \
        GL.Name = (decltype(GL.Name))wglGetProcAddress(#Name); \
        if (!GL.Name) { \
            XI_LOG_ERROR("Failed to load OpenGL function: %s", #Name); \
            return false; \
        }

    // 现代核心函数（OpenGL 1.5+）：通过 wglGetProcAddress 加载
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
// OpenGLBuffer — 实现 IRHIBuffer
// ═══════════════════════════════════════════════

FOpenGLBuffer::FOpenGLBuffer(const FBufferDesc& Desc, GLenum InTarget)
    : Target(InTarget)
    , Size(Desc.SizeInBytes)
{
    GL.glGenBuffers(1, &BufferID);
    GL.glBindBuffer(Target, BufferID);
    GL.glBufferData(Target, Size, Desc.InitialData,
        Desc.bIsDynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
    GL.glBindBuffer(Target, 0);  // 解绑
}

FOpenGLBuffer::~FOpenGLBuffer()
{
    if (BufferID)
    {
        GL.glDeleteBuffers(1, &BufferID);
    }
}

void FOpenGLBuffer::SetData(const void* Data, int32 SizeInBytes)
{
    if (SizeInBytes > Size)
    {
        Size = SizeInBytes;
    }
    // 重新分配缓冲区（简单的策略：删除旧缓冲区，创建新的）
    GL.glDeleteBuffers(1, &BufferID);
    GL.glGenBuffers(1, &BufferID);
    GL.glBindBuffer(Target, BufferID);
    GL.glBufferData(Target, SizeInBytes, Data, GL_DYNAMIC_DRAW);
    GL.glBindBuffer(Target, 0);
}

// ═══════════════════════════════════════════════
// OpenGLShader — 实现 IRHIShader
// ═══════════════════════════════════════════════

FOpenGLShader::FOpenGLShader(const char* VertexSource, const char* PixelSource)
{
    GLuint VS = CompileStage(GL_VERTEX_SHADER, VertexSource);
    GLuint PS = CompileStage(GL_FRAGMENT_SHADER, PixelSource);

    if (!VS || !PS)
    {
        XI_LOG_ERROR("Shader compilation failed");
        if (VS) GL.glDeleteShader(VS);
        if (PS) GL.glDeleteShader(PS);
        return;
    }

    ProgramID = GL.glCreateProgram();
    GL.glAttachShader(ProgramID, VS);
    GL.glAttachShader(ProgramID, PS);
    GL.glLinkProgram(ProgramID);

    // 检查链接状态
    GLint LinkStatus = GL_FALSE;
    GL.glGetProgramiv(ProgramID, GL_LINK_STATUS, &LinkStatus);
    if (LinkStatus != GL_TRUE)
    {
        char InfoLog[512];
        GL.glGetProgramInfoLog(ProgramID, sizeof(InfoLog), nullptr, InfoLog);
        XI_LOG_ERROR("Shader program linking failed: %s", InfoLog);
        GL.glDeleteProgram(ProgramID);
        ProgramID = 0;
    }

    // 标记着色器可删除（已链接到程序中）
    GL.glDeleteShader(VS);
    GL.glDeleteShader(PS);

    XI_LOG("Shader program created (ID=%u)", ProgramID);
}

FOpenGLShader::~FOpenGLShader()
{
    if (ProgramID)
    {
        GL.glDeleteProgram(ProgramID);
    }
}

GLuint FOpenGLShader::CompileStage(GLenum StageType, const char* Source)
{
    GLuint Shader = GL.glCreateShader(StageType);
    GL.glShaderSource(Shader, 1, &Source, nullptr);
    GL.glCompileShader(Shader);

    GLint CompileStatus = GL_FALSE;
    GL.glGetShaderiv(Shader, GL_COMPILE_STATUS, &CompileStatus);
    if (CompileStatus != GL_TRUE)
    {
        char InfoLog[512];
        GL.glGetShaderInfoLog(Shader, sizeof(InfoLog), nullptr, InfoLog);
        const char* StageName = (StageType == GL_VERTEX_SHADER) ? "Vertex" : "Fragment";
        XI_LOG_ERROR("%s shader compilation failed: %s", StageName, InfoLog);
        GL.glDeleteShader(Shader);
        return 0;
    }

    return Shader;
}

void FOpenGLShader::Bind()
{
    GL.glUseProgram(ProgramID);
}

void FOpenGLShader::SetUniform1f(const char* Name, float Value)
{
    GLint Location = GL.glGetUniformLocation(ProgramID, Name);
    if (Location != -1)
    {
        GL.glUniform1f(Location, Value);
    }
}

void FOpenGLShader::SetUniformMat4(const char* Name, const float* Matrix)
{
    GLint Location = GL.glGetUniformLocation(ProgramID, Name);
    if (Location != -1)
    {
        GL.glUniformMatrix4fv(Location, 1, GL_FALSE, Matrix);
    }
}

// ═══════════════════════════════════════════════
// OpenGLRHI — 实现 IRHIDevice
// ═══════════════════════════════════════════════

FOpenGLRHI::FOpenGLRHI(void* InHwnd, void* InHDC, void* InGLContext, int32 InWidth, int32 InHeight)
    : WindowHandle(InHwnd)
    , DeviceContext(InHDC)
    , GLRenderContext(InGLContext)
    , Width(InWidth)
    , Height(InHeight)
{
    // 创建默认 VAO（OpenGL 核心配置要求）
    GL.glGenVertexArrays(1, &CurrentVAO);
    GL.glBindVertexArray(CurrentVAO);

    // 启用面剔除（可选）
    // glEnable(GL_CULL_FACE);

    XI_LOG("FOpenGLRHI initialized (%d x %d)", Width, Height);
}

FOpenGLRHI::~FOpenGLRHI()
{
    if (CurrentVAO)
    {
        GL.glDeleteVertexArrays(1, &CurrentVAO);
    }
    // 窗口和上下文由外部管理
}

// ── 资源创建 ─────────────────────────────────────

TSharedPtr<IRHIBuffer> FOpenGLRHI::CreateVertexBuffer(const FBufferDesc& Desc)
{
    return MakeShared<FOpenGLBuffer>(Desc, GL_ARRAY_BUFFER);
}

TSharedPtr<IRHIBuffer> FOpenGLRHI::CreateIndexBuffer(const FBufferDesc& Desc)
{
    return MakeShared<FOpenGLBuffer>(Desc, GL_ELEMENT_ARRAY_BUFFER);
}

TSharedPtr<IRHIShader> FOpenGLRHI::CreateShader(
    const char* VertexSource,
    const char* PixelSource)
{
    auto Shader = MakeShared<FOpenGLShader>(VertexSource, PixelSource);
    if (Shader->GetProgramID() == 0)
    {
        return nullptr;  // 编译失败
    }
    return Shader;
}

// ── 渲染命令 ─────────────────────────────────────

void FOpenGLRHI::SetViewport(const FViewport& Viewport)
{
    GL.glViewport(Viewport.X, Viewport.Y, Viewport.Width, Viewport.Height);
}

void FOpenGLRHI::Clear(const FClearColor& Color)
{
    GL.glClearColor(Color.R, Color.G, Color.B, Color.A);
    GL.glClear(GL_COLOR_BUFFER_BIT);
}

void FOpenGLRHI::SetVertexBuffer(IRHIBuffer* Buffer, const TArray<FVertexElement>& Layout)
{
    FOpenGLBuffer* GLBuffer = static_cast<FOpenGLBuffer*>(Buffer);
    if (!GLBuffer) return;

    GL.glBindVertexArray(CurrentVAO);
    GL.glBindBuffer(GL_ARRAY_BUFFER, GLBuffer->GetGLBuffer());

    // 设置顶点属性布局
    for (int32 i = 0; i < Layout.Num(); ++i)
    {
        const FVertexElement& Elem = Layout[i];

        GLenum Type   = GL_FLOAT;
        GLboolean Norm = Elem.bNormalized ? GL_TRUE : GL_FALSE;
        int32 CompCount = 1;

        switch (Elem.Type)
        {
            case EVertexElementType::Float:  Type = GL_FLOAT; CompCount = 1; break;
            case EVertexElementType::Float2: Type = GL_FLOAT; CompCount = 2; break;
            case EVertexElementType::Float3: Type = GL_FLOAT; CompCount = 3; break;
            case EVertexElementType::Float4: Type = GL_FLOAT; CompCount = 4; break;
            case EVertexElementType::UByte4: Type = GL_UNSIGNED_BYTE; CompCount = 4; Norm = GL_TRUE; break;
        }

        GL.glEnableVertexAttribArray(i);
        GL.glVertexAttribPointer(
            i,                   // 属性索引
            CompCount,           // 分量数
            Type,                // 数据类型
            Norm,                // 是否归一化
            Elem.Stride,         // 步长
            (const void*)(intptr_t)Elem.Offset  // 偏移
        );
    }

    CurrentVertexBuffer = GLBuffer->GetGLBuffer();
    CurrentStride       = (Layout.Num() > 0) ? Layout[0].Stride : 0;
}

void FOpenGLRHI::SetIndexBuffer(IRHIBuffer* Buffer)
{
    FOpenGLBuffer* GLBuffer = static_cast<FOpenGLBuffer*>(Buffer);
    if (!GLBuffer) return;

    GL.glBindVertexArray(CurrentVAO);
    GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GLBuffer->GetGLBuffer());
    CurrentIndexBuffer = GLBuffer->GetGLBuffer();
}

void FOpenGLRHI::DrawIndexed(int32 IndexCount, int32 StartIndex)
{
    GL.glDrawElements(
        GL_TRIANGLES,
        IndexCount,
        GL_UNSIGNED_BYTE,  // 假设索引类型是 uint8（简单三角形用）
        (const void*)(intptr_t)(StartIndex * sizeof(uint8))
    );
}

// ── 帧管理 ───────────────────────────────────────

void FOpenGLRHI::Present()
{
    SwapBuffers(reinterpret_cast<HDC>(this->DeviceContext));
}

bool FOpenGLRHI::IsWindowOpen() const
{
    return IsWindow((HWND)WindowHandle) != 0;
}

// ═══════════════════════════════════════════════
// RHI 工厂函数
// ═══════════════════════════════════════════════

IRHIDevice* CreateRHI(ERHIType Type, void* WindowHandle, int32 Width, int32 Height)
{
    if (Type == ERHIType::OpenGL)
    {
        // OpenGL 平台自行创建了窗口，这里直接使用传入的句柄
        void* hWnd      = WindowHandle;
        void* hDC       = nullptr;  // 由 CreateRHI 调用方提供
        void* glContext = nullptr;

        // 注意：如果外部已创建窗口和上下文，这里只需创建设备
        // 当前设计是 OpenGLPlatform_CreateWindow 创建窗口+上下文
        // 然后传入 FOpenGLRHI 管理
        return nullptr;  // 实际创建由外部管理（见下文工厂逻辑）
    }

    XI_LOG_ERROR("Unsupported RHI type");
    return nullptr;
}
