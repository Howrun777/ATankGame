# BattleBlaster API 参考手册

> **扫描范围**：`Source/BattleBlaster/` 下全部 66 个 .h 头文件（不含 Target/、Ops/、BattleBlasterEditor/）  
> **筛选原则**：仅收录核心业务逻辑的公共方法；已省略基础 Getter/Setter、Unreal 原生覆盖函数（BeginPlay / Tick / SetupInputComponent 等）  
> **适用版本**：基于当前代码库（2026-03-25）

---

## 1. 核心单例与全局接口

### 1.1 UBattleBlasterGameInstance — 游戏进程全局单例
**文件**：`BattleBlasterGameInstance.h` · parent: `UGameInstance`

游戏进程中唯一实例，通过 `GetGameInstance()` 随时获取。承担存档、关卡配置、跨关继承、多人设置等全局职责。

#### 存档读写

| 方法 | 说明 |
|---|---|
| `void LoadGameData()` | 从 `UBattleBlasterSaveGame` 加载战役进度（当前关卡、最高关卡）。 |
| `void SaveGameData()` | 将当前战役进度写入磁盘。 |

#### 关卡管理

| 方法 | 说明 |
|---|---|
| `FName GetRandomLevelName() const` | 从 `CampaignLevelNames` 返回一个随机关卡名称，用于首次进入游戏。 |
| `void LoadNextLevel(const FString& Options = TEXT(""))` | 推进到下一个战役关卡。 |
| `void RestartCurrentLevel(const FString& Options = TEXT(""))` | 重置当前关卡（保留进度）。 |
| `void RestartGame(const FString& Options = TEXT(""))` | 从第 1 关重新开始，清除所有状态。 |
| `void ResetCurrentLevel()` | 将关卡序号重置为 1（保留历史最高记录）。 |
| `int32 GetCurrentLevelIndex() const` | 返回当前关卡序号（从 1 开始）。 |
| `int32 GetBestLevelRecord() const` | 返回历史到达的最高关卡序号。 |
| `float GetCampaignStartTime() const` | 返回战役开始时的时间戳。 |

#### 难度系统

| 方法 | 说明 |
|---|---|
| `float GetDifficultyMultiplier(int32 Level) const` | 返回关卡难度系数：`k^(Level-1)`（例：k=1.2，第3关 → 1.44x）。 |
| `float GetCurrentDifficultyMultiplier() const` | 返回当前关卡的难度系数。 |
| `void SetDifficultyCoefficientK(float K)` | 设置难度增长系数 k。 |
| `float GetDifficultyCoefficientK() const` | 获取当前 k 值。 |

#### 战役计时

| 方法 | 说明 |
|---|---|
| `void ResetCampaignTimer()` | 重置并启动战役计时器。 |
| `void MarkCampaignLevelStart(UWorld* World)` | 记录当前关卡开始时刻（每个关卡仅调用一次）。 |
| `void MarkCampaignLevelEnd(UWorld* World)` | 将本关用时累加到 `CampaignAccumulatedTime`（关卡退出时调用）。 |
| `float GetCampaignTotalTime(UWorld* World) const` | 返回累计用时（已通关关卡 + 当前关卡已用时间）。 |

#### 玩家状态（跨关继承）

| 方法 | 说明 |
|---|---|
| `void SavePlayerCarryState(float Health, int32 Ammo, ...)` | 保存玩家当前生命值、弹药和 7 种 Buff（类型、持续时间、图标），用于关卡间继承。 |
| `FPlayerCarryState GetPlayerCarryState() const` | 返回保存的携带状态结构体。 |
| `void ResetPlayerCarryState()` | 清除携带状态（新游戏时调用）。 |
| `bool HasPlayerCarryState() const` | 返回 true 表示有非零状态（用于区分"首次进入"和"重玩当前关"）。 |

#### 玩家生命管理（关卡闯关模式）

| 方法 | 说明 |
|---|---|
| `int32 GetPlayerDeathCount() const` | 返回当前已死亡次数。 |
| `void IncrementPlayerDeathCount()` | 死亡次数 +1。 |
| `int32 GetRemainingLives() const` | 返回剩余生命数 = `MaxDeathCount - PlayerDeathCount`。 |
| `int32 GetMaxDeathCount() const` | 返回最大允许死亡次数。 |
| `void ResetPlayerDeathCount()` | 重置死亡计数器（新游戏时）。 |

#### 多人对战配置

| 方法 | 说明 |
|---|---|
| `void SetReturnToMenuType(EReturnToMenuType Type)` | 设置比赛结束后返回哪个菜单（`MainMenu` / `SinglePlayerMenu` / `MOBASetupMenu`）。 |
| `EReturnToMenuType GetReturnToMenuType() const` | 获取返回菜单类型。 |
| `void SetPendingMainMenuWidgetClass(TSubclassOf<UUserWidget> WidgetClass)` | 设置主菜单加载完成后需要额外创建的 Widget（如 MOBA 流程）。 |
| `TSubclassOf<UUserWidget> GetPendingMainMenuWidgetClass() const` | 获取待创建的 Widget 类。 |
| `void ClearPendingMainMenuWidgetClass()` | 清除待创建的 Widget。 |

#### 历史战绩（多人对战排行榜）

| 方法 | 说明 |
|---|---|
| `int32 AddMultiBattleHistoryRecord(...)` | 添加一条战绩并维护 Top-50 有序列表，返回该记录的排名（未进入 Top-50 返回 -1）。 |
| `TArray<int32> AddMultiBattleHistoryRecordsFromMatch(...)` | 批量添加一场比赛所有玩家的战绩，返回各自排名。 |
| `const TArray<FMultiBattleHistoryEntry>& GetMultiBattleHistory() const` | 返回当前历史战绩列表（最多 50 条）。 |
| `void LoadMultiBattleHistory()` | 从磁盘加载历史战绩。 |
| `void SaveMultiBattleHistory()` | 将历史战绩写入磁盘。 |

