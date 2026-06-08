# 共享战斗模块开发者文档

更新日期：2026-06-07

## 职责

共享战斗模块负责生命、伤害、Projectile 命中、碰撞通道和跨模式通用战斗事件。

主要目录：

```text
Source/BattleBlaster/Shared/Combat/
Source/BattleBlaster/Core/BattleBlasterCollisionChannels.h
```

## 关键类

| 类 | 职责 |
| --- | --- |
| `UHealthComponent` | 生命、护盾、受伤、死亡事件 |
| `AProjectile` | Tank / Tower 使用的普通炮弹 |
| `ATurretProjectile` | MOBA Turret 使用的追踪弹，代码在 MOBA 模块 |
| `BattleBlasterCollisionChannels.h` | Projectile 等自定义碰撞通道 |

## 战斗数据流

```text
Projectile hit
-> 判断命中对象和碰撞规则
-> 找 HealthComponent
-> 计算伤害 / Buff / 友伤规则
-> HealthComponent 广播死亡或受伤
-> Pawn / GameMode / PlayerState 结算击杀、助攻、分数
```

## 当前设计要点

- 炮弹相撞抵消是游戏特色，不应当作为 Bug 移除。
- 子弹穿墙 Buff 和 GhostMode 不能让玩家彻底免疫子弹；玩家无论如何都应该能被子弹击中，穿墙只应该影响世界阻挡。
- Trigger Box 这类虚无体积应该通过碰撞预设对 Projectile 使用 Overlap 或 Ignore，不建议在 Projectile 里硬编码忽略一堆通道。

## 当前风险

- 伤害、阵营、友伤判断分散在 Projectile、AI、Turret、GameMode 中，后续容易出现“某个来源伤害规则不一致”。
- UI 结算和 GameMode 结算有时会重复推导击杀结果。
- 网络模式下伤害必须服务器权威，本地表现不能直接决定最终生命值。

## 推荐优化

建议新增轻量 Combat Policy，而不是马上重做战斗系统：

```text
UCombatRuleLibrary / FCombatPolicy
├── CanDamage(Instigator, Target)
├── AreEnemies(A, B)
├── ShouldProjectileBlock(Hit)
└── ResolveDamageContext(...)
```

优先把“阵营判断”和“Projectile 是否应该造成伤害”收口。这样可以降低 Turret 打自己人、NPC 击杀归因、网络伤害不一致这类问题复发概率。

## Editor 注意

- 修改碰撞规则优先在 Project Settings -> Collision 或具体蓝图 Collision Preset 中处理。
- Projectile 的碰撞 Object Channel 应保持项目统一，不要让某个蓝图单独变成普通 WorldDynamic。
- 修改伤害规则后，至少测试：Tank 打 Tank、Tank 打 Tower、Tower 打 Tank、Turret 打 Tank、爆炸桶伤害、尖刺伤害。
