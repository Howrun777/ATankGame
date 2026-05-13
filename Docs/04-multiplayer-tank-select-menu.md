# 多人 Tank 选择菜单 Widget 系统解析

> **适用版本**：基于当前代码库（2026-04-02）
> **目标读者**：想深入理解多人菜单输入分配、坦克选择架构的在职同事
> **前置知识**：熟悉 Unreal Engine 5 C++ UserWidget 开发、Enhanced Input System、GameInstance 生命周期

---

## 1. 系统概述

四个菜单 Widget 均属于"选人阶段"的 UI，它们的共同核心职责是：

> **允许多名本地玩家（最多 4 人）同时使用各自的手柄或键鼠，独立选择一辆 Tank，选定后各自保存到 `UBattleBlasterGameInstance`，随后跳转到地图选择或直接加载地图。**

四人之间的输入完全独立，互不干扰。每个玩家槽位有自己独立的：
- Tank 图片（`TankImage_1` ~ `TankImage_4`）
- 高亮边框（`HoverFrame_1` ~ `HoverFrame_4`）
- 手柄图标（`Image_Gamepad_1` ~ `Image_Gamepad_4`，显示手柄或 AI 图标）
- Tank 选择下标（`PlayerTankIndices[i]`）
- 切换冷却计时器（`PlayerSwitchTimers[i]`）

---

## 2. 四个 Widget 的定位差异

| Widget | 模式 | 人数 | 背景切换 | 下一跳 |
|---|---|---|---|---|
| `UMutiBattleMenuWidget` | 多人死斗（Free-for-all） | 2/3/4 人，可动态设置 | 按人数切换 2P/3P/4P 背景 | `USelectMapWidget`（共用地图片） → `OpenLevel` |
| `UTeamBattleMenuWidget` | 团队死斗（2v2） | 固定 4 人 | 无背景切换 | `USelectMapWidget` → `OpenLevel` |
| `UTankStageStartWidget` | 单人闯关 / 守卫 | 固定 1 人 | 无 | `OpenLevel` 直接进入关卡 |
| `UMOBASetupWidget` | MOBA 对战 | 2/3/4 人，可动态设置 | 按人数切换背景 | 根据人数自动选 `MOBAMaps[Index]` → `OpenLevel` |

关键区别：
- `MutiBattleMenuWidget` 和 `MOBASetupWidget` 支持人数加减，人数变化时重新初始化 `ActivePlayerCount`，触发 `InitPlayerTankState`。
- `TeamBattleMenuWidget` 固定 4 人，用编译期常量 `TeamBattlePlayerCount = 4`。
- `TankStageStartWidget` 是单人专用，无多玩家逻辑，是四个中最简化的。

---

## 3. 核心数据结构

### 3.1 FTankOption / FTeamBattleTankOption / FTankOptionSingle

Tank 选项结构体，在每个 Widget 中独立定义（避免 USTRUCT 命名冲突）：

```cpp
USTRUCT(BlueprintType)
struct FTankOption
{
    GENERATED_BODY()

    // UI 上显示的 Tank 头像（Texture2D）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TankSelect")
    UTexture2D* Icon = nullptr;

    // 这个图标对应的 Tank 蓝图类（必须继承自 Pawn/Tank）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TankSelect")
    TSubclassOf<APawn> TankClass = nullptr;
};
```

> **注意**：`FTeamBattleTankOption` 和 `FTankOptionSingle` 与 `FTankOption` 结构完全一致，仅命名不同——这是 Unreal 的 USTRUCT 命名隔离要求，不能跨模块/跨 Widget 共享同一个 struct 类型。

### 3.2 GameInstance 中的持久数据

`UBattleBlasterGameInstance` 是四人选择数据的"中转站"：

| 字段 | 类型 | 含义 |
|---|---|---|
| `TargetPlayerCount` | `int32` | 目标玩家数量（2~4） |
| `TargetMatchScore` | `int32` | 胜利目标分数 |
| `SelectedTankClasses` | `TArray<TSubclassOf<APawn>>` | 每个玩家选中的 Tank 蓝图类（长度 = `TargetPlayerCount`） |
| `ConnectedGamepadCount` | `int32` | 当前物理连接的手柄数 |
| `AIControlledPlayerIndices` | `TArray<int32>` | 哪些玩家槽位需要 AI 控制（`[手柄数, 目标人数)`） |
| `PlayerDeviceIdMap` | `TArray<FInputDeviceId>` | `PlayerIndex → DeviceId` 的设备追踪映射 |

