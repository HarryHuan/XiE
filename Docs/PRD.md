# 羲 (Xi) — 产品需求文档 (PRD)

## 品牌

> 自伏羲布卦，万物循数而生；自逸少挥毫，万象由笔而成。
>
> 羲引擎，以华夏本源数理搭建底层渲染架构，以笔墨造物的创作理念打造编辑器生态。
>
> 从光栅管线、跨平台 RHI，到场景编辑、界面渲染，落笔即是山河，运算自成寰宇。

## 1. 概述

### 1.1 项目名称
**羲 (Xi)** — 从零实现的 Unreal Engine 5 核心功能精简版（不含蓝图系统）

### 1.2 项目目标
用 C++ 从头实现 UE5 除蓝图外的全部基本功能，包括但不限于：
- Slate UI 框架（声明式 C++ Widget 体系）
- 反射系统（UProperty / UFunction / UClass）
- 序列化与资产管线
- 素材导入管线（FBX/OBJ 模型、PNG/JPG 贴图、WAV 音频）
- 编辑器框架（多文档界面、可停靠面板、布局持久化）
- 基础渲染管线（RHI 抽象层 + OpenGL/DX12/Vulkan）
- 输入与命令绑定系统

最终产出为一个**可运行的桌面编辑器应用**。

### 1.3 非目标（明确不做的）
- 蓝图可视化脚本系统（用户指定排除）
- 与 Epic 的 UE5 二进制或源码兼容（独立实现，不依赖 Epic 代码）
- Nanite / Lumen 等高端渲染特性（第一阶段不做）
- 网络同步 / 多人游戏框架
- 完整的游戏运行时（重点是编辑器基础框架）

---

## 2. 技术选型

| 维度       | 选择                          | 理由                                                                                                          |
| ---------- | ----------------------------- | ------------------------------------------------------------------------------------------------------------- |
| 实现语言   | **C++20**                     | UE5 官方语言，Slate 声明式宏需要用 C++ 预处理器实现                                                           |
| 构建系统   | **CMake + Ninja**             | 跨平台，业界标准                                                                                              |
| 窗口管理   | **Win32 API（原生 Windows）** | 第一阶段只在 Windows 跑，最底层控制                                                                           |
| 渲染架构   | **RHI 抽象层 + 多后端**       | 仿 UE 的 RHI 设计，接口与实现分离。第一阶段 OpenGL 4.5+，后续扩展 DX12 / Vulkan                               |
| 第三方依赖 | **ThirdParty/ 统一管理** | `glfw`（窗口/上下文）+ `glad`（OpenGL）+ `spirv-cross`（shader 跨编译）+ `assimp`（模型解析）+ `stb_image`（贴图解码）+ `dr_libs`（音频解码）。后期增加 `dxc`（DX12）、`VulkanSDK` |

---

## 3. 模块架构

整个项目按以下模块分层实现：

```
XiE/
├── Source/
│   ├── Runtime/
│   │   ├── Core/           # 基础类型、容器、字符串、日志
│   │   ├── CoreUObject/    # 反射系统（UObject / UClass / UProperty）
│   │   ├── Serialization/  # 序列化（FArchive, 二进制/文本）
│   │   ├── RHI/            # 渲染硬件接口抽象层
│   │   │   ├── Public/     # RHI 抽象接口（IRHIDevice, IRHIBuffer 等）
│   │   │   ├── OpenGL/     # OpenGL 4.5+ 后端实现
│   │   │   ├── DX12/       # DirectX 12 后端（后续扩展）
│   │   │   └── Vulkan/     # Vulkan 后端（后续扩展）
│   │   └── SlateCore/      # Slate 基础：布局、绘制、事件（依赖 RHI）
│   ├── Slate/              # Slate Widget 库（具体控件）
│   ├── Editor/
│   │   ├── MainFrame/      # 编辑器主窗口框架
│   │   ├── DockSystem/     # 可停靠面板系统
│   │   ├── DetailsPanel/   # 属性面板
│   │   ├── ContentBrowser/ # 内容浏览器
│   │   └── Importer/       # 素材导入管线
│   │       ├── Common/     # 导入基类与导入配置 UI
│   │       ├── MeshImport/ # FBX/OBJ 模型解析
│   │       ├── TextureImport/ # PNG/JPG 贴图导入
│   │       └── AudioImport/   # WAV 音频导入
│   └── Renderer/           # 高层渲染编排（场景渲染、后处理等）
├── ThirdParty/             # 第三方库
│   ├── glfw/               # 窗口 + OpenGL 上下文
│   ├── glad/               # OpenGL 加载器
│   ├── spirv-cross/        # SPIR-V 跨编译（用于 shader）
│   ├── assimp/             # 模型解析（FBX, OBJ, glTF 等）
│   ├── stb_image.h/        # 贴图解码（PNG, JPG, BMP, TGA）
│   ├── dr_libs/            # 音频解码（dr_wav, dr_mp3）
│   └── dxc/                # DX12 着色器编译（后续）
├── Docs/                   # 文档
└── CMakeLists.txt
```

