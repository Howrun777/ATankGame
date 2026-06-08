# BattleBlaster 开发者指南

> 基于当前代码库整理：2026-06-07
> 目标：给继续开发本项目的人一张真实的代码地图，同时说明各功能模块的边界、数据归属和后续优化方向。

---

## 0. 先说结论

当前项目的 `Source/BattleBlaster` 已经从早期扁平目录整理成了比较清晰的三层结构：

```text
Core/      全局配置、存档、会话、跨关卡数据
Shared/    坦克、控制器、AI、战斗、Buff、地图交互物、共享 UI
Modes/     主菜单、本地玩法、网络玩法、测试模式
```

所以现在的主要问题已经不是“文件能不能分类”，而是“各模块内部还有多少重复流程、哪些公共能力应该抽出来”。真正需要谨慎的是两件事：

1. **不要第一步就重命名 UCLASS / USTRUCT / UENUM 名字。** 只移动 `.h/.cpp` 文件并更新 `#include`，风险比重命名类小得多。
2. **不要为了“看起来更干净”强行抽象所有重复代码。** 这个项目已经接近末期，优先抽稳定、低风险、复用价值高的公共能力，例如生成玩家、复活、结算、地图配置、伤害规则。

特别纠正一个容易误判的点：

- `ATower` 是游戏里的 NPC / 敌方塔楼，继承自 `ABasePawn`，当前主要被 `TankStageGameMode` 和 AI 目标系统使用。它不是 Defense 模式代码。
- `ATurret` 才是 MOBA 防御塔，继承自 `ADestructibleProp`，由 `TankMOBAGameState` 注册和管理。
- `ADefenseGameMode` 当前还没有实际玩法代码，只有空类。

---

### 0.1 模块文档索引

本文是总览文档。每个模块的独立开发说明放在 `Docs/Modules/` 下：

| 模块 | 文档 |
| --- | --- |
| 模块总索引 | `Docs/Modules/00-module-index.md` |
| Core / 存档 / 会话 | `Docs/Modules/core.md` |
| 共享 Pawn 与 Controller | `Docs/Modules/shared-pawns-and-controllers.md` |
| 共享战斗层 | `Docs/Modules/shared-combat.md` |
| 共享 AI | `Docs/Modules/shared-ai.md` |
| Buff 系统 | `Docs/Modules/shared-buffs.md` |
| 共享状态层 | `Docs/Modules/shared-state.md` |
| 地图交互物 | `Docs/Modules/shared-world.md` |
| 共享 UI | `Docs/Modules/shared-ui.md` |
| 主菜单 | `Docs/Modules/main-menu.md` |
| 本地自由死斗 | `Docs/Modules/free-for-all.md` |
| 本地团队死斗 | `Docs/Modules/team-battle.md` |
| 本地 MOBA | `Docs/Modules/moba.md` |
| 单人闯关 / PVE | `Docs/Modules/stage.md` |
| 网络模式 | `Docs/Modules/network.md` |
| Defense 与 Test | `Docs/Modules/defense-and-test.md` |

### 0.2 当前优化总判断

当前最值得优化的是“流程重复”，不是继续挪文件。优先级建议如下：

1. **低风险：** 统一 PlayerStart 查找、地图配置、全局 UI 主控 Controller 获取方式、模式参数读取方式。
2. **中风险：** 抽本地模式 Spawn / Respawn / GameOver 公共服务，先让 FreeForAll 接入，再迁 TeamBattle 和 MOBA。
3. **中高风险：** 拆 `AAIBotPlayerController`、`ATank`、`UBattleBlasterGameInstance` 这类大脑型类。
4. **暂缓：** 大规模重命名 UCLASS、重做选人菜单继承体系、一次性统一所有本地和网络玩法模式。

---

## 1. 真实类图

### 1.1 Pawn / Actor

```text
APawn
└── ABasePawn
    ├── ATank                         玩家 / AI 坦克
    └── ATower                        NPC 塔楼敌人，继承 BasePawn

AActor
├── AProjectile                       Tank / Tower 使用的普通炮弹
├── ATurretProjectile                 MOBA Turret 使用的追踪弹
├── ABuffPickup                       地图 Buff 拾取物
├── ADestructibleProp                 可破坏物基类
│   ├── AExplosiveBarrel              爆炸油桶
│   ├── AWoodenCrate                  木箱
│   └── ATurret                       MOBA 防御塔，继承 DestructibleProp
├── ARisingGate                       升降门
├── ASlideTrack                       滑道 / 加减速轨道
├── ASpikeTrap                        尖刺陷阱
└── ATeleportPortal                   传送门
```

### 1.2 Controller

```text
APlayerController
├── ATankPlayerController             战斗内玩家控制器，管理 HUD、暂停、回城、MOBA 死亡/淘汰 UI
└── AUIPlayerController               菜单控制器，记录输入设备映射

AAIController
├── AAIBotPlayerController            完整战斗 AI，带目标列表、状态机、预测射击
└── ABotTankController                简单随机移动 AI
```

### 1.3 GameMode / GameState / PlayerState

```text
AGameMode
├── AMainMenuGameMode                 主菜单
├── ABattleBlasterGameMode            多人自由死斗
├── ATeamBattleGameMode               2v2 团队死斗
├── ATankMOBAGameMode                 MOBA 模式
├── ATankStageGameMode                单人闯关 / PVE
├── ADefenseGameMode                  Defense 占位空类
└── ATestGameMode                     测试 UI 模式

AGameState
└── ATankGameState
    ├── ATankBattleGameState
    ├── ATeamBattleGameState
    ├── ATankMOBAGameState
    └── ATankStageGameState

APlayerState
└── ATankPlayerState
    ├── ATankBattlePlayerState
    ├── ATeamBattlePlayerState
    ├── ATankMOBAPlayerState
    └── ATankStagePlayerState
```

---

## 2. 全局入口与跨关卡数据

### 2.1 模块配置

`BattleBlaster.Build.cs` 当前依赖：

- `Core`, `CoreUObject`, `Engine`
- `InputCore`, `EnhancedInput`, `ApplicationCore`
- `UMG`, `Slate`, `SlateCore`
- `Niagara`
- `AIModule`

