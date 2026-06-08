# BattleBlaster 模块文档索引

更新日期：2026-06-07

本目录用于补充 `Docs/02-developer-guide.md`。总开发者文档负责解释全局架构；这里的每个文件负责一个模块，说明“这个模块管什么、改功能该去哪、后续怎么优化”。

## 模块分层

```text
Source/BattleBlaster/
├── Core/       全局配置、存档、会话、跨关卡状态
├── Shared/     多个模式共用的战斗、Pawn、AI、Buff、UI、地图物件
└── Modes/      具体玩法模式和模式专属 UI
```

## 文档清单

| 模块 | 文档 | 主要代码目录 |
| --- | --- | --- |
| Core / 存档 / 会话 | `core.md` | `Source/BattleBlaster/Core` |
| 共享 Pawn 与 Controller | `shared-pawns-and-controllers.md` | `Source/BattleBlaster/Shared/Pawns`, `Shared/Controllers` |
| 共享战斗层 | `shared-combat.md` | `Source/BattleBlaster/Shared/Combat` |
| 共享 AI | `shared-ai.md` | `Source/BattleBlaster/Shared/AI` |
| Buff 系统 | `shared-buffs.md` | `Source/BattleBlaster/Shared/Buffs` |
| 共享状态层 | `shared-state.md` | `Source/BattleBlaster/Shared/State` |
| 地图交互物 | `shared-world.md` | `Source/BattleBlaster/Shared/World` |
| 共享 UI | `shared-ui.md` | `Source/BattleBlaster/Shared/UI` |
| 主菜单 | `main-menu.md` | `Source/BattleBlaster/Modes/MainMenu` |
| 本地自由死斗 | `free-for-all.md` | `Source/BattleBlaster/Modes/FreeForAll` |
| 本地团队死斗 | `team-battle.md` | `Source/BattleBlaster/Modes/TeamBattle` |
| 本地 MOBA | `moba.md` | `Source/BattleBlaster/Modes/MOBA` |
| 单人闯关 / PVE | `stage.md` | `Source/BattleBlaster/Modes/Stage` |
| 网络模式 | `network.md` | `Source/BattleBlaster/Modes/Network`, `Core/Networking` |
| Defense 与 Test | `defense-and-test.md` | `Source/BattleBlaster/Modes/Defense`, `Modes/Test` |

## 当前最建议的简化路线

1. 先把全局工具收口：Primary UI Controller、PlayerStart 查找、地图配置、模式参数读取。
2. 再抽本地模式公共流程：Spawn、Respawn、Invincibility、GameOver，不要一开始就硬做大基类。
3. 然后拆大类：`AAIBotPlayerController`、`ATank`、`UBattleBlasterGameInstance`。
4. 最后处理命名和反射类重命名，例如 `Muti` 改 `Multi`。这类改动需要 Core Redirects，风险比普通代码重构高。

## 文档维护约定

- 改某个模块的玩法规则时，同步更新对应模块文档。
- 新增网络玩法模式时，同时更新 `network.md`、`Docs/06-network-mode-developer-guide.md` 和 `Docs/network-module-implementation-plan.md`。
- 新增优化项时，同步更新 `Docs/05-code-optimization-todo.md`。