---


## 4. 阶段规划（里程碑）

### 阶段 0：工程基础设施 + RHI（OpenGL）
**目标：搭建工程骨架，实现 RHI 抽象层 + OpenGL 后端，能跑出 GPU 窗口**

- [x] 搭建 CMake 工程，配置模块目录（Core / RHI / Testbed）
- [x] 窗口与 OpenGL 上下文：使用 **Win32 API + WGL** 原生创建（未使用 glfw/glad，更贴近"从零实现"理念）
- [x] 实现基础类型：`FString`、`FName`、`TArray`、`TMap`、`TSharedPtr` / `TSharedRef` / `TWeakPtr`
- [x] 实现日志系统：`UE_LOG` 宏（五级日志 + 时间戳 + 文件行号）
- [x] **RHI 抽象接口设计**：`IRHIDevice`、`IRHIBuffer`、`IRHIShader`、`IRHIPipelineState`
- [x] **OpenGL 后端实现**：`FOpenGLRHI` 继承上述接口
- [x] OpenGL 上下文创建与窗口循环（Win32 原生）
- [x] 实现顶点缓冲、索引缓冲、统一着色器、正交投影
- [x] 基础 Shader 系统：编译 GLSL → 链接着色器程序
- [x] 验证：通过 RHI 接口绘制一个彩色三角形到窗口 ✅

### 阶段 1：SlateCore + GPU Batch 渲染
**目标：能用声明式语法创建 UI，通过 RHI 合批渲染到窗口**

- [ ] 实现 `SWidget` 基类（`TSharedFromThis` 模式）
- [ ] 实现 `SCompoundWidget` / `SPanel` 基类
- [ ] 实现布局系统：`SOverlay`、`SVerticalBox`、`SHorizontalBox`
- [ ] 实现声明式宏：`SNew`、`SAssignNew`，`.Slot()` `[ ]` 操作符
- [ ] 实现事件冒泡：`OnMouseButtonDown`、`OnPaint` 等虚函数
- [ ] 实现 `FSlateDrawBuffer` — 帧绘制命令收集
- [ ] 实现 `FSlateElementBatcher` — 将绘制元素转为 RHI 顶点数据
- [ ] 实现合批渲染器（按纹理/着色器合并批次，减少 DrawCall）
- [ ] 实现 `SWindow` 顶层窗口控件
- [ ] 验证：用声明式语法画出一个带按钮和文本的窗口，经 RHI → OpenGL 绘制

### 阶段 2：Slate Widget 库
**目标：有完整的常用控件集**

- [ ] `STextBlock` — 文本显示（GPU 文字图集渲染）
- [ ] `SEditableTextBox` — 文本输入
- [ ] `SButton` — 按钮
- [ ] `SCheckBox` — 复选框
- [ ] `SComboBox` — 下拉框
- [ ] `SListBox` / `STreeView` — 列表与树
- [ ] `SScrollBar` / `SScrollBox` — 滚动
- [ ] `SSplitter` — 可拖拽分割条
- [ ] `SImage` — 图片显示
- [ ] `SBorder` — 边框/背景装饰（含圆角、九宫格）
- [ ] 文字渲染优化（字体图集、glyph cache）
- [ ] 验证：组装出一个类似编辑器启动界面的 UI，60fps

### 阶段 3：反射系统（CoreUObject）
**目标：类型信息在运行时可知，支持属性反射**

