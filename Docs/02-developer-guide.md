# BattleBlaster 开发者指南 — 架构与系统说明

> **适用版本**：基于当前代码库（2026-03-25）  
> **目标读者**：接手本项目的新程序、想深入理解架构的在职同事  
> **前置知识**：熟悉 Unreal Engine 5 C++ 开发，了解 Gameplay 框架（GameMode、GameState、PlayerState、Controller、Component）

---

## 1. 核心类继承关系图（Class Hierarchy）

### 1.1 Pawn / Actor 体系

```
APawn
└── ABasePawn                          （战斗体系基类）
    ├── ATank                          （玩家/AI 坦克）
    └── ATower                         （敌人塔楼，PvE 用）
        └── [注意] ATurret             （MOBA 防御塔，继承自 ADestructibleProp，非 ABasePawn）

AActor                                 （所有可放置物体的根类）
├── AProjectile                        （玩家炮弹）
├── ATurretProjectile                  （MOBA 塔的追踪弹）
├── ABuffPickup                        （地图可拾取 Buff）
├── ADestructibleProp                  （可破坏物基类）
│   ├── AExplosiveBarrel               （油桶）
│   ├── AWoodenCrate                   （木箱）
│   └── ATurret                        （MOBA 防御塔，继承 ADestructibleProp）
├── ARisingGate                        （升降闸门）
├── ASpikeTrap                         （尖刺陷阱）
├── ASlideTrack                        （滑动轨道）
└── ATeleportPortal                   （传送门）
```

**ABasePawn 职责**：封装所有坦克类共有的组件和逻辑。
- `UCapsuleComponent* CapsuleComp` — 碰撞体
- `UStaticMeshComponent* BaseMesh` — 坦克底座
- `UStaticMeshComponent* TurretMesh` — 炮塔（独立旋转）
- `USceneComponent* ProjectileSpawnPoint` — 炮弹生成点
- `UHealthComponent* HealthComp` — 生命值管理
- `TSubclassOf<AProjectile> ProjectileClass` — 炮弹类引用
- `float Fire_LastTime` — 上次开火时间（控制射速）

**ATank 扩展**（继承 ABasePawn）：
- `UTankBuffComponent* BuffComp` — Buff 状态管理
- `USpringArmComponent* SpringArmComp` + `UCameraComponent* CameraComp` — 第三人称相机
- `UCameraComponent* ScopeCameraComp` — 开镜时使用的炮管相机
- Enhanced Input 动作映射（MoveAction、FireAction、IA_Aim_Toggle、IA_Aim_Hold 等）
- Buff 标记（`bHasInfiniteAmmo`、`bHasDamageBoost` 等，均为 bool）
- 击杀奖励：`AmmoReward`（默认 10）、`HealthReward`（默认 25）

**ATower 扩展**（继承 ABasePawn）：
- `USphereComponent* DetectionSphere` — 警戒网
- `TArray<ATank*> TargetsInRange` — 范围内目标列表
- 敌人塔楼的复活逻辑（多玩家模式下 60 秒后满血重生）
- 难度系数扩展（`CurrentDifficultyMultiplier`）

---

### 1.2 Controller 体系

```
AController
├── APlayerController                  （玩家控制器基类）
│   ├── ATankPlayerController          （战斗 UI 管理、输入处理）
│   └── AUIPlayerController            （菜单导航专用）
└── AAIController                     （AI 控制器基类）
    ├── AAIBotPlayerController         （完整战斗 AI，含状态机）
    └── ABotTankController             （简单随机移动 AI）
```

**ATankPlayerController 职责**：
不直接控制坦克移动（移动由 Enhanced Input System 绑定到 Pawn），而是负责：
- 在 `BeginPlay` 中创建所有 HUD Widget 实例（HUDWidget、AmmoWidget、BuffListWidget、PassWidget、KDAWidget）
- 响应 Tank 的事件来刷新 UI（`UpdateHealthHUD`、`UpdateKDA`）
- 管理暂停菜单、死亡界面、观战系统的显示/隐藏
- 在 `Tick` 中每帧更新 ReturnProgressWidget（返回出生点进度条）

**AAIBotPlayerController 职责**（最复杂的 AI 类）：
- 完整状态机：`EAICombatState = { Idle, Chase, Strafe, KeepDistance, Flee, TakeCover, Ambush }`
- 战术动作：`ETacticalMoveType = { CircleLeft, CircleRight, ForwardStrafe, BackwardStrafe, RandomStrafe }`
- 威胁评估：维护 `AttackTargetList[]`，选择最优目标
- 预测射击：计算提前量
- 躲避行为：DodgeChance、PredictDodgeInterval
- 感知回调：`OnAttackedBy()` 接收伤害时更新仇恨目标