这说明项目核心方向很明确：本地分屏、UMG UI、Niagara 表现、AI Controller。

### 2.2 `UBattleBlasterGameInstance`

`UBattleBlasterGameInstance` 是菜单和关卡之间传递数据的中心。它不属于某一个模式，而是跨关卡保存运行期状态。

主要职责：

- 多人设置：`TargetPlayerCount`, `TargetMatchScore`
- 坦克选择：`SelectedTankClasses`
- 手柄 / AI 配置：`ConnectedGamepadCount`, `AIControlledPlayerIndices`, `RegisterPlayerDeviceMapping`
- 单人闯关进度：`CurrentLevelIndex`, `BestLevelRecord`, `CampaignLevelNames`
- 难度：`DifficultyCoefficientK`, `GetDifficultyMultiplier`
- 单人跨关携带状态：`FPlayerCarryState`
- 多人死斗历史榜：`MultiBattleHistory`
- 菜单返回目标：`ReturnToMenuType`, `PendingMainMenuWidgetClass`

注意：`GameInstance` 数据只在本次进程内存在。持久化由两个 SaveGame 类负责：

- `UBattleBlasterSaveGame`：当前关卡、历史最高关卡。
- `UBattleBlasterHistorySaveGame`：多人死斗历史战绩前 50 条。

### 2.3 碰撞通道

项目新增了一个自定义 Object Channel：

```cpp
#define BB_COLLISION_PROJECTILE ECC_GameTraceChannel1
```

配置位置：

- `Source/BattleBlaster/BattleBlasterCollisionChannels.h`
- `Config/DefaultEngine.ini`

约定：

- `Projectile` 通道用于炮弹。
- `ATank` / `ABasePawn` 的胶囊体必须 `Block` 这个通道。
- `AProjectile` 和 `ATurretProjectile` 默认 `BlockAll`，因此炮弹相撞会阻挡。这是当前设计特性，不应该随手取消。
- 穿墙 Buff 只让炮弹忽略 `WorldStatic` / `WorldDynamic` / `PhysicsBody`，不能忽略 `Pawn`。
- Ghost Mode 只改变坦克对世界几何的响应，不能让坦克忽略 `Projectile`。

### 2.4 网络同步数据归属速查

当前项目的联网模式采用“状态归属分散，但规则集中”的方式：不要把所有同步数据塞进 `PlayerState` 或某个全局类，而是谁拥有这个状态，谁负责同步。

| 数据类型 | 推荐归属 |
| --- | --- |
| 玩家身份、槽位、队伍、KDA、跨 Pawn 弹药显示 | `ATankPlayerState` / `ANetworkPlayerStateBase` |
| 对局人数、比赛阶段、全局比分、胜负状态 | `ANetworkGameStateBase` 或具体模式 GameState |
| 网络死斗个人分数、目标分、胜者、时间 | `ANetworkDeathmatchGameState` |
| 网络团队死斗团队分数、胜利队伍 | `ANetworkTeamDeathmatchGameState` |
| 网络 MOBA 核心塔数量、队伍淘汰、胜利队伍、时间 | `ANetworkMOBAGameState` |
| Tank 移动、转向、炮塔输入、开火请求、死亡表现 | `ATank` |
| 血量和护盾 | `UHealthComponent` |
| Tank 身上的 Buff UI 状态 | `UTankBuffComponent` |
| 地图 Buff 类型和是否可拾取 | `ABuffPickup` |
| 子弹移动、穿透、强化表现、命中特效 | `AProjectile` |
| Tower 死亡状态和炮塔朝向 | `ATower` |
| 木箱、油桶等可破坏物的破坏状态和推动后位置 | `ADestructibleProp` |
| 尖刺状态机和状态开始时间 | `ASpikeTrap` |

判断规则：

- 描述“玩家是谁”的数据放 `PlayerState`。
- 描述“比赛整体怎样”的数据放 `GameState`。
- 描述“这个角色当前怎样”的数据放 Pawn 或组件。
- 描述“世界里这个物体怎样”的数据放该 Actor 自己。
- 表现类事件优先用 Multicast 或本地表现，不要长期存进中心状态。

网络玩法规则集中在 `ANetworkGameModeBase` 及其子类中，而不是塞进 `PlayerState`。当前网络玩法子类包括：

- `ANetworkDeathmatchGameMode`：个人死斗，个人分数和胜者写入 `ANetworkDeathmatchGameState`。
- `ANetworkTeamDeathmatchGameMode`：团队死斗，覆盖队伍分配和友伤规则，团队分数写入 `ANetworkTeamDeathmatchGameState`。
- `ANetworkMOBAGameMode`：网络 MOBA，核心塔状态和淘汰状态写入 `ANetworkMOBAGameState`。
- `ANetworkTeamMOBAGameMode`：团队 MOBA，复用网络 MOBA 规则，只改变 `SlotId -> TeamId` 的分配方式。

---

## 3. 共享战斗层

### 3.1 `ABasePawn`

`ABasePawn` 是坦克和 NPC Tower 的共同父类。

组件：

- `UCapsuleComponent* CapsuleComp`
- `UStaticMeshComponent* BaseMesh`
- `UStaticMeshComponent* TurretMesh`
- `USceneComponent* ProjectileSpawnPoint`
- `UHealthComponent* HealthComp`

核心函数：

- `RotateTurret(FVector LookAtTarget)`：让炮塔水平旋转到目标方向。
- `Fire()`：按 `Fire_Interval` 冷却生成 `AProjectile`。
- `HandleDestruction()`：播放死亡 Niagara、音效、CameraShake。

当前 `ABasePawn` 构造函数里已经设置：

```cpp
CapsuleComp->SetCollisionObjectType(ECC_Pawn);
CapsuleComp->SetCollisionResponseToChannel(BB_COLLISION_PROJECTILE, ECR_Block);
```

这意味着所有继承 `ABasePawn` 的单位都应该被炮弹命中，包括 `ATank` 和 `ATower`。

### 3.2 `ATank`

`ATank` 是玩家和 AI 都使用的坦克 Pawn。

扩展组件：

- `UTankBuffComponent* BuffComp`
- `USpringArmComponent* SpringArmComp`
- `UCameraComponent* CameraComp`
- `UCameraComponent* ScopeCameraComp`
- `UFloatingPawnMovement* PawnMovementComponent`

