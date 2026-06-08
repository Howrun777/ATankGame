# 共享 AI 模块开发者文档

更新日期：2026-06-07

## 职责

AI 模块负责 AI Tank 的目标选择、移动策略、闪避、瞄准和开火。

主要目录：

```text
Source/BattleBlaster/Shared/AI/
```

## 关键类

| 类 | 职责 |
| --- | --- |
| `AAIBotPlayerController` | 完整战斗 AI，包含目标列表、威胁评估、移动策略、闪避、瞄准、射击 |
| `ABotTankController` | 简单随机移动 AI，适合测试或低复杂度场景 |

## 当前 AI 流程

```text
BeginPlay / OnPossess
-> 建立可攻击目标列表
-> 定时或事件刷新目标
-> 根据距离、威胁、阵营选择目标
-> 状态机执行追击、绕圈、闪避、逃跑
-> 炮塔瞄准
-> 满足条件时开火
```

## 当前风险

- `AAIBotPlayerController.cpp` 超过千行，感知、决策、行动都在一个类里。
- 多处 `GetAllActorsOfClass` 用于查找 Tank / Tower，玩家和 AI 数量增加后会更难排查性能问题。
- AI 目标系统和阵营规则依赖多个类的约定，后续新增模式时容易漏接。

## 推荐优化

分三步做，避免一次性拆坏：

1. 抽目标查询和目标缓存：
   - `UAITargetRegistry` 或普通 helper。
   - GameMode / Actor Spawn / Actor Death 时注册或移除目标。
2. 抽目标评分：
   - `FAITargetScore`
   - `CalculateThreat`
   - `SelectBestTarget`
3. 抽行动策略：
   - Chase
   - Strafe
   - Circle
   - Dodge
   - Flee

拆分目标不是为了“代码更漂亮”，而是为了以后新增 MOBA AI、守塔 AI、网络 AI 填充时能复用目标选择规则。

## 性能建议

- 目标全量扫描可以保留在 BeginPlay 或低频兜底路径。
- 高频 Tick 内不要持续全图扫描。
- AI 数量很多时，目标选择可以错峰刷新，例如每个 AI 用不同初始延迟。

## Editor 注意

- AI Controller Class 需要在 Tank 蓝图或生成逻辑中保持一致。
- AI 难度参数如果继续增加，建议放到 DataAsset，而不是继续堆在 Controller 上。
