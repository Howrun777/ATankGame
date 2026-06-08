# 共享 Pawn 与 Controller 模块开发者文档

更新日期：2026-06-07

## 职责

该模块提供所有玩法模式共用的可控制角色和控制器逻辑。

主要目录：

```text
Source/BattleBlaster/Shared/Pawns/
Source/BattleBlaster/Shared/Pawns/NPC/
Source/BattleBlaster/Shared/Controllers/
Source/BattleBlaster/Shared/State/
```

## 关键类

| 类 | 职责 |
| --- | --- |
| `ABasePawn` | Tank / Tower 的基础 Pawn，持有通用组件和基础接口 |
| `ATank` | 玩家和 AI 坦克，负责移动、开火、弹药、死亡、开镜、网络移动入口 |
| `ATower` | NPC 塔楼敌人，继承 `ABasePawn`，不是 Defense 模式专属类 |
| `ATankPlayerController` | 本地战斗控制器，负责 HUD、暂停、回城、震动、死亡倒计时、输入映射 |
| `AUIPlayerController` | 菜单控制器，主要用于 UI 输入和本地设备映射 |
| `ATankPlayerState` | Slot、Team、KDA、仇人队列、死亡结算基础数据 |
| `ATankGameState` | 本地模式共享 GameState 基类 |

## 开发入口

- 改坦克移动：`ATank::Move`、`ATank::Turn`、网络 RPC 相关函数。
- 改坦克开火：`ATank::Fire`、`ApplyFire`、`ServerFire`、弹药字段。
- 改死亡和击杀奖励：`ATank::HandleDeath`、`ExecuteDeathAndReturnKiller`、`ATankPlayerState` 仇人队列。
- 改暂停菜单或死亡黑屏：`ATankPlayerController`。
- 改 Tower NPC 行为：`Shared/Pawns/NPC/Tower.*`。

## 当前设计要点

- `SlotId` 表示本地或网络对局中的玩家槽位。
- `TeamId` 表示阵营，用于敌我判断和友伤规则。
- `LocalPlayerIndex` 只应该用于本地输入设备 / LocalPlayer 语义，不应该再承担阵营或玩家身份职责。
- 网络模式下，服务器拥有最终权威；客户端可以做本地表现，但不能最终决定伤害、死亡、分数。

## 当前风险

- `ATank.cpp` 较大，多个能力混在同一个类里。
- `ATankPlayerController.cpp` 同时管理输入、HUD、暂停、震动、死亡 UI，后续网络和本地 UI 继续增长时会变重。
- `Tower` 既有 NPC 行为又有网络状态表现，后续如果继续扩展 NPC 类型，建议抽出更通用的 NPC Combat 接口。

## 推荐优化

短期建议只做函数边界整理，不急着拆很多组件：

```text
ATank
├── Identity: SlotId / TeamId
├── Movement: input, AI movement, network correction
├── Weapon: ammo, fire transform, projectile spawn
├── Death: health event, reward, respawn handoff
└── Presentation: aim mode, camera / local visual
```

中期可以考虑：

- `UTankWeaponComponent`：弹药、开火、炮口 transform、Projectile class。
- `UTankDeathComponent`：死亡事件、击杀归因、复活准备。
- `UPlayerBattleUIComponent`：HUD / KDA / Ammo / Death Screen 绑定。

## Editor 注意

- Tank 的炮口位置通常在蓝图里调整，不要把 Pawn 位置和炮口位置混为一谈。
- 修改 PlayerController 基类后，需要检查所有 BP PlayerController 子类是否仍然继承正确。
- 修改输入映射后，需要检查 `IMC_Default`、`IMC_System`、`IMC_MutiBattleMenu` 是否在对应 Controller 中挂载。