主要系统：

- Enhanced Input：移动、转向、开火、炮塔旋转、开镜。
- 开镜：键鼠切换、手柄按住，使用 `AddToPlayerScreen()` 适配分屏。
- 弹药：`CurrentAmmo`, `MaxAmmo`, `SetAmmo()` 同步到 PlayerState。
- Buff 标记：无限子弹、伤害提升、子弹穿墙、双发、Ghost。
- 回城协作：移动和开火都会检查 `ATankPlayerController::bIsHoldingReturnToSpawn`。
- AI 移动入口：`MoveAI()` 和 `MoveWithAI()`。

真实死亡链路：

```text
HealthComponent::OnDeath
  -> ATank::HandleDeath
      -> CachedKiller = InstigatedBy Pawn 或 DamageCauser
      -> HandleDestruction()
      -> ATankPlayerState::ProcessDeath()
          -> DeathCount++
          -> 从 AttackerQueue 结算 Killer / Assist
          -> KillerTank->HandleKillReward()
          -> DeadTank->OnKilled.Broadcast(DeadTank, KillerTank)
```

也就是说，当前真正广播 `OnKilled` 的地方是 `ATankPlayerState::ProcessDeath()`，不是 `ATank::HandleDeath()` 直接广播。

### 3.3 `UHealthComponent`

`UHealthComponent` 是通用生命组件，不写 Tank 专属逻辑。

数据：

- `MaxHealth`
- `CurrentHealth`
- `MaxShield`
- `CurrentShield`

事件：

- `OnHealthChanged`
- `OnDeath`

运行方式：

```text
BeginPlay
  -> Owner->OnTakeAnyDamage.AddDynamic(OnDamageTaken)

ApplyDamage / ApplyRadialDamage
  -> Owner::TakeDamage
  -> OnTakeAnyDamage
  -> UHealthComponent::OnDamageTaken
      -> 先扣 Shield
      -> 再扣 Health
      -> Broadcast OnHealthChanged
      -> Health <= 0 时 Broadcast OnDeath
```

`Heal()` 和 `AddShield()` 也会广播 `OnHealthChanged`，所以 UI 依赖这个事件刷新是合理的。

### 3.4 `AProjectile`

`AProjectile` 是普通炮弹，当前由 `ATank` 和 `ATower` 使用。

核心行为：

- `ProjectileMesh` 是根组件。
- ObjectType 是 `BB_COLLISION_PROJECTILE`。
- 默认 `BlockAll`，并且 `Block Pawn`。
- `BeginPlay()` 绑定 `OnComponentHit`。
- 默认生命周期 6 秒。

命中处理：

```text
OnHit
  -> 空目标 / 自己：Destroy
  -> 命中 Owner：Destroy
  -> AttackerTank vs VictimTank 阵营检查
      -> MOBA：同 TeamId/CampIndex 不伤害
      -> TeamBattle：同红蓝阵营不伤害
  -> 命中 APawn：ApplyDamage，然后 Destroy
  -> 命中 ADestructibleProp：ApplyDamage，然后 Destroy
  -> 命中墙/障碍：
      -> 无 Pierce：播放命中特效并 Destroy
      -> 有 Pierce：通常不会进这里，因为已忽略世界碰撞
```

Pierce Mode 当前只忽略：

- `ECC_WorldStatic`
- `ECC_WorldDynamic`
- `ECC_PhysicsBody`

并保留：

- `ECC_Pawn = Block`

所以玩家子弹穿墙时仍然应该击中 Tank / Tower。

### 3.5 `ATurretProjectile`

`ATurretProjectile` 是 MOBA 防御塔专用追踪弹。

特点：

- ObjectType 也是 `BB_COLLISION_PROJECTILE`。
- 默认 `BlockAll`。
- 持有 `TargetActor`, `Damage`, `Speed`, `CampIndex`。
- `Tick()` 中持续把速度方向指向目标。
- `OnHit()` 对 APawn 做阵营判断后 ApplyDamage。

它不处理穿墙 Buff，因为它不是玩家 Tank 的普通炮弹。

### 3.6 `UTankBuffComponent`

Buff 类型来自 `BuffTypes.h`：

- `Heal`
- `Ammo`
- `Speed`
- `Pierce`
- `Ghost`
- `Damage`
- `DoubleShot`
- `Shield`
- `RandomIcon`

一次性 Buff：

- `Heal`：`HealthComp->ResetHealth()`
- `Shield`：添加 `MaxHealth * 0.5f` 护盾

持续性 Buff：

- `Ammo`：`bHasInfiniteAmmo = true`，UI 显示 9999。
- `Speed`：速度变为 `BaseSpeed * 2`，最后 10 秒线性衰减。
- `Pierce`：`bHasBulletPierce = true`，Tank 开火时调用 `Projectile->EnablePierceMode()`。
- `Ghost`：`bIsGhostMode = true`，世界几何改为 Overlap，但 Projectile 保持 Block。
- `Damage`：炮弹伤害翻倍并换强化表现。
- `DoubleShot`：每次发两发，左右偏移。

Ghost 结束时有两条路径：

```text
不在墙里
  -> bIsGhostMode = false
  -> WorldStatic / WorldDynamic 恢复 Block
  -> Projectile 继续 Block

卡在墙里
  -> 保持 bIsGhostMode = true
  -> 进入窒息状态
  -> 定时扣血
  -> 离开墙体后 OnEscapedFromGeometry()
      -> 恢复世界碰撞
      -> Projectile 继续 Block
```

### 3.7 `ATankPlayerState`

这是 KDA 和死亡归因的核心。

关键数据：

- `SlotId`
- `TeamId`
- `IsAlive`
- `HomeSpawnLocation`, `HomeSpawnRotation`
- `CurrentAmmo`
- `KillCount`, `DeathCount`, `AssistCount`
- `CurrentBuffs`
- `AttackerQueue`

`AttackerQueue` 是当前项目的“仇人队列”击杀归因算法，用来解决玩家助攻、NPC 补刀、环境伤害和最终击杀归属问题。

算法规则：