- [ ] 实现 `UObject` 基类
- [ ] 实现 `UClass` — 类型描述符（名称、父类、属性列表、函数列表）
- [ ] 实现 `UProperty` — 属性描述（类型、偏移、序列化标签）
- [ ] 实现 `UFunction` — 函数描述
- [ ] 实现宏：`UCLASS()`、`UPROPERTY()`、`UFUNCTION()`
- [ ] 实现 `FObjectInitializer` / `StaticConstructObject`
- [ ] 实现 GC（简单标记-清扫）
- [ ] 验证：`GetClass()->GetProperties()` 能遍历到类的所有反射属性

### 阶段 4：序列化 & 资产管线 & 素材导入
**目标：对象能读写到文件，资产可注册管理，支持从外部导入素材**

- [ ] 实现 `FArchive`（抽象读写接口）
- [ ] 实现 `FBufferArchive` / `FMemoryArchive`
- [ ] 实现 `FArchiveSave` / `FArchiveLoad` 序列化代理
- [ ] 属性级自动序列化（遍历 `UProperty` 列表读写）
- [ ] 实现 `UPackage` — 包/文件的抽象
- [ ] 实现资产路径系统：`/Game/MyAsset.MyAsset`
- [ ] 实现 `UAssetManager` — 资产注册表
- [ ] **导入基类**：`UFactory` — 工厂模式，每种素材一个 Factory
- [ ] **模型导入**：基于 assimp 解析 FBX/OBJ → 生成 `UStaticMesh` 资产
- [ ] **贴图导入**：基于 stb_image 解码 PNG/JPG → 生成 `UTexture2D` 资产
- [ ] **音频导入**：基于 dr_wav 解析 WAV → 生成 `USoundWave` 资产
- [ ] **导入配置 UI**：`SImportOptionsPanel`，导入前显示配置选项
- [ ] 验证：拖入 FBX → 弹出导入选项 → 确认后生成 UStaticMesh 资产 → 内容浏览器可见

### 阶段 5：编辑器框架
**目标：一个可用的多文档编辑器界面**

- [ ] `FEditorApp` — 主应用程序类
- [ ] `SDockTab` — 可停靠标签页
- [ ] `SDockingArea` — 停靠区域管理器
- [ ] 布局持久化（保存/恢复停靠布局到 ini）
- [ ] `SDetailsPanel` — 属性面板（基于反射自动生成 UI）
- [ ] `SContentBrowser` — 资产浏览树
- [ ] `SMainFrame` — 主框架（菜单栏、工具栏）
- [ ] 菜单系统：`FMenuBar` / `FMenuBuilder`
- [ ] 命令系统：`FUICommandList` — 命令绑定、快捷键
- [ ] 验证：能启动编辑器，拖拽停靠面板，点击菜单执行命令

### 阶段 6：（后续扩展）DX12 / Vulkan RHI 后端
**目标：RHI 新增后端，验证多 API 切换能力**

- [ ] DX12 后端：`FDX12DynamicRHI` 实现全部 RHI 接口
- [ ] Vulkan 后端：`FVulkanDynamicRHI` 实现全部 RHI 接口
- [ ] Shader 跨编译：GLSL ↔ SPIR-V ↔ DXIL
- [ ] RHI 工厂模式：运行时选择 `CreateRHI(ERHIType::DX12)` 切换
- [ ] 验证：同一套 Slate UI 在 OpenGL / DX12 / Vulkan 下输出一致

---

## 5. 架构详细设计

### 5.1 Slate 声明式语法设计

仿照 UE5 Slate 的宏体系：

```cpp
// 控件声明
SNew(SVerticalBox)
    + SVerticalBox::Slot()
    .AutoHeight()
    .Padding(5)
    [
        SNew(STextBlock)
        .Text("Hello, Xi!")
        .ColorAndOpacity(FColor::White)
    ]
    + SVerticalBox::Slot()
    [
        SNew(SButton)
        .Text("Click Me")
        .OnClicked(this, &MyClass::OnButtonClick)
    ]
```

实现方案：利用 C++ 运算符重载和代理对象：
- `SNew(T)` → `T::FSlot` 代理对象
- `operator+` → 向容器添加 Slot
- `operator[]` → 设置 Slot 的子控件
- `.Text()` / `.Padding()` → 链式调用 setter（返回 `this` 引用）

