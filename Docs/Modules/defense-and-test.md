# Defense 与 Test 模块开发者文档

更新日期：2026-06-07

## Defense

目录：

```text
Source/BattleBlaster/Modes/Defense/
```

当前状态：

- `ADefenseGameMode` 目前是占位空类。
- 当前项目里的 `ATower` 不是 Defense 模式专属代码，它是共享 NPC，位于 `Shared/Pawns/NPC`。

建议：

- 暂时不要把 Tower、Turret、Stage 逻辑误归到 Defense。
- 真正开发 Defense 时，先写玩法 PRD，再决定是否复用：
  - `ATower`
  - `ATurret`
  - `ADestructibleProp`
  - `AAIBotPlayerController`
  - `UHealthComponent`

可能的 Defense 架构：

```text
ADefenseGameMode
├── WaveManager
├── DefenseObjective
├── BuildableTower / Existing Tower reuse
├── EnemySpawnPoint
└── DefenseHUD
```

## Test

目录：

```text
Source/BattleBlaster/Modes/Test/
```

当前状态：

- `ATestGameMode` 用于测试 UI 或临时功能。
- 允许存在少量实验代码，但不要让正式模式依赖 Test 模块。

建议：

- 临时测试完成后，把可复用逻辑迁到正确模块。
- Test 模块可以保留小型验证入口，例如 UI 测试、输入测试、碰撞测试。
- 不建议在 Test GameMode 中维护正式玩法规则。
