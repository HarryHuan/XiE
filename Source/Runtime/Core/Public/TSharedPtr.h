// TSharedPtr.h — 羲引擎引用计数智能指针
// 仿 UE TSharedPtr / TSharedRef / TWeakPtr，非侵入式引用计数
#pragma once

#include "CoreTypes.h"
#include <utility>     // std::move, std::forward
#include <type_traits> // std::enable_if_t, std::is_base_of_v

/// @brief 引用计数控制块（内部使用）
struct FReferenceControllerBase
{
    int32 SharedRefCount = 0;  // 共享引用计数
    int32 WeakRefCount   = 0;  // 弱引用计数

    virtual ~FReferenceControllerBase() = default;
    virtual void DestroyObject() = 0;   // 销毁被引用对象
};

/// @brief 控制块实现（内嵌对象实例）
template<typename T>
struct TReferenceController : public FReferenceControllerBase
{
    T Instance;  // 对象实例直接存储于控制块中

    template<typename... Args>
    TReferenceController(Args&&... args)
        : Instance(std::forward<Args>(args)...)
    {
    }

    virtual void DestroyObject() override
    {
        Instance.~T();  // 手动析构（控制块仍存活以服务弱引用）
    }
};

// ── 前向声明 ────────────────────────────────────
template<typename T> class TSharedPtr;
template<typename T> class TWeakPtr;

/// @brief 创建 TSharedPtr 的工厂函数（仿 UE MakeShareable）
template<typename T, typename... Args>
TSharedPtr<T> MakeShared(Args&&... args)
{
    auto* Controller = new TReferenceController<T>(std::forward<Args>(args)...);
    Controller->SharedRefCount = 1;

    TSharedPtr<T> Result;
    Result.Object        = &Controller->Instance;
    Result.RefController = Controller;
    return Result;
}

/// @brief 共享指针（仿 UE TSharedPtr）
template<typename T>
class TSharedPtr
{
    friend class TWeakPtr<T>;
    template<typename U> friend class TSharedPtr;  // 允许派生类到基类的转换访问私有成员
    template<typename U, typename... Args>
    friend TSharedPtr<U> MakeShared(Args&&...);

public:
    // ── 构造 ─────────────────────────────────────
    TSharedPtr() = default;

    /// @brief 从 nullptr 构造（空指针）
    TSharedPtr(std::nullptr_t) : TSharedPtr() {}

    /// @brief 从派生类 TSharedPtr 隐式转换（如 TSharedPtr<FOpenGLBuffer> → TSharedPtr<IRHIBuffer>）
    template<typename Derived, typename = std::enable_if_t<std::is_base_of_v<T, Derived>>>
    TSharedPtr(const TSharedPtr<Derived>& Other)
        : Object(Other.Object), RefController(Other.RefController)
    {
        AddSharedRef();
    }

    /// @brief 从派生类 TSharedPtr 移动转换
    template<typename Derived, typename = std::enable_if_t<std::is_base_of_v<T, Derived>>>
    TSharedPtr(TSharedPtr<Derived>&& Other) noexcept
        : Object(Other.Object), RefController(Other.RefController)
    {
        Other.Object        = nullptr;
        Other.RefController = nullptr;
    }

    TSharedPtr(const TSharedPtr& Other)
        : Object(Other.Object), RefController(Other.RefController)
    {
        AddSharedRef();
    }

    TSharedPtr(TSharedPtr&& Other) noexcept
        : Object(Other.Object), RefController(Other.RefController)
    {
        Other.Object        = nullptr;
        Other.RefController = nullptr;
    }

    ~TSharedPtr()
    {
        ReleaseSharedRef();
    }

    // ── 赋值 ─────────────────────────────────────
    TSharedPtr& operator=(const TSharedPtr& Other)
    {
        if (this != &Other)
        {
            ReleaseSharedRef();
            Object        = Other.Object;
            RefController = Other.RefController;
            AddSharedRef();
        }
        return *this;
    }

    TSharedPtr& operator=(TSharedPtr&& Other) noexcept
    {
        if (this != &Other)
        {
            ReleaseSharedRef();
            Object        = Other.Object;
            RefController = Other.RefController;
            Other.Object        = nullptr;
            Other.RefController = nullptr;
        }
        return *this;
    }

    // ── 指针操作 ─────────────────────────────────
    T*  Get()        const { return Object; }
    T*  operator->() const { xiCheck(IsValid()); return Object; }
    T&  operator*()  const { xiCheck(IsValid()); return *Object; }

    bool IsValid()   const { return Object != nullptr; }
    explicit operator bool() const { return IsValid(); }