---

## 2. 实体与战斗组件 API

### 2.1 UHealthComponent — 生命值、受击与死亡广播
**文件**：`HealthComponent.h` · parent: `UActorComponent`

#### 核心多播委托

| 委托 | 参数 | 触发时机 |
|---|---|---|
| `FOnHealthChangedSignature OnHealthChanged` | `(HealthComp, Health, HealthDelta, DamageType, InstigatedBy, DamageCauser)` | 护盾或生命值发生变化时（伤害、治疗） |
| `FOnDeathSignature OnDeath` | `(HealthComp, InstigatedBy, DamageCauser)` | 生命值降至 0 及以下时 |

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void Heal(float Amount)` | 增加生命值，上限为 `MaxHealth`。 |
| `void AddShield(float Amount)` | 增加护盾值，上限为 `MaxShield`；护盾优先于生命值承受伤害。 |
| `void ResetHealth()` | 重置生命值为 `MaxHealth`，护盾归零。 |
| `float GetHealthPercent() const` | 返回 `CurrentHealth / MaxHealth`（0.0-1.0）。 |
| `float GetShieldPercent() const` | 返回 `CurrentShield / MaxShield`（0.0-1.0）。 |
| `void UpdateHUD()` | 通知绑定的 Tank 刷新 HUD（内部调用 `ATankPlayerController::UpdateHealthHUD`）。 |

---

### 2.2 UTankBuffComponent — Buff 状态管理
**文件**：`TankBuffComponent.h` · parent: `UActorComponent`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void AddBuff(EBuffType BuffType, float Duration, UTexture2D* Icon)` | 添加或续期一个 Buff：一次性 Buff（Heal/Shield）立即生效；持续性 Buff 新建或叠加时间，并在 Tank 上应用效果。 |
| `TArray<FActiveBuffUIInfo> GetActiveBuffsForUI()` | 返回所有激活的持续性 Buff 列表，供 HUD 渲染倒计时。 |
| `void ClearAllBuffs()` | 清除所有 Buff 并撤销其对 Tank 的所有效果（复活/重置时调用）。 |
| `TArray<FActiveBuffUIInfo> GetAllActiveBuffs()` | 返回所有激活 Buff（用于死亡时保存状态）。 |
| `void RestoreBuffs(const TArray<FActiveBuffUIInfo>& SavedBuffs)` | 从保存的数组恢复所有 Buff（复活后调用）。 |
| `bool IsStuckInGeometry()` | 检测坦克是否卡在几何体内（Ghost 模式结束后触发窒息判断）。 |
| `void OnEscapedFromGeometry()` | 坦克逃离卡住位置时调用，清除 Ghost 模式遗留状态。 |
| `float GetSuffocationRemainingTime() const` | 返回窒息倒计时（单位：秒）。 |
| `bool IsInSuffocation() const` | 返回 true 表示当前正处于窒息状态（每秒扣血中）。 |

#### Tick 行为（重要）
`TickComponent` 每帧执行：递减所有 Buff 持续时间 → 最后 10 秒 SpeedBuff 线性衰减速度 → 若 Ghost 结束且卡在墙内 → 触发窒息状态（每秒扣血 + 显示窒息 UI）。

---