- `ATank` 每次受到伤害时，只会把 `ATank` 类型攻击者传给 `RecordAttacker(ATank* Attacker)`。
- 无效攻击者和自己攻击自己不会入队。
- 同一个攻击者重复造成伤害时，会先从旧位置移除，再插入队头。
- 队头永远表示最近一次有效 Tank 攻击者。
- 队列记录 `TWeakObjectPtr<ATank>` 和最后攻击时间，避免攻击者销毁后留下野指针。
- `CleanUpExpiredAttackers()` 每 1 秒运行一次，移除超过 7 秒或已经失效的攻击者。
- 队列为空时停止清理 Timer，下一次受击时再重新启动。
- PlayerState `EndPlay()`、死亡结算、重置新游戏时都会停止清理 Timer，避免生命周期残留。

死亡结算规则：

- `ProcessDeath()` 会先让自己 `DeathCount + 1`。
- 结算前再调用一次 `CleanUpExpiredAttackers()`，确保最后结果只包含有效攻击者。
- 队头攻击者算 Killer。
- 其他有效攻击者算 Assist。
- Killer 触发 `HandleKillConfirmed()` 和 `HandleKillReward()`。
- 刷新死亡者、Killer、Assist 相关真人玩家的 KDA UI。
- 广播 `DeadTank->OnKilled(DeadTank, KillerTank)` 给 GameMode。
- 结算后清空 `AttackerQueue`，等待复活后的新一轮战斗。

这个算法的关键价值是：如果 NPC、Tower、尖刺、油桶等非玩家对象最终击杀了玩家，只要 7 秒内有玩家 Tank 对该玩家造成过伤害，击杀仍然可以归给最近的玩家攻击者；如果 7 秒内没有任何有效 Tank 攻击者，`KillerTank` 会是 `nullptr`，具体模式可以按自杀、环境死亡或不计分处理。

各模式 PlayerState 的差异：

- `ATankBattlePlayerState`：无敌状态、复活时间。
- `ATeamBattlePlayerState`：红蓝阵营、团队贡献；团队分数由 `TeamBattleGameMode` 统一结算。
- `ATankMOBAPlayerState`：CampIndex、死亡、等待复活、永久淘汰、复活时间增长。
- `ATankStagePlayerState`：剩余生命、关卡分、PVE 击杀数。

---

## 4. 共享 AI 层

### 4.1 `AAIBotPlayerController`

这是当前项目的主要 AI。

默认攻击过滤类型：

- `ATank`
- `ATower`
- `ATurret`

重点机制：

- `bWantsPlayerState = true`，让 AI 也有 PlayerState，能参与 KDA / 分数逻辑。
- `AttackTargetList` 保存候选目标。
- `IsEnemy()` 在团队模式下检查红蓝阵营；非 Tank 目标默认视为敌对。
- `RefreshTargetFromAttackList()` 固定频率清理无效目标并选择最近目标。
- `OnAttackedBy()` 让 AI 受到攻击后把攻击者加入目标列表。
- 战斗状态机：Idle、Chase、Strafe、KeepDistance、Flee、TakeCover、Ambush。
- 难度档位：Easy、Normal、Hard、Insane。

对分类很重要的一点：AI 同时知道 `ATower` 和 `ATurret`，所以它不属于某个单一玩法模式。更适合放到共享 AI 模块。

### 4.2 `ABotTankController`

简单 AI，用于轻量场景。它不承担复杂战斗判断。

---

## 5. 玩法模式

### 5.1 主菜单模块

相关文件：

- `MainMenuGameMode`
- `MainMenuWidget`
- `MutiPlayerMenuWidget`
- `MutiBattleMenuWidget`
- `TeamBattleMenuWidget`
- `MOBASetupWidget`
- `TankStageStartWidget`
- `SelectMapWidget`
- `GameSettingsMenuWidget`
- `PauseMenuWidget`
- `UIPlayerController`

主菜单流程：

```text
AMainMenuGameMode::BeginPlay
  -> 禁用分屏
  -> 最多创建 4 个 LocalPlayer 供菜单输入使用
  -> 按 GameInstance::ReturnToMenuType 选择打开：
      -> MainMenuWidget
      -> SinglePlayerSelectWidget
      -> PendingMainMenuWidgetClass
  -> UI Only 输入模式
```

多人设置菜单流程：

```text
MainMenuWidget
  -> MutiPlayerMenuWidget
      -> MutiBattleMenuWidget
      -> TeamBattleMenuWidget
      -> MOBASetupWidget
```

这些设置 Widget 会写入 `GameInstance`：

- `TargetPlayerCount`
- `TargetMatchScore`
- `ConnectedGamepadCount`
- `AIControlledPlayerIndices`
- `SelectedTankClasses`

`SelectMapWidget` 最后用 `OpenLevel(LevelName, true, OptionsString)` 进入关卡，并把目标 GameMode 写进 `OptionsString`。

### 5.2 自由死斗：`ABattleBlasterGameMode`

职责：

- 从 GameInstance 读取玩家数、分数、AI 配置、坦克选择。
- 创建足够 LocalPlayer。
- 三人模式用四宫格视口，并给多余视口加黑屏 Widget。
- 查找 `PlayerStartTag = P0/P1/P2/P3` 的出生点。
- Spawn `ATank`，真人用 PlayerController Possess，AI 用 `AAIBotPlayerController` Possess。
- 绑定 `NewTank->OnKilled` 到 `HandleTankKilled()`。
- 创建全局 `ScreenMessage` 和 `ScoresDisplayWidget`。
- 倒计时后开始比赛。

击杀流程：

```text
HandleTankKilled(DeadTank, KillerTank)
  -> 保存死者 Buff
  -> Killer 有效且不是自己：Killer +1 分
  -> 自杀：Victim -1 分
  -> 刷新比分 UI
  -> 达到 TargetScore：胜利，显示 MultiBattleGameOverWidget
  -> 未结束：定时 RespawnPlayer(SlotId)
```

复活：

- 按 SlotId 找出生点。
- 使用该玩家选择的 TankClass。
- 找原 Controller 重新 Possess。
- 生命值和弹药按 `RespawnHealthPercent` / `RespawnAmmoPercent` 恢复。
- 恢复死亡前 Buff。
- 播放复活特效。
- 短暂无敌。

