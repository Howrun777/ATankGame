# 本地团队死斗模块开发者文档

更新日期：2026-06-07

## 职责

团队死斗是本地 2v2 或团队对抗玩法：玩家按队伍分组，击杀敌队得分，达到目标分数后结算。

主要目录：

```text
Source/BattleBlaster/Modes/TeamBattle/
Source/BattleBlaster/Modes/TeamBattle/UI/
```

## 关键类

| 类 | 职责 |
| --- | --- |
| `ATeamBattleGameMode` | 团队规则、分队、友伤判断、复活、团队计分、结算 |
| `ATeamBattleGameState` | 队伍分数等共享状态 |
| `ATeamBattlePlayerState` | 团队模式玩家状态 |
| `UTeamBattleMenuWidget` | 团队模式选 Tank / AI / 地图菜单 |
| `UTeamBattleGameOverWidget` | 团队结算 UI |

## 核心流程

```text
菜单选择队伍和玩家
-> GameMode 按 SlotId / TeamId 初始化
-> Spawn Tank
-> CanDealDamage 判断敌我
-> 击杀敌方后团队加分
-> 达到目标分数后结算
```

## 当前设计要点

- `TeamId` 是敌我判断的核心，不应再用 PlayerIndex 推导阵营。
- 友伤规则应该集中，避免 Turret / AI / Projectile 各自判断。
- 团队分数应由 GameState 或 GameMode 权威维护，UI 只显示。

## 当前风险

- 与 FreeForAll 重复大量 Spawn / Respawn / Timer / GameOver 代码。
- 友伤判断如果不统一，容易出现“自家防御塔或 AI 打自己人”的问题。
- GameOver Widget 仍有扫描场景数据的逻辑，后续建议收口到结果快照。

## 推荐优化

TeamBattle 适合第二个接入本地公共流程服务：

1. 先复用 FreeForAll 抽出的 Spawn / Respawn 工具。
2. 保留 TeamBattle 自己的 `CanDealDamage` 和团队得分规则。
3. 把团队分数 UI 改成只读 GameState。

## 验收清单

- 同队玩家不能被错误计为敌人。
- 敌方击杀加团队分。
- AI 所在队伍正确。
- 2v2、1v1 + AI、多人手柄组合都能正常进入。