### 2.3 ABasePawn — 坦克战斗体系基类
**文件**：`BasePawn.h` · parent: `APawn`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void RotateTurret(FVector LookAtTarget)` | 计算炮塔 Z 轴旋转，使炮管朝向目标世界坐标（仅水平旋转）。 |
| `virtual void Fire()` | 在炮管生成点生成一颗炮弹（使用 `ProjectileClass`）。 |
| `virtual void HandleDestruction()` | 处理销毁逻辑：停止武器音频、播放死亡粒子/音效、触发 CameraShake。 |

---

### 2.4 ATank — 玩家/AI 坦克
**文件**：`Tank.h` · parent: `ABasePawn`

#### 核心多播事件

| 事件 | 参数 | 触发时机 |
|---|---|---|
| `FOnTankDeathSignature OnKilled` `(BlueprintAssignable)` | `ATank* DeadTank, ATank* KillerTank` | Tank 死亡结算完成后；`ABattleBlasterGameMode`、`ATeamBattleGameMode`、`ATankStageGameMode` 均绑定此事件 |

#### 公共方法

**玩家索引与存活状态**：

| 方法 | 说明 |
|---|---|
| `int32 GetPlayerIndex() const` | 从 `ATankPlayerState` 读取 PlayerIndex（0-3）。 |
| `bool GetIsAlive() const` | 返回 Tank 的存活标志（同步自 PlayerState）。 |
| `void SetIsAlive(bool bAlive)` | 同时设置 Tank 和 PlayerState 的存活标志。 |

**出生点管理**：

| 方法 | 说明 |
|---|---|
| `FVector GetHomeSpawnLocation() const` | 返回记录的出生位置（用于复活/返回出生点）。 |
| `FRotator GetHomeSpawnRotation() const` | 返回记录的出生旋转。 |
| `bool HasSpawnLocation() const` | 检查是否已记录过出生点。 |

**弹药管理**：

| 方法 | 说明 |
|---|---|
| `int32 GetAmmo() const` | 从 PlayerState 读取当前弹药数。 |
| `void SetAmmo(int32 NewAmmo)` | 更新 PlayerState 和 HUD 的弹药显示。 |

**输入 / 瞄准 / 相机**：

| 方法 | 说明 |
|---|---|
| `void MoveInput(const FInputActionValue& Value)` | 处理 WASD/手柄左摇杆的移动输入。 |
| `void TurnInput(const FInputActionValue& Value)` | 处理鼠标 X / 右摇杆的 Tank 整体转向。 |
| `void TurretTurnInput(const FInputActionValue& Value)` | 处理炮塔独立转向输入（Tank 车身不动）。 |
| `void OnAimToggle(const FInputActionValue& Value)` | 键鼠：右键单击切换开镜状态（单击开，松开关）。 |
| `void OnAimHoldStarted(const FInputActionValue& Value)` | 手柄：扳机按下时进入开镜状态。 |
| `void OnAimHoldCompleted(const FInputActionValue& Value)` | 手柄：扳机松开时退出开镜状态。 |
| `void UpdateAimView()` | 切换主相机到炮管相机（或切回第三人称），调整臂长实现放大。 |

**AI 移动**：

| 方法 | 说明 |
|---|---|
| `void MoveAI(const FVector2D& MoveInput)` | 使用 AI 提供的方向向量移动坦克。 |
| `void MoveWithAI(float ForwardInput, float RightInput)` | 使用分离的前后/左右轴值移动（更精细的 AI 行为控制）。 |

**战斗与奖励**：

| 方法 | 说明 |
|---|---|
| `void SetPlayerEnabled(bool Enabled)` | 启用或禁用玩家输入（AI 不受影响）。 |
| `void HandleKillReward()` | 为击杀方坦克增加弹药和生命奖励。 |
| `void NotifyAttacked(AActor* Attacker)` | 记录最后伤害来源（供 `AttackerQueue` 使用）。 |

**死亡处理**：

| 方法 | 说明 |
|---|---|
| `ATank* ExecuteDeathAndReturnKiller()` | 广播 `OnKilled`、标记 Tank 死亡、缓存并返回 KillerTank 引用。 |

**组件访问**：

| 方法 | 说明 |
|---|---|
| `UTankBuffComponent* GetBuffComponent() const` | 返回挂载的 `UTankBuffComponent` 指针。 |
| `ATankPlayerController* GetTankPlayerController() const` | 返回缓存的 PlayerController 引用。 |

---

### 2.5 AProjectile — 玩家炮弹
**文件**：`Projectile.h` · parent: `AActor`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void OnHit(UPrimitiveComponent*, AActor* OtherActor, ..., const FHitResult& Hit)` | 碰撞回调：对命中目标应用伤害、触发命中粒子/音效/CameraShake；处理穿透逻辑。 |
| `void EnableBoostVisuals()` | 切换炮弹外观为强化版（伤害翻倍 Buff 激活时调用）。 |
| `void EnablePierceMode(bool bInfinitePierce = true, int32 InMaxPenetrationCount = -1)` | 启用穿透模式；`InMaxPenetrationCount = -1` 表示无限穿透。 |
| `void SetProjectileLifeSpan(float InLifeSpan)` | 设置炮弹最大存活时间（超时自动销毁，防止子弹永存）。 |

---

### 2.6 ATower — 敌人塔楼（关卡闯关）
**文件**：`Tower.h` · parent: `ABasePawn`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `ATank* GetTargetInRange()` | 在警戒网范围内查找最近的可攻击 Tank（排除障碍物遮挡的目标）。 |
| `bool IsTargetBlocked(ATank* Target)` | 从塔楼向目标做视线检测（LineTrace），判断是否有障碍物遮挡。 |
| `void ApplyDifficultyMultiplier(float Multiplier)` | 应用关卡难度系数（攻速 ÷ 系数，射程和警戒网 × 系数）。 |
| `float GetCurrentDifficultyMultiplier() const` | 获取当前难度系数。 |

---

### 2.7 ATurret — MOBA 防御塔
**文件**：`Turret.h` · parent: `ADestructibleProp`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `bool IsDamageImmune() const` | 返回主塔是否处于伤害免疫状态（有外塔存活时为 true）。 |
| `void UpdateDamageImmunity()` | 根据游戏状态更新免疫标志（主塔受伤时由 GameMode 调用）。 |
| `virtual float TakeDamage(...)` | **重写 UE 原生伤害**：友军命中不造成伤害；主塔免疫窗口内不受伤；友军命中触发治疗（`HealPercent`）。 |
| `void ToggleVisionRange()` | 切换攻击范围球体（Vision Sphere）的可见性。 |
| `void SetVisionRangeVisible(bool bVisible)` | 直接设置范围球体可见性。 |
| `void StartAttacking()` | 启动定时攻击循环（每 `AttackInterval` 秒执行一次 `DetectAndAttack`）。 |
| `void StopAttacking()` | 停止攻击定时器。 |
| `void DetectAndAttack()` | 在视野范围内检测目标，若有合法目标则调用 `FireProjectile`。 |
| `void FireProjectile()` | 在炮口位置生成一颗 `ATurretProjectile`，朝 `CurrentTarget` 飞行。 |
| `FVector GetMuzzleLocation() const` | 返回炮弹生成点的世界坐标。 |
| `void SetTarget(AActor* NewTarget)` | 设置当前攻击目标。 |
| `bool CanAttackTarget(AActor* Target) const` | 检查目标是否在攻击范围内且有效。 |
| `bool ShouldAttackTarget(AActor* Target) const` | 检查目标是否属于敌方阵营（非友军）。 |

---

### 2.8 ATurretProjectile — MOBA 塔的追踪弹
**文件**：`TurretProjectile.h` · parent: `AActor`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void InitializeProjectile(AActor* InTargetActor, float InDamage, float InSpeed, int32 InCampIndex)` | 初始化追踪弹：设置目标、伤害值、飞行速度、阵营索引，激活追踪行为。 |
| `float GetDamage() const` | 返回炮弹伤害值。 |
| `int32 GetCampIndex() const` | 返回炮弹所属阵营索引（用于伤害归属判定）。 |

---

### 2.9 ATeleportPortal — 传送门
**文件**：`TeleportPortal.h` · parent: `AActor`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void OnOverlapBegin(...)` | 玩家进入触发区：检查冷却和配对 ID → 传送到配对门 → 加入免疫列表 → 启动冷却。 |
| `void OnOverlapEnd(...)` | 玩家离开触发区：从免疫列表中移除。 |
| `void StartCooldown()` | 启动传送冷却计时器。 |
| `void ResetCooldown()` | 重置冷却状态，允许再次触发。 |