### 5.3 团队死斗：`ATeamBattleGameMode`

团队死斗固定 4 人，阵营规则：

- 玩家 0 / 2 是红队。
- 玩家 1 / 3 是蓝队。

核心差异：

- 固定 `TeamBattlePlayerCount = 4`。
- 使用 `TeamScores[0]` 和 `TeamScores[1]`。
- `CanDealDamage()` 禁止友军伤害。
- `ATeamBattlePlayerState::HandleKillConfirmed()` 只记录个人团队贡献，团队分数由 `TeamBattleGameMode` 统一结算。
- GameMode 内也有 `AddTeamScore()` 和胜负检查。

复活、Buff 保存、黑屏视口、分数 UI 的结构和自由死斗很像，所以将来可以考虑抽一个本地多人对战基类，但第一轮整理不建议立刻抽象。

### 5.4 MOBA：`ATankMOBAGameMode`

MOBA 模块包含：

- `TankMOBAGameMode`
- `TankMOBAGameState`
- `TankMOBAPlayerState`
- `Turret`
- `TurretProjectile`
- `MOBASetupWidget`
- `MOBAGameOverWidget`
- `MOBATopStateUI`
- `DeathScreenWidget`
- `EliminatedScreenWidget`

玩家规则：

- 每个玩家一个独立 Camp。
- `CampIndex` 和 `TeamId` 在 MOBA 中对应；`SlotId` 只表示比赛槽位。
- 死亡不等于淘汰。
- 核心塔被摧毁后，该阵营玩家再次死亡才会进入永久淘汰。

防御塔：

- `ATurret` 继承 `ADestructibleProp`。
- `CampIndex` 表示所属阵营。
- `bIsCoreTurret` 表示主防御塔。
- `BeginPlay()` 注册到 `ATankMOBAGameState`。
- 外塔存活时，核心塔 `IsDamageImmune()`。
- 同阵营 Tank 攻击防御塔会治疗，不造成伤害。
- 被摧毁后替换为废墟 Mesh，并通知 GameState。

结束条件：

```text
CheckGameOverByElimination()
  -> 遍历当前对局阵营
  -> 统计未 IsEliminated() 的阵营数量
  -> 只剩 1 个未淘汰阵营
  -> SetGameOver(true)
  -> SetWinningCampIndex()
  -> 延迟显示 MOBAGameOverWidget
```

`TankMOBAGameState::OnTurretDestroyed()` 会更新塔数量，但当前设计不是核心塔一倒就直接结束。核心塔数量只决定阵营玩家下一次死亡时能否复活，不直接决定胜负。

### 5.5 单人闯关 / PVE：`ATankStageGameMode`

单人闯关模块包含：

- `TankStageGameMode`
- `TankStageGameState`
- `TankStagePlayerState`
- `TankStageStartWidget`
- `TankStageOverWidget`
- `PassWidget`
- `Tower`

关卡开始：

```text
BeginPlay
  -> GameInstance::MarkCampaignLevelStart
  -> 随机选择 PlayerStart
  -> 读取 SelectedTankClasses[0]
  -> Spawn 玩家 Tank 并 Possess
  -> 绑定玩家 OnKilled
  -> 扫描全部 ATower
  -> 绑定 Tower HealthComp::OnDeath 到 HandleTowerDestroyed
  -> 应用难度系数到 Tower
  -> 开场倒计时
```

`ATower` 在这个模式下是 PVE 敌人：

- 检测范围内 `ATank`。
- 选择最近存活 Tank。
- 用 `Visibility` LineTrace 检查路径是否被挡。
- 开火生成 `AProjectile`。
- 死亡时调用 `HandleDestruction()`。
- 在 `TankStageGameMode` 中禁止复活。
- `ApplyDifficultyMultiplier()` 会增强生命、射程、攻速，并同步 DetectionSphere 半径。

胜利条件：

- `HandleTowerDestroyed()` 统计剩余 Tower。
- 全部 Tower 被摧毁则通关。
- 通关前保存玩家携带状态。
- `GameInstance::LoadNextLevel()` 随机关卡列表进入下一关。

失败 / 复活：

- 玩家死亡会增加死亡次数。
- 未达到最大死亡次数：延迟复活。
- 达到上限：显示 GameOver。

### 5.6 Defense：`ADefenseGameMode`

当前只有空类：

```cpp
class BATTLEBLASTER_API ADefenseGameMode : public AGameMode
{
    GENERATED_BODY()
};
```

所以现在不要把 `Tower`、`Turret` 或任何已有模式逻辑归到 Defense 下。等 Defense 真正写代码时，建议单独建立 `Modes/Defense`。

### 5.7 Test：`ATestGameMode`

测试模式只负责创建测试 UI，不参与主玩法架构。

---

## 6. 地图交互物与可破坏物

### 6.1 `ADestructibleProp`

可破坏物基类。

组件：

- `DefaultSceneRoot`
- `PropMesh`
- `HealthComp`
- `HealthBarWidgetComp`

逻辑：

- `HealthComp->OnDeath` 绑定 `OnPropDestroyed()`。
- `TakeDamage()` 更新血条并记录最后攻击者。
- `CheckPlayerDistance()` 遍历所有 PlayerController，只要任意本地玩家在范围内就显示血条。
- 死亡后关闭碰撞并隐藏血条。

当前这个类已经适配本地分屏：不再只检查 Player 0。

### 6.2 `AExplosiveBarrel`

继承 `ADestructibleProp`。

死亡时：

- 隐藏模型。
- 关闭碰撞。
- 播放爆炸 Niagara 和音效。
- 用 `ApplyRadialDamageWithFalloff()` 造成范围伤害。
- 通过 `LastAttacker` 尽量把击杀归属给引爆者。
- 最后 Destroy。

### 6.3 `AWoodenCrate`

继承 `ADestructibleProp`。

死亡时：

- 关闭碰撞。
- 隐藏模型。
- 播放破碎效果和音效。
- `SetLifeSpan(TimeToDisappear)`。

### 6.4 `ASpikeTrap`

尖刺陷阱不是只针对 Tank，而是针对拥有 `UHealthComponent` 的 Actor。

特点：

