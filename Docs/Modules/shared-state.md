# 共享状态层模块开发者文档

更新日期：2026-06-07

## 职责

共享状态层负责多个本地模式都能复用的 GameState / PlayerState 基础数据。它是“模式规则”和“Pawn 表现”之间的中间层。

主要目录：

```text
Source/BattleBlaster/Shared/State/
```

## 关键类

| 类 | 职责 |
| --- | --- |
| `ATankGameState` | 本地玩法 GameState 基类 |
| `ATankPlayerState` | SlotId、TeamId、KDA、仇人队列、攻击者清理 Timer 等共享玩家状态 |

模式专属状态类位于各自模式目录：

| 模式 | GameState / PlayerState |
| --- | --- |
| FreeForAll | `ATankBattleGameState`, `ATankBattlePlayerState` |
| TeamBattle | `ATeamBattleGameState`, `ATeamBattlePlayerState` |
| MOBA | `ATankMOBAGameState`, `ATankMOBAPlayerState` |
| Stage | `ATankStageGameState`, `ATankStagePlayerState` |
| Network | `ANetworkGameStateBase`, `ANetworkPlayerStateBase` 及子类 |

## 当前设计要点

- `SlotId` 负责玩家槽位。
- `TeamId` 负责阵营和敌我判断。
- `LocalPlayerIndex` 不应该再承担玩法身份语义。
- 仇人队列用于击杀 / 助攻归因：玩家受到 Tank 伤害时记录攻击者和时间，死亡时结算最近有效攻击者。

## 仇人队列规则

当前建议规则：

```text
受到 Tank 伤害
-> 记录攻击者和时间
-> 定时清理过期记录
死亡
-> 只结算有效时间窗口内的记录
-> 队尾为击杀者
-> 其他为助攻者
-> 没有有效攻击者时按自杀或环境死亡处理
```

这个设计可以覆盖：

- NPC 最终击杀时，最近攻击玩家仍可获得击杀。
- 多玩家助攻。
- 最终有效攻击者获得击杀。
- 环境死亡或无攻击者时扣分或按模式规则处理。

## 当前风险

- 不同模式对“无攻击者死亡”的处理可能不同，不能全部写死在共享 PlayerState。
- 网络模式需要 replicated PlayerState，不能直接复用所有本地 PlayerState 行为。
- UI 不应该自己推导 KDA 规则，应读取 PlayerState 已计算好的数据。

## 推荐优化

短期：

- 保持 `ATankPlayerState` 只存通用字段和通用击杀归因能力。
- 模式特有分数、胜负、淘汰状态放模式专属 PlayerState / GameState。

中期：

- 给击杀归因抽一个小结构：

```text
FDamageCreditEntry
├── Attacker
├── DamageTime
├── DamageType
└── OptionalDamageAmount
```

- GameMode 结算死亡时从 PlayerState 取结果，但最终是否加分、扣分、淘汰由模式自己决定。

## Editor 注意

- 修改 PlayerState 类后，需要检查对应 GameMode 蓝图的 PlayerStateClass。
- 网络模式不要把本地模式 PlayerState 蓝图误挂到网络 GameMode 上。