---

### 2.10 ASlideTrack — 滑动轨道
**文件**：`SlideTrack.h` · parent: `AActor`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void ApplySpeedEffect(ATank* Tank, float SpeedMultiplier)` | 在坦克进入轨道时，将其速度乘以当前状态的倍率。 |
| `void RemoveSpeedEffect(ATank* Tank)` | 坦克离开轨道时，恢复其基础速度。 |
| `void SwitchToNextState()` | 在加速态（4.0×）和减速态（0.5×）之间切换。 |
| `void UpdateMeshAndMaterial()` | 切换轨道的静态网格体和材质，以匹配当前状态。 |
| `float GetCurrentStateDuration() const` | 返回当前状态的剩余持续时间。 |
| `float GetCurrentStateSpeedMultiplier() const` | 返回当前状态的速度倍率。 |

---

## 3. 多模式 GameMode 通用接口

### 3.1 ABattleBlasterGameMode — 自由死斗（BattleBlaster）
**文件**：`BattleBlasterGameMode.h` · parent: `AGameMode`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `ATankBattleGameState* GetTankBattleGameState() const` | 获取本模式的 `ATankBattleGameState` 指针。 |
| `void HandleTankKilled(ATank* DeadTank, ATank* KillerTank)` | **核心结算**：更新 GameState 分数 → 保存死亡方 Buff → 胜负判定 → 刷新比分板 → 启动复活计时器或触发胜利。 |
| `void RespawnPlayer(int32 PlayerIndex)` | 在对应出生点重生玩家：恢复 50% 生命/弹药 → 恢复保存的 Buff → 赋予 3 秒无敌并播放 VFX。 |
| `void EndInvincibility(ATank* Tank, UNiagaraComponent* SpawnedSystem)` | 取消无敌状态并停止复活特效。 |
| `void ShowMultiBattleGameOver()` | 创建并显示 `MultiBattleGameOverWidget`，锁定鼠标到结算界面。 |
| `void OnGameOverTimerTimeOut()` | 游戏结束延迟到期时的回调（可在此扩展自动重开逻辑）。 |
| `void OnCountdownTimerTimeout()` | 开场倒计时递减（`CountdownSeconds--`）；到 0 时解锁玩家输入并开始比赛计时。 |
| `void UpdateMatchTime()` | 每秒 `MatchTimeSeconds++`，刷新比分板计时显示。 |
| `void ApplyBlackScreenToExtraViewports()` | 为多余视口（如 3 人模式的第 4 个视口）贴上纯黑遮罩。 |

---

### 3.2 ATankStageGameMode — 关卡闯关（TankStage）
**文件**：`TankStageGameMode.h` · parent: `AGameMode`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void HandleTankKilled(ATank* DeadTank, ATank* KillerTank)` | 玩家死亡：消耗 1 条生命 → 保存 Buff → 启动复活计时器 → 生命归零时触发游戏结束。 |
| `void HandleTowerDestroyed(UHealthComponent* InHealthComp, AController* InstigatedBy, AActor* DamageCauser)` | 塔楼被摧毁：GameState 剩余塔楼数 -1 → 检查是否全部摧毁 → 通关。 |
| `void RespawnPlayer()` | 在保存的出生点重生玩家，继承跨关状态（Buff、生命、弹药）。 |
| `ATank* GetPlayerTank() const` | 返回唯一的玩家 Tank。 |
| `int32 GetMaxDeathCount() const` | 返回最大允许死亡次数。 |
| `int32 GetRemainingLives() const` | 返回剩余生命数。 |

---

