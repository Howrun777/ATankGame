# BattleBlaster API 参考手册

> 版本: v1.2
> 扫描范围: `Source/BattleBlaster` 下 68 个 `.h` 与 66 个 `.cpp`
> 最后更新: 2026-05-19
> 说明: 本文记录当前 C++ 代码中对玩法开发最重要的类、函数、委托和调用约束。完整签名以源码为准。

## 1. 阅读约定

- `public`: 推荐外部玩法代码直接调用的接口。
- `protected/private`: Unreal 回调、内部流程或 Widget 绑定函数，文档会标明用途，但不建议当作跨模块公共 API 依赖。
- `BlueprintCallable/BlueprintImplementableEvent`: 蓝图可直接调用或实现的接口。
- 当前项目仍是扁平 `Source/BattleBlaster` 结构，本文按功能模块归档。
- 若本文和代码冲突，以代码为准，并优先更新本文。

## 2. 全局与核心类型

### 2.1 `BattleBlasterCollisionChannels.h`

统一定义项目自定义碰撞通道。

| 名称 | 值 | 用途 |
| --- | --- | --- |
| `BB_COLLISION_PROJECTILE` | `ECC_GameTraceChannel1` | 炮弹专用通道，需与 `DefaultEngine.ini` 中的 `Projectile` 通道保持一致 |

约束:

- Pawn、Tank、Tower、Turret、DestructibleProp 需要 Block Projectile。
- Ghost Buff 不能把 Projectile 改成 Overlap。
- Pierce Buff 只应忽略世界障碍，不应忽略 Pawn。

### 2.2 `UBattleBlasterGameInstance`

跨地图运行数据中心，负责模式配置、坦克选择、Stage 进度、多人历史战绩、玩家携带状态和输入设备映射。

常用字段:

| 字段 | 类型 | 用途 |
| --- | --- | --- |
| `TargetPlayerCount` | `int32` | 目标玩家数量 |
| `TargetMatchScore` | `int32` | 多人模式目标分 |
| `SelectedTankClasses` | `TArray<TSubclassOf<APawn>>` | 每个玩家选择的坦克类 |
| `ConnectedGamepadCount` | `int32` | 已连接手柄数量 |
| `AIControlledPlayerIndices` | `TArray<int32>` | 由 AI 接管的玩家槽位 |
| `CampaignLevelNames` | `TArray<FName>` | Stage 关卡名列表 |
| `CurrentLevelIndex` / `BestLevelRecord` | `int32` | 当前关卡与历史最高关卡 |
| `DifficultyCoefficientK` | `float` | Stage 难度增长系数 |
| `PlayerCarryState` | `FPlayerCarryState` | 跨关卡携带状态 |
| `MultiBattleHistory` | `TArray<FMultiBattleHistoryEntry>` | 多人死斗历史战绩 |

常用接口:

| 函数 | 用途 |
| --- | --- |
| `LoadGameData()` / `SaveGameData()` | 读取/保存单人进度 |
| `GetRandomLevelName()` | 获取随机 Stage 关卡名 |
| `LoadNextLevel(...)` / `RestartCurrentLevel(...)` / `RestartGame(...)` | Stage 关卡流转 |
| `ResetCurrentLevel()` | 重置当前关卡为 1 |
| `GetCurrentLevelIndex()` / `GetBestLevelRecord()` | 读取当前/最高关卡 |
| `ResetCampaignTimer()` / `MarkCampaignLevelStart(UWorld*)` / `MarkCampaignLevelEnd(UWorld*)` | 管理战役计时 |
| `GetCampaignTotalTime(UWorld*)` | 获取累计战役时间 |
| `AddMultiBattleHistoryRecord(...)` | 添加一条多人死斗历史 |
| `AddMultiBattleHistoryRecordsFromMatch(...)` | 从整局数据批量写入历史 |
| `GetMultiBattleHistory()` | 读取多人死斗历史 |
| `LoadMultiBattleHistory()` / `SaveMultiBattleHistory()` | 读取/保存多人死斗历史 |
| `GetDifficultyMultiplier(int32)` / `GetCurrentDifficultyMultiplier()` | 读取 Stage 难度倍率 |
| `SetDifficultyCoefficientK(float)` / `GetDifficultyCoefficientK()` | 设置/读取难度系数 |
| `GetPlayerDeathCount()` / `IncrementPlayerDeathCount()` / `ResetPlayerDeathCount()` | 管理 Stage 死亡次数 |
| `GetRemainingLives()` / `GetMaxDeathCount()` | 读取剩余生命和最大死亡次数 |
| `SavePlayerCarryState(...)` / `GetPlayerCarryState()` | 保存/读取跨关卡携带状态 |
| `ResetPlayerCarryState()` / `HasPlayerCarryState()` | 重置/检查携带状态 |
| `SetReturnToMenuType(EReturnToMenuType)` / `GetReturnToMenuType()` | 设置/读取返回菜单类型 |
| `GetConnectedGamepadCountWithMapping(...)` / `GetConnectedGamepadCount(...)` | 检测手柄数量 |
| `GetPlayerDeviceId(int32)` / `RegisterPlayerDeviceMapping(...)` | 读取/注册玩家设备映射 |
| `ResetDeviceMappings()` | 清空设备映射 |

使用约束:

- GameMode BeginPlay 通常从 GameInstance 读取配置。
- Widget 菜单只应写入配置，不应直接生成战斗对象。
- 跨地图携带状态应通过 GameInstance 明确保存和清除，避免复用脏数据。

### 2.3 SaveGame 类型

| 类型 | 关键字段 | 用途 |
| --- | --- | --- |
| `UBattleBlasterSaveGame` | `CurrentLevelIndex`, `BestLevelRecord` | 保存单人战役进度 |
| `UBattleBlasterHistorySaveGame` | `MultiBattleHistory`, `NextSequenceId` | 保存多人死斗历史战绩池 |

## 3. Pawn 与战斗

### 3.1 `UHealthComponent`

