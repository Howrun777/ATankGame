# 本地自由死斗模块开发者文档

更新日期：2026-06-07

## 职责

自由死斗是本地多人基础玩法：所有玩家互为敌人，通过击杀获得分数，到达目标分数后结算。

主要目录：

```text
Source/BattleBlaster/Modes/FreeForAll/
Source/BattleBlaster/Modes/FreeForAll/UI/
```

## 关键类

| 类 | 职责 |
| --- | --- |
| `ABattleBlasterGameMode` | 本地自由死斗规则、玩家生成、复活、计分、结算 |
| `ATankBattleGameState` | 自由死斗共享状态 |
| `ATankBattlePlayerState` | 自由死斗玩家状态 |
| `UMutiBattleMenuWidget` | 自由死斗选 Tank / 人数 / AI 菜单 |
| `UMultiBattleGameOverWidget` | 自由死斗结算 UI |

## 核心流程

```text
菜单选择人数 / AI / Tank
-> OpenLevel 到战斗地图
-> GameMode 读取 GameInstance 设置
-> 根据 SlotId 找 PlayerStart
-> Spawn Tank 并 Possess
-> 击杀计分
-> 达到目标分数后 GameOver
```

## 当前设计要点

- 本模式没有队伍概念，所有有效 Tank 都可以互相攻击。
- 击杀归因依赖 `ATankPlayerState` 的仇人队列。
- 本地分屏下全局结算 UI 使用 Player 0 创建属于合理设计，但要保持注释清楚。

## 当前风险

- Spawn / Respawn / Invincibility / GameOver 与 TeamBattle 高度重复。
- `UMutiBattleMenuWidget` 和其他选人菜单仍有重复 UI 逻辑，但当前阶段已标记暂缓，不建议强抽继承。
- GameOver Widget 自己扫描 Actor / PlayerState 推导数据，长期建议改为 GameState 提供结果。

## 推荐优化

优先抽 FreeForAll 和 TeamBattle 的共同服务：

```text
LocalDeathmatchFlow
├── BuildLocalSlots
├── SpawnHumanPlayers
├── SpawnAIPlayers
├── RespawnTank
└── ShowGameOver
```

先让 FreeForAll 接入，测试稳定后再迁 TeamBattle。

## 验收清单

- 1/2/3/4 人本地分屏都能进入。
- 每个玩家能选择自己的 Tank。
- 键盘和手柄 1 控制第一个玩家。
- AI 补位正确。
- 达到目标分数后正确结算。