### 3.3 ATankMOBAGameMode — MOBA 对战
**文件**：`TankMOBAGameMode.h` · parent: `AGameMode`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void HandleStartingNewPlayer(APlayerController* NewPlayer)` | 生成玩家坦克、挂载相机、设置输入、记录出生点。 |
| `AActor* GetPlayerStartForIndex(int32 PlayerIndex)` | 按索引查找出生点（Tag: P{PlayerIndex}）。 |
| `void HandleTankKilled(ATank* DeadTank, ATank* KillerTank)` | **死亡判定**：检查目标阵营是否还有主塔 → 若有则进入复活等待；若无则玩家淘汰 → 触发游戏结束检查。**（2026-04-01 修订：只在玩家被淘汰时触发游戏结束检查）** |
| `void CheckGameOver()` | **游戏结束判定**：检查是否只剩 1 个核心塔存活，且除获胜阵营外所有玩家都已淘汰。**（2026-04-01 修订）** |
| `void StartPlayerRespawn(ATankMOBAPlayerState* MOBAState)` | 为等待复活的玩家启动倒计时（延迟随游戏时间递增）。 |
| `void RespawnPlayer(ATankMOBAPlayerState* MOBAState)` | 在阵营出生点重生玩家，恢复部分生命/弹药。 |
| `void EliminatePlayer(ATankMOBAPlayerState* MOBAState)` | 标记玩家永久淘汰 → 隐藏 UI → 切换观战模式。 |
| `void UpdateRespawnTimers(float DeltaTime)` | 每帧递减所有等待玩家的复活计时器，到期自动复活。 |
| `void NotifyAllPlayersTowerDestroyed(int32 CampIndex, bool bIsCoreTurret)` | 通知所有玩家的 PlayerController 更新 MOBA HUD 图标（塔被摧毁）。 |
| `void NotifyAllPlayersCoreDestroyed(int32 CampIndex)` | 通知所有玩家某阵营主塔被摧毁（触发淘汰广播）。 |
| `float GetCurrentGameTime() const` | 返回比赛已进行的秒数。 |
| `int32 GetActiveCampCount() const` | 返回仍有存活的阵营数量（用于胜负判定）。 |
| `void HideCoreTurretImage(int32 CampIndex)` | 通知所有 MOBA HUD 隐藏某阵营的主塔图标（主塔被摧毁时）。 |

---

### 3.4 ATeamBattleGameMode — 团队死斗
**文件**：`TeamBattleGameMode.h` · parent: `AGameMode`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `ATeamBattleGameState* GetTeamBattleGameState() const` | 获取团队战斗 GameState。 |
| `void HandleTankKilled(ATank* DeadTank, ATank* KillerTank)` | 跨阵营击杀 → TeamGameState +1 分 → 保存 Buff → 启动复活 → 刷新团队比分板 → 胜负判定。 |
| `void RespawnPlayer(int32 PlayerIndex)` | 在己方出生点复活，3 秒无敌。 |
| `void EndInvincibility(ATank* Tank, UNiagaraComponent* SpawnedSystem)` | 取消无敌状态。 |
| `void ShowTeamBattleGameOver()` | 显示团队结算 Widget（红色/蓝色阵营胜）。 |
| `void UpdateTeamScoresDisplay()` | 刷新团队比分板（红 vs 蓝）。 |
| `ETeamCamp GetPlayerCamp(int32 PlayerIndex) const` | 返回玩家的阵营（Red=0, Blue=1）。 |
| `TArray<int32> GetPlayersInCamp(ETeamCamp Camp) const` | 返回指定阵营的所有玩家索引。 |
| `bool IsSameCamp(int32 PlayerIndexA, int32 PlayerIndexB) const` | 判断两名玩家是否同阵营。 |
| `void AddTeamScore(int32 CampIndex, int32 Amount)` | 为指定阵营加分（被 PlayerState 的 `HandleKillConfirmed` 调用）。 |
| `bool CanDealDamage(AController* DamageCauser, AActor* DamageVictim) const` | **友军伤害判定**：若双方同阵营则返回 false，炮弹不对友军造成伤害。 |

---

### 3.5 GameState 辅助接口

#### ATankBattleGameState（自由死斗）

| 方法 | 说明 |
|---|---|
| `void InitializePlayerData(int32 InPlayerCount, int32 InTargetScore)` | 初始化 `PlayerScores` 数组（按玩家数分配）。 |
| `void AddPlayerScore(int32 PlayerIndex, int32 Amount = 1)` | 加分并检查是否达到目标分 → 若达到则设置 `WinnerIndex`。 |
| `int32 GetPlayerScore(int32 PlayerIndex) const` | 读取某玩家的当前得分。 |
| `bool HasWinner() const` | 是否已产生胜者。 |
| `virtual void ResetForNewGame()` | 重置所有分数和胜者。 |

#### ATankStageGameState（关卡闯关）

| 方法 | 说明 |
|---|---|
| `void SetRemainingTowerCount(int32 Count)` / `int32 GetRemainingTowerCount() const` | 设置/获取剩余塔楼数。 |
| `void DecreaseTowerCount()` | 塔楼摧毁时调用，剩余数 -1。 |
| `void SetVictory(bool bVictory)` / `bool IsVictory() const` | 胜负标记。 |
| `void SetCurrentWave(int32 Wave)` / `int32 GetCurrentWave() const` | 当前波次（用于多波次关卡）。 |
| `int32 GetCurrentStageId() const` | 当前关卡序号。 |

#### ATankMOBAGameState（MOBA）

| 方法 | 说明 |
|---|---|
| `void RegisterTurret(ATurret* Turret)` | 注册塔楼，更新各阵营内外塔计数。 |
| `void UnregisterTurret(ATurret* Turret)` | 注销塔楼，减少对应计数。 |
| `int32 GetAliveCoreTurretCount() const` | 全地图存活的主塔数量。 |
| `int32 GetAliveCoreTurretCountByCamp(int32 CampIndex) const` | 指定阵营存活的主塔数量。 |
| `int32 GetAliveOuterTurretCountByCamp(int32 CampIndex) const` | 指定阵营存活的外塔数量。 |
| `bool HasAliveTurretsByCamp(int32 CampIndex) const` | 指定阵营是否还有任意塔楼。 |
| `int32 GetAliveCampCount() const` | 仍有塔楼存活的阵营数量。 |
| `int32 GetAliveCampIndex() const` | 返回第一个存活阵营的索引（用于判定胜负）。 |
| `bool IsGameOver() const` | 游戏是否已结束。 |
| `void OnTurretDestroyed(ATurret* DestroyedTurret)` | 塔楼销毁回调：更新计数。**（2026-04-01 修订：不再直接判定游戏结束）** |
| `void CheckGameOverCondition()` | 保留方法，但不再被调用（游戏结束判定已移至 GameMode）。 |

#### ATeamBattleGameState（团队死斗）

| 方法 | 说明 |
|---|---|
| `void AddTeamScore(ETeamCamp Camp, int32 Points)` | 为指定阵营加分。 |
| `int32 GetRedTeamScore() const` / `int32 GetBlueTeamScore() const` | 获取两队比分。 |
| `void SetWinnerCamp(int32 InWinnerCampIndex)` / `int32 GetWinnerCampIndex() const` | 胜负阵营。 |
| `bool HasWinner() const` | 是否已产生胜者。 |

---

### 3.6 PlayerState 核心接口

#### ATankPlayerState（通用玩家状态）

| 方法 | 说明 |
|---|---|
| `void RecordAttacker(ATank* Attacker)` | 将攻击者及其时间戳记录到 `AttackerQueue`（用于 7 秒内仇人追踪）。 |
| `ATank* ProcessDeath()` | 死亡结算核心：取 `AttackerQueue[0]` 作为凶手 → 记录助攻 → 调用子类 `HandleKillConfirmed` → 刷新 KDA UI → 返回凶手 Tank 或 nullptr。 |
| `void AddKill()` / `void AddDeath()` / `void AddAssist()` | 增减击杀/死亡/助攻计数。 |
| `void SaveCurrentBuffs(const TArray<FActiveBuffUIInfo>& Buffs)` | 保存当前激活 Buff（复活恢复用）。 |
| `void ClearBuffs()` | 清除所有 Buff。 |
| `const TArray<FActiveBuffUIInfo>& GetBuffs() const` | 获取当前激活 Buff 列表。 |
| `void RecordSpawnLocation(const FVector& Location, const FRotator& Rotation)` | 记录出生点（复活和返回出生点时使用）。 |
| `void UpdateAmmo(int32 NewAmmo)` | 从 Tank 的射击事件同步弹药数。 |
| `virtual void ResetForNewGame()` | 重置所有状态（KDA、弹药、Buff、出生点）。 |

#### ATankMOBAPlayerState（MOBA 玩家状态）

| 方法 | 说明 |
|---|---|
| `int32 GetCampIndex() const` / `void SetCampIndex(int32 Index)` | 获取/设置玩家所属阵营。 |
| `FLinearColor GetCampColor() const` | 返回阵营对应颜色（用于 UI 着色）。 |
| `bool IsEliminated() const` / `void SetEliminated(bool bEliminated)` | 永久淘汰标记（主塔被摧毁时设为 true）。 |
| `float GetRespawnTimeRemaining() const` / `void SetRespawnTimeRemaining(float Time)` | 复活倒计时。 |
| `float CalculateRespawnDelay(...) const` | 计算复活延迟：`InitialDelay + floor(GameTime / GrowthInterval) * GrowthAmount`，上限 `MaxDelay`（每 30 秒 +1 秒，上限 10 秒）。 |
| `void AddTurretDestroyed()` | 记录摧毁塔楼数（战绩统计用）。 |
| `void InitializeMOBAState(int32 InCampIndex)` | MOBA 模式初始化（设置阵营、死亡状态）。 |

#### ATankStagePlayerState（关卡闯关玩家状态）

| 方法 | 说明 |
|---|---|
| `void UseLife()` | 消耗一条生命（PVE 不记 KDA）。 |
| `int32 GetRemainingLives() const` | 返回剩余生命数。 |
| `void AddStageScore(int32 Points)` | 为当前关卡加分。 |
| `void AddEnemyKill()` | 记录击杀敌人数（包含对父类 `AddKill` 的调用）。 |
| `void SetInvincible(bool bInvincible)` / `bool IsInvincible() const` | 无敌状态（复活后 3 秒）。 |

#### ATeamBattlePlayerState（团队死斗玩家状态）

| 方法 | 说明 |
|---|---|
| `void SetTeamCamp(uint8 InCamp)` / `uint8 GetTeamCamp() const` | 获取/设置阵营（0=红, 1=蓝）。 |
| `void SetInvincible(bool bInvincible)` / `bool IsInvincible() const` | 无敌状态（复活后 3 秒）。 |

---

## 4. UI 核心交互层

### 4.1 UHUDWidget — 血量条
**文件**：`HUDWidget.h` · parent: `UUserWidget`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void SetHealthBarPercent(float NewPercent)` | 将血条 `UProgressBar` 设置为指定百分比（0.0-1.0）。 |
| `void SetShieldBarPercent(float NewPercent)` | 将护盾条设置为指定百分比（护盾值等效于 `MaxShield / MaxHealth`）。 |