统一生命/护盾组件，负责伤害处理、生命变化广播和死亡广播。

重要字段:

| 字段 | 默认/用途 |
| --- | --- |
| `MaxHealth` | 最大生命值 |
| `CurrentHealth` | 当前生命值，复制给客户端 |
| `MaxShield` | 最大护盾值 |
| `CurrentShield` | 当前护盾值，复制给客户端 |

委托:

| 委托 | 用途 |
| --- | --- |
| `OnHealthChanged` | 生命或护盾变化时广播 |
| `OnDeath` | 生命归零时广播 |

常用接口:

| 函数 | 用途 |
| --- | --- |
| `OnDamageTaken(...)` | protected 伤害回调，绑定到 Owner 的 `OnTakeAnyDamage` |
| `Heal(float)` | 恢复生命 |
| `AddShield(float)` | 增加护盾 |
| `ResetHealth()` | 恢复满生命并清空护盾 |
| `GetHealthPercent()` | 获取生命百分比 |
| `GetShieldPercent()` | 获取护盾百分比 |
| `UpdateHUD()` | 广播当前生命/护盾，具体 UI 更新由监听者完成 |

注意:

- `UpdateHUD()` 不直接调用 `ATankPlayerController`，它只广播生命变化。
- 模式胜负不写在 HealthComponent 中，由 GameMode/PlayerState 处理。
- 网络模式下生命和护盾由服务器修改，客户端通过复制值和 `OnRep_HealthState()` 刷新表现。

### 3.2 `ABasePawn`

坦克、Tower 等可战斗 Pawn 的基础类。

组件:

| 组件 | 用途 |
| --- | --- |
| `CapsuleComp` | 根碰撞，ObjectType 为 Pawn，阻挡 Projectile |
| `BaseMesh` | 车身/底座 |
| `TurretMesh` | 炮塔 |
| `ProjectileSpawnPoint` | 炮弹生成点 |
| `HealthComp` | 生命组件 |

常用接口:

| 函数 | 用途 |
| --- | --- |
| `Fire()` | 生成基础炮弹 |
| `RotateTurret(FVector LookAtTarget)` | 朝目标点旋转炮塔 |
| `HandleDestruction()` | 播放死亡表现和通用清理 |

使用约束:

- 子类覆写 `Fire()` 时要保留碰撞 Owner/Instigator 语义。
- 子类覆写 `HandleDestruction()` 时应决定是否调用 Super，以及调用顺序。

### 3.3 `ATank`

玩家与 AI 坦克主体，继承自 `ABasePawn`。

关键组件:

| 组件 | 用途 |
| --- | --- |
| `SpringArmComp` | 第三人称相机臂 |
| `CameraComp` | 普通视角相机 |
| `ScopeCameraComp` | 开镜/瞄准相机 |
| `PawnMovementComponent` | `UFloatingPawnMovement` |
| `BuffComp` | Buff 状态组件 |

重要字段:

| 字段 | 用途 |
| --- | --- |
| `SlotId` | 比赛槽位 |
| `CurrentAmmo` / `MaxAmmo` | 当前/最大弹药 |
| `Speed` / `TurnRate` | 移动与转向参数 |
| `bIsAiming` | 当前是否瞄准 |
| `CurrentAmmo` / `MaxAmmo` | 当前/最大弹药 |
| `CachedKiller` | 最近一次死亡的击杀者缓存 |

常用接口:

| 函数 | 用途 |
| --- | --- |
| `Fire()` | 玩家开火，处理弹药、Buff、双发、穿墙弹和 UI |
| `MoveInput(const FInputActionValue&)` | 输入移动 |
| `TurnInput(const FInputActionValue&)` | 输入旋转 |
| `TurretTurnInput(const FInputActionValue&)` | 炮塔旋转输入 |
| `OnAimToggle(...)` / `OnAimHoldStarted(...)` / `OnAimHoldCompleted(...)` | 瞄准控制 |
| `UpdateAimView()` | 切换相机和瞄准 UI |
| `SetSlotId(int32)` / `GetSlotId()` | 设置/读取比赛槽位 |
| `SetAmmo(int32)` / `GetAmmo()` | 设置/读取弹药 |
| `GetIsAlive()` / `SetIsAlive(bool)` | 读取/同步存活状态 |
| `SetPlayerEnabled(bool)` | 启用/禁用玩家控制 |
| `HandleKillReward()` | 击杀奖励，恢复弹药和生命 |
| `ExecuteDeathAndReturnKiller()` | 返回并清空缓存的 `CachedKiller` |
| `ServerMoveInput(float)` | 网络模式下把前后移动输入发给服务器 |
| `ServerTurnInput(float)` | 网络模式下把车身转向输入发给服务器 |
| `ServerTurretTurnInput(float)` | 网络模式下把炮塔转向输入发给服务器 |
| `ServerFire(FTransform)` | 网络模式下请求服务器开火 |
| `ClientCorrectTankTransform(...)` | 服务器定期校正客户端位置和炮塔朝向 |
| `MulticastHandleDestruction()` | 广播 Tank 死亡表现 |

死亡流程说明:

- `HandleDeath(AActor* Killer)` 缓存 Killer，执行死亡表现，然后调用 PlayerState 的死亡处理。
- `ATankPlayerState::ProcessDeath()` 会广播死亡坦克自身的 `OnKilled` 委托。
- `ExecuteDeathAndReturnKiller()` 只是读取并清空缓存 Killer，不负责广播 `OnKilled`。

Buff 使用建议:

- 外部玩法代码优先通过 `UTankBuffComponent::AddBuff(...)` 添加 Buff。
- `ATank` 自身保存弹药、速度、双发、穿墙、Ghost 等运行时状态，但没有公开一组统一的 `SetXXXBuffEnabled` API。

网络同步备注:

- `Tank` 是当前 Pawn 生命周期内的战斗实体，负责移动、转向、开火 RPC 和死亡表现。
- 玩家身份、队伍、KDA、跨 Pawn 弹药显示以 `ATankPlayerState` 为权威来源。
- 开火时只有服务器成功生成 Projectile 后才扣弹药，避免客户端出现扣弹药但没有子弹的情况。

