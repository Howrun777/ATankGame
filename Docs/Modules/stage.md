# 单人闯关 / PVE 模块开发者文档

更新日期：2026-06-07

## 职责

Stage 模式负责单人闯关、关卡进度、难度、Tower 敌人、通关和失败结算。

主要目录：

```text
Source/BattleBlaster/Modes/Stage/
Source/BattleBlaster/Modes/Stage/UI/
```

## 关键类

| 类 | 职责 |
| --- | --- |
| `ATankStageGameMode` | 单人闯关规则、Tower 目标、胜负、复活、跨关卡携带状态 |
| `ATankStageGameState` | Stage 共享状态 |
| `ATankStagePlayerState` | Stage 玩家状态 |
| `UTankStageStartWidget` | Stage 开始界面 |
| `UTankStageOverWidget` | Stage 失败 / 结束界面 |
| `UPassWidget` | 通关界面 |
| `ATower` | Stage 中常见的 NPC 塔楼敌人，代码在 Shared/Pawns/NPC |

## 核心流程

```text
主菜单选择单人 / 难度 / Tank
-> Stage GameMode 读取存档和 GameInstance
-> 生成玩家
-> 应用关卡难度到 Tower
-> 玩家击败所有 Tower 或达到通关条件
-> 保存携带状态
-> 进入下一关或返回菜单
```

## 当前设计要点

- Stage 是 PVE，不是本地多人死斗。
- `ATower` 是共享 NPC，可被 Stage、AI 目标系统、网络测试等复用。
- Stage 有跨关卡携带状态，不能简单套用多人模式的 Respawn / GameOver 流程。

## 当前风险

- `ATankStageGameMode` 较大，关卡进度、存档、复活、胜负、Tower 难度都在一个类里。
- 关卡名、下一关、菜单返回目标等仍有硬编码倾向。
- UI 和存档流程耦合，后续新增关卡或难度时容易漏改。

## 推荐优化

优先拆数据，不急着拆 GameMode：

```text
FStageLevelConfig
├── LevelName
├── NextLevelName
├── Difficulty
├── MaxDeaths
├── TowerDifficultyScale
└── Rewards
```

然后把存档和关卡推进放到 Campaign helper：

```text
CampaignProgress
├── LoadStageProgress
├── SaveStageProgress
├── ApplyCarryState
└── BuildNextLevelTravel
```

## 验收清单

- 死亡次数限制正确。
- 通关后携带状态保存正确。
- 下一关加载正确。
- Tower 难度随关卡或设置变化。
- 返回主菜单后 LocalPlayer 状态干净。
