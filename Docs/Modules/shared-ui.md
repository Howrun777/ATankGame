# 共享 UI 模块开发者文档

更新日期：2026-06-07

## 职责

共享 UI 模块负责多个模式都能复用的战斗内界面。

主要目录：

```text
Source/BattleBlaster/Shared/UI/
```

## 关键类

| 类 | 职责 |
| --- | --- |
| `UHUDWidget` | 玩家血量等 HUD 容器 |
| `UBulletsWidget` | 弹药显示 |
| `UKDAWidget` | 击杀 / 死亡 / 助攻显示 |
| `UScoresDisplayWidget` | 旧分数显示 Widget，网络模式已倾向使用新的 C++ UI |
| `UPauseMenuWidget` | 暂停菜单 |
| `UReturnToSpawnWidget` | 回出生点提示 |
| `UBuffListWidget` | Buff 列表 |
| `UScreenMessage` | 屏幕消息 |

## UI 数据归属

- 玩家个人 HUD：从 owning `PlayerController` / `PlayerState` / Pawn 读取。
- 全局结算 UI：应由 GameMode / GameState 准备结果，不建议 Widget 自己重新扫描场景并推导。
- 网络 UI：只能依赖客户端可见的 replicated `GameState` / `PlayerState` 和本地 `PlayerController`。

## 当前风险

- 本地模式里部分全局 UI 使用 Player 0 创建，这是合理的，但需要明确“这是全局 UI 主控”，不是忘记分屏兼容。
- 部分 Widget 仍有 `NativeTick` 或场景扫描逻辑，需要逐个判断是否必要。
- 网络模式和本地分屏模式 UI 生命周期不同，不建议强行共用同一个 PlayerController 蓝图。

## 推荐优化

短期建议新增或约定三个工具入口：

```text
GetPrimaryUIController(World)
GetOwningBattleController(Widget)
BuildGameOverViewModel(GameState)
```

中期建议把 Score / GameOver 这类 UI 改成 ViewModel 输入：

```text
GameMode / GameState
-> FScoreboardViewData
-> Widget::SetScoreboardData(...)
```

这样 Widget 只负责表现层，策划和美术后续可以用蓝图子类覆盖外观。

## Editor 注意

- 死亡黑屏倒计时 UI、暂停菜单、网络分数 UI 都应暴露 `TSubclassOf<UUserWidget>` 或可替换 Widget Class。
- 修改 PlayerController 蓝图后，检查 GameMode 里的 PlayerControllerClass。
- 网络模式 PlayerController 应单独有 BP 子类，避免和本地分屏输入映射互相污染。