- `DetectionSphere` 唤醒陷阱。
- 状态机：Dormant、Hidden、Thrusting、Active、Retracting。
- 用 `TSet<AActor*>` 记录范围内对象，避免重复 overlap 计数错误。
- 每轮刺出只伤害一次。
- `EndPlay()` 清理计时器和集合。

### 6.5 `ASlideTrack`

滑道 / 加减速轨道。

特点：

- 默认关闭 Tick，玩家进入 DetectionSphere 后唤醒。
- `BoxComp` 检测 Tank 是否在轨道上。
- 可配置状态切换：加速 / 减速。
- 离开轨道时会检查是否仍在其他滑道上，避免重叠滑道导致速度错误恢复。

### 6.6 `ARisingGate`

升降门。

特点：

- `TriggerBox` 检测 `ATank`。
- 有玩家进入则 Rising。
- 玩家离开则 Lowering。
- Tick 中用 `VInterpConstantTo` 移动 GateMesh。

### 6.7 `ATeleportPortal`

传送门。

特点：

- `PortalPairID` 相同的两个门互为目标。
- 任意进入 Trigger 的 Actor 都可以传送，不再限制 Tank / Projectile。
- 会重定向 `UProjectileMovementComponent` 的速度。
- 对模拟物理的 Actor 会重定向线速度。
- 用 `IgnoredActors` 防止刚传过去又传回来。
- 两端 Portal 同时进入冷却。

---

## 7. UI 分层

### 7.1 战斗内共享 UI

适合放到共享 UI 模块：

- `HUDWidget`
- `BulletsWidget`
- `BuffListWidget`
- `BuffSlotWidget`
- `KDAWidget`
- `ScoresDisplayWidget`
- `ScreenMessage`
- `PauseMenuWidget`
- `ReturnToSpawnWidget`
- `GameSettingsMenuWidget`

创建位置多在 `ATankPlayerController`。分屏玩家自己的 UI 应使用 `AddToPlayerScreen()`，全局 UI 才用 `AddToViewport()`。

### 7.2 模式专属 UI

自由死斗：

- `MutiBattleMenuWidget`
- `MultiBattleGameOverWidget`

团队死斗：

- `TeamBattleMenuWidget`
- `TeamBattleGameOverWidget`

MOBA：

- `MOBASetupWidget`
- `MOBAGameOverWidget`
- `MOBATopStateUI`
- `DeathScreenWidget`
- `EliminatedScreenWidget`

单人闯关：

- `TankStageStartWidget`
- `TankStageOverWidget`
- `PassWidget`

主菜单：

- `MainMenuWidget`
- `MutiPlayerMenuWidget`
- `SelectMapWidget`

---

## 8. 常见开发任务该改哪里

### 8.1 新增一个 Buff

需要看这些文件：

- `BuffTypes.h`
- `TankBuffComponent.h/.cpp`
- `Tank.h/.cpp`
- `Projectile.cpp`，如果影响炮弹行为。
- `BuffPickup.h/.cpp`，如果需要地图拾取生成。
- `BuffListWidget` / `BuffSlotWidget`，如果 UI 需要特殊显示。

注意：

- 只影响玩家状态的 Buff 放 `TankBuffComponent`。
- 影响炮弹生成时属性的 Buff 放 `ATank::Fire()`。
- 影响炮弹命中结果的 Buff 放 `AProjectile::OnHit()`。
- 如果要支持复活保留 Buff，确认 `GetAllActiveBuffs()` 和 `RestoreBuffs()` 覆盖它。

### 8.2 新增一个模式

建议创建一组：

- `XXXGameMode`
- `XXXGameState`
- `XXXPlayerState`
- `XXXSetupWidget`
- `XXXGameOverWidget`

入口流程参考：

```text
菜单 Widget
  -> 写入 GameInstance
  -> SelectMapWidget / OpenLevel Options
  -> GameMode::BeginPlay 读取 GameInstance
  -> Spawn / Possess / Bind OnKilled
```

不要把模式特有逻辑写进 `ATank` 或 `ATankPlayerState` 基类，除非多个模式真的共享。

### 8.3 新增一个地图机关

参考：

- 简单触发：`RisingGate`
- 持续影响玩家：`SlideTrack`
- 周期伤害：`SpikeTrap`
- 任意 Actor 传送：`TeleportPortal`
- 可破坏：`DestructibleProp`

如果机关要伤害玩家，优先用 `UGameplayStatics::ApplyDamage()`，让 `UHealthComponent` 统一处理扣血和死亡事件。

### 8.4 修改 AI

主要文件：

- `AIBotPlayerController.h/.cpp`
- `Tank.cpp` 的 AI 移动入口
- `TankPlayerState` 的攻击者记录，如果你需要更准确仇恨来源

注意：

- AI 现在会攻击 `ATank`、`ATower`、`ATurret`。
- 团队模式下 `IsEnemy()` 会过滤同队 Tank。
- 如果新增可攻击目标，要把它加入 `AttackFilterTypes` 或蓝图默认值。

### 8.5 修改碰撞规则

先检查：

- `BattleBlasterCollisionChannels.h`
- `DefaultEngine.ini`
- `BasePawn.cpp`
- `Tank.cpp`
- `Projectile.cpp`
- `TurretProjectile.cpp`
- `TankBuffComponent.cpp`

当前项目最重要的碰撞设计：

- 炮弹互相碰撞是游戏特色。
- 玩家无论是否 Ghost，都应该被炮弹击中。
- 玩家炮弹无论是否 Pierce，都应该击中 Pawn。
- Pierce 只能穿世界，不能穿玩家。

---

## 9. 当前目录结构和维护建议

当前源码已经基本按下面的功能模块结构落地。后续新增文件时，优先放到对应功能目录，不建议再回到按文件类型或随手堆在根目录的方式：