**ABotTankController 职责**：
仅提供随机方向移动（`DirectionChangeInterval = 2.0s`），用于不需要复杂 AI 的简单坦克行为场景。

---

### 1.3 GameState / PlayerState 体系

#### GameState 继承链（每个游戏模式一份）

```
AGameState
└── ATankGameState                     （所有模式的基类）
    ├── ATankBattleGameState            （自由死斗模式）
    ├── ATankStageGameState             （关卡闯关模式）
    ├── ATankMOBAGameState              （MOBA 对战模式）
    └── ATeamBattleGameState            （团队死斗模式）
```

- `ATankGameState`：记录 `MatchTimeSeconds`、`CountdownSeconds`、`EGameStatus`（Waiting/Countdown/Playing/Ended）
- `ATankBattleGameState`：记录 `WinnerIndex`、`TargetScore`、`PlayerScores[]`（每人独立分数）
- `ATankStageGameState`：记录 `RemainingTowerCount`、`CurrentWave`/`TotalWaves`、`bIsVictory`
- `ATankMOBAGameState`：记录 `AllTowers[]`、`CoreTurretCountByCamp[]`（每阵营主塔数）、`OuterTurretCountByCamp[]`
- `ATeamBattleGameState`：记录 `TeamScores[]`（红=0、蓝=1）、`WinnerCampIndex`

#### PlayerState 继承链

```
APlayerState
└── ATankPlayerState                   （通用玩家状态）
    ├── ATankBattlePlayerState          （死斗：无敌标记、复活时间）
    ├── ATankStagePlayerState           （闯关：剩余生命、关卡分）
    ├── ATankMOBAPlayerState            （MOBA：阵营索引、永久淘汰标记）
    └── ATeamBattlePlayerState          （团队：红/蓝阵营、团队得分贡献）
```

- `ATankPlayerState` 是核心基类：KDA（`KillCount`/`DeathCount`/`AssistCount`）、`AttackerQueue[]`（7 秒仇人追踪）、`CurrentAmmo`、`RecordAttacker()`、`ProcessDeath()`、`HandleKillConfirmed()`（子类重写）
- `ATankMOBAPlayerState`：`bIsEliminated`（永久淘汰）、`CalculateRespawnDelay()`（随时间递增的复活延迟，上限 10 秒）

---

### 1.4 模式类的派生关系

```
AGameMode
├── ABattleBlasterGameMode             （2.5 自由死斗）
├── ATankStageGameMode                 （2.3 关卡闯关）
├── ATankMOBAGameMode                  （2.2 MOBA 对战）
├── ATeamBattleGameMode                （2.1 团队死斗）
├── ADefenseGameMode                   （2.4 防守模式，占位空壳）
├── ATestGameMode                      （测试用）
└── AMainMenuGameMode                  （主菜单）
```

每个 GameMode 的核心职责都是：
1. 在 `BeginPlay` 中生成玩家坦克和出生点
2. 绑定死亡事件到 `HandleTankKilled`
3. 驱动复活逻辑
4. 判定胜负条件
5. 显示/隐藏结算 UI

---

## 2. 组件化设计（Component Pattern）

### 2.1 UHealthComponent — 生命值、受击与死亡广播

**注册位置**：`ABasePawn::SetupPlayerInputComponent()` 或直接在 ABasePawn 构造函数中 `HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"))`

**核心数据**：
```cpp
UPROPERTY(EditAnywhere, Category = "Health")
float MaxHealth = 200.0f;

UPROPERTY(VisibleAnywhere, Category = "Health")
float CurrentHealth;

UPROPERTY(EditAnywhere, Category = "Shield")
float MaxShield = 100.0f;

UPROPERTY(VisibleAnywhere, Category = "Shield")
float CurrentShield;
```

**两个核心 Multicast Delegate**：

```cpp
// 生命值变化时广播（护盾优先扣减）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnHealthChangedSignature,
    UHealthComponent*, HealthComp,
    float, Health,
    float, HealthDelta,
    const class UDamageType*, DamageType,
    class AController*, InstigatedBy,
    AActor*, DamageCauser);

// 死亡时广播
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDeathSignature,
    UHealthComponent*, HealthComp,
    class AController*, InstigatedBy,
    AActor*, DamageCauser);
```