### 3.4 `AProjectile`

普通炮弹，供玩家、AI 和 Tower 使用。

关键字段:

| 字段 | 默认/用途 |
| --- | --- |
| `Damage` | 基础伤害 |
| `ProjectileMovement` | 炮弹移动组件 |
| `bBoostVisualsEnabled` | 是否启用强化炮弹表现，复制给客户端 |
| `bCanPierce` | 是否穿墙，复制给客户端 |
| `MaxPenetrationCount` | 穿透计数上限，复制给客户端 |

常用接口:

| 函数 | 用途 |
| --- | --- |
| `OnHit(...)` | 命中回调，处理友伤、Pawn、可破坏物和世界碰撞 |
| `EnableBoostVisuals()` | 启用强化炮弹视觉表现 |
| `EnablePierceMode(bool bEnable, int32 MaxPierceCount = -1)` | 开关穿墙弹 |
| `SetProjectileLifeSpan(float)` | 设置炮弹生命周期 |

碰撞规则:

- 默认 ObjectType 为 Projectile。
- 默认阻挡 Pawn 和 Projectile。
- Pierce 开启后忽略 WorldStatic、WorldDynamic、PhysicsBody。
- Pierce 开启后仍阻挡 Pawn，因此 Ghost 状态玩家仍会被击中。
- 命中 Pawn 或 `ADestructibleProp` 后会造成伤害并销毁。
- 命中普通世界障碍时，非 Pierce 炮弹销毁，Pierce 炮弹通常不会产生阻挡命中。
- 炮弹和炮弹相撞抵消是设计特色，应保留。
- 网络模式下 Projectile 由服务器生成并判定命中，Actor 和移动组件复制给客户端；命中特效通过 `MulticastPlayHitEffects(...)` 播放。

### 3.5 `ATurretProjectile`

MOBA Turret 使用的追踪炮弹。

常用接口:

| 函数 | 用途 |
| --- | --- |
| `InitializeProjectile(AActor*, float, float, int32)` | 设置目标、伤害、速度和阵营 |
| `GetDamage()` | 获取伤害 |
| `GetCampIndex()` | 获取阵营索引 |
| `Tick(float)` | protected，持续朝目标更新方向 |
| `OnHit(...)` | protected，命中后造成伤害并销毁 |

使用约束:

- 主要由 `ATurret::FireProjectile()` 生成。
- 应继续使用 Projectile 通道，保证能命中 Ghost 状态坦克。

### 3.6 `ATower`

PVE/NPC 敌方单位，继承自 `ABasePawn`。注意它不是 MOBA Turret，也不是当前 Defense 模式代码。

关键字段:

| 字段 | 用途 |
| --- | --- |
| `DetectionSphere` | 目标检测范围 |
| `TargetsInRange` | 当前范围内坦克列表 |
| `FireRange` | 开火距离 |
| `FireRate` | 开火间隔 |
| `RespawnTotalTime` | 非 Stage 模式复活时间 |
| `bIsDead` | Tower 死亡状态，复制给客户端 |
| `ReplicatedTurretRotation` | Tower 炮塔朝向，复制给客户端 |

常用接口:

| 函数 | 用途 |
| --- | --- |
| `GetTargetInRange()` | 返回范围内最近的存活坦克 |
| `Fire()` | 向当前目标发射 `AProjectile` |
| `IsTargetBlocked(ATank*)` | 使用 Visibility 射线判断目标是否被遮挡 |
| `HandleDestruction()` | 清理目标、隐藏禁用、按模式决定是否复活 |
| `ApplyDifficultyMultiplier(float)` | 按难度调整生命、射程和射速 |
| `GetCurrentDifficultyMultiplier()` | 获取当前难度倍率 |

回调:

| 函数 | 用途 |
| --- | --- |
| `OnDetectionSphereBeginOverlap(...)` | 记录进入范围的坦克 |
| `OnDetectionSphereEndOverlap(...)` | 移除离开范围的坦克 |
| `HandleTowerHealthChanged(...)` | 更新血条 |
| `HandleTowerDeath(UHealthComponent*, AController*, AActor*)` | 处理死亡和击杀奖励 |
| `ReviveTower()` / `StopRespawnEffect()` / `SetTowerState(bool)` | private 复活与启停辅助流程 |

注意:

- `GetTargetInRange()` 只负责选择最近存活目标，不负责遮挡过滤。
- 遮挡判断在 Tick 中通过 `IsTargetBlocked()` 决定是否开火。
- Stage 模式中 Tower 死亡后禁用复活。
- 网络模式下 Tower AI、开火、死亡只在服务器执行；客户端通过复制的死亡状态和炮塔朝向显示结果。

## 4. Buff 与拾取物

### 4.1 `EBuffType`

定义在 `BuffTypes.h`。

| 枚举 | 用途 |
| --- | --- |
| `Heal` | 恢复生命 |
| `Ammo` | 无限弹药窗口 |
| `Speed` | 加速 |
| `Pierce` | 穿墙弹 |
| `Ghost` | 穿越世界几何 |
| `Damage` | 伤害增强 |
| `DoubleShot` | 双发 |
| `Shield` | 护盾 |
| `RandomIcon` | 随机图标 |

### 4.2 `UTankBuffComponent`

坦克 Buff 状态管理组件。

常用接口:

| 函数 | 用途 |
| --- | --- |
| `AddBuff(EBuffType, float, UTexture2D*)` | 添加 Buff |
| `GetActiveBuffsForUI()` | 获取当前 UI 可显示 Buff |
| `GetAllActiveBuffs()` | 获取所有持续 Buff，常用于复活恢复 |
| `RestoreBuffs(...)` | 恢复 Buff 状态 |
| `ClearAllBuffs()` | 清空 Buff |
| `IsStuckInGeometry()` | Ghost 结束后检测是否卡在几何体内 |
| `OnEscapedFromGeometry()` | 逃离几何体后结束窒息/卡墙状态 |
| `GetSuffocationRemainingTime()` | 获取窒息剩余时间 |
| `IsInSuffocation()` | 查询是否处于窒息状态 |