---

### 4.2 UBulletsWidget — 弹药计数
**文件**：`BulletsWidget.h` · parent: `UUserWidget`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void SetAmmoText(int32 CurrentAmmo, int32 MaxAmmo)` | 设置弹药文本（例如 `"20 / 40"`）；无限弹药模式下显示 `"9999 / 9999"`。 |

---

### 4.3 UScreenMessage — 屏幕中央消息
**文件**：`ScreenMessage.h` · parent: `UUserWidget`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void SetMessageText(FString Message)` | 设置居中显示的文本（如 `"GO!"`、`"PLAYER 1 WINS!"`）。 |
| `void SetMessageColor(FLinearColor Color)` | 设置文字颜色（红色 = 红队胜利，蓝色 = 蓝队胜利等）。 |

---

### 4.4 UScoresDisplayWidget — 比分板
**文件**：`ScoresDisplayWidget.h` · parent: `UUserWidget`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void InitTargetScore(int32 TargetScore)` | 初始化目标分（如 `/7`），在比分文本后追加 `/7`。 |
| `void SetVisiblePlayerCount(int32 PlayerCount)` | 根据玩家数量显示/隐藏第二行分数格（2 人用一行，3-4 人用两行）。 |
| `void UpdateScores(int32 ScoreP0, int32 ScoreP1)` | 更新 2P 模式的两人比分。 |
| `void UpdateScoresFour(int32 ScoreP0, int32 ScoreP1, int32 ScoreP2, int32 ScoreP3)` | 更新 4P 模式的四人比分。 |
| `void UpdateTeamScores(int32 RedScore, int32 BlueScore)` | 更新团队模式的红蓝比分。 |
| `void UpdateMatchTimer(int32 TimeInSeconds)` | 更新比赛计时器（格式化为 `MM:SS`）。 |

---

### 4.5 UKDAWidget — KDA 显示
**文件**：`KDAWidget.h` · parent: `UUserWidget`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void UpdateKDA(int32 Kills, int32 Deaths, int32 Assists)` | 更新 K/D/A 文本显示。 |
| `void SetColor(FLinearColor Color)` | 设置 KDA 文字颜色（随阵营变色）。 |

