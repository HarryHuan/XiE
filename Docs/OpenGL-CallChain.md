# 羲引擎 OpenGL 调用链文档

> 记录从 Testbed 验证程序到 GPU 的全链路调用序列，便于理解渲染管线各层职责。

---

## 1. 初始化阶段

```
Main.cpp
  │
  ├─ OpenGLPlatform_CreateWindow()
  │     ├─ RegisterClassExA()         ← 注册 Win32 窗口类
  │     ├─ CreateWindowExA()          ← 创建窗口
  │     ├─ ChoosePixelFormat()        ← 像素格式
  │     ├─ SetPixelFormat()           ← 设置像素格式
  │     ├─ wglCreateContext()         ← 创建临时 OpenGL 上下文
  │     ├─ wglMakeCurrent()           ← 绑定到线程
  │     └─ wglCreateContextAttribsARB() ← 升级到 OpenGL 4.5 Core
  │
  ├─ OpenGLPlatform_LoadFunctions()
  │     ├─ wglGetProcAddress("glGenBuffers")       → g_GL.glGenBuffers
  │     ├─ wglGetProcAddress("glBindBuffer")       → g_GL.glBindBuffer
  │     ├─ wglGetProcAddress("glCreateShader")     → g_GL.glCreateShader
  │     ├─ wglGetProcAddress("glUseProgram")       → g_GL.glUseProgram
  │     ├─ ... (27 个函数全部动态加载)
  │     └─ opengl32.dll 直接导入:
  │           glViewport / glClearColor / glClear / glDrawElements
  │
  └─ YaoOpenGLRHI RHI(hWnd, hDC, glContext, 800, 600)
        └─ glGenVertexArrays(1, &m_CurrentVAO)     ← 创建默认 VAO
```

---

## 2. 资源创建阶段

```
Main.cpp
  │
  ├─ RHI.CreateVertexBuffer(VBDesc)   → YaoMakeShared<YaoOpenGLBuffer>
  │     └─ YaoOpenGLBuffer::YaoOpenGLBuffer()
  │           ├─ g_GL.glGenBuffers(1, &m_BufferID)         ← 分配 GPU 缓冲
  │           ├─ g_GL.glBindBuffer(GL_ARRAY_BUFFER, id)     ← 绑定
  │           └─ g_GL.glBufferData(..., Vertices, GL_STATIC_DRAW) ← 上传顶点数据到 GPU
  │
  ├─ RHI.CreateIndexBuffer(IBDesc)    → YaoMakeShared<YaoOpenGLBuffer>
  │     └─ 同上，target = GL_ELEMENT_ARRAY_BUFFER
  │
  └─ RHI.CreateShader(vsGLSL, psGLSL) → YaoMakeShared<YaoOpenGLShader>
        └─ YaoOpenGLShader::YaoOpenGLShader()
              ├─ glCreateShader(GL_VERTEX_SHADER)
              ├─ glShaderSource(vs, 1, &source, nullptr)
              ├─ glCompileShader(vs)
              ├─ glCreateShader(GL_FRAGMENT_SHADER)
              ├─ glShaderSource(ps, 1, &source, nullptr)
              ├─ glCompileShader(ps)
              ├─ glCreateProgram()
              ├─ glAttachShader(program, vs)
              ├─ glAttachShader(program, ps)
              ├─ glLinkProgram(program)
              ├─ glDeleteShader(vs)
              └─ glDeleteShader(ps)
```

---

## 3. 每帧渲染循环