### 5.2 反射系统设计

```cpp
// 用户声明的类
UCLASS()
class MyClass : public UObject
{
    UCLASS_BODY()
public:
    UPROPERTY(EditAnywhere, Category = "Transform")
    FVector Position;

    UPROPERTY(VisibleAnywhere)
    float Scale;

    UFUNCTION(Category = "Action")
    void DoSomething();
};

// 宏展开后生成:
// 1. 静态 UClass* MyClass::StaticClass()
// 2. 反射属性数组: UProperty* MyClass::__PropertyList[]
// 3. 构造函数注册
// 4. 序列化回调
```

生成方式：第一阶段**手动写注册代码**（不用 UHT），后续可用 Python 脚本做代码生成。

### 5.3 RHI 架构设计

仿 UE 的 RHI（Rendering Hardware Interface）层，目的是将 Slate/渲染器与底层图形 API 解耦。

```
┌─────────────────────────────────────────────┐
│  Slate / Editor / Renderer（上层消费者）      │
├─────────────────────────────────────────────┤
│  RHI 抽象接口层                              │
│  IRHIDevice  IRHIBuffer  IRHITexture        │
│  IRHIShader  IRHIPipeline  IRHISwapChain    │
├────────────────┬──────────────┬─────────────┤
│ OpenGL 后端    │ DX12 后端    │ Vulkan 后端  │
│ FOpenGLRHI     │ FDX12RHI     │ FVulkanRHI  │
└────────────────┴──────────────┴─────────────┘
```

核心接口设计：

```cpp
// RHI 抽象设备 —— 所有 GPU 操作的入口
class IRHIDevice
{
public:
    virtual ~IRHIDevice() = default;

    // 资源创建
    virtual TRefCountPtr<IRHIBuffer>       CreateBuffer(...) = 0;
    virtual TRefCountPtr<IRHITexture>      CreateTexture(...) = 0;
    virtual TRefCountPtr<IRHIShader>       CreateShader(...) = 0;
    virtual TRefCountPtr<IRHIPipelineState> CreatePipelineState(...) = 0;
    virtual TRefCountPtr<IRHISwapChain>    CreateSwapChain(...) = 0;

    // 渲染命令
    virtual void Clear(...) = 0;
    virtual void DrawIndexed(uint32 IndexCount, ...) = 0;
    virtual void SetViewport(...) = 0;

    // 帧管理
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Present() = 0;
};

// 工厂模式创建 RHI
enum class ERHIType { OpenGL, DX12, Vulkan };
IRHIDevice* CreateRHI(ERHIType Type, const FWindowHandle& Window);
```

设计要点：
- **所有后端实现相同的抽象接口**，上层零改动即可切换 API
- **Shader 系统**：统一用 SPIR-V 作为中间表示，GLSL / DXIL 通过 `spirv-cross` 转换
- **资源生命周期**：RHI 层负责 GPU 资源的创建与销毁，上层只管接口引用
- **调试层**：每个 RHI 后端包装 `Debug` 前缀的验证层调用（如 Vulkan Validation Layers）

### 5.4 Slate 渲染管线（基于 RHI）

仿 UE5 Slate 的 GPU Batch 渲染方案：

```
每一帧：
  1. SWidget::OnPaint() 递归调用子控件
  2. 每个控件向 FSlateDrawBuffer 追加 FSlateDrawElement（矩形、文本、图片等）
  3. FSlateElementBatcher 将元素转成顶点数据（位置、UV、颜色）
  4. 按纹理/着色器合并批次，减少 DrawCall
  5. 通过 RHI 提交：IRHIDevice::DrawIndexed()  →  具体后端（OpenGL/DX12/Vulkan）
```

关键着色器：
- **基础颜色着色器** — 纯色矩形填充（顶点颜色插值）
- **纹理着色器** — 图片及文字图集采样
- **圆角/裁剪** — fragment shader 中 discard 实现

文字渲染：首先生成字体位图图集（glyph atlas），每个字符对应一个 quad，采样图集纹理。

### 5.5 编辑器布局系统