```text
Source/BattleBlaster/
├── BattleBlaster.h/.cpp
├── BattleBlaster.Build.cs
│
├── Core/
│   ├── BattleBlasterCollisionChannels.h
│   ├── BattleBlasterGameInstance.h/.cpp
│   └── Persistence/
│       ├── BattleBlasterSaveGame.h/.cpp
│       └── BattleBlasterHistorySaveGame.h/.cpp
│
├── Shared/
│   ├── Pawns/
│   │   ├── BasePawn.h/.cpp
│   │   ├── Tank.h/.cpp
│   │   └── NPC/
│   │       └── Tower.h/.cpp
│   ├── Combat/
│   │   ├── HealthComponent.h/.cpp
│   │   └── Projectile.h/.cpp
│   ├── Buffs/
│   │   ├── BuffTypes.h
│   │   ├── BuffPickup.h/.cpp
│   │   └── TankBuffComponent.h/.cpp
│   ├── AI/
│   │   ├── AIBotPlayerController.h/.cpp
│   │   └── BotTankController.h/.cpp
│   ├── World/
│   │   ├── DestructibleProp.h/.cpp
│   │   ├── ExplosiveBarrel.h/.cpp
│   │   ├── WoodenCrate.h/.cpp
│   │   ├── RisingGate.h/.cpp
│   │   ├── SlideTrack.h/.cpp
│   │   ├── SpikeTrap.h/.cpp
│   │   └── TeleportPortal.h/.cpp
│   ├── Controllers/
│   │   ├── TankPlayerController.h/.cpp
│   │   └── UIPlayerController.h/.cpp
│   ├── State/
│   │   ├── TankGameState.h/.cpp
│   │   └── TankPlayerState.h/.cpp
│   └── UI/
│       ├── HUDWidget.h/.cpp
│       ├── BulletsWidget.h/.cpp
│       ├── BuffListWidget.h/.cpp
│       ├── BuffSlotWidget.h/.cpp
│       ├── KDAWidget.h/.cpp
│       ├── ScoresDisplayWidget.h/.cpp
│       ├── ScreenMessage.h/.cpp
│       ├── PauseMenuWidget.h/.cpp
│       └── ReturnToSpawnWidget.h/.cpp
│
└── Modes/
    ├── MainMenu/
    │   ├── MainMenuGameMode.h/.cpp
    │   └── UI/
    │       ├── MainMenuWidget.h/.cpp
    │       ├── GameSettingsMenuWidget.h/.cpp
    │       ├── MutiPlayerMenuWidget.h/.cpp
    │       └── SelectMapWidget.h/.cpp
    ├── FreeForAll/
    │   ├── BattleBlasterGameMode.h/.cpp
    │   ├── TankBattleGameState.h/.cpp
    │   ├── TankBattlePlayerState.h/.cpp
    │   └── UI/
    │       ├── MutiBattleMenuWidget.h/.cpp
    │       └── MultiBattleGameOverWidget.h/.cpp
    ├── TeamBattle/
    │   ├── TeamBattleGameMode.h/.cpp
    │   ├── TeamBattleGameState.h/.cpp
    │   ├── TeamBattlePlayerState.h/.cpp
    │   └── UI/
    │       ├── TeamBattleMenuWidget.h/.cpp
    │       └── TeamBattleGameOverWidget.h/.cpp
    ├── MOBA/
    │   ├── TankMOBAGameMode.h/.cpp
    │   ├── TankMOBAGameState.h/.cpp
    │   ├── TankMOBAPlayerState.h/.cpp
    │   ├── Turret.h/.cpp
    │   ├── TurretProjectile.h/.cpp
    │   └── UI/
    │       ├── MOBASetupWidget.h/.cpp
    │       ├── MOBAGameOverWidget.h/.cpp
    │       ├── MOBATopStateUI.h/.cpp
    │       ├── DeathScreenWidget.h/.cpp
    │       └── EliminatedScreenWidget.h/.cpp
    ├── Network/
    │   ├── NetworkGameModeBase.h/.cpp
    │   ├── NetworkGameStateBase.h/.cpp
    │   ├── NetworkPlayerStateBase.h/.cpp
    │   ├── NetworkPlayerControllerBase.h/.cpp
    │   ├── NetworkDeathmatchGameMode.h/.cpp
    │   ├── NetworkDeathmatchGameState.h/.cpp
    │   ├── NetworkTeamDeathmatchGameMode.h/.cpp
    │   ├── NetworkTeamDeathmatchGameState.h/.cpp
    │   ├── NetworkMOBAGameMode.h/.cpp
    │   ├── NetworkMOBAGameState.h/.cpp
    │   ├── NetworkTeamMOBAGameMode.h/.cpp
    │   └── UI/
    │       ├── CppShowScoresWidget.h/.cpp
    │       ├── NetworkTeamScoresWidget.h/.cpp
    │       ├── NetworkMOBAStateWidget.h/.cpp
    │       ├── NetworkDeathmatchGameOverWidget.h/.cpp
    │       └── Menu/
    │           ├── NetworkMenuWidgetBase.h/.cpp
    │           ├── NetworkModeSelectWidget.h/.cpp
    │           ├── LANMenuWidget.h/.cpp
    │           ├── LANHostSettingsWidget.h/.cpp
    │           ├── NetworkMapSelectWidget.h/.cpp
    │           └── LANJoinWidget.h/.cpp
    ├── Stage/
    │   ├── TankStageGameMode.h/.cpp
    │   ├── TankStageGameState.h/.cpp
    │   ├── TankStagePlayerState.h/.cpp
    │   └── UI/
    │       ├── TankStageStartWidget.h/.cpp
    │       ├── TankStageOverWidget.h/.cpp
    │       └── PassWidget.h/.cpp
    ├── Defense/
    │   └── DefenseGameMode.h/.cpp
    └── Test/
        └── TestGameMode.h/.cpp
```

为什么 `Tower` 放 `Shared/Pawns/NPC`，而不是 `Modes/Stage`：

- 代码上 `ATower` 是 `ABasePawn` 派生的 NPC 战斗单位。
- 从继承关系看它仍然属于 Pawn 大类。
- `AAIBotPlayerController` 默认把 `ATower` 当攻击目标。
- `ATurret` 里也显式跳过 `ATower`，说明两者概念不同。
- 当前 Stage 使用它最多，但未来 MOBA / Defense / 测试关也可能复用。

如果你确认 `Tower` 永远只用于单人闯关，也可以放到 `Modes/Stage/NPC`。但不要放到 `Modes/Defense`。

---

## 10. 后续优化顺序建议

### 第一阶段：保持目录稳定，只做小范围迁移

当前目录已经基本整理完。后续如果发现少量文件仍然放错位置，可以按小步迁移处理：