---

## 4. 输入分配机制

### 4.1 双轨输入：每帧轮询 + IMC 回调

四人独立选择 Tank 依赖两条输入路径并存：

**路径 A — NativeTick 每帧轮询（左摇杆 Y 轴）：**

```cpp
void UMutiBattleMenuWidget::HandleTankSelectionInput(float DeltaTime)
{
    for (int32 PlayerIndex = 0; PlayerIndex < ActivePlayerCount; ++PlayerIndex)
    {
        HandleSinglePlayerInput(PlayerIndex, DeltaTime);
    }
}

void UMutiBattleMenuWidget::HandleSinglePlayerInput(int32 PlayerIndex, float DeltaTime)
{
    // 1. 冷却计时
    PlayerSwitchTimers[PlayerIndex] += DeltaTime;
    if (PlayerSwitchTimers[PlayerIndex] < SwitchCooldown) return;

    // 2. 取对应 PlayerController
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), PlayerIndex);
    if (!PC) return;

    // 3. 读左摇杆 Y 轴
    float AxisY = PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY);
    if (FMath::Abs(AxisY) < AxisDeadZone) return;

    // 4. 设备来源追踪
    ULocalPlayer* LP = PC->GetLocalPlayer();
    if (LP) {
        FPlatformUserId UserId = LP->GetPlatformUserId();
        FInputDeviceId ActiveDevice = IPlatformInputDeviceMapper::Get().GetPrimaryInputDeviceForUser(UserId);
        NotifyPlayerInputDevice(PlayerIndex, ActiveDevice);
    }

    // 5. 方向判断（摇杆上推 = AxisY < 0，上一 Tank；摇杆下拉 = AxisY > 0，下一 Tank）
    int32 Direction = (AxisY > 0.0f) ? +1 : -1;
    PlayerTankIndices[PlayerIndex] = (PlayerTankIndices[PlayerIndex] + Direction + TankCount) % TankCount;

    // 6. 重置冷却 + 刷新图片
    PlayerSwitchTimers[PlayerIndex] = 0.0f;
    UpdateTankImageForPlayer(PlayerIndex);
}
```

**路径 B — IMC 注册的 Enhanced Input Action 回调（`OnTankSelectAxisInput`）：**

```cpp
void UMutiBattleMenuWidget::OnTankSelectAxisInput(int32 PlayerIndex, float AxisValue)
{
    // 仅处理当前活跃玩家范围
    if (PlayerIndex < 0 || PlayerIndex >= ActivePlayerCount) return;
    if (FMath::Abs(AxisValue) < AxisDeadZone) return;

    // 冷却（用 World 时间戳，不受 DeltaTime 影响）
    const float Now = World->GetTimeSeconds();
    if ((Now - LastSwitchTimestamp[PlayerIndex]) < SwitchCooldown) return;

    // 方向：AxisValue > 0 表示上一（因为 IMC 里 LeftY Scalar=-1）
    if (AxisValue > 0.0f)
        Index = (Index - 1 + TankCount) % TankCount;
    else
        Index = (Index + 1) % TankCount;

    LastSwitchTimestamp[PlayerIndex] = Now;
    UpdateTankImageForPlayer(PlayerIndex);
}
```

**为什么需要两条路径？**

| 特性 | NativeTick 轮询 | IMC 回调 |
|---|---|---|
| 精度 | 依赖 `DeltaTime` + 冷却计时器 | 精确时间戳，响应更快 |
| 设备追踪 | 天然通过 `PlayerController[PlayerIndex]` | 需要显式映射 `DeviceId → PlayerIndex` |
| 兼容性 | 始终有效（读原始输入键值） | 需要在 IMC 中正确配置绑定 |
| 用途 | 主路径，确保始终能选 | 辅助/备用，与 IMC 深度绑定 |

### 4.2 设备来源追踪（DeviceId Mapping）

每当手柄输入发生，两个 Widget 都会调用：

```cpp
void UMutiBattleMenuWidget::NotifyPlayerInputDevice(int32 PlayerIndex, FInputDeviceId DeviceId)
{
    UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    if (GI)
    {
        GI->RegisterPlayerDeviceMapping(PlayerIndex, DeviceId);
    }
}
```

`GameInstance` 中的映射实现：

```cpp
void UBattleBlasterGameInstance::RegisterPlayerDeviceMapping(int32 PlayerIndex, FInputDeviceId DeviceId)
{
    while (PlayerDeviceIdMap.Num() <= PlayerIndex)
        PlayerDeviceIdMap.Add(FInputDeviceId());
    PlayerDeviceIdMap[PlayerIndex] = DeviceId;
}
```