重要行为:

- Ammo Buff 期间 UI 显示 9999，真实弹药在 Buff 结束后恢复。
- Speed Buff 后 10 秒会进入衰减逻辑。
- Ghost Buff 将世界几何改为 Overlap，但 Projectile 保持 Block。
- Ghost Buff 会检测是否卡在几何体内，必要时做防卡处理。
- Pierce Buff 通过 Tank 开火时传递给生成的 Projectile。
- 网络模式下 `ReplicatedActiveBuffs` 负责同步给客户端 UI；Buff 的真实添加、移除和效果结算应由服务器执行。

### 4.3 `ABuffPickup`

场景 Buff 拾取物。

常用接口:

| 函数 | 用途 |
| --- | --- |
| `OnOverlapBegin(...)` | private，坦克拾取 Buff |
| `InitializeRandomBuff()` | private，随机初始化 Buff 外观 |
| `HideAndStartRespawnTimer()` | private，隐藏并启动重生计时 |
| `RespawnBuff()` | private，重新随机外观并恢复显示 |

约束:

- Stage 模式中拾取后不自动重生。
- `RandomIcon` 会在拾取时映射成实际 Buff。
- 网络模式下 `CurrentVisualType` 和 `bIsPickupAvailable` 复制给客户端，保证刷出的 Buff 类型和可拾取状态一致。

## 5. AI

### 5.1 `AAIBotPlayerController`

普通 AI 坦克控制器。

关键枚举:

| 枚举 | 用途 |
| --- | --- |
| `EAIBotState` | Idle、Searching、Chasing、Attacking、Retreating、Dead |
| `EAIDifficulty` | Easy、Normal、Hard |

常用接口:

| 函数 | 用途 |
| --- | --- |
| `ApplyDifficultySettings(EAIDifficulty)` | 应用难度参数 |
| `AddTargetToAttackList(AActor*)` | 加入攻击目标列表 |
| `RemoveTargetFromAttackList(AActor*)` | 移除攻击目标 |
| `OnAttackedBy(AActor*)` | 被攻击后响应攻击者 |
| `SelectNearestTargetFromList()` | 从目标列表中选择最近目标 |
| `SetTarget(AActor*)` | 设置当前目标 |
| `StopChasing()` | 停止追击 |
| `ResetAIState()` | 重置 AI 状态 |

目标类型:

- `ATank`
- `ATower`
- `ATurret`

### 5.2 `ABotTankController`

保留/辅助 Bot 控制器类型。若后续继续使用，建议明确它与 `AAIBotPlayerController` 的职责边界，避免双套 AI 控制逻辑并行演化。

## 6. 世界对象

### 6.1 `ADestructibleProp`

可破坏对象基类。

关键字段:

| 字段 | 用途 |
| --- | --- |
| `DetectionRadius` | 血条显示检测半径 |
| `LastAttacker` | 最近攻击者 |
| `HealthBarWidget` | 世界空间血条 |
| `bIsDestroyed` | 是否已被破坏，复制给客户端 |
| `ReplicatedPropMeshTransform` | 可推动 Mesh 的同步 Transform |

常用接口:

| 函数 | 用途 |
| --- | --- |
| `TakeDamage(...)` | 扣血并记录攻击者 |
| `HandleDestruction()` | 死亡表现和清理 |
| `SetHealthBarVisibility(bool)` | 控制血条可见性 |
| `UpdateHealthBar()` | 更新血条数值 |
| `CheckPlayerDistance()` | 检查本地所有玩家距离 |

注意:

- `CheckPlayerDistance()` 应遍历所有 PlayerController。
- 本地分屏下，只检查 Player 0 会导致其他玩家看不到血条。
- 网络模式下破坏状态由 `bIsDestroyed` 复制；油桶、木箱这类可推动物体的渲染位置通过 `ReplicatedPropMeshTransform` 平滑同步。

### 6.2 `ATurret`

MOBA 防御塔，继承自 `ADestructibleProp`。

关键字段:

| 字段 | 用途 |
| --- | --- |
| `CampIndex` | 所属阵营 |
| `bIsCoreTurret` | 是否核心塔 |
| `VisionRadius` | 搜敌半径 |
| `AttackInterval` | 攻击间隔 |
| `ProjectileSpeed` | 炮弹速度 |
| `AttackDamage` | 攻击伤害 |
| `HealPercent` | 同阵营治疗比例 |

常用接口:

| 函数 | 用途 |
| --- | --- |
| `StartAttacking()` / `StopAttacking()` | 启动/停止攻击循环 |
| `DetectAndAttack()` | 搜索敌方坦克并攻击 |
| `FireProjectile()` | 向当前目标发射 `ATurretProjectile` |
| `SetTarget(AActor*)` | 设置当前目标 |
| `CanAttackTarget(AActor*)` | 判断目标是否可攻击 |
| `ShouldAttackTarget(AActor*)` | 判断是否应攻击目标 |
| `GetMuzzleLocation()` | 获取发射点位置 |
| `IsDamageImmune()` | 判断核心塔是否因外围塔存在而免疫 |
| `UpdateDamageImmunity()` | 更新伤害免疫状态 |
| `TakeDamage(...)` | 处理免疫、同阵营治疗和伤害 |
| `HandleDestruction()` | 通知 MOBA GameState 并切换废墟表现 |
| `ToggleVisionRange()` / `SetVisionRangeVisible(bool)` | 切换/设置攻击范围显示 |

约束:

- Turret 不应攻击 `ATower`。
- 同阵营 Tank 攻击 Turret 是治疗。
- 核心 Turret 在同阵营外围 Turret 存活时免疫。

### 6.3 `AExplosiveBarrel`

爆炸桶，继承自 `ADestructibleProp`。

关键字段:

| 字段 | 默认/用途 |
| --- | --- |
| `ExplosionDamage` | 爆炸中心伤害 |
| `ExplosionRadius` | 爆炸半径 |
| `DamageFalloff` | 衰减 |

常用接口:

| 函数 | 用途 |
| --- | --- |
| `HandleDestruction()` | 触发爆炸表现和范围伤害 |

### 6.4 `AWoodenCrate`

木箱，继承自 `ADestructibleProp`。

常用接口:

| 函数 | 用途 |
| --- | --- |
| `HandleDestruction()` | 播放破坏效果，延迟销毁 |

### 6.5 `ASpikeTrap`

地刺陷阱。

关键字段:

| 字段 | 用途 |
| --- | --- |
| `DetectionSphere` | 激活检测范围 |
| `DamageAmount` | 每次伤害 |
| `HiddenDuration` / `ActiveDuration` | 隐藏/激活持续时间 |
| `ThrustSpeed` / `RetractSpeed` | 伸出/收回速度 |
| `ReplicatedState` | 复制尖刺状态和服务器状态开始时间 |

关键内部流程:

| 函数 | 用途 |
| --- | --- |
| `OnDetectionBeginOverlap(...)` / `OnDetectionEndOverlap(...)` | 玩家进入/离开检测范围 |
| `CheckInitialOverlaps()` | BeginPlay 后补扫初始重叠 |
| `ChangeState(ESpikeState)` | 切换地刺状态 |
| `OnHiddenTimerExpired()` / `OnActiveTimerExpired()` | 状态计时回调 |
| `DealDamageToActors()` | 对合法目标造成伤害 |
| `OnOverlapBegin(...)` | 激活状态下的伤害触发 |

网络同步备注:

- 服务器只同步尖刺进入哪个状态以及状态开始的服务器时间。
- 客户端根据 `ReplicatedState.StateStartServerTime` 自行计算动画进度，避免每帧同步尖刺位置。

### 6.6 `ASlideTrack`

滑行轨道。

关键字段:

| 字段 | 用途 |
| --- | --- |
| `SpeedMultiplierState1` | 加速倍率 |
| `SpeedMultiplierState2` | 减速倍率 |
| `DetectionSphere` | 玩家接近检测 |

关键内部流程:

| 函数 | 用途 |
| --- | --- |
| `OnDetectionBeginOverlap(...)` / `OnDetectionEndOverlap(...)` | 玩家进入/离开休眠检测范围 |
| `OnOverlapBegin(...)` | 坦克进入轨道 |
| `OnOverlapEnd(...)` | 坦克离开轨道 |
| `ApplySpeedEffect(ATank*, float)` | 应用速度效果 |
| `RemoveSpeedEffect(ATank*)` | 移除速度效果 |
| `SwitchToNextState()` | 切换轨道状态 |
| `UpdateMeshAndMaterial()` | 更新轨道外观 |

注意:

- 多条轨道重叠时，需要用计数避免提前恢复速度。

### 6.7 `ATeleportPortal`

传送门。

常用字段:

| 字段 | 用途 |
| --- | --- |
| `PortalPairID` | 配对 ID |
| `CooldownTime` | 传送冷却 |
| `TeleportOffset` | 出口偏移 |

关键内部流程:

| 函数 | 用途 |
| --- | --- |
| `OnOverlapBegin(...)` | 触发传送 |
| `OnOverlapEnd(...)` | 离开触发区后解除忽略 |
| `StartCooldown()` | 开始传送冷却 |
| `ResetCooldown()` | 定时器回调，结束冷却 |

特点:

- 支持传送炮弹和物理对象。
- 会尝试保留或重定向速度。
- 使用忽略列表和冷却避免反复触发。

### 6.8 `ARisingGate`

升降门。

关键内部流程:

| 函数 | 用途 |
| --- | --- |
| `ChangeState(EGateState)` | 切换门状态 |
| `CheckPlayerProximity()` | 检测玩家是否仍在触发范围 |
| `OnOverlapBegin(...)` | 坦克进入触发区，门升起 |
| `OnOverlapEnd(...)` | 坦克离开触发区，门下降 |
| `Tick(float)` | 插值移动门 |

## 7. GameMode

### 7.1 `ABattleBlasterGameMode`

Free For All 模式。

关键职责:

- 从 GameInstance 读取玩家数量、AI 数量、目标分和坦克选择。
- 创建本地玩家。
- 按 PlayerStart 生成玩家和 AI。
- 绑定 Tank 死亡事件。
- 处理积分、复活、胜利和结算。

常用接口:

| 函数 | 用途 |
| --- | --- |
| `BeginPlay()` | 初始化整局 |
| `HandleTankKilled(ATank* DeadTank, ATank* KillerTank)` | 处理死亡、得分和胜负 |
| `RespawnPlayer(int32 SlotId)` | 延迟复活指定玩家 |
| `ShowMultiBattleGameOver()` | 显示多人死斗结算 |
| `OnGameOverTimerTimeOut()` | 游戏结束定时回调 |

### 7.2 `ATeamBattleGameMode`

团队战模式。

关键接口:

| 函数 | 用途 |
| --- | --- |
| `GetPlayerCamp(int32)` | 获取玩家阵营 |
| `IsSameCamp(int32, int32)` | 判断是否同阵营 |
| `CanDealDamage(AController*, AActor*)` | 友军伤害过滤 |
| `AddTeamScore(int32 CampIndex, int32 Amount)` | 增加队伍分 |
| `HandleTankKilled(ATank* DeadTank, ATank* KillerTank)` | 处理死亡、扣分、加分和复活 |
| `RespawnPlayer(int32 SlotId)` | 复活 |

当前阵营:

- P0/P2: Red。
- P1/P3: Blue。

### 7.3 `ATankMOBAGameMode`

MOBA 模式。

关键职责:

- 初始化本地玩家和阵营。
- 生成每个玩家对应 Tank。
- 管理死亡、复活、淘汰。
- 协调 MOBA GameState 和 UI。
- 判断游戏结束。

常用接口:

| 函数 | 用途 |
| --- | --- |
| `BeginPlay()` | 初始化 MOBA 对局 |
| `HandleStartingNewPlayer(APlayerController*)` | 兜底初始化 MOBA PlayerState，不是主要生成入口 |
| `HandleTankKilled(ATank* DeadTank, ATank* KillerTank)` | 处理死亡、复活或淘汰 |
| `StartPlayerRespawn(ATankMOBAPlayerState*)` | 开始复活倒计时 |
| `RespawnPlayer(ATankMOBAPlayerState*)` | 复活指定玩家状态 |
| `HandleCampEliminated(int32)` | 某阵营真正淘汰后触发胜负检查 |
| `CheckGameOverByElimination()` | 判断 MOBA 是否只剩一个未淘汰阵营，当前为 protected 内部流程 |
| `NotifyAllPlayersTowerDestroyed(int32, bool)` | 通知 Turret 摧毁 |
| `GetActiveCampCount()` | 当前实现返回 `ActiveTanks.Num()`，命名上更像活跃槽位数 |
| `HideCoreTurretImage(int32)` | 隐藏指定阵营核心塔 UI 图标 |

当前胜负条件:

- 只剩一个未淘汰阵营。
- 核心 Turret 数量不直接决定胜负，只影响玩家死亡后是否可以复活。
- 满足后写入 GameState 胜利阵营并显示结算。

注意:

- 核心塔被摧毁不会立刻结束游戏。
- 核心塔被摧毁后，玩家下一次死亡才进入淘汰状态。

### 7.4 `ATankStageGameMode`

Stage/PVE 模式。

关键职责:

- 生成单人玩家。
- 扫描并管理当前关卡全部 `ATower`。
- 按难度调整 Tower。
- 处理玩家生命数、死亡、复活、胜利和失败。
- 保存关卡结束状态和历史记录。

常用接口:

| 函数 | 用途 |
| --- | --- |
| `BeginPlay()` | 初始化 Stage |
| `HandleTankKilled(ATank* DeadTank, ATank* KillerTank)` | 处理玩家死亡 |
| `HandleTowerDestroyed(UHealthComponent*, AController*, AActor*)` | 处理 Tower 被摧毁 |
| `RespawnPlayer()` | 玩家复活 |
| `GetPlayerTank()` | 获取当前玩家坦克 |
| `GetMaxDeathCount()` / `GetRemainingLives()` | 获取最大死亡次数和剩余生命 |
| `SavePlayerStateBeforeLevelEnd()` | 保存玩家携带状态，当前为 private 内部流程 |
| `ApplyPlayerCarryState()` | 应用上一关携带状态，当前为 private 内部流程 |

### 7.5 `ADefenseGameMode`

Defense 模式占位类。

当前状态:

- 类存在。
- 玩法规则未实现。
- 不应把 `ATower` 自动归入 Defense 模式。

### 7.6 `AMainMenuGameMode` / `ATestGameMode`

| 类 | 用途 |
| --- | --- |
| `AMainMenuGameMode` | 主菜单地图 GameMode |
| `ATestGameMode` | 测试地图 GameMode |

## 8. GameState 与 PlayerState

### 8.1 `ATankGameState`

多人基础 GameState。

常用接口:

| 函数 | 用途 |
| --- | --- |
| `GetGameStatus()` / `SetGameStatus(EGameStatus)` | 读取/设置游戏状态 |
| `GetMatchTime()` | 获取比赛时间 |
| `GetCountdown()` | 获取倒计时 |
| `IsPlaying()` / `IsEnded()` | 查询当前局状态 |
| `ResetForNewGame()` | 为新对局重置状态 |

### 8.2 `ATankBattleGameState`

Free For All 结算/排行数据。

常用接口:

| 函数 | 用途 |
| --- | --- |
| `InitializePlayerData(int32, int32)` | 初始化玩家数量和目标分 |
| `AddPlayerScore(int32, int32)` | 增加玩家分数 |
| `GetPlayerScore(int32)` | 获取玩家分数 |
| `SetWinner(int32)` / `GetWinnerIndex()` | 设置/获取胜利玩家 |
| `HasWinner()` | 判断是否已有胜者 |
| `SetTargetScore(int32)` / `GetTargetScore()` | 设置/获取目标分 |

### 8.3 `ATeamBattleGameState`

团队战 GameState。

常用接口:

| 函数 | 用途 |
| --- | --- |
| `InitializePlayerData(int32)` | 初始化团队战玩家数据 |
| `AddTeamScore(ETeamCamp, int32)` | 修改队伍分 |
| `GetTeamScore(ETeamCamp)` | 读取指定队伍分 |
| `GetRedTeamScore()` / `GetBlueTeamScore()` | 读取红蓝队分数 |
| `SetWinnerCamp(int32)` / `GetWinnerCampIndex()` | 设置/获取胜利阵营 |
| `HasWinner()` | 判断是否已有胜者 |
| `SetTargetScore(int32)` / `GetTargetScore()` | 设置/获取目标分 |

### 8.4 `ATankMOBAGameState`

MOBA 状态中心。

关键字段:

| 字段 | 用途 |
| --- | --- |
| `TurretCountsByCamp` | 各阵营 Turret 数量 |
| `CoreTurretAlive` | 核心塔存活状态 |
| `WinningCampIndex` | 胜利阵营 |
| `GameStatus` | MOBA 局内状态 |

常用接口:

| 函数 | 用途 |
| --- | --- |
| `RegisterTurret(ATurret*)` | 注册 Turret |
| `OnTurretDestroyed(ATurret*)` | Turret 被摧毁时更新计数和 UI |
| `GetAliveCoreTurretCount()` | 获取存活核心塔数量 |
| `GetAliveCampIndex()` | 获取唯一仍有任意塔存活的阵营；不是胜负判定 |
| `HasAliveTowersByCamp(int32)` | 查询指定阵营是否还有外围塔 |
| `SetGameOver(bool)` / `IsGameOver()` | 设置/读取结束状态 |
| `SetWinningCampIndex(int32)` / `GetWinningCampIndex()` | 设置/读取胜利阵营 |
| `CheckGameOverCondition()` | 废弃兼容入口，当前胜负流程由 GameMode 的淘汰判定触发 |