---

### 4.6 UBuffListWidget — Buff 列表
**文件**：`BuffListWidget.h` · parent: `UUserWidget`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void UpdateBuffList()` | 每帧调用：遍历 `BuffComp` 的激活 Buff → 更新或移除过期条目 → 刷新每个 `UBuffSlotWidget` 的倒计时。 |

---

### 4.7 UReturnToSpawnWidget — 返回出生点进度条
**文件**：`ReturnToSpawnWidget.h` · parent: `UUserWidget`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void UpdateProgress(float InProgress)` | 设置进度条值（0.0-1.0），玩家按住按键期间逐渐增加。 |

---

### 4.8 UDeathScreenWidget — 死亡复活界面
**文件**：`DeathScreenWidget.h` · parent: `UUserWidget`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void UpdateRespawnCountdown(float TimeRemaining)` | 更新复活倒计时（精确到 0.1 秒）。 |
| `void Show()` | 显示全屏死亡遮罩。 |
| `void Hide()` | 隐藏死亡遮罩。 |

---

### 4.9 UEliminatedScreenWidget — 永久淘汰界面
**文件**：`EliminatedScreenWidget.h` · parent: `UUserWidget`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void Show()` | 显示永久淘汰遮罩（MOBA 模式主塔被摧毁后触发）。 |
| `void Hide()` | 隐藏淘汰界面。 |
| `void OnSwitchSpectateClicked()` | 切换到观战模式。 |

---

### 4.10 UMultiBattleGameOverWidget — 自由死斗结算
**文件**：`MultiBattleGameOverWidget.h` · parent: `UUserWidget`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void InitResultData(int32 InWinnerIndex)` | **核心初始化**：传入胜利者索引 → 从各 PlayerState 读取 KDA → 计算 SkillScore → 构建历史战绩 → 填充所有行 → 写入存档。 |
| `UFUNCTION() void HandleRestartClicked()` | 重新开始本局。 |
| `UFUNCTION() void HandleReturnMenuClicked()` | 返回多人菜单。 |
| `void GetCampInfo(int32 PlayerIndex, FText&, FLinearColor&) const` | 将玩家索引映射为阵营名称（`P0`/`P1` 等）和颜色。 |
| `void GetRowWidgets(...) const` | 获取指定玩家索引对应的 KDA 文本、技能分文本和行容器指针。 |

---

### 4.11 UTeamBattleGameOverWidget — 团队死斗结算
**文件**：`TeamBattleGameOverWidget.h` · parent: `UUserWidget`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void InitResultData(int32 InWinnerCampIndex)` | 传入胜利阵营索引（0=红, 1=蓝）→ 读取 KDA → 计算 SkillScore → 填充行（按阵营分组）。 |
| `UFUNCTION() void HandleRestartClicked()` | 重新开始。 |
| `UFUNCTION() void HandleReturnMenuClicked()` | 返回团队死斗菜单。 |

---

### 4.12 UMOBAGameOverWidget — MOBA 结算
**文件**：`MOBAGameOverWidget.h` · parent: `UUserWidget`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void InitResultData()` | 从 `ATankMOBAGameState` 获取胜者阵营 → 从 GameInstance 获取战绩 → 从 PlayerState 获取 KDA → 填充所有行。 |
| `UFUNCTION() void HandleRestartClicked()` | 重新开始。 |
| `UFUNCTION() void HandleReturnMenuClicked()` | 返回 MOBA 设置菜单。 |

---

### 4.13 UTankStageOverWidget — 关卡结算
**文件**：`TankStageOverWidget.h` · parent: `UUserWidget`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void RefreshDisplay(int32 CurrentLevel, int32 HighestLevel, float GameTimeSeconds, TSubclassOf<ATank> PlayerTankClass)` | **核心填充**：设置当前关卡、历史最高、累计时间，查询坦克肖像贴图。 |
| `UFUNCTION() void OnRestartClicked()` | 重玩本关（保留历史最高）。 |
| `UFUNCTION() void OnReturnMenuClicked()` | 返回单人菜单。 |
| `UTexture2D* GetTankPortrait(TSubclassOf<ATank> TankClass) const` | 从 `TankImageMap` 查找坦克肖像。 |
| `FString FormatGameTime(float TotalSeconds) const` | 将秒数格式化为 `HH:MM:SS`。 |

---

### 4.14 UMOBATopStateUI — MOBA 顶部 HUD
**文件**：`MOBATopStateUI.h` · parent: `UUserWidget`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void HideTurretImage(int32 CampIndex)` | 隐藏指定阵营的防御塔图标（该阵营主塔被摧毁时调用）。 |
| `void SetupVisibleTurretCount(int32 CampCount)` | 根据当前阵营数量显示对应数量的防御塔图标（2v2 显示 2 个阵营，4 人显示 4 个）。 |
| `void RefreshGameTime()` | 从 GameState 读取比赛时间并刷新显示。 |
| `FText FormatTime(int32 TotalSeconds) const` | 将秒数格式化为 `MM:SS`。 |

---

### 4.15 UPauseMenuWidget — 暂停菜单
**文件**：`PauseMenuWidget.h` · parent: `UUserWidget`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void Setup()` | 将暂停菜单添加到视口并暂停游戏。 |
| `void Teardown()` | 从视口移除菜单并恢复游戏。 |
| `UFUNCTION() void OnResumeClicked()` | 继续游戏并关闭菜单。 |
| `UFUNCTION() void OnRestartClicked()` | 重启当前比赛。 |
| `UFUNCTION() void OnMainMenuClicked()` | 返回主菜单。 |
| `UFUNCTION() void OnChangeMapClicked()` | 打开地图选择界面。 |