**护盾扣减逻辑（ApplyDamage）**：
```
当前护盾 > 0 → 优先扣护盾，护盾扣完后才开始扣生命
当前护盾 = 0 → 直接扣生命
护盾不影响生命上限显示（护盾耗尽时 HealthBar 始终显示 100%）
```

**受击处理流程**：
```
ApplyDamage()
  → 当前护盾 > 0？
      是 → CurrentShield -= Damage，HealthDelta = 0，广播 OnHealthChanged
      否 → CurrentHealth -= Damage，HealthDelta = -Damage，广播 OnHealthChanged
  → CurrentHealth <= 0？
      是 → 广播 OnDeath → 触发 Owner 的 HandleDestruction()
```

**子类使用方式**：
```cpp
// 在 ATank 的 BeginPlay 中：
HealthComp->OnHealthChanged.AddDynamic(this, &ATank::HandleHealthChanged);
HealthComp->OnDeath.AddDynamic(this, &ATank::HandleDeath);

// ATank::HandleDeath 内部：
// 1. 调用父类 ABasePawn::HandleDeath()（销毁炮弹特效等）
// 2. 计算 KillerTank（通过 AttackerQueue[0]）
// 3. 播放死亡特效和音效
// 4. 隐藏 Tank 5 秒（5 秒后物理体重生）
// 5. 广播 OnKilled（GameMode 监听）
// 6. 调用 PlayerState->ProcessDeath()
// 7. 记录最后伤害来源
```

---

### 2.2 UTankBuffComponent — Buff 状态管理

**Owner 引用**：
```cpp
UPROPERTY()
TWeakObjectPtr<ATank> OwnerTank;
```

**数据结构**：
```cpp
TMap<EBuffType, FActiveBuffUIInfo> ActiveBuffs;
// EBuffType = { None, Heal, Ammo, Speed, Pierce, Ghost, Damage, DoubleShot, Shield, RandomIcon }
// FActiveBuffUIInfo = { Type, Icon, RemainingTime }
```

**一次性 Buff（立刻生效，无持续时间）**：
| Buff | 效果 |
|------|------|
| **Heal** | 生命值恢复至 MaxHealth |
| **Shield** | 添加 MaxHealth×50% 的护盾值 |

**持续性 Buff（通过 Timer 管理）**：
| Buff | 持续时间 | 效果 |
|------|---------|------|
| **Ammo** | 30s | 弹药无限（UI 显示 9999） |
| **Speed** | 30s | 移速 200%，最后 10 秒线性衰减至 100% |
| **Damage** | 30s | 炮弹伤害 ×2，炮弹外观替换 |
| **Pierce** | 30s | 炮弹可穿透墙体和敌人 |
| **DoubleShot** | 30s | 每次开火 2 发，偏移 50cm |
| **Ghost** | 30s | 可穿越墙体；结束时若卡墙触发窒息状态 |

**Ghost 窒息系统（最危险的边界情况）**：
```
Ghost Buff 结束
  → 检查 Tank 是否在墙体内部（LineTraceByObject）
  → 是 → 进入窒息状态：
        每隔 SuffocationDamageInterval（1s）扣 SuffocationDamagePerTick（10）点生命
        最多持续 MaxSuffocationTime（999s）
        显示 SuffocationWidget
        播放 SuffocationSound
  → 否 → 正常退出 Ghost
```

**AddBuff 内部流程**：
```
AddBuff(Type, Duration, Icon)
  1. 若为一次性（Heal/Shield）：立刻执行效果
  2. 若为持续性：
     - 若已存在该 Buff：叠加时间（RemainingTime += Duration）
     - 若不存在：新建 ActiveBuffs[Type]，启动定时器
  3. 通知 UI 更新（BuffListWidget 刷新）
```

**RestoreBuffs**：复活时从 GameMode 保存的 `PlayerSavedBuffs[]` 恢复所有激活 Buff 的类型、剩余时间、图标。

---

## 3. 游戏模式与状态管理

### 3.1 UBattleBlasterGameInstance — 全局单例

`UBattleBlasterGameInstance` 是整个游戏进程中**唯一**存在的非 Actor 实例（通过 `GetGameInstance()` 随时获取），承担以下全局职责：