**意义**：在 GameMode 正式生成玩家坦克时，可以查询"这个 `PlayerIndex` 是哪个手柄控制的"，从而正确设置 `PlayerStart` 或分配出生阵营。但当前代码中此映射**尚未在 GameMode 层被消费**——仅在 Widget 层注册，作为预留能力。

### 4.3 鼠标滚轮选择（玩家 1 专属）

玩家 1（键盘/鼠标）的 Tank 选择通过**鼠标滚轮**实现，滚轮悬停在哪个 Tank 槽位上，就切换对应玩家的选择：

```cpp
void UMutiBattleMenuWidget::HandleMouseWheelTargeting(float DeltaTime)
{
    // 冷却
    if (MouseWheelCooldownTimer > 0.0f) { MouseWheelCooldownTimer -= DeltaTime; return; }

    // 读取鼠标滚轮值
    float MouseWheel = PC0->GetInputAnalogKeyState(EKeys::MouseWheelAxis);
    if (FMath::Abs(MouseWheel) < 0.1f) return;

    // 判断鼠标悬停在哪个 Tank 槽位上
    int32 TargetPlayerIndex = -1;
    if (TankImage_1->IsHovered()) TargetPlayerIndex = 0;
    else if (TankImage_2->IsHovered()) TargetPlayerIndex = 1;
    else if (TankImage_3->IsHovered()) TargetPlayerIndex = 2;
    else if (TankImage_4->IsHovered()) TargetPlayerIndex = 3;

    if (TargetPlayerIndex == -1 || TargetPlayerIndex >= ActivePlayerCount) return;

    // 执行选择
    int32 Direction = (MouseWheel > 0.0f) ? +1 : -1;
    PlayerTankIndices[TargetPlayerIndex] = (PlayerTankIndices[TargetPlayerIndex] + Direction + TankCount) % TankCount;
    UpdateTankImageForPlayer(TargetPlayerIndex);
    MouseWheelCooldownTimer = SwitchCooldown;
}
```

> **注意**：这条路径**只能控制玩家 1**（`GetPlayerController(GetWorld(), 0)`），因为鼠标悬停只能指向一个位置，不支持真正的多人并发鼠标选择。多人时其他玩家必须使用手柄。

---

## 5. 手柄数量检测

### 5.1 检测策略

```cpp
int32 UMutiBattleMenuWidget::GetConnectedDeviceCount()
{
    // 优先使用 GameInstance 缓存的精确过滤结果
    UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    if (GI) {
        int32 Count = GI->GetConnectedGamepadCount(/* bForceRefresh */ true);
        return FMath::Clamp(Count, 1, 4);
    }

    // 回退：原始实现（仅在 GameInstance 不可用时）
    IPlatformInputDeviceMapper& DeviceMapper = IPlatformInputDeviceMapper::Get();
    TArray<FInputDeviceId> ConnectedDevices;
    DeviceMapper.GetAllConnectedInputDevices(ConnectedDevices);
    int32 RawDeviceCount = ConnectedDevices.Num();
    int32 EstimatedGamepadCount = FMath::Max(0, RawDeviceCount - 1); // 减去键鼠
    return FMath::Clamp(FMath::Max(1, EstimatedGamepadCount), 1, 4);
}
```

### 5.2 GameInstance 中的精确过滤

`UBattleBlasterGameInstance::GetConnectedGamepadCountWithMapping` 的核心逻辑：

```
RawDeviceCount >= 2:
    → GamepadCount = RawDeviceCount - 1  （扣除键鼠）
    → EffectivePlayerCount = max(1, clamp(GamepadCount, 0, 4))

RawDeviceCount == 1:
    → 若唯一设备 ID > 1 → 单手柄（GamepadCount = 1）
    → 否则 → 单键鼠（GamepadCount = 0）
    → EffectivePlayerCount = max(1, GamepadCount)  （至少1，保证有键鼠玩家）
```

### 5.3 手柄图标显示规则

设备图标（`Image_Gamepad_1` ~ `Image_Gamepad_4`）按如下规则显示：

```
ConfiguredCount = clamp(CurrentPlayerCount, 2, 4)

for i = 0 .. 3:
    if i < ConfiguredCount:
        if i < DeviceCount:  显示手柄图标（DeviceIconTextures[0]）
        else:                显示 AI 图标（DeviceIconTextures[1]）
    else:
        隐藏该槽位
```