注意:

- `OnTurretDestroyed()` 不直接结束游戏。
- 当前 MOBA 游戏结束由 `ATankMOBAGameMode::CheckGameOverByElimination()` 在淘汰流程中判断。

### 8.5 `ATankStageGameState`

Stage 状态。

常用接口:

| 函数 | 用途 |
| --- | --- |
| `SetRemainingTowerCount(int32)` / `GetRemainingTowerCount()` | 设置/读取剩余 Tower 数 |
| `DecreaseTowerCount()` | Tower 被摧毁时减少计数 |
| `SetCurrentWave(int32)` / `GetCurrentWave()` | 设置/读取当前波次 |
| `SetVictory(bool)` / `IsVictory()` | 设置/读取胜利状态 |
| `SetCurrentStageId(int32)` / `GetCurrentStageId()` | 设置/读取当前关卡 ID |
| `SetTotalWaves(int32)` / `GetTotalWaves()` | 设置/读取总波次 |

### 8.6 PlayerState

| 类 | 主要职责 |
| --- | --- |
| `ATankPlayerState` | 通用击杀、死亡、助攻、攻击者队列和死亡结算 |
| `ATankBattlePlayerState` | Free For All 数据 |
| `ATeamBattlePlayerState` | 团队战数据和阵营 |
| `ATankMOBAPlayerState` | MOBA 死亡次数、等待复活、淘汰状态、保存 Buff |
| `ATankStagePlayerState` | Stage 玩家状态 |

`ATankPlayerState` 重点:

| 函数 | 用途 |
| --- | --- |
| `RecordAttacker(ATank*)` | 记录最近 Tank 攻击者；同一攻击者重复伤害会移动到队头，并启动 1 秒一次的过期清理 Timer |
| `CleanUpExpiredAttackers()` | 清理超过 7 秒或已经失效的攻击者记录 |
| `StartAttackerCleanupTimer()` / `StopAttackerCleanupTimer()` | 管理仇人队列的 1 秒清理 Timer，队列为空、死亡、重置或 EndPlay 时停止 |
| `ProcessDeath()` | 处理死亡结算，队头算 Killer，其他有效记录算 Assist，返回 Killer，并广播死亡 Tank 的 `OnKilled` |
| `AddKill()` / `AddDeath()` / `AddAssist()` | 修改战绩 |
| `SaveCurrentBuffs(...)` / `GetBuffs()` / `ClearBuffs()` | 保存、读取或清空 Buff |
| `RecordSpawnLocation(...)` | 记录出生点 |
| `UpdateAmmo(int32)` / `GetAmmo()` | 同步弹药 |
| `SetAlive(bool)` | 同步存活状态 |
| `ResetForNewGame()` | 重置状态 |

当前复制字段:

| 字段 | 用途 |
| --- | --- |
| `SlotId` | 本局比赛槽位 |
| `TeamId` | 队伍 / 阵营 ID |
| `IsAlive` | 玩家存活状态 |
| `CurrentAmmo` | 弹药显示和跨 Pawn 保留 |
| `KillCount` / `DeathCount` / `AssistCount` | KDA |

网络模式规则:

- `PlayerState` 保存玩家身份和跨 Pawn 保留的信息。
- Tank 死亡重生后可以换 Pawn，但 PlayerState 中的身份、队伍、KDA 不应丢失。
- `AttackerQueue` 是服务端临时归因数据，用于死亡时推导 Killer / Assist；最终复制给客户端的是 `KillCount`、`DeathCount`、`AssistCount`，不是整条队列。
- 非 Tank 对象造成最终击杀时，`ProcessDeath()` 仍可通过 7 秒内的最近 Tank 攻击记录把击杀归给玩家；如果没有有效记录，则返回 `nullptr`，由具体 GameMode 决定扣分或不计分。
- 不要把 Tank 位置、Tower 炮塔角度、油桶位置这类世界对象状态塞进 PlayerState。

## 9. Controller 与 UI

### 9.1 `ATankPlayerController`

局内玩家 Controller，负责 HUD、震动、暂停、死亡 UI、淘汰 UI 和旁观/回出生点流程。

常用接口:

| 函数 | 用途 |
| --- | --- |
| `UpdateHealthHUD(float HealthPercent, float ShieldPercent)` | 更新生命/护盾百分比 |
| `SetHUDAmmo(int32 Ammo, int32 MaxAmmo)` | 更新弹药 |
| `UpdateKDA()` | 更新战绩 |
| `TriggerFireVibration()` | 开火震动 |
| `TriggerDamageVibration()` | 受击震动 |
| `StopVibration()` | 停止震动 |
| `ShowDeathScreen(float RespawnTime)` | 显示死亡 UI |
| `HideDeathScreen()` | 隐藏死亡 UI |
| `UpdateDeathScreenCountdown(float)` | 更新死亡倒计时 |
| `ShowEliminatedScreen()` / `HideEliminatedScreen()` | 显示/隐藏淘汰 UI |
| `EnterSpectatorMode()` | 进入旁观 |

约束:

- 本地分屏 HUD 应添加到对应玩家屏幕。
- 不要默认只更新 Player 0。
- 当前暂停逻辑主要由 Player 0 控制。

### 9.2 `AUIPlayerController`

菜单/选择界面 Controller，负责 UI 输入模式、设备映射注册和菜单交互基础流程。

注意:

- 它服务菜单和选择流程，不是局内坦克控制器。
- 设备映射应写入 GameInstance，供 GameMode 生成玩家时读取。

### 9.3 通用 HUD Widget