**跨关卡状态继承**（`FPlayerCarryState`）：
```cpp
// 当前生命值（跨关继承）
float Health;
// 当前弹药数（跨关继承）
int32 Ammo;
// 7 种 Buff 是否激活（跨关继承）
bool bHasInfiniteAmmo, bHasDamageBoost, bHasBulletPierce,
     bHasDoubleShot, bIsGhostMode, bHasSpeedBoost, bHasShield;
// 每种 Buff 剩余持续时间
float BuffTimeRemaining[6];
// 每种 Buff 对应图标
UTexture2D* BuffIcons[6];
```

**游戏设置（菜单 → 游戏的数据传递）**：
```cpp
int32 TargetPlayerCount;           // 目标玩家数（默认 2）
int32 TargetMatchScore;            // 胜利目标分数（默认 7）
TArray<TSubclassOf<ATank>> SelectedTankClasses;  // 各槽位选择的坦克类
int32 ConnectedGamepadCount;       // 检测到的手柄数量
```

**存档管理**：
```cpp
// 战役进度存档（当前关卡、历史最高）
// 历史战绩存档（MultiBattleHistory 前 50 条）
```

**关卡管理**：
```cpp
TArray<FName> CampaignLevelNames;     // 关卡名称列表
int32 CurrentLevelIndex;               // 当前关卡序号（从 1 开始）
int32 DifficultyCoefficientK;          // 难度系数（默认 1.2）
```

**计时**：`CampaignAccumulatedTime` — 跨关累计游戏时长（精确到秒）

**重要提示**：GameInstance 中的数据在**进程退出后不持久化**。持久化依赖 `UBattleBlasterSaveGame` 和 `UBattleBlasterHistorySaveGame`。

---

### 3.2 各 GameMode 的游戏进程驱动方式

所有模式共享以下通用模式：

```
BeginPlay
  → 读取 GameInstance 中的配置（TargetPlayerCount、TargetMatchScore 等）
  → 初始化 GameState（InitializePlayerData）
  → 查找 PlayerStart 出生点（Tag: P0/P1/P2/P3）
  → Spawn 坦克 + Controller（真人或 AI）
  → 创建 HUD / ScoreBoard UI
  → 启动开场倒计时（Get Ready! → 3 → 2 → 1 → GO!）
  → 比赛计时（MatchTimerHandle，每秒 +1）

比赛过程中：
  Tank 死亡 → OnKilled → GameMode::HandleTankKilled → 更新分数 → 胜负判定 / 复活

比赛结束：
  触发胜利特效（Niagara）→ 停止比赛计时 → 显示结算 Widget
```

---

## 4. 所有系统工作流（Workflow）

### 4.1 伤害与死亡结算流（完整链路）

