# BattleBlaster 开发者指南

> 基于当前代码库整理：2026-05-15
> 目标：给继续开发本项目的人一张真实的代码地图，同时给后续按功能模块整理 `Source/BattleBlaster` 提供迁移建议。

---

## 0. 先说结论

当前项目的 `Source/BattleBlaster` 目录确实已经变成了一个扁平的大目录，但它不是“改不了了”。真正需要谨慎的是两件事：

1. **不要第一步就重命名 UCLASS / USTRUCT / UENUM 名字。** 只移动 `.h/.cpp` 文件并更新 `#include`，风险比重命名类小得多。
2. **不要把代码文件分类成 Header/Source/UI 这种文件类型结构。** 这个项目更适合按玩法功能模块分类：开发 MOBA 时，MOBA 的 GameMode、GameState、PlayerState、UI、Turret 应该靠在一起。

特别纠正一个容易误判的点：

- `ATower` 是游戏里的 NPC / 敌方塔楼，继承自 `ABasePawn`，当前主要被 `TankStageGameMode` 和 AI 目标系统使用。它不是 Defense 模式代码。
- `ATurret` 才是 MOBA 防御塔，继承自 `ADestructibleProp`，由 `TankMOBAGameState` 注册和管理。
- `ADefenseGameMode` 当前还没有实际玩法代码，只有空类。

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

`RecordAttacker(ATank* Attacker)` 会把最近攻击者放到队头。`ProcessDeath()` 中：

- 自己死亡数 +1。
- 队头攻击者算 Killer。
- 其他有效攻击者算 Assist。
- Killer 触发 `HandleKillConfirmed()` 和 `HandleKillReward()`。
- 刷新相关真人玩家 KDA UI。
- 广播 `DeadTank->OnKilled` 给 GameMode。

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
CheckGameOver()
  -> 只剩 1 个核心塔存活
  -> 除获胜阵营外，其他玩家都 IsEliminated()
  -> SetGameOver(true)
  -> SetWinningCampIndex()
  -> 延迟显示 MOBAGameOverWidget
```

`TankMOBAGameState::OnTurretDestroyed()` 会更新塔数量，但当前设计不是核心塔一倒就直接结束。

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

## 9. 推荐目录分类

第一阶段建议只移动文件，不重命名类。推荐结构：

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

## 10. 迁移顺序建议

### 第一阶段：只整理目录，不重命名类

目标：改善查找体验，降低风险。

步骤：

1. 先提交一次当前可工作的版本。
2. 关闭 Unreal Editor。
3. 按上面的目录移动 `.h/.cpp` 文件。
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
8. 跑一遍主菜单、自由死斗、团队死斗、MOBA、单人闯关。

只移动 C++ 文件且不改类名时，蓝图父类一般不会丢，因为反射类名仍然是同一个。

### 第二阶段：抽重复逻辑

自由死斗和团队死斗有很多相似逻辑：

- 创建 LocalPlayer。
- 查找 P0-P3 PlayerStart。
- Spawn / Possess Tank。
- AI 补位。
- 保存 Buff。
- 复活。
- 黑屏第四视口。

可以考虑抽一个内部基类，例如：

```text
ALocalMultiplayerTankGameModeBase
```

但建议在目录整理稳定后再做，不要和移动文件混在同一次修改里。

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

如果以后真的按目录移动 C++ 文件：

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

1. **文件目录扁平化**
   先按模块移动文件，立刻改善开发体验。

2. **多人死斗和团队死斗重复代码**
   目录稳定后，再抽共同基类。

3. **命名不一致**
   `Muti`、`But` 这类命名可以最后处理，因为它们影响蓝图和反射，风险比移动文件更高。

4. **注释编码混乱**
   部分源码注释在当前终端输出中显示乱码。功能不受影响，但后续清理时建议统一文件编码为 UTF-8。

5. **文档与代码同步**
   以后每次新增模式或移动模块，都应该顺手更新本文件的“快速索引”和“推荐目录分类”。