1. 先提交一次当前可工作的版本。
2. 关闭 Unreal Editor。
3. 只移动本轮确认要迁移的 `.h/.cpp` 文件。
4. 全项目更新 `#include`，例如：

```cpp
#include "Tank.h"
```

改为：

```cpp
#include "Shared/Pawns/Tank.h"
```

5. 重新生成 Visual Studio / Rider 工程文件。
6. 编译项目。
7. 打开 Unreal Editor，让蓝图重新加载 C++ 类。
8. 跑一遍受影响模式。

只移动 C++ 文件且不改类名时，蓝图父类一般不会丢，因为反射类名仍然是同一个。

### 第二阶段：抽重复流程，而不是抽大而全继承

自由死斗、团队死斗、MOBA、单人闯关都有一些相似流程：

- 创建 LocalPlayer。
- 查找 P0-P3 PlayerStart。
- Spawn / Possess Tank。
- AI 补位。
- 保存 Buff。
- 复活。
- 黑屏第四视口。

建议优先抽工具函数或服务类，例如：

```text
ULocalMatchFlowService
UPlayerStartResolver
UTankSpawnHelper
```

确认 FreeForAll 接入稳定后，再迁 TeamBattle。MOBA 和 Stage 规则更特殊，最后处理。

### 第三阶段：命名规范化

命名问题可以分批做：

- `Muti` 可以改成 `Multi`。
- `But` 可以改成 `Btn`。
- `TankStage` 是否统一叫 `Stage` 或 `Campaign`。

但重命名 UCLASS 会影响蓝图引用，必须加 Core Redirects。

示例：

```ini
[/Script/Engine.Engine]
+ActiveClassRedirects=(OldClassName="/Script/BattleBlaster.MutiPlayerMenuWidget",NewClassName="/Script/BattleBlaster.MultiPlayerMenuWidget")
```

重命名时不要只改文件名，要同步：

- 文件名
- 类名
- `UCLASS` 生成头文件 include
- `.generated.h`
- 蓝图父类
- Core Redirects

---

## 11. 需要在 Unreal Editor 做什么

这次只是改文档，不需要进编辑器操作。

本轮只是补文档，不需要打开 Unreal Editor，也不需要重新编译。

如果以后继续移动 C++ 文件：

- 移动 C++ 文件前建议关闭 Unreal Editor。
- 移动后需要重新生成项目文件并编译。
- 如果只是移动 `.h/.cpp` 且不改 UCLASS 名，通常不需要在 Content Browser 里修蓝图。
- 如果移动 `.uasset`，必须在 Unreal Editor 的 Content Browser 里移动，然后执行 Fix Up Redirectors。
- 如果重命名反射类、属性、枚举，必须加 Core Redirects，并检查蓝图父类和变量引用。

---

## 12. 快速索引

| 需求 | 优先查看 |
| --- | --- |
| 坦克移动 / 开火 / 开镜 | `Tank.h/.cpp` |
| 基础炮塔 / NPC 公共 Pawn 行为 | `BasePawn.h/.cpp`, `Tower.h/.cpp` |
| 生命、护盾、死亡事件 | `HealthComponent.h/.cpp` |
| 普通炮弹命中逻辑 | `Projectile.h/.cpp` |
| MOBA 防御塔与追踪弹 | `Turret.h/.cpp`, `TurretProjectile.h/.cpp` |
| Buff 效果 | `BuffTypes.h`, `TankBuffComponent.h/.cpp` |
| 地图 Buff 拾取 | `BuffPickup.h/.cpp` |
| 玩家 HUD / 暂停 / 回城 / MOBA 死亡 UI | `TankPlayerController.h/.cpp` |
| AI 行为 | `AIBotPlayerController.h/.cpp` |
| 自由死斗 | `BattleBlasterGameMode`, `TankBattleGameState`, `TankBattlePlayerState` |
| 团队死斗 | `TeamBattleGameMode`, `TeamBattleGameState`, `TeamBattlePlayerState` |
| MOBA | `TankMOBAGameMode`, `TankMOBAGameState`, `TankMOBAPlayerState` |
| 单人闯关 | `TankStageGameMode`, `TankStageGameState`, `TankStagePlayerState` |
| 可破坏物 | `DestructibleProp`, `ExplosiveBarrel`, `WoodenCrate` |
| 地图机关 | `RisingGate`, `SlideTrack`, `SpikeTrap`, `TeleportPortal` |
| 菜单入口 | `MainMenuGameMode`, `MainMenuWidget`, `MutiPlayerMenuWidget`, `SelectMapWidget` |
| 全局配置 / 存档 / 手柄映射 | `BattleBlasterGameInstance`, `BattleBlasterSaveGame`, `BattleBlasterHistorySaveGame` |

---

## 13. 当前最值得优先清理的问题

1. **本地玩法 Spawn / Respawn / GameOver 流程重复**
   FreeForAll、TeamBattle、MOBA、Stage 都有自己的生成玩家、复活、无敌、结算和返回菜单流程。建议先抽小服务或工具函数，不急着做一个巨大基类。

2. **`AAIBotPlayerController` 职责过重**
   AI 感知、目标列表、威胁评估、移动、闪避、瞄准、射击都在一个类里。后续可以拆成 Target Selector、Tactical Movement、Aim/Fire 三块。

3. **`UBattleBlasterGameInstance` 职责过多**
   当前同时承担本地人数、坦克选择、关卡进度、存档、历史记录、设备映射、网络菜单设置等工作。建议后续分成 Settings、Campaign、History、InputDevice、Network Session 这些更小的 Subsystem 或辅助类。

4. **UI 数据源需要收口**
   部分 GameOver / Score UI 仍会自己扫描 Actor 或 PlayerState。长期更好的方式是由 GameState / PlayerState 提供已整理好的展示数据，Widget 只负责读数据和渲染。

5. **地图和模式参数数据化**
   本地地图选择、网络地图选择、PlayerStart Tag、目标分数、复活时间等仍有硬编码痕迹。建议逐步用 DataAsset 或统一配置结构管理。

6. **文档与代码同步**
   新增或迁移模块时，同时更新本文、`Docs/05-code-optimization-todo.md` 和对应的 `Docs/Modules/*.md`。