    int32 GetSharedReferenceCount() const
    {
        return RefController ? RefController->SharedRefCount : 0;
    }

    bool IsUnique() const { return GetSharedReferenceCount() == 1; }

    void Reset()
    {
        ReleaseSharedRef();
        Object        = nullptr;
        RefController = nullptr;
    }

private:
    void AddSharedRef()
    {
        if (RefController) ++RefController->SharedRefCount;
    }

    void ReleaseSharedRef()
    {
        if (RefController)
        {
            --RefController->SharedRefCount;
            if (RefController->SharedRefCount == 0)
            {
                RefController->DestroyObject();
                // 没有弱引用时连控制块一起释放
                if (RefController->WeakRefCount == 0)
                {
                    delete RefController;
                }
            }
        }
    }

    T*                        Object        = nullptr;
    FReferenceControllerBase* RefController = nullptr;
};

/// @brief 共享引用 — 非空共享指针（仿 UE TSharedRef）
template<typename T>
class TSharedRef
{
public:
    /// @brief 从可变参数原地构造对象
    template<typename... Args>
    static TSharedRef<T> Create(Args&&... args)
    {
        TSharedRef<T> Result;
        auto* Controller = new TReferenceController<T>(std::forward<Args>(args)...);
        Controller->SharedRefCount = 1;
        Result.Object        = &Controller->Instance;
        Result.RefController = Controller;
        return Result;
    }

    TSharedRef(const TSharedRef& Other)
        : Object(Other.Object), RefController(Other.RefController)
    {
        AddSharedRef();
    }

    TSharedRef(TSharedRef&& Other) noexcept
        : Object(Other.Object), RefController(Other.RefController)
    {
        Other.Object        = nullptr;
        Other.RefController = nullptr;
    }

    ~TSharedRef() { ReleaseSharedRef(); }

    TSharedRef& operator=(const TSharedRef& Other)
    {
        if (this != &Other)
        {
            ReleaseSharedRef();
            Object        = Other.Object;
            RefController = Other.RefController;
            AddSharedRef();
        }
        return *this;
    }

    T*  Get()        const { return Object; }
    T*  operator->() const { return Object; }
    T&  operator*()  const { return *Object; }

    int32 GetSharedReferenceCount() const
    {
        return RefController ? RefController->SharedRefCount : 0;
    }

private:
    TSharedRef() = default;

    void AddSharedRef()
    {
        if (RefController) ++RefController->SharedRefCount;
    }

    void ReleaseSharedRef()
    {
        if (RefController)
        {
            --RefController->SharedRefCount;
            if (RefController->SharedRefCount == 0)
            {
                RefController->DestroyObject();
                if (RefController->WeakRefCount == 0)
                {
                    delete RefController;
                }
            }
        }
    }

    T*                        Object        = nullptr;
    FReferenceControllerBase* RefController = nullptr;
};

/// @brief 弱指针 — 不增加共享引用的观察者（仿 UE TWeakPtr）
template<typename T>
class TWeakPtr
{
public:
    TWeakPtr() = default;

    TWeakPtr(const TSharedPtr<T>& SharedPtr)
        : Object(SharedPtr.Object), RefController(SharedPtr.RefController)
    {
        AddWeakRef();
    }

    TWeakPtr(const TWeakPtr& Other)
        : Object(Other.Object), RefController(Other.RefController)
    {
        AddWeakRef();
    }

    ~TWeakPtr() { ReleaseWeakRef(); }

    TWeakPtr& operator=(const TWeakPtr& Other)
    {
        if (this != &Other)
        {
            ReleaseWeakRef();
            Object        = Other.Object;
            RefController = Other.RefController;
            AddWeakRef();
        }
        return *this;
    }

    /// @brief 提升为共享指针（若对象仍存活）
    TSharedPtr<T> Pin() const
    {
        TSharedPtr<T> Result;
        if (RefController && RefController->SharedRefCount > 0)
        {
            Result.Object        = Object;
            Result.RefController = RefController;
            ++RefController->SharedRefCount;  // Result 析构时会减掉
        }
        return Result;
    }

    bool IsExpired() const
    {
        return !RefController || RefController->SharedRefCount == 0;
    }

private:
    void AddWeakRef()
    {
        if (RefController) ++RefController->WeakRefCount;
    }

    void ReleaseWeakRef()
    {
        if (RefController)
        {
            --RefController->WeakRefCount;
            if (RefController->SharedRefCount == 0 && RefController->WeakRefCount == 0)
            {
                delete RefController;
            }
        }
    }

    T*                        Object        = nullptr;
    FReferenceControllerBase* RefController = nullptr;
};
