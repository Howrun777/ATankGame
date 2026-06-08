# 本地 MOBA 模块开发者文档

更新日期：2026-06-07

## 职责

MOBA 模式负责核心塔、防御塔、阵营淘汰、玩家复活倒计时和 MOBA 专属 UI。

主要目录：

```text
Source/BattleBlaster/Modes/MOBA/
Source/BattleBlaster/Modes/MOBA/UI/
```

## 关键类

| 类 | 职责 |
| --- | --- |
| `ATankMOBAGameMode` | MOBA 规则、玩家生成、核心塔状态、淘汰、复活、结算 |
| `ATankMOBAGameState` | 核心塔、防御塔、时间等共享状态 |
| `ATankMOBAPlayerState` | MOBA 玩家状态 |
| `ATurret` | MOBA 防御塔，继承 `ADestructibleProp` |
| `ATurretProjectile` | MOBA 防御塔追踪弹 |
| `UMOBASetupWidget` | MOBA 选人和设置 |
| `UMOBATopStateUI` | MOBA 顶部状态 UI |
| `UDeathScreenWidget` | 死亡黑屏倒计时 UI |
| `UEliminatedScreenWidget` | 阵营淘汰 UI |
| `UMOBAGameOverWidget` | MOBA 结算 UI |

## 核心规则

玩家死亡时是否能复活，取决于该玩家阵营的核心塔状态：

- 玩家死亡时核心塔还在：玩家进入复活倒计时。
- 玩家死亡后，复活倒计时期间核心塔被推掉：这次仍然允许复活。
- 核心塔已经被推掉后，玩家再次死亡：该玩家所在阵营才彻底淘汰。
- 不能简单用“只剩一个核心塔”或“只剩一个玩家存活”判断游戏结束。
- 游戏结束条件应该是只剩一个未淘汰阵营。

## 当前设计要点

- `ATurret` 是 MOBA 防御塔，不是 `ATower`。
- 防御塔目标选择必须走阵营判断，避免自家塔打自己人。
- 复活倒计时 UI 已经可以作为所有模式的死亡黑屏 UI 基础，但应通过蓝图 Class 暴露，不要写死某个 WBP。

## 当前风险

- `ATankMOBAGameMode` 较大，而且含有 Tick / Timer / UI / 规则判断。
- MOBA 规则比 FreeForAll / TeamBattle 特殊，不适合最早接入公共 GameMode 基类。
- Turret 和 TurretProjectile 的网络同步、目标选择、伤害归因需要持续保持服务器权威。

## 推荐优化

优先把规则判断函数化：

```text
CanPlayerRespawn(PlayerState)
IsCampCoreAlive(TeamId)
IsCampEliminated(TeamId)
CheckGameOverByElimination()
NotifyTowerDestroyed(Turret)
```

后续可以把核心塔 / 防御塔状态整理成一个 `FMOBACampState`，由 GameState 提供给 UI。

## 验收清单

- 核心塔被推后，已进入倒计时的玩家仍可复活。
- 核心塔被推后，再次死亡才淘汰阵营。
- 自家 Turret 不攻击自家玩家。
- 只剩一个未淘汰阵营时才结束游戏。
- 死亡黑屏倒计时在每个本地玩家视口中正确显示。