```
① AProjectile::OnHit()           （炮弹命中某个 Actor）
  └─ UGameplayStatics::ApplyDamage(Target, Damage, InstigatedController, this, DamageTypeClass)
       ├─ 命中 ATank
       │    └─ ATank::TakeDamage()（继承自 AActor::TakeDamage）
       │         ├─ 检查 friendly fire（若是友军炮弹则忽略伤害）
       │         └─ HealthComp->ApplyDamage(Damage, InstigatedController, DamageCauser)
       │              ├─ ATank::HandleHealthChanged()
       │              │    ├─ ATankPlayerController::UpdateHealthHUD()
       │              │    ├─ ATankPlayerController::TriggerDamageVibration()
       │              │    └─ ATank::NotifyAttackedBy(KillerController)
       │              │         └─ AAIBotPlayerController::OnAttackedBy()
       │              │              └─ 更新仇恨目标，开始反击
       │              │
       │              └─ 若 CurrentHealth <= 0 → 广播 HealthComp->OnDeath
       │
       ├─ 命中 ADestructibleProp（油桶/木箱）
       │    └─ DestructibleProp->HealthComp->ApplyDamage()
       │         └─ HealthComp->OnDeath 广播
       │              └─ ADestructibleProp::OnPropDestroyed()（自定义行为）
       │                   ├─ AExplosiveBarrel::HandleDestruction() → 范围伤害
       │                   ├─ AWoodenCrate::HandleDestruction() → 破碎特效
       │                   └─ ATurret::HandleDestruction() → 通知 MOBAGameState
       │
       └─ 命中 ATower（敌人塔楼）
            └─ ATower::TakeDamage()
                 └─ ATower::HealthComp->ApplyDamage()
                      └─ ATower::HandleTowerDeath()
                           └─ ATankStageGameMode::HandleTowerDestroyed()
                                └─ ATankStageGameState::DecreaseTowerCount()
                                     └─ 检查是否全部塔楼已摧毁 → 通关

② ATank::HandleDeath()（由 HealthComp->OnDeath 触发）
  ├─ ABasePawn::HandleDestruction()         （销毁炮弹、停止武器音频）
  ├─ 计算 KillerTank（通过 ATankPlayerState::AttackerQueue[0]）
  ├─ 播放死亡特效 + 音效 + CameraShake
  ├─ 5 秒后隐藏 + 物理体重生（5s 后调用 DestroyActor）
  ├─ ATank::OnKilled.Broadcast(DeadTank, KillerTank)
  │    └─ 各 GameMode::HandleTankKilled()  ← 核心结算点
  │         ├─ 保存死亡玩家的 Buff 信息
  │         ├─ 若有凶手（KillerTank != nullptr 且 KillerIndex != VictimIndex）
  │         │    └─ BBGameState->AddPlayerScore(KillerIndex, +1)
  │         │         ATankMOBAPlayerState->HandleKillConfirmed()（空实现）
  │         │         ATeamBattlePlayerState->HandleKillConfirmed() → TeamGameState->AddTeamScore()
  │         │    否则（自杀/塔杀）
  │         │         BBGameState->AddPlayerScore(VictimIndex, -1)（最低 0）
  │         ├─ 更新比分板 UI（ScoresDisplayWidget）
  │         ├─ 胜负判定（遍历所有玩家分数 >= TargetScore？）
  │         │    ├─ 有胜者 → 触发胜利特效 → 弹出 GameOverWidget
  │         │    └─ 无胜者 → 启动复活倒计时（RespawnPlayer，2s 后）
  │         └─ ATankPlayerState::ProcessDeath()（在 HandleDeath 内调用）
  │              ├─ 记录死亡（DeathCount++）
  │              ├─ ATankPlayerState::RefreshKDAUI()
  │              │    └─ ATankPlayerController::UpdateKDA()
  │              ├─ 若有凶手 → AssistCount 记录到助攻者
  │              └─ ATankPlayerState::HandleKillConfirmed()（子类重写）
  │
  └─ ATankPlayerState::ProcessDeath()（上面第 7 步）
       ├─ ATankBattlePlayerState::HandleKillConfirmed() → 空实现（分数已由 GameMode 处理）
       ├─ ATankStagePlayerState::HandleKillConfirmed() → 空实现（PvE 不记分）
       ├─ ATankMOBAPlayerState::HandleKillConfirmed() → 空实现（塔杀不计分）
       └─ ATeamBattlePlayerState::HandleKillConfirmed()
            └─ 检查跨阵营击杀 → TeamGameState->AddTeamScore(KillerCamp, +1)

③ ATank::RespawnPlayer()（复活流程）
  ├─ 在己方出生点 Spawn 新 Tank
  ├─ 找到原来的 Controller（通过 PlayerIndex 匹配 PlayerState）→ Possess 新 Tank
  ├─ 若为 AI Controller → 重置战斗状态（CurrentCombatState = Idle, CurrentTarget = nullptr）
  ├─ 设置生命值 = MaxHealth × 0.5
  ├─ 设置弹药 = max(默认弹药 × 0.5, 死亡前保存的弹药)
  ├─ 恢复保存的 Buff（PlayerSavedBuffs 数组）
  ├─ 开启无敌状态（SetCanBeDamaged(false)，持续 3 秒）
  ├─ 播放复活 Niagara 特效
  └─ 3 秒后 EndInvincibility() → SetCanBeDamaged(true)

### 4.1.1 MOBA 模式游戏结束判定逻辑（2026-04-01 修订）

**核心设计原则**：死亡 ≠ 淘汰，复活期间不会触发游戏结束检查。

#### 游戏结束触发时机

当玩家**被击杀** 时，会尝试检擦一次游戏是否结束。触发点在 `ATankMOBAGameMode::HandleTankKilled()` 的 else 分支（核心塔已摧毁时）。

#### 玩家死亡 vs 淘汰的区分

| 场景 | 玩家状态 | 核心塔状态 | 后续行为 |
|------|---------|-----------|---------|
| 玩家死亡，核心塔存活 | 死亡（`Dead=true`，`WaitingForRespawn=true`） | 存活 | 进入复活等待倒计时，计时结束后复活 |
| 玩家死亡，核心塔已摧毁 | 淘汰（`Eliminated=true`，`WaitingForRespawn=false`） | 已摧毁 | 显示淘汰界面，触发游戏结束检查 |

#### 游戏结束判定条件（必须同时满足）

1. **场上只剩 1 个核心塔存活**（`GetAliveCoreTurretCount() == 1`）
2. **除了获胜阵营外，所有玩家都已被淘汰**（`IsEliminated() == true`）

#### 关键实现代码位置

- `TankMOBAGameMode.cpp` - `HandleTankKilled()`：玩家死亡时判断是否淘汰
- `TankMOBAGameMode.cpp` - `CheckGameOver()`：判定游戏是否结束
- `TankMOBAGameState.cpp` - `OnTurretDestroyed()`：**不再直接判定游戏结束**

#### 典型场景分析

| 场景 | 玩家B状态 | 核心塔B | 游戏是否结束 |
|------|-----------|---------|-------------|
| A击杀B，B核心塔存活 | 死亡（等待复活） | 存活 | ❌ 不结束 |
| A击杀B后摧毁B核心塔 | 死亡（等待复活）→ 计时结束复活 → 再次死亡 → 淘汰 | 已摧毁 | ❌ 不结束（B复活后） |
| B复活后再次死亡 | 淘汰 | 已摧毁 | ✅ 检查通过后结束 |

---



---

### 4.2 UI 刷新机制

BattleBlaster 的 UI 刷新采用两种互补模式：**Delegate 绑定**（事件驱动）和 **Tick 每帧刷新**（持续显示）。

#### 4.2.1 事件驱动型 Delegate 绑定

```
ATankPlayerController 中的绑定（事件驱动，无性能开销）：
┌─────────────────────────────────────────────────────────────┐
│ HealthComp->OnHealthChanged.AddDynamic(                    │
│     this, &ATankPlayerController::UpdateHealthHUD);         │
│                                                              │
│ HealthComp->OnDeath.AddDynamic(                             │
│     this, &ATankPlayerController::OnPlayerDeath);           │
│                                                              │
│ Tank->OnKilled.AddDynamic(                                 │
│     this, &ATankPlayerController::OnTankKilled);            │
│                                                              │
│ PS->RefreshKDAUI.AddUObject(                               │
│     PC, &ATankPlayerController::UpdateKDA);                  │
└─────────────────────────────────────────────────────────────┘
```

**各 Widget 的刷新方式**：

| Widget | 刷新方式 | 刷新函数 | 触发条件 |
|--------|---------|---------|---------|
| `UHUDWidget` | Delegate | `SetHealthBarPercent()`、`SetShieldBarPercent()` | HealthComp.OnHealthChanged |
| `UBulletsWidget` | Delegate | `SetAmmoText()` | ATank::OnAmmoChanged（广播） |
| `UBuffListWidget` | Delegate + Tick | `UpdateBuffList()`（Tick 每帧） | Buff 变化时 + 每帧检查持续时间 |
| `UKDAWidget` | Delegate | `UpdateKDA()`、`SetColor()` | PlayerState.RefreshKDAUI |
| `UScoresDisplayWidget` | Delegate | `UpdateScores()`、`UpdateMatchTimer()` | GameMode 处理击杀后 |
| `UDeathScreenWidget` | Delegate | `UpdateRespawnCountdown()`（Tick） | ATankPlayerState 的复活计时器 |
| `UEliminatedScreenWidget` | Delegate | `Show()`、`Hide()` | MOBAGameMode::HandleElimination() |
| `UPassWidget` | Tick | 每帧读取 ATankStageGameState | ATankStageGameState 属性变化 |
| `UMultiBattleGameOverWidget` | 一次性构建 | `InitResultData()`（GameMode 调用一次） | 胜负确定后 |

#### 4.2.2 Tick 每帧刷新型

以下 Widget 在 `NativeTick` 中每帧刷新（因为需要实时显示动态数据）：

- **`UBuffListWidget`**：每帧遍历 `ActiveBuffs`，更新每个 `UBuffSlotWidget` 的倒计时文本，若某 Buff 剩余时间 <= 0 则移除
- **`UPassWidget`**：每帧读取 `ATankStageGameState::CurrentStageId`、`RemainingLives`、当前时间
- **`UDeathScreenWidget`**：每帧更新复活倒计时文本（精确到 0.01 秒）

#### 4.2.3 数据流向总结图

```
底层数据（代码层）
│
├─ UHealthComponent::CurrentHealth/Health
│    └─ OnHealthChanged → ATankPlayerController::UpdateHealthHUD()
│         └─ HUDWidget->SetHealthBarPercent() / SetShieldBarPercent()
│
├─ ATankPlayerState::KillCount/DeathCount/AssistCount
│    └─ ATankPlayerState::RefreshKDAUI → ATankPlayerController::UpdateKDA()
│         └─ KDAWidget->UpdateKDA()
│
├─ ATankBattleGameState::PlayerScores[i]
│    └─ ABattleBlasterGameMode::HandleTankKilled() → ScoresWidgetInstance->UpdateScores()
│         └─ ScoresDisplayWidget->UpdateScores() / UpdateScoresFour()
│
├─ ATankStageGameState::RemainingTowerCount
│    └─ 每帧 PassWidget->NativeTick() 读取
│         └─ 显示当前关卡 / 剩余敌人 / 生命
│
└─ UTankBuffComponent::ActiveBuffs
     └─ 每帧 BuffListWidget->NativeTick() 遍历
          └─ 每帧调用每个 BuffSlotWidget->UpdateSlot()
