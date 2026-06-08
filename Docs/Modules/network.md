# 网络模式模块开发者文档

更新日期：2026-06-07

更详细的网络架构见：

- `Docs/06-network-mode-developer-guide.md`
- `Docs/network-module-implementation-plan.md`

本文件只作为模块入口索引。

## 职责

网络模式模块负责 LAN Listen Server、后续 Dedicated Server、网络玩法模式、网络 UI、网络地图选择和 AI 填充。

主要目录：

```text
Source/BattleBlaster/Modes/Network/
Source/BattleBlaster/Modes/Network/UI/
Source/BattleBlaster/Modes/Network/UI/Menu/
Source/BattleBlaster/Core/Networking/
```

## 关键类

| 类 | 职责 |
| --- | --- |
| `ANetworkGameModeBase` | 网络玩法基类，负责服务器权威生成玩家、AI 填充、复活基础流程 |
| `ANetworkGameStateBase` | 网络共享状态基类 |
| `ANetworkPlayerStateBase` | 网络玩家状态基类，分数、Slot、Team 等同步数据 |
| `ANetworkPlayerControllerBase` | 网络玩家控制器，输入映射、网络 HUD、暂停 UI |
| `ANetworkDeathmatchGameMode` | 网络多人死斗 |
| `ANetworkTeamDeathmatchGameMode` | 网络团队死斗 |
| `ANetworkMOBAGameMode` | 网络 MOBA |
| `ANetworkTeamMOBAGameMode` | 网络团队 MOBA |
| `UCppShowScoresWidget` | 网络死斗分数 UI |
| `UNetworkTeamScoresWidget` | 网络团队分数 UI |
| `UNetworkMOBAStateWidget` | 网络 MOBA 状态 UI |
| `UNetworkMapSelectWidget` | 网络地图选择 UI |
| `ULANHostSettingsWidget` | LAN Host 设置 |
| `ULANJoinWidget` | LAN Join |

## 当前设计要点

- LAN 和 Dedicated Server 是连接方式差异，不应该写两套玩法规则。
- 网络玩法模式继承 `ANetworkGameModeBase`。
- Dedicated Server 没有本地玩家、没有 Viewport、没有 UMG，因此服务器规则不能依赖 UI。
- UI 读取 replicated `GameState` / `PlayerState`，不能从服务器-only GameMode 读数据。
- 网络地图选择使用“当前地图卡片 + Change Map + 模式专属地图列表”的结构。

## 当前风险

- 目前网络模块仍处于扩展期，死亡、复活、AI、地图选择、不同网络玩法还需要持续验收。
- Listen Server 和 PIE 测试容易被本地分屏 LocalPlayer 干扰，Host / Join 前必须清理 LocalPlayer。
- 大量网络表现依赖蓝图子类挂载 Widget Class，C++ 需要保持默认可测试 UI，但最终表现交给 UMG。

## 推荐优化

近期优先级：

1. 固化网络地图选择配置，避免手填地图名。
2. 完善网络 TeamDeathmatch / MOBA / TeamMOBA 的规则验收。
3. 给网络 AI 填充补完整测试清单。
4. 网络 UI 继续保持 C++ 默认实现 + 蓝图可覆盖。
5. 等 Listen Server 稳定后再推进 Dedicated Server。

## Editor 注意

- 每个网络 GameMode 建议有自己的 BP 子类，方便挂 UI、地图、规则参数。
- `BP_NetworkPlayerControllerBase` 应挂网络模式所需的输入映射和暂停菜单。
- 网络 Host 测试时，不要用本地分屏 PIE 去模拟独立进程；需要多进程或独立客户端时使用对应 Play 模式。
- 同机多窗口只适合烟测连接流程；移动、开火、物理同步和卡顿判断应以两台真实设备 LAN 测试为准。