---

## 6. 背景图片切换（MutiBattleMenuWidget / MOBASetupWidget）

两人、三人、四人各自有独立的背景图片，通过双缓冲渐隐实现平滑切换：

```
UpdateBackgroundImage():
    1. BGImage_Overlay ← BGImage（旧图），透明度 = 1.0，设为 HitTestInvisible
    2. BGImage ← 新图（BackgroundImages[CurrentPlayerCount - 2]）
    3. 启动渐隐：CurrentFadeAlpha = 1.0, bIsFading = true

NativeTick 每帧：
    if bIsFading:
        CurrentFadeAlpha -= InDeltaTime * FadeSpeed
        if CurrentFadeAlpha <= 0:
            CurrentFadeAlpha = 0.0
            bIsFading = false
            BGImage_Overlay->SetVisibility(Hidden)
        BGImage_Overlay->SetOpacity(CurrentFadeAlpha)
```

> **双缓冲的意义**：避免切换背景时出现空白帧。旧图在 Overlay 层渐隐，新图在底层立即显示，全程视觉连贯。

---

## 7. 确认流程与数据传递

### 7.1 MutiBattleMenuWidget 确认流程

```
OnConfirmClicked():
    1. GameInst->TargetPlayerCount = CurrentPlayerCount
    2. GameInst->TargetMatchScore = CurrentScore
    3. GameInst->ConnectedGamepadCount = GetConnectedDeviceCount()
    4. 计算 AIControlledPlayerIndices = [DeviceCount, TargetPlayerCount)
    5. 循环写入 SelectedTankClasses[i] = TankOptions[PlayerTankIndices[i]].TankClass
    6. 创建 USelectMapWidget（设置 ParentUI + TargetGameModeClass）
    7. MapWidget->AddToViewport()
    8. 当前 Widget 隐藏（SetVisibility(Hidden)）
```

### 7.2 TeamBattleMenuWidget 确认流程

与 MutiBattle 几乎相同，区别：
- `TargetPlayerCount = 4`（固定）
- 多了 `EnsureLocalPlayers(4)` 确保 4 个 LocalPlayerController 存在
- 跳转到 `USelectMapWidget`，但 `TargetGameModeClass = TeamBattleGameModeClass`

### 7.3 MOBASetupWidget 确认流程

直接加载地图，**不经过地图选择界面**：

```
OnConfirmClicked():
    1. 保存 TargetPlayerCount, TargetMatchScore, SelectedTankClasses
    2. MapIndex = CurrentPlayerCount - 2   // 2人→0, 3人→1, 4人→2
    3. LevelToLoad = MOBAMaps[MapIndex].LevelName
    4. Options = "?game=BP_TankMOBAGameMode"
    5. OpenLevel(LevelToLoad, true, Options)
```

### 7.4 TankStageStartWidget 确认流程

单人专用，保存单个 Tank 到 GameInstance：

```cpp
void UTankStageStartWidget::SaveSelectedTankToGameInstance()
{
    GI->SelectedTankClasses.Empty();
    GI->SelectedTankClasses.SetNum(1);
    GI->SelectedTankClasses[0] = TankOptions[SelectedTankIndex].TankClass;
}
```

---

## 8. 输入参数配置

所有 Widget 共享相同的输入参数，默认值一致：

| 参数 | 默认值 | 含义 |
|---|---|---|
| `AxisDeadZone` | `0.3f` | 摇杆死区，低于此值不响应 |
| `SwitchCooldown` | `0.25f` | 切换 Tank 的冷却时间（秒） |

---

## 9. Hover 边框处理

四人 Tank 图片上的高亮边框（`HoverFrame_1` ~ `HoverFrame_4`）始终跟随鼠标悬停状态：

```cpp
if (HoverFrame_1 && TankImage_1)
    HoverFrame_1->SetVisibility(
        TankImage_1->IsHovered() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
```

- `HitTestInvisible`：边框可见但响应鼠标事件（允许鼠标穿透到下层图片）
- `Hidden`：完全隐藏，释放渲染资源

---

## 10. 生命周期关键时序

