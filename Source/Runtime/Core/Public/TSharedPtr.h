// YaoSharedPtr — 羲引擎引用计数智能指针（Yao 层 / 羲爻）
// 非侵入式引用计数，包含 YaoSharedPtr / YaoSharedRef / YaoWeakPtr
#pragma once

#include "CoreTypes.h"
#include <utility>      // std::move, std::forward
#include <type_traits>  // std::enable_if_t, std::is_base_of_v

namespace Xi::Yao
{

/// @brief 引用计数控制块基类（内部使用，不直接暴露）
struct YaoRefControllerBase
{
    int32 m_SharedRefCount = 0;  // 共享引用计数
    int32 m_WeakRefCount   = 0;  // 弱引用计数

    virtual ~YaoRefControllerBase() = default;
    virtual void DestroyObject() = 0;  // 销毁被引用对象
};

/// @brief 引用计数控制块实现（内嵌对象实例）
template<typename T>
struct YaoRefController : public YaoRefControllerBase
{
    T m_Instance;  // 对象实例直接存储于控制块中

    template<typename... Args>
    YaoRefController(Args&&... args)
        : m_Instance(std::forward<Args>(args)...)
    {
    }

    virtual void DestroyObject() override
    {
        m_Instance.~T();  // 手动析构（控制块仍存活以服务弱引用）
    }
};

// ── 前向声明 ────────────────────────────────────
template<typename T> class YaoSharedPtr;
template<typename T> class YaoWeakPtr;

/// @brief 创建 YaoSharedPtr 的工厂函数（仿 UE MakeShareable）
template<typename T, typename... Args>
YaoSharedPtr<T> YaoMakeShared(Args&&... args)
{
    auto* Controller = new YaoRefController<T>(std::forward<Args>(args)...);
    Controller->m_SharedRefCount = 1;

    YaoSharedPtr<T> Result;
    Result.m_Object        = &Controller->m_Instance;
    Result.m_RefController = Controller;
    return Result;
}

/// @brief 共享指针（仿 UE TSharedPtr）
template<typename T>
class YaoSharedPtr
{
    friend class YaoWeakPtr<T>;
    template<typename U> friend class YaoSharedPtr;  // 允许派生类到基类的转换
    template<typename U, typename... Args>
    friend YaoSharedPtr<U> YaoMakeShared(Args&&...);

public:
    // ── 构造 ─────────────────────────────────────
    /// @brief 默认构造（空指针）
    YaoSharedPtr() = default;

    /// @brief 从 nullptr 构造（空指针）
    YaoSharedPtr(std::nullptr_t) : YaoSharedPtr() {}

    /// @brief 从派生类 YaoSharedPtr 隐式转换（如 YaoSharedPtr<YaoOpenGLBuffer> → YaoSharedPtr<YaoRHIBuffer>）
    template<typename Derived, typename = std::enable_if_t<std::is_base_of_v<T, Derived>>>
    YaoSharedPtr(const YaoSharedPtr<Derived>& Other)
        : m_Object(Other.m_Object), m_RefController(Other.m_RefController)
    {
        AddSharedRef();
    }

    /// @brief 从派生类 YaoSharedPtr 移动转换
    template<typename Derived, typename = std::enable_if_t<std::is_base_of_v<T, Derived>>>
    YaoSharedPtr(YaoSharedPtr<Derived>&& Other) noexcept
        : m_Object(Other.m_Object), m_RefController(Other.m_RefController)
    {
        Other.m_Object        = nullptr;
        Other.m_RefController = nullptr;
    }

    /// @brief 拷贝构造（增加引用计数）
    YaoSharedPtr(const YaoSharedPtr& Other)
        : m_Object(Other.m_Object), m_RefController(Other.m_RefController)
    {
        AddSharedRef();
    }

    /// @brief 移动构造（转移所有权）
    YaoSharedPtr(YaoSharedPtr&& Other) noexcept
        : m_Object(Other.m_Object), m_RefController(Other.m_RefController)
    {
        Other.m_Object        = nullptr;
        Other.m_RefController = nullptr;
    }

    /// @brief 析构（减少引用计数，归零时销毁）
    ~YaoSharedPtr()
    {
        ReleaseSharedRef();
    }

    // ── 赋值 ─────────────────────────────────────
    YaoSharedPtr& operator=(const YaoSharedPtr& Other)
    {
        if (this != &Other)
        {
            ReleaseSharedRef();
            m_Object        = Other.m_Object;
            m_RefController = Other.m_RefController;
            AddSharedRef();
        }
        return *this;
    }

    YaoSharedPtr& operator=(YaoSharedPtr&& Other) noexcept
    {
        if (this != &Other)
        {
            ReleaseSharedRef();
            m_Object        = Other.m_Object;
            m_RefController = Other.m_RefController;
            Other.m_Object        = nullptr;
            Other.m_RefController = nullptr;
        }
        return *this;
    }

    // ── 指针操作 ─────────────────────────────────
    T*  Get()        const { return m_Object; }
    T*  operator->() const { xiCheck(IsValid()); return m_Object; }
    T&  operator*()  const { xiCheck(IsValid()); return *m_Object; }

    /// @brief 是否有效（非空）
    bool IsValid()   const { return m_Object != nullptr; }
    explicit operator bool() const { return IsValid(); }

    /// @brief 当前共享引用计数
    int32 GetSharedReferenceCount() const
    {
        return m_RefController ? m_RefController->m_SharedRefCount : 0;
    }

