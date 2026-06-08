# Core / 存档 / 会话模块开发者文档

更新日期：2026-06-07

## 职责

Core 模块负责跨关卡、跨模式、跨 UI 的全局数据和基础设施。它不应该承载具体玩法规则。

主要目录：

```text
Source/BattleBlaster/Core/
├── BattleBlasterGameInstance.h/.cpp
├── BattleBlasterCollisionChannels.h
├── Networking/
└── Persistence/
```

## 关键类

| 类 | 职责 |
| --- | --- |
| `UBattleBlasterGameInstance` | 本地玩家数量、Tank 选择、关卡进度、历史记录、输入设备映射、网络菜单设置等跨关卡数据 |
| `UBattleBlasterSessionSubsystem` | Host / Join / LocalPlayer 清理等网络会话入口 |
| `FBattleBlasterNetworkTypes` | 网络模式设置结构、网络连接类型、网络玩法类型 |
| `UBattleBlasterSaveGame` | 单人闯关存档 |
| `UBattleBlasterHistorySaveGame` | 战绩历史存档 |
| `BattleBlasterCollisionChannels.h` | 项目自定义碰撞通道常量 |

## 数据归属

- 本地菜单设置可以暂存在 `GameInstance`，例如玩家数量、AI 数量、地图选择、Tank 选择。
- 对局内需要同步的数据不能只放 `GameInstance`。网络模式必须放到 `GameMode`、`GameState`、`PlayerState`、Pawn 或对应 Actor 上。
- 存档数据只应该在切关卡、通关、失败、返回主菜单等明确时机写入，不建议在战斗 Tick 中频繁写存档。

## 开发入口

- 修改本地多人玩家数量或 Tank 选择：看 `UBattleBlasterGameInstance`。
- 修改网络 Host / Join：看 `UBattleBlasterSessionSubsystem`。
- 新增网络 Host 设置字段：先改 `FBattleBlasterNetworkTypes`，再改网络菜单和 `ANetworkGameModeBase`。
- 修改存档字段：改 `Persistence` 下 SaveGame 类，并注意旧存档兼容。

## 当前风险

- `UBattleBlasterGameInstance` 职责偏多，是当前最大的全局状态类。
- 本地分屏和网络模式都需要清理 LocalPlayer，但语义不同：本地模式需要多个 LocalPlayer，网络 Host / Join 前通常只保留一个 LocalPlayer。
- 网络模式不要依赖 `GameInstance` 数据自动同步；它只存在于本机。

## 推荐优化

优先拆职责，不急着改行为：

```text
GameInstance
├── LocalMatchSettings
├── CampaignProgress
├── MatchHistory
├── InputDeviceMapping
└── NetworkSessionSettings
```

可以先用普通 helper 类或结构体收口，等稳定后再升级为 `UGameInstanceSubsystem`。

## Editor 注意

- 修改 GameInstance 类后，需要确认 Project Settings 中的 GameInstance Class 仍然指向正确蓝图或 C++ 类。
- 修改碰撞通道后，要检查 Project Settings -> Collision，以及相关蓝图碰撞预设。
- 修改网络菜单设置结构后，需要重新打开相关 UMG 蓝图，检查暴露属性是否还在。