---

### 4.16 USelectMapWidget — 地图选择
**文件**：`SelectMapWidget.h` · parent: `UUserWidget`

#### 公共方法

| 方法 | 说明 |
|---|---|
| `void UpdatePageDisplay()` | 刷新当前页的 4 张地图；若地图数量不足 4 的倍数，隐藏多余按钮。 |
| `void HighlightSelectedMap()` | 为选中的地图应用边框高亮样式。 |
| `UFUNCTION() void OnMapClicked(int32 ButtonIndex)` | 选择指定按钮索引对应的地图。 |
| `UFUNCTION() void OnPrevPageClicked()` / `UFUNCTION() void OnNextPageClicked()` | 地图列表翻页。 |
| `UFUNCTION() void OnConfirmClicked()` | 使用选中的地图和目标 GameMode 类加载关卡。 |
| `UFUNCTION() void OnBackClicked()` | 返回上一级菜单。 |

---

### 4.17 TankSelectionWidget — 坦克选择菜单（三套）
**文件**：`MutiBattleMenuWidget.h`、`TeamBattleMenuWidget.h`、`MOBASetupWidget.h`、`TankStageStartWidget.h`

三套坦克选择 Widget（多人死斗、团队死斗、MOBA、单人闯关）共享相同的核心交互 API：

| 方法 | 说明 |
|---|---|
| `void OnTankSelectAxisInput(int32 PlayerIndex, float AxisValue)` | 接收玩家控制器的坦克选择摇杆输入并转发。 |
| `void HandleSinglePlayerInput(int32 PlayerIndex, float DeltaTime)` | 处理单个玩家的坦克轮换（含死区判断和冷却定时器）。 |
| `void HandleMouseWheelTargeting(float DeltaTime)` | P1 通过鼠标滚轮切换坦克（独立于手柄）。 |
| `void UpdateBackgroundImage()` | 根据玩家数量切换背景贴图（2/3/4 人分别对应不同背景）。 |
| `int32 GetConnectedDeviceCount()` | 检测已连接手柄数量（决定哪些槽位是真人 vs AI）。 |
| `void UpdateDeviceIcons(int32 DeviceCount)` | 更新槽位图标（手柄图标 = 真人，AI 图标 = Bot）。 |
| `void UpdateDisplay()` | 刷新玩家数量文本和目标分数文本。 |
| `UFUNCTION() void OnConfirmClicked()` | 确认选择并进入地图选择。 |
| `UFUNCTION() void OnBackClicked()` | 返回上一级菜单。 |
| `UFUNCTION() void OnPlayerMinusClicked()` / `OnPlayerPlusClicked()` | 调整玩家数量（仅多人对战模式）。 |
| `UFUNCTION() void OnScoreMinusClicked()` / `OnScorePlusClicked()` | 调整胜利目标分数。 |

### 4.18 UGameSettingsMenuWidget — 游戏设置菜单
**文件**：`GameSettingsMenuWidget.h` · parent: `UUserWidget`

游戏设置菜单，支持手柄、键鼠、自定义三种配置页签切换，点击返回按钮可回到上一级菜单。

**核心属性**：

| 属性 | 类型 | 说明 |
|---|---|---|
| `ParentUI` | `UUserWidget*` | 记住是谁打开了我（用于返回功能）。 |
| `ContentSwitcher` | `UWidgetSwitcher*` | 页面切换器，控制显示哪个配置页。 |
| `Btn_TabGamepad` | `UButton*` | 手柄配置页签按钮。 |
| `Btn_TabKBM` | `UButton*` | 键鼠配置页签按钮。 |
| `Btn_TabCustom` | `UButton*` | 自定义配置页签按钮。 |
| `Btn_Back` | `UButton*` | 返回按钮。 |

**核心方法**：

| 方法 | 说明 |
|---|---|
| `UFUNCTION() void OnGamepadTabClicked()` | 切换到手柄配置页（索引 0）。 |
| `UFUNCTION() void OnKBMTabClicked()` | 切换到键鼠配置页（索引 1）。 |
| `UFUNCTION() void OnCustomTabClicked()` | 切换到自定义配置页（索引 2）。 |
| `UFUNCTION() void OnBackClicked()` | 返回上一级菜单（根据 `ParentUI` 是否有效决定返回到哪里）。 |
| `void UpdateTabButtonStyles(int32 ActiveIndex)` | 更新页签按钮样式（禁用当前选中按钮）。 |

**使用说明**：
- 在主菜单蓝图（`WBP_MainMenuWidget`）中配置 `SettingsMenuClass` 指向 `WBP_GameSettingsMenuWidget`。
- 点击设置按钮后，`UGameSettingsMenuWidget` 会记录 `ParentUI` 并隐藏主菜单。
- 点击返回按钮时，若 `ParentUI` 有效，则恢复主菜单显示并移除设置菜单；否则直接移除设置菜单。

---

## 附录：关键委托汇总

| 委托名称 | 定义类 | 参数 | 触发时机 |
|---|---|---|---|
| `OnKilled` | `ATank` | `ATank* DeadTank, ATank* KillerTank` | 坦克死亡结算完成后；GameMode 绑定以驱动分数/复活/胜负 |
| `OnHealthChanged` | `UHealthComponent` | `(HealthComp, Health, HealthDelta, DamageType, InstigatedBy, DamageCauser)` | 护盾或生命值发生任何变化时 |
| `OnDeath` | `UHealthComponent` | `(HealthComp, InstigatedBy, DamageCauser)` | 生命值降至 0 时；触发 Tank/Prop 的 `HandleDestruction` |

---

*本手册基于 2026-03-25 代码库扫描生成。接口签名以 .h 文件中的声明为准。