| 类 | 主要职责 |
| --- | --- |
| `UPlayerHUD` | 玩家局内主 HUD |
| `UHealthBar` | 生命/护盾显示 |
| `UAmmoWidget` | 弹药显示 |
| `UBuffListWidget` | Buff 列表 |
| `UKDAWidget` | KDA 显示 |
| `UPassWidget` | Pass/提示 UI |
| `UDeathScreenWidget` | 死亡倒计时 |
| `UEliminatedScreenWidget` | 淘汰提示 |

常用接口:

| 函数 | 用途 |
| --- | --- |
| `UHUDWidget::SetHealthBarPercent(float)` | 设置生命百分比 |
| `UHUDWidget::SetShieldBarPercent(float)` | 设置护盾百分比 |
| `UBulletsWidget::SetAmmoText(int32, int32)` | 设置弹药文本 |
| `UBuffListWidget::InitBuffUI(ATankPlayerController*)` | 绑定所属玩家控制器 |
| `UKDAWidget::UpdateKDA(int32, int32, int32)` | 更新战绩 |

### 9.4 菜单 Widget

| 类 | 主要职责 |
| --- | --- |
| `UMainMenuWidget` | 主菜单 |
| `UGameSettingsMenuWidget` | 局数、玩家数、AI 数、目标分等配置 |
| `UMutiPlayerMenuWidget` | 多人入口菜单 |
| `UMutiBattleMenuWidget` | Free For All 菜单 |
| `UTeamBattleMenuWidget` | Team Battle 菜单 |
| `UMOBASetupWidget` | MOBA 设置菜单 |
| `USelectMapWidget` | 地图选择 |
| `UTankStageStartWidget` | Stage 起始菜单和 Defense 占位入口 |

注意:

- 大量按钮回调是 Widget 内部绑定函数，不建议外部直接依赖。
- Defense 按钮当前只是占位入口，不代表 Defense 玩法已完成。

### 9.5 结算 Widget

| 类 | 主要职责 |
| --- | --- |
| `UMultiBattleGameOverWidget` | Free For All 结算 |
| `UTeamBattleGameOverWidget` | 团队战结算 |
| `UMOBAGameOverWidget` | MOBA 结算 |
| `UTankStageOverWidget` | Stage 结算 |

常用入口:

| 函数 | 用途 |
| --- | --- |
| `UMultiBattleGameOverWidget::InitResultData(int32)` | 初始化 Free For All 结算 |
| `UTeamBattleGameOverWidget::InitResultData(int32)` | 初始化团队战结算 |
| `UMOBAGameOverWidget::InitResultData()` | 从 GameState/GameInstance 读取 MOBA 结算数据 |
| `UTankStageOverWidget::RefreshDisplay(...)` | 刷新 Stage 结算显示 |

### 9.6 MOBA UI

| 类 | 主要职责 |
| --- | --- |
| `UMOBATopStateUI` | MOBA 顶部核心塔/防御塔状态 |
| `UMOBAPlayerStateUI` | 单个玩家/阵营状态 |
| `UMOBATurretStateUI` | 单个 Turret 状态图标 |

常用流程:

- GameMode/GameState 在 Turret 状态变化时更新 UI。
- 核心塔摧毁后隐藏或置灰对应核心塔图标。
- 淘汰、死亡和胜利 UI 由 PlayerController 与 GameMode 协作触发。

## 10. 关键跨类流程

### 10.1 普通炮弹命中流程

1. `ATank::Fire()` 或 `ATower::Fire()` 生成 `AProjectile`。
2. 设置 Owner、Instigator、Damage 和 Buff 状态。
3. 炮弹命中后进入 `AProjectile::OnHit()`。
4. 根据模式过滤友军或同阵营伤害。
5. 对 Pawn 或可破坏物调用 `UGameplayStatics::ApplyDamage()`。
6. 命中有效目标后销毁炮弹。

### 10.2 坦克死亡流程

1. `UHealthComponent` 收到伤害并扣到 0。
2. `OnDeath` 广播。
3. `ATank::HandleDeath()` 缓存 Killer 并执行表现。
4. `ATankPlayerState::ProcessDeath()` 广播死亡 Tank 的 `OnKilled`。
5. 当前 GameMode 的 `HandleTankKilled()` 响应死亡。
6. GameMode 根据模式决定复活、扣分、淘汰或结束游戏。

### 10.3 MOBA Turret 摧毁流程

1. `ATurret::TakeDamage()` 处理免疫/治疗/伤害。
2. 生命归零后进入 `ATurret::HandleDestruction()`。
3. Turret 通知 `ATankMOBAGameState::OnTurretDestroyed()`。
4. GameState 更新核心塔/外围塔计数和 UI。
5. 如果是核心塔，相关玩家后续死亡会被淘汰。
6. GameMode 在淘汰流程中调用 `HandleCampEliminated()` 和 `CheckGameOverByElimination()`。

### 10.4 Stage 关卡胜负流程

1. `ATankStageGameMode::BeginPlay()` 扫描全部 `ATower`。
2. Tower 死亡时触发 `HandleTowerDestroyed()`。
3. 已摧毁 Tower 数达到总数后胜利。
4. 玩家死亡时扣生命并决定复活或失败。
5. GameMode 保存结果和历史，并进入 `UTankStageOverWidget`/返回菜单流程。

## 11. 易错 API 备注

| 场景 | 正确口径 |
| --- | --- |
| 分屏血条 | 遍历所有 PlayerController，不只用 `GetFirstPlayerController()` |
| Ghost 与炮弹 | Ghost 仍 Block Projectile |
| Pierce 与玩家 | Pierce 忽略世界，不忽略 Pawn |
| 炮弹互撞 | 炮弹互相阻挡并抵消是特色 |
| Tower/Turret | `ATower` 是 NPC Pawn，`ATurret` 是 MOBA 防御塔 |
| MOBA 核心塔 | 核心塔摧毁不是即时游戏结束 |
| `ExecuteDeathAndReturnKiller()` | 只返回并清空 Killer，不广播死亡事件 |
| `UpdateHUD()` | HealthComponent 只广播，不直接操作 Controller |
| Defense | 当前只是占位，不要把现有 Tower 代码解释成 Defense 实现 |