```
主菜单关卡加载
    │
    ├─ AMainMenuGameMode::BeginPlay
    │       ├─ 创建 MainMenuWidget → AddToViewport
    │       └─ 创建 MutiPlayerMenuWidget（预加载）
    │
    ▼ 用户点击"多人死斗"按钮
    ├─ MutiPlayerMenuWidget::OnMutiBattleClicked
    │       └─ 创建 MutiBattleMenuWidget → AddToViewport
    │
    ▼ MutiBattleMenuWidget::NativeConstruct
    │       ├─ 绑定所有按钮事件
    │       ├─ UpdateDisplay() / UpdateBackgroundImage()
    │       ├─ ActivePlayerCount = GetConnectedDeviceCount()
    │       ├─ InitPlayerTankState(ActivePlayerCount)
    │       └─ UpdateAllTankImages()
    │
    ▼ 用户在 NativeTick 中用手柄/鼠标选 Tank
    │       ├─ HandleTankSelectionInput (手柄)
    │       ├─ HandleMouseWheelTargeting (滚轮)
    │       └─ UpdateTankImageForPlayer(i) 刷新每个人
    │
    ▼ 用户点击"确认"
    ├─ MutiBattleMenuWidget::OnConfirmClicked
    │       ├─ 写入 GameInstance
    │       └─ 创建 SelectMapWidget → AddToViewport
    │
    ▼ SelectMapWidget::OnConfirmClicked
    │       └─ OpenLevel(LevelName, true, "?game=BP_BattleBlasterGameMode")
    │
    ▼ 地图关卡加载
            ├─ ABattleBlasterGameMode::BeginPlay
            │       ├─ 读取 GameInstance::SelectedTankClasses
            │       ├─ 读取 GameInstance::TargetPlayerCount
            │       ├─ 读取 GameInstance::AIControlledPlayerIndices
            │       ├─ CreatePlayer(0..TargetPlayerCount-1)
            │       ├─ Spawn TankPawn for each player
            │       └─ Bind DeathEvent → HandleTankKilled
            │
            ▼ 游戏进行中...
```

---

## 11. 设计模式总结

### 11.1 单例式数据中枢（GameInstance）

所有菜单 Widget 不直接互相通信，而是通过 `UBattleBlasterGameInstance` 间接传递数据：
- **写入**：`OnConfirmClicked` 时写入
- **读取**：`GameMode::BeginPlay` 时读取
- 优点：解耦 UI 与 Gameplay 逻辑，支持中途取消/返回

### 11.2 设备映射预留

`PlayerDeviceIdMap` 虽未在 GameMode 层消费，但设计上已支持：
- 动态手柄热插拔识别
- 同一手柄中途切换控制目标
- AI 补位后仍追踪原手柄

### 11.3 零拷贝直接引用

四个 Widget 均直接在蓝图中绑定 `BindWidget`（或 `BindWidgetOptional`），C++ 直接持有 `UImage*`、`UButton*` 指针，**无需额外查找或缓存**，编译期验证命名一致性。

### 11.4 双轨冷却系统

- **帧累计冷却**（`PlayerSwitchTimers += DeltaTime`）：简单可靠，但受帧率影响
- **时间戳冷却**（`LastSwitchTimestamp`）：精确，但依赖 World 时间单调递增

两者并存允许 IMC 回调和 NativeTick 轮询各自使用最合适的冷却方式。

---

## 12. 已知局限与改进建议

1. **设备映射未在 GameMode 层消费**：目前 `RegisterPlayerDeviceMapping` 仅写入 `PlayerDeviceIdMap`，GameMode 在 Spawn 坦克时并不知道哪个 PlayerIndex 对应哪个物理手柄。建议在 `ABattleBlasterGameMode::BeginPlay` 中读取该映射用于分配 `PlayerStart`。

2. **鼠标滚轮只支持玩家 1**：`HandleMouseWheelTargeting` 硬编码使用 `GetPlayerController(0)`。若需要支持多人键鼠场景，需要将滚轮输入通过 IMC 路由到对应 PlayerIndex。

3. **AI 补位逻辑分散**：AI 控制的玩家槽位（`AIControlledPlayerIndices`）在 Widget 层计算，在 GameMode 层消费，但 AI Tank 的实际控制由 `ABotTankController` 或 `AAIBotPlayerController` 实现，两者之间缺少显式契约文档。

4. **Tank 选项无去重检查**：两个玩家可以选同一辆 Tank。当前代码允许（无限制），如果业务要求"每辆 Tank 只能被一名玩家选择"，需要在 `OnTankSelectAxisInput` / `HandleSinglePlayerInput` 中额外增加冲突检测逻辑。

5. **四人以上不可扩展**：代码硬编码 `TankImage_1` ~ `TankImage_4`，`Image_Gamepad_1` ~ `Image_Gamepad_4`，若要支持更多玩家需要重构为 `TArray<UImage*>` 动态管理。