```

---

## 5. 新手开发避坑指南

### 5.1 高耦合度模块（修改时极易出错）

#### ⚠️ ATank::HandleDeath — 所有结算逻辑的交叉点

这是全项目**最中心**的函数，串联了：
- 死亡特效
- 伤害归属（AttackerQueue）
- GameMode 结算分数
- PlayerState KDA 更新
- UI 显示/隐藏
- 复活延迟

**问题**：如果要在死亡时新增任何行为（如触发 Buff、触发成就、触发特殊音效），最安全的做法是**在 HandleDeath 末尾通过事件广播**，而不是直接改 HandleDeath 本身。直接修改容易破坏已有的结算顺序。

#### ⚠️ ABasePawn::TakeDamage — 伤害入口的多种判断分支

`ATank::TakeDamage` 继承自 `AActor::TakeDamage`，目前包含：
- 友军伤害检测（`CanDealDamage`）
- 无敌状态检测
- 伤害数值应用

如果要在某个模式中修改伤害规则（如 MOBA 模式中塔的伤害逻辑），**不要只改 ABasePawn::TakeDamage**，而应该在对应的 GameMode 中重写或拦截：

```cpp
// ATankMOBAGameMode 中的做法：
void ATankMOBAGameMode::HandleTankKilled(ATank* DeadTank, ATank* KillerTank)
{
    // 先按默认流程处理复活和分数
    Super::HandleTankKilled(DeadTank, KillerTank);
    // 再处理 MOBA 特殊逻辑：例如某阵营全灭判定
}
```

#### ⚠️ AAIBotPlayerController::Tick — 每帧执行的复杂状态机

这个 AI 控制器在 `Tick` 中做了大量工作：移动计算、瞄准更新、武器冷却检查、战术动作执行等。修改时注意：
- 不要在 Tick 中进行 Spawn/Actor 创建（应在事件触发时做）
- 战术动作（CircleLeft/CircleRight）和主移动方向是**叠加关系**，不是替代关系

#### ⚠️ UTankBuffComponent::AddBuff — Ghost 窒息系统的边界情况

Ghost Buff 结束时会做线检测（`LineTraceByObject`），判断坦克是否在墙内。这个检测依赖碰撞体设置，如果新地图的墙体使用了非标准碰撞通道，可能导致检测失效（坦克卡在墙内但不触发窒息）。建议在测试新地图时专门测试 Ghost Buff。

#### ⚠️ 多模式共存时的 GameState/PlayerState 类混用

BattleBlaster 有 4 套 GameState 和 5 套 PlayerState 子类。在切换模式时，UE 的 GameMode/GameState/PlayerState 类映射由 `DefaultEngine.ini` 或 `Project Settings → Maps & Modes` 中的 `GameMode Override` 控制。**如果在编辑器中直接改地图的 World Settings 而忘记更新 GameModeOverride，会导致类型不匹配**（如 MOBA 地图使用了 BattleBlasterGameMode），引发 Crash 或空指针。

---

### 5.2 修改建议

#### ① 扩展 GameMode 的正确姿势

新增一个模式或修改现有模式时，**始终在对应的 Mode 类中重写**，不要在基类改：

```cpp
// 错误：在 ATankGameState 中加死斗特有逻辑
// 正确：在 ATankBattleGameState 中新增