```
FEditorApp
 ├── SMainFrame (窗口边框 + 菜单栏 + 工具栏)
 └── SDockingArea (根停靠管理)
      ├── SDockTab (左侧面板)
      │    ├── SContentBrowser
      │    └── SAssetTree
      ├── SDockTab (中央区域)
      │    └── SEditorViewport (占位)
      └── SDockTab (右侧面板)
           └── SDetailsPanel (基于反射自动生成)
```

### 5.6 导入管线设计

素材导入流程，仿 UE 的 Factory 模式：

```
外部文件 (.fbx/.png/.wav)
    │
    ▼
UFactory::Import()
    │
    ├── DetectFileType()  ← 根据扩展名匹配 Factory
    │
    ▼
    ├── UMeshFactory (assimp)
    │    ├── ReadFile() → aiScene
    │    ├── ConvertToEngine() → FStaticMeshPayload
    │    ├── CreateAsset() → UStaticMesh
    │    └── PostEditChange() 刷新编辑器
    │
    ├── UTextureFactory (stb_image)
    │    ├── Decode() → RGBA pixel data
    │    ├── UploadToGPU() → RHI Texture
    │    └── CreateAsset() → UTexture2D
    │
    └── UAudioFactory (dr_wav)
         ├── Decode() → PCM float samples
         └── CreateAsset() → USoundWave
```

支持的文件格式：

| 类别 | 格式 | 库 |
|------|------|------|
| 模型 | FBX, OBJ, glTF, DAE | assimp |
| 贴图 | PNG, JPG, BMP, TGA | stb_image.h |
| 音频 | WAV, MP3, FLAC, OGG | dr_libs (dr_wav, dr_mp3, dr_flac) |

Factory 注册机制：`UFactory` 子类通过 `FRegisteredFactoryList` 自动注册，编辑器根据文件扩展名自动匹配合适的 Factory。

---

## 6. 验收标准

| 阶段 | 验收标准                                                          |
| ---- | ----------------------------------------------------------------- |
| 0    | `cmake --build .` 成功，通过 RHI(OpenGL) 绘制三角形到窗口         |
| 1    | 用声明式语法画出包含按钮、文本的窗口，经 RHI → OpenGL 绘制，60fps |
| 2    | 所有 Widget 可用，能组装出完整的编辑器布局，文字渲染正常          |
| 3    | 声明一个含 5 个属性的类，反射遍历出全部属性名和类型               |
| 4 | 序列化读写一致 + FBX/PNG/WAV 导入后生成资产，内容浏览器可见 |
| 5    | 启动后看到类似 UE 的编辑器界面，面板可拖拽重组，布局可保存        |
| 6    | 同一套 Slate UI 在 OpenGL / DX12 / Vulkan 下渲染结果一致          |

---

## 7. 风险与应对

| 风险 | 影响 | 应对 |
|------|------|------|
| C++ 声明式宏实现复杂 | 阶段1延期 | 先实现函数式 API，再包装宏语法糖 |
| RHI 抽象接口设计不合理 | 后期扩展难 | 参考 UE RHI + 现代图形 API 共性提炼接口 |
| 反射系统代码生成 | 阶段3延期 | 初期全部手写注册，不依赖代码生成器 |
| 导入格式解析复杂（FBX 网格/材质/骨骼） | 阶段4延期 | assimp 已处理大部分格式差异，先做静态网格，骨骼动画后续 |
| 窗口管理系统复杂 | 阶段5延期 | 第一阶段用固定布局，停靠系统后续迭代 |
| DX12/Vulkan 后端工作量大 | 阶段6延期 | 先只做 OpenGL 验证架构，后端按需追加 |
| 范围蔓延 | 项目失控 | 严格按阶段划分，完成一个再进入下一个 |

---

## 8. 开发约定

- **从下往上构建**：Core → RHI → SlateCore → Slate → CoreUObject → Serialization → Editor
- **RHI 先行**：所有渲染代码必须通过 RHI 接口，不允许直接调用 OpenGL/DX12/Vulkan API
- **每阶段必须有可运行的验证程序**
- **尽量零第三方依赖**：所有容器、智能指针、字符串自实现
- **编码风格**：仿 UE 命名规范（`F` 前缀 = 结构体/非GC类，`U` 前缀 = UObject，`S` 前缀 = Slate Widget，`I` 前缀 = 接口）
- **中文注释**，代码中标识符保持英文

---

*本文档随项目推进持续更新。*