    /// @brief 是否唯一持有对象
    bool IsUnique() const { return GetSharedReferenceCount() == 1; }

    /// @brief 重置为空
    void Reset()
    {
        ReleaseSharedRef();
        m_Object        = nullptr;
        m_RefController = nullptr;
    }

private:
    /// @brief 增加共享引用计数
    void AddSharedRef()
    {
        if (m_RefController) ++m_RefController->m_SharedRefCount;
    }

    /// @brief 减少共享引用计数，归零时销毁对象和控制块
    void ReleaseSharedRef()
    {
        if (m_RefController)
        {
            --m_RefController->m_SharedRefCount;
            if (m_RefController->m_SharedRefCount == 0)
            {
                m_RefController->DestroyObject();
                // 没有弱引用时连控制块一起释放
                if (m_RefController->m_WeakRefCount == 0)
                {
                    delete m_RefController;
                }
            }
        }
    }

    // ── 成员变量 ─────────────────────────────────
    T*                      m_Object        = nullptr;  // 被引用的对象指针
    YaoRefControllerBase*   m_RefController = nullptr;  // 引用计数控制块
};

/// @brief 共享引用 — 非空共享指针（仿 UE TSharedRef）
template<typename T>
class YaoSharedRef
{
public:
    /// @brief 从可变参数原地构造对象
    template<typename... Args>
    static YaoSharedRef<T> Create(Args&&... args)
    {
        YaoSharedRef<T> Result;
        auto* Controller = new YaoRefController<T>(std::forward<Args>(args)...);
        Controller->m_SharedRefCount = 1;
        Result.m_Object        = &Controller->m_Instance;
        Result.m_RefController = Controller;
        return Result;
    }

    YaoSharedRef(const YaoSharedRef& Other)
        : m_Object(Other.m_Object), m_RefController(Other.m_RefController)
    {
        AddSharedRef();
    }

    YaoSharedRef(YaoSharedRef&& Other) noexcept
        : m_Object(Other.m_Object), m_RefController(Other.m_RefController)
    {
        Other.m_Object        = nullptr;
        Other.m_RefController = nullptr;
    }

    ~YaoSharedRef() { ReleaseSharedRef(); }

    YaoSharedRef& operator=(const YaoSharedRef& Other)
    {
        if (this != &Other)
        {
            ReleaseSharedRef();
            m_Object        = Other.m_Object;
            m_RefController = Other.m_RefController;
            AddSharedRef();
        }
        return *this;
    }

    T*  Get()        const { return m_Object; }
    T*  operator->() const { return m_Object; }
    T&  operator*()  const { return *m_Object; }

    int32 GetSharedReferenceCount() const
    {
        return m_RefController ? m_RefController->m_SharedRefCount : 0;
    }

private:
    YaoSharedRef() = default;

    void AddSharedRef()
    {
        if (m_RefController) ++m_RefController->m_SharedRefCount;
    }

    void ReleaseSharedRef()
    {
        if (m_RefController)
        {
            --m_RefController->m_SharedRefCount;
            if (m_RefController->m_SharedRefCount == 0)
            {
                m_RefController->DestroyObject();
                if (m_RefController->m_WeakRefCount == 0)
                {
                    delete m_RefController;
                }
            }
        }
    }

    T*                      m_Object        = nullptr;
    YaoRefControllerBase*   m_RefController = nullptr;
};

/// @brief 弱指针 — 不增加共享引用的观察者（仿 UE TWeakPtr）
template<typename T>
class YaoWeakPtr
{
public:
    YaoWeakPtr() = default;

    YaoWeakPtr(const YaoSharedPtr<T>& SharedPtr)
        : m_Object(SharedPtr.m_Object), m_RefController(SharedPtr.m_RefController)
    {
        AddWeakRef();
    }

    YaoWeakPtr(const YaoWeakPtr& Other)
        : m_Object(Other.m_Object), m_RefController(Other.m_RefController)
    {
        AddWeakRef();
    }

    ~YaoWeakPtr() { ReleaseWeakRef(); }

    YaoWeakPtr& operator=(const YaoWeakPtr& Other)
    {
        if (this != &Other)
        {
            ReleaseWeakRef();
            m_Object        = Other.m_Object;
            m_RefController = Other.m_RefController;
            AddWeakRef();
        }
        return *this;
    }

    /// @brief 提升为共享指针（若对象仍存活）
    YaoSharedPtr<T> Pin() const
    {
        YaoSharedPtr<T> Result;
        if (m_RefController && m_RefController->m_SharedRefCount > 0)
        {
            Result.m_Object        = m_Object;
            Result.m_RefController = m_RefController;
            ++m_RefController->m_SharedRefCount;
        }
        return Result;
    }

    /// @brief 对象是否已销毁
    bool IsExpired() const
    {
        return !m_RefController || m_RefController->m_SharedRefCount == 0;
    }

private:
    void AddWeakRef()
    {
        if (m_RefController) ++m_RefController->m_WeakRefCount;
    }

    void ReleaseWeakRef()
    {
        if (m_RefController)
        {
            --m_RefController->m_WeakRefCount;
            if (m_RefController->m_SharedRefCount == 0 && m_RefController->m_WeakRefCount == 0)
            {
                delete m_RefController;
            }
        }
    }

    T*                      m_Object        = nullptr;
    YaoRefControllerBase*   m_RefController = nullptr;
};

} // namespace Xi::Yao