// 若想在多个模式间共享逻辑，使用 protected virtual 函数
protected:
    virtual void OnPlayerScored(int32 PlayerIndex, int32 NewScore);
```

#### ② 新增 Buff 的正确姿势

1. 在 `EBuffType` 枚举中添加新类型（如 `MyNewBuff`）
2. 在 `UTankBuffComponent::AddBuff` 中添加 `case EBuffType::MyNewBuff:` 的处理分支
3. 在 `ATank` 中添加对应的 `bool bHasMyNewBuff`
4. 在 `AProjectile::OnHit` 中检查 `OwnerTank->bHasMyNewBuff` 并修改炮弹行为
5. 在 `UTankBuffComponent::GetAllActiveBuffs()` / `RestoreBuffs()` 中序列化新 Buff

#### ③ 新增伤害来源的正确姿势

如果要给塔或陷阱添加伤害来源，需要：
1. 确保伤害来源携带 `InstigatedController`（伤害归属）
2. 如果需要仇人追踪，在 `ATankPlayerState::RecordAttacker()` 中注册
3. 如果需要助攻判断，在 `ATankPlayerState::ProcessDeath()` 中检查 AttackerQueue

**不要**在 `AProjectile::OnHit` 中直接修改 `AttackerQueue`，正确的入口是 `ATankPlayerState::RecordAttacker()`。

#### ④ UI 绑定的安全检查

所有 `AddDynamic` 绑定前建议加 `IsValid` 检查：
```cpp
if (HealthComp && HealthComp->OnHealthChanged.IsBound())
{
    HealthComp->OnHealthChanged.AddDynamic(...);
}
```
虽然当前代码大部分遵循此规范，但新增 UI 绑定时务必注意。

#### ⑤ 存档系统的数据一致性

`UBattleBlasterGameInstance` 中的 `FPlayerCarryState` 和 `UBattleBlasterSaveGame` 是两套独立数据：
- `GameInstance` 的 `FPlayerCarryState`：进程内有效，用于关卡间的状态继承
- `SaveGame`：持久化，用于退出后重新开始时的进度恢复

如果新增需要跨进程保存的 Buff 或属性，**同时**修改这两个类，确保一致性。

#### ⑥ 分屏模式下的 UI 创建位置

所有战斗 HUD（`HUDWidget`、`AmmoWidget`、`BuffListWidget` 等）在 `ATankPlayerController::BeginPlay` 中创建，每个 PlayerController 创建自己的 UI 实例。只有 **比分板（ScoresDisplayWidget）** 和 **屏幕消息（ScreenMessage）** 是全局的（由 `PC0` 创建，所有人可见）。

如果要给某个玩家单独显示 UI，挂在对应玩家的 `PlayerController` 上；如果要给所有人显示，放在全局 Widget 中。

---

### 5.3 关键代码文件速查表

| 需求 | 文件 |
|------|------|
| 修改坦克移动/射击逻辑 | `Tank.cpp` / `Tank.h` |
| 修改炮弹飞行/碰撞行为 | `Projectile.cpp` |
| 修改生命值/死亡事件 | `HealthComponent.cpp` |
| 修改 Buff 系统 | `TankBuffComponent.cpp` + `BuffTypes.h` |
| 修改 AI 行为 | `AIBotPlayerController.cpp` |
| 修改多人死斗规则 | `BattleBlasterGameMode.cpp` |
| 修改关卡闯关规则 | `TankStageGameMode.cpp` |
| 修改 MOBA 规则 | `TankMOBAGameMode.cpp` |
| 修改团队死斗规则 | `TeamBattleGameMode.cpp` |
| 修改战斗 UI | `HUDWidget.cpp`、`ScoresDisplayWidget.cpp` |
| 修改结算界面 | `MultiBattleGameOverWidget.cpp` 等 |
| 修改主菜单 | `MainMenuWidget.cpp`（含设置按钮） |
| 修改设置菜单 | `GameSettingsMenuWidget.cpp` |
| 修改存档格式 | `BattleBlasterSaveGame.cpp` / `BattleBlasterHistorySaveGame.cpp` |
| 修改全局配置 | `BattleBlasterGameInstance.cpp` |
| 修改可破坏物行为 | `DestructibleProp.cpp`、`ExplosiveBarrel.cpp` |
| 修改陷阱行为 | `SpikeTrap.cpp`、`SlideTrack.cpp`、`RisingGate.cpp` |
| 修改传送门 | `TeleportPortal.cpp` |
| 修改地图可拾取 Buff | `BuffPickup.cpp` |
| 修改 MOBA 塔 | `Turret.cpp` |

---

*本指南基于当前代码库生成，如有代码变更请同步更新。*
