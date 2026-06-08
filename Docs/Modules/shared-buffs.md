# Buff 系统模块开发者文档

更新日期：2026-06-07

## 职责

Buff 模块负责地图 Buff 拾取、Tank Buff 状态、Buff 持续时间、死亡继承和 UI 展示数据。

主要目录：

```text
Source/BattleBlaster/Shared/Buffs/
Source/BattleBlaster/Shared/UI/BuffListWidget.*
Source/BattleBlaster/Shared/UI/BuffSlotWidget.*
```

## 关键类

| 类 | 职责 |
| --- | --- |
| `ABuffPickup` | 地图上的 Buff 拾取物，处理随机类型、拾取、隐藏、复活 |
| `UTankBuffComponent` | Tank 身上的 Buff 状态、持续时间、效果应用 |
| `FBuffTypes` | Buff 类型定义 |
| `UBuffListWidget` | Buff 列表 UI，当前使用低频 Timer 刷新 |
| `UBuffSlotWidget` | 单个 Buff Slot 的图标、倒计时、颜色 |

## 当前设计要点

- Buff UI 已从每帧刷新优化为低频 Timer 刷新。
- BuffPickup 在网络模式下需要同步 Buff 类型、可见性和复活状态。
- Buff 对战斗规则有影响时，最终效果应由服务器权威判断。

## 当前风险

- Buff 类型、图标、颜色、持续时间和效果逻辑仍然分散。
- 新增 Buff 时需要同时改类型、逻辑、UI、拾取物配置，容易漏。
- 完整事件驱动 Buff UI 还没做，当前是轻量优化版。

## 推荐优化

中期建议把 Buff 配置数据化：

```text
UBuffDefinitionDataAsset
├── BuffType
├── DisplayName
├── Icon
├── Color
├── Duration
├── StackRule
└── GameplayParams
```

短期更稳的做法：

- 新增 Buff 时先补一个“开发清单”注释或文档条目。
- 所有 UI 展示字段从同一处函数读取。
- Buff 状态变化时广播事件，Widget 订阅；保留低频 Timer 作为兜底也可以。

## 测试清单

- 拾取 Buff 后效果生效。
- Buff 倒计时正确。
- 死亡 / 复活后需要继承的 Buff 正确恢复。
- 网络模式下客户端看到的 Buff 类型和服务器一致。
- 子弹穿墙、GhostMode、炮弹相撞抵消三者组合仍符合设计。