```
while (窗口未关闭)
{
  OpenGLPlatform_PumpMessages()
  │
  ├─ RHI.Clear(clearColor)
  │     ├─ g_GL.glClearColor(0.1, 0.1, 0.15, 1.0)   ← 设置清屏颜色
  │     └─ g_GL.glClear(GL_COLOR_BUFFER_BIT)          ← 清除帧缓冲
  │
  ├─ RHI.SetViewport(viewport)
  │     └─ g_GL.glViewport(0, 0, 800, 600)            ← 视口变换
  │
  ├─ Shader->Bind()
  │     └─ g_GL.glUseProgram(programID)                ← 绑定着色器
  │
  ├─ Shader->SetUniformMat4("ProjectionMatrix", ...)
  │     └─ g_GL.glUniformMatrix4fv(location, 1, GL_FALSE, matrix)
  │                                                    ← 设置 uniform
  │
  ├─ RHI.SetVertexBuffer(vertexBuf, layout)
  │     ├─ g_GL.glBindVertexArray(m_CurrentVAO)
  │     ├─ g_GL.glBindBuffer(GL_ARRAY_BUFFER, bufID)
  │     ├─ g_GL.glEnableVertexAttribArray(0)           ← position
  │     ├─ g_GL.glVertexAttribPointer(0, 3, GL_FLOAT, ...)
  │     ├─ g_GL.glEnableVertexAttribArray(1)           ← color
  │     └─ g_GL.glVertexAttribPointer(1, 3, GL_FLOAT, ...)
  │
  ├─ RHI.SetIndexBuffer(indexBuf)
  │     ├─ g_GL.glBindVertexArray(m_CurrentVAO)
  │     └─ g_GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufID)
  │
  ├─ RHI.DrawIndexed(3)
  │     └─ g_GL.glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_BYTE, 0)
  │                                                    ← GPU 执行绘制！
  │
  └─ RHI.Present()
        └─ SwapBuffers(hDC)                             ← 交换双缓冲
}
```

---

## 4. 数据流示意

```
CPU 内存                              GPU 内存
─────────────────────────────────────────────────────
Vertices[] ───→ glBufferData() ───→ Vertex Buffer (VBO)
Indices[]  ───→ glBufferData() ───→ Index Buffer  (IBO)
Shader GLSL──→ glCompileShader()──→ Shader Program   │
               glLinkProgram()                       │
                                        ┌───────────┘
                                        ▼
                              glDrawElements()
                              ┌─────────────────┐
                              │  顶点着色器运行   │  ← vertex shader
                              │  gl_Position =   │
                              │  Projection *    │
                              │  vec4(pos, 1.0)  │
                              ├─────────────────┤
                              │  片段着色器运行   │  ← fragment shader
                              │  Out = vec4(     │
                              │   FragColor,1.0) │
                              └─────────────────┘
                              Frame Buffer → SwapBuffers() → 屏幕
```

---

## 5. 层级总览

| 层 | 文件 | 做了什么 | 为什么 |
|----|------|---------|--------|
| **Testbed** | `Main.cpp` | 定义顶点/索引/着色器字符串，调用 RHI API | 完全不知道底层是 OpenGL |
| **YaoRHIDevice** | `RHIDevice.h` | 纯虚接口定义 | 解耦上层与 API 后端 |
| **YaoOpenGLRHI** | `OpenGLRHI.h/.cpp` | 实现全部接口，调用 `g_GL.xxx` | 通过函数指针表调用，不直接 link OpenGL |
| **g_GL 函数表** | `OpenGLRHI.h` | 存储 28 个动态加载的 OpenGL 函数指针 | 不需要 gl.h/glad，从零加载核心函数 |
| **Win32** | `OpenGLRHI.cpp` | 窗口 + WGL 上下文 | 零第三方依赖 |

| 阶段 | 核心 OpenGL 调用 | 对应 C++ 方法 |
|------|-------------------|---------------|
| 初始化 | `wglCreateContextAttribsARB`, `wglGetProcAddress` | `OpenGLPlatform_CreateWindow`, `OpenGLPlatform_LoadFunctions` |
| 创建 VAO | `glGenVertexArrays` | `YaoOpenGLRHI` 构造函数 |
| 创建 VBO/IBO | `glGenBuffers`, `glBindBuffer`, `glBufferData` | `YaoOpenGLBuffer` 构造函数 |
| 编译着色器 | `glCreateShader`, `glShaderSource`, `glCompileShader`, `glCreateProgram`, `glAttachShader`, `glLinkProgram` | `YaoOpenGLShader` 构造函数 |
| 每帧渲染 | `glClear`, `glViewport`, `glUseProgram`, `glUniform*`, `glBind*`, `glVertexAttribPointer`, `glEnableVertexAttribArray`, `glDrawElements`, `SwapBuffers` | `YaoOpenGLRHI::Clear/SetViewport/SetVertexBuffer/SetIndexBuffer/DrawIndexed/Present` |

---

*本文档由工具自动生成，对应阶段 0（工程骨架 + RHI OpenGL 后端 + 三角形验证）的完整渲染调用链。*
