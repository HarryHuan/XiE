// DeclarativeSyntax.h — Slate 声明式语法宏（Yan 层 / 羲砚）
// 提供 SNew / SAssignNew 宏，简化控件创建
#pragma once

#include "YaoSharedPtr.h"

using namespace Xi::Yao;

namespace Xi::Yan
{

// ── 控件创建宏 ──────────────────────────────────

/// @brief 创建 Slate 控件的共享指针（仿 UE SNew）
/// 用法：auto Box = SNew(YanVerticalBox);
#define SNew(WidgetType) \
    YaoMakeShared<WidgetType>()

/// @brief 创建控件并赋值给变量（仿 UE SAssignNew）
/// 用法：YaoSharedPtr<YanBorder> Border; SAssignNew(Border, YanBorder);
#define SAssignNew(Var, WidgetType) \
    Var = YaoMakeShared<WidgetType>()

} // namespace Xi::Yan
