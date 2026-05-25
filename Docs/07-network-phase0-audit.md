# 联网模式阶段 0 审计报告

> 日期：2026-05-17  
> 范围：`Source/BattleBlaster` 当前 C++ 代码。  
> 目标：确认现有代码中哪些地方依赖本地分屏，哪些状态尚未支持网络复制，后续开发联网模式时应保留、隔离或迁移哪些逻辑。

> 维护说明：这是阶段 0 的历史审计快照。2026-05-19 之后项目已经新增 `Modes/Network`、RPC、Replication、网络化 Projectile / Health / Buff / Tower / DestructibleProp / SpikeTrap 等实现。当前联网同步归属以 `Docs/06-network-mode-developer-guide.md` 的“当前同步数据归属清单”为准。

---

## 1. 阶段 0 结论

当前项目的多人玩法仍然是“本地分屏多人”架构，不是“网络多人”架构。代码中没有发现正式的 UE 网络复制/RPC 实现：

- 没有 `DOREPLIFETIME`
- 没有 `GetLifetimeReplicatedProps`
- 没有 `UPROPERTY(Replicated)`
- 没有 `UFUNCTION(Server)`
- 没有 `UFUNCTION(Client)`
- 没有 `UFUNCTION(NetMulticast)`
- 没有 `bReplicates = true`
- 没有 `SetReplicateMovement(true)`

这不是坏事，反而说明边界很清楚：不要直接把现有 FreeForAll、TeamBattle、MOBA 硬改成联网模式，应该新增独立 `Modes/Network`，让旧模式继续服务本地分屏，新联网模式按服务器权威重新接入共享战斗类。

最重要的好消息是：`PlayerIndex` 已经拆成 `SlotId / TeamId / LocalPlayerIndex`，这个改造让联网身份体系有了正确地基。

---

## 2. 审计范围

本次重点扫描了这些模式和共享系统：

- `Modes/FreeForAll`
- `Modes/TeamBattle`
- `Modes/MOBA`
- `Modes/MainMenu`
- `Shared/Pawns`
- `Shared/Combat`
- `Shared/Buffs`
- `Shared/State`
- `Shared/Controllers`
- `Shared/UI`
- `Core/BattleBlasterGameInstance`

重点搜索了：

```text
CreatePlayer / RemovePlayer / GetNumLocalPlayerControllers
GetPlayerController / GetFirstPlayerController / GetGameViewport
GetAuthGameMode / OpenLevel / ClientTravel
Replicated / DOREPLIFETIME / Server / Client / NetMulticast
SpawnActor / Possess / ApplyDamage / CurrentHealth / CurrentAmmo
SlotId / TeamId / LocalPlayerIndex / SelectedTankClasses
```

---

## 3. 本地分屏依赖点

### 3.1 MainMenu 创建 4 个本地玩家

位置：

- `Source/BattleBlaster/Modes/MainMenu/MainMenuGameMode.cpp:16`
- `Source/BattleBlaster/Modes/MainMenu/MainMenuGameMode.cpp:25`
- `Source/BattleBlaster/Modes/MainMenu/MainMenuGameMode.cpp:33`
- `Source/BattleBlaster/Modes/MainMenu/MainMenuGameMode.cpp:36`
- `Source/BattleBlaster/Modes/MainMenu/MainMenuGameMode.cpp:88`

现状：

- 菜单 GameMode 使用 `GetFirstPlayerController()`。
- 菜单阶段强制禁用分屏。
- 菜单阶段最多创建 4 个 `LocalPlayer`，供本地多人菜单复用。
- 菜单 UI 由 GameMode 直接 `CreateWidget` 并 `AddToViewport`。

联网影响：

- 本地菜单可以继续这样写。
- 联网 Lobby 不应该复用这套“提前创建 4 个 LocalPlayer”的逻辑。
- 联网 Lobby 应该只为本机玩家创建 UI，其他玩家状态来自 `GameState->PlayerArray`。

### 3.2 FreeForAll GameMode 强依赖 LocalPlayer

位置：

- `Source/BattleBlaster/Modes/FreeForAll/BattleBlasterGameMode.cpp:48`
- `Source/BattleBlaster/Modes/FreeForAll/BattleBlasterGameMode.cpp:77`
- `Source/BattleBlaster/Modes/FreeForAll/BattleBlasterGameMode.cpp:81`
- `Source/BattleBlaster/Modes/FreeForAll/BattleBlasterGameMode.cpp:94`
- `Source/BattleBlaster/Modes/FreeForAll/BattleBlasterGameMode.cpp:159`
- `Source/BattleBlaster/Modes/FreeForAll/BattleBlasterGameMode.cpp:197`
- `Source/BattleBlaster/Modes/FreeForAll/BattleBlasterGameMode.cpp:201`
- `Source/BattleBlaster/Modes/FreeForAll/BattleBlasterGameMode.cpp:208`
- `Source/BattleBlaster/Modes/FreeForAll/BattleBlasterGameMode.cpp:239`
- `Source/BattleBlaster/Modes/FreeForAll/BattleBlasterGameMode.cpp:252`
- `Source/BattleBlaster/Modes/FreeForAll/BattleBlasterGameMode.cpp:532`
- `Source/BattleBlaster/Modes/FreeForAll/BattleBlasterGameMode.cpp:555`
- `Source/BattleBlaster/Modes/FreeForAll/BattleBlasterGameMode.cpp:583`
- `Source/BattleBlaster/Modes/FreeForAll/BattleBlasterGameMode.cpp:700`

现状：

- GameMode 根据 `ViewportPlayerCount` 主动 `CreatePlayer` / `RemovePlayer`。
- GameMode 在 `BeginPlay` 里一次性按 `TargetPlayerCount` 生成所有 Tank。
- 人类玩家通过 `GetPlayerController(GetWorld(), i)` 获取本地 Controller 并 Possess。
- UI 由 GameMode 使用 `PC0` 创建：开始提示、计分板、黑屏、结算界面。
- 复活时按 `SlotId` 在本地数组里找 Controller 和 Tank。

联网影响：

- 网络玩家不是由 `CreatePlayer` 生成，而是连接进来后由 `PostLogin` 产生。
- 网络 GameMode 不应该使用 `GetPlayerController(World, i)` 作为全局玩家表。
- 服务器可以保留 `SlotId -> Tank` 的数组，但 Controller 来源应来自 `PostLogin` / `PlayerState`。
- UI 必须移到 PlayerController 或 Widget 侧，不能由 GameMode 创建给 `PC0`。

### 3.3 TeamBattle GameMode 与 FreeForAll 同类问题

位置：

- `Source/BattleBlaster/Modes/TeamBattle/TeamBattleGameMode.cpp:83`
- `Source/BattleBlaster/Modes/TeamBattle/TeamBattleGameMode.cpp:121`
- `Source/BattleBlaster/Modes/TeamBattle/TeamBattleGameMode.cpp:125`
- `Source/BattleBlaster/Modes/TeamBattle/TeamBattleGameMode.cpp:138`
- `Source/BattleBlaster/Modes/TeamBattle/TeamBattleGameMode.cpp:215`
- `Source/BattleBlaster/Modes/TeamBattle/TeamBattleGameMode.cpp:270`
- `Source/BattleBlaster/Modes/TeamBattle/TeamBattleGameMode.cpp:276`
- `Source/BattleBlaster/Modes/TeamBattle/TeamBattleGameMode.cpp:286`
- `Source/BattleBlaster/Modes/TeamBattle/TeamBattleGameMode.cpp:322`
- `Source/BattleBlaster/Modes/TeamBattle/TeamBattleGameMode.cpp:337`
- `Source/BattleBlaster/Modes/TeamBattle/TeamBattleGameMode.cpp:571`
- `Source/BattleBlaster/Modes/TeamBattle/TeamBattleGameMode.cpp:596`
- `Source/BattleBlaster/Modes/TeamBattle/TeamBattleGameMode.cpp:625`
- `Source/BattleBlaster/Modes/TeamBattle/TeamBattleGameMode.cpp:757`
- `Source/BattleBlaster/Modes/TeamBattle/TeamBattleGameMode.cpp:786`

现状：

- 固定 4 人团队分屏。
- 主动创建/删除本地玩家。
- 直接按 SlotId 取本地 PlayerController。
- GameMode 创建团队 UI 和结算 UI。
- `CanDealDamage` 已经改成 `TeamId` 判断，这是联网友好的部分。

联网影响：

- `TeamId` 判断可以复用。
- 固定 4 人的规则可以复用。
- 玩家连接、Ready、选队、生成、UI 创建都要重写。

### 3.4 MOBA GameMode 与 LocalPlayer 和 Tick 绑定

位置：

- `Source/BattleBlaster/Modes/MOBA/TankMOBAGameMode.cpp:60`
- `Source/BattleBlaster/Modes/MOBA/TankMOBAGameMode.cpp:92`
- `Source/BattleBlaster/Modes/MOBA/TankMOBAGameMode.cpp:96`
- `Source/BattleBlaster/Modes/MOBA/TankMOBAGameMode.cpp:109`
- `Source/BattleBlaster/Modes/MOBA/TankMOBAGameMode.cpp:178`
- `Source/BattleBlaster/Modes/MOBA/TankMOBAGameMode.cpp:244`
- `Source/BattleBlaster/Modes/MOBA/TankMOBAGameMode.cpp:250`
- `Source/BattleBlaster/Modes/MOBA/TankMOBAGameMode.cpp:288`
- `Source/BattleBlaster/Modes/MOBA/TankMOBAGameMode.cpp:296`
- `Source/BattleBlaster/Modes/MOBA/TankMOBAGameMode.cpp:596`
- `Source/BattleBlaster/Modes/MOBA/TankMOBAGameMode.cpp:638`
- `Source/BattleBlaster/Modes/MOBA/TankMOBAGameMode.cpp:658`
- `Source/BattleBlaster/Modes/MOBA/TankMOBAGameMode.cpp:779`
- `Source/BattleBlaster/Modes/MOBA/TankMOBAGameMode.cpp:901`
- `Source/BattleBlaster/Modes/MOBA/TankMOBAGameMode.cpp:913`

现状：

- 按 ViewportPlayerCount 创建/删除本地玩家。
- 延迟 0.5 秒后按 `GetPlayerController(GameMode->GetWorld(), i)` 让本地 Controller Possess。
- 顶部状态 UI 和结算 UI 由 GameMode 创建。
- 复活倒计时由 GameMode Tick 驱动。
- MOBA 的 `CampIndex` 已经能同步到 `TeamId`，这是有利点。

联网影响：

- MOBA 不是第一阶段好目标，迁移成本高。
- 防御塔、核心塔、淘汰、复活倒计时都需要复制到 GameState/PlayerState。
- 顶部 UI 必须从 `GameState` 读复制数据。

---

## 4. 选人菜单依赖点

### 4.1 FreeForAll 选人菜单

位置：

- `Source/BattleBlaster/Modes/FreeForAll/UI/MutiBattleMenuWidget.cpp:136`
- `Source/BattleBlaster/Modes/FreeForAll/UI/MutiBattleMenuWidget.cpp:288`
- `Source/BattleBlaster/Modes/FreeForAll/UI/MutiBattleMenuWidget.cpp:310`
- `Source/BattleBlaster/Modes/FreeForAll/UI/MutiBattleMenuWidget.cpp:424`
- `Source/BattleBlaster/Modes/FreeForAll/UI/MutiBattleMenuWidget.cpp:440`
- `Source/BattleBlaster/Modes/FreeForAll/UI/MutiBattleMenuWidget.cpp:526`
- `Source/BattleBlaster/Modes/FreeForAll/UI/MutiBattleMenuWidget.cpp:535`
- `Source/BattleBlaster/Modes/FreeForAll/UI/MutiBattleMenuWidget.cpp:553`
- `Source/BattleBlaster/Modes/FreeForAll/UI/MutiBattleMenuWidget.cpp:609`

现状：

- 菜单通过 `SlotId` 对应本地 PlayerController。
- 选人结果写入 `GameInstance->SelectedTankClasses`。
- 目标人数写入 `GameInstance->TargetPlayerCount`。
- 设备映射写入 GameInstance。

联网影响：

- 联网 Lobby 不能直接把每个玩家选人写入本机 GameInstance。
- 正确流程应是：本地选择 -> `ServerSelectTank` -> 服务器验证 -> 写入 PlayerState -> 复制给所有客户端。

### 4.2 TeamBattle / MOBA 选人菜单

位置：

- `Source/BattleBlaster/Modes/TeamBattle/UI/TeamBattleMenuWidget.cpp:202`
- `Source/BattleBlaster/Modes/TeamBattle/UI/TeamBattleMenuWidget.cpp:225`
- `Source/BattleBlaster/Modes/TeamBattle/UI/TeamBattleMenuWidget.cpp:232`
- `Source/BattleBlaster/Modes/TeamBattle/UI/TeamBattleMenuWidget.cpp:253`
- `Source/BattleBlaster/Modes/TeamBattle/UI/TeamBattleMenuWidget.cpp:265`
- `Source/BattleBlaster/Modes/TeamBattle/UI/TeamBattleMenuWidget.cpp:322`
- `Source/BattleBlaster/Modes/MOBA/UI/MOBASetupWidget.cpp:273`
- `Source/BattleBlaster/Modes/MOBA/UI/MOBASetupWidget.cpp:293`
- `Source/BattleBlaster/Modes/MOBA/UI/MOBASetupWidget.cpp:329`
- `Source/BattleBlaster/Modes/MOBA/UI/MOBASetupWidget.cpp:497`
- `Source/BattleBlaster/Modes/MOBA/UI/MOBASetupWidget.cpp:506`
- `Source/BattleBlaster/Modes/MOBA/UI/MOBASetupWidget.cpp:522`

现状：

- 和 FreeForAll 类似，都是本地多 Controller 菜单。
- TeamBattle 固定 4 人。
- MOBA 根据当前玩家数写入 GameInstance。

联网影响：

- 第一版联网模式不建议复用这些 UI。
- 可以参考它们的 TankOptions 配置和图片刷新逻辑，但玩家列表/Ready/选择同步要重写。

---

## 5. 状态复制缺口

### 5.1 PlayerState 尚未复制

位置：

- `Source/BattleBlaster/Shared/State/TankPlayerState.h:44`
- `Source/BattleBlaster/Shared/State/TankPlayerState.h:48`
- `Source/BattleBlaster/Shared/State/TankPlayerState.h:74`
- `Source/BattleBlaster/Shared/State/TankPlayerState.h:157`
- `Source/BattleBlaster/Shared/State/TankPlayerState.h:160`

当前关键字段：

- `SlotId`
- `TeamId`
- `IsAlive`
- `CurrentAmmo`
- `KillCount`
- `DeathCount`
- `AssistCount`
- `CurrentBuffs`
- 出生点信息

联网影响：

- 网络模式至少要复制 `SlotId`、`TeamId`、KDA、死亡状态、选择坦克、Ready 状态。
- `CurrentAmmo` 放 PlayerState 还是 Tank，需要后续统一。第一版可以放 Tank 复制，PlayerState 保留战绩和身份。

### 5.2 GameState 尚未复制

位置：

- `Source/BattleBlaster/Shared/State/TankGameState.h`
- `Source/BattleBlaster/Modes/FreeForAll/TankBattleGameState.h`
- `Source/BattleBlaster/Modes/TeamBattle/TeamBattleGameState.h`
- `Source/BattleBlaster/Modes/MOBA/TankMOBAGameState.h`

当前状态：

- `MatchTimeSeconds`
- `CountdownSeconds`
- `GameStatus`
- FFA 玩家分数数组
- TeamBattle 队伍分数
- MOBA 防御塔数量和胜者

联网影响：

- 这些都必须成为服务器写、客户端读的复制状态。
- `TMap<int32, int32>` 用于复制要谨慎；第一版建议用固定数组或结构数组，降低复制复杂度。

### 5.3 HealthComponent 尚未复制

位置：

- `Source/BattleBlaster/Shared/Combat/HealthComponent.h:34`
- `Source/BattleBlaster/Shared/Combat/HealthComponent.h:40`
- `Source/BattleBlaster/Shared/Combat/HealthComponent.cpp:20`
- `Source/BattleBlaster/Shared/Combat/HealthComponent.cpp:48`
- `Source/BattleBlaster/Shared/Combat/HealthComponent.cpp:53`
- `Source/BattleBlaster/Shared/Combat/HealthComponent.cpp:85`

现状：

- 组件监听 Owner 的 `OnTakeAnyDamage`。
- 直接修改 `CurrentHealth` / `CurrentShield`。
- 用委托刷新本地 UI。

联网影响：

- 应改为服务器修改生命/护盾。
- `CurrentHealth` / `CurrentShield` 需要 `ReplicatedUsing=OnRep_Health` 或类似机制。
- 客户端 UI 不应该依赖服务器端委托直接广播，而应响应复制或由本地 Owner 刷新。

### 5.4 BuffComponent 尚未复制

位置：

- `Source/BattleBlaster/Shared/Buffs/TankBuffComponent.h:84`
- `Source/BattleBlaster/Shared/Buffs/TankBuffComponent.h:169`
- `Source/BattleBlaster/Shared/Buffs/TankBuffComponent.cpp:50`
- `Source/BattleBlaster/Shared/Buffs/TankBuffComponent.cpp:149`
- `Source/BattleBlaster/Shared/Buffs/TankBuffComponent.cpp:172`
- `Source/BattleBlaster/Shared/Buffs/TankBuffComponent.cpp:376`
- `Source/BattleBlaster/Shared/Buffs/TankBuffComponent.cpp:549`

现状：

- `ActiveBuffs` 是本地 `TMap`。
- Tick 每帧扣持续时间。
- `AddBuff` 直接修改 Tank 属性。
- 窒息 UI 在组件里直接创建 Widget。

联网影响：

- Buff 添加/移除必须由服务器决定。
- Buff 状态应复制给客户端显示。
- Buff 表现 UI 不宜由 BuffComponent 直接 `CreateWidget`，网络模式应通过 PlayerController/HUD 显示。
- 第一版可先复制简化结构：BuffType + RemainingTime，不必复制图标指针。

---

## 6. 移动与战斗权威缺口

### 6.1 Tank 移动当前是本地直接执行

位置：

- `Source/BattleBlaster/Shared/Pawns/Tank.cpp:348`
- `Source/BattleBlaster/Shared/Pawns/Tank.cpp:368`
- `Source/BattleBlaster/Shared/Pawns/Tank.cpp:384`
- `Source/BattleBlaster/Shared/Pawns/Tank.cpp:437`

现状：

- Pawn 直接绑定 Enhanced Input。
- `MoveInput` 在本地根据输入修改位置。
- AI 使用 `MoveAI` / `MoveWithAI`。

联网影响：

- 第一版联网模式要加 `ServerMoveInput`，由服务器执行移动。
- `ATank` 需要开启 Actor 复制和 Movement 复制，或者做自定义同步。
- 局域网第一版可以先用服务器权威移动，不做客户端预测。

### 6.2 Tank 开火当前是本地直接生成 Projectile

位置：

- `Source/BattleBlaster/Shared/Pawns/Tank.cpp:597`
- `Source/BattleBlaster/Shared/Pawns/Tank.cpp:633`
- `Source/BattleBlaster/Shared/Pawns/Tank.cpp:667`

现状：

- `Fire()` 检查弹药和冷却。
- 直接扣 `CurrentAmmo`。
- 直接 `SpawnActorDeferred<AProjectile>`。
- 直接刷新 HUD。

联网影响：

- 网络模式必须改为 `ServerFire`。
- 服务器检查弹药、冷却、死亡、比赛状态。
- 服务器扣弹药、生成 Projectile。
- HUD 从复制弹药刷新。

### 6.3 Projectile 当前没有服务器 Authority 保护

位置：

- `Source/BattleBlaster/Shared/Combat/Projectile.cpp:62`
- `Source/BattleBlaster/Shared/Combat/Projectile.cpp:73`
- `Source/BattleBlaster/Shared/Combat/Projectile.cpp:105`
- `Source/BattleBlaster/Shared/Combat/Projectile.cpp:147`
- `Source/BattleBlaster/Shared/Combat/Projectile.cpp:188`
- `Source/BattleBlaster/Shared/Combat/Projectile.cpp:211`
- `Source/BattleBlaster/Shared/Combat/Projectile.cpp:328`

现状：

- `OnHit` 直接进行敌我判断和 `ApplyDamage`。
- 命中后直接 `Destroy()`。
- 穿墙弹通过碰撞通道忽略环境，但仍 Block Pawn。
- 炮弹相撞抵消依赖 Projectile 通道阻挡，这是设计特性，应保留。

联网影响：

- `OnHit` 只能在服务器处理伤害和 Destroy。
- Projectile 应开启复制和移动复制。
- 客户端可以播放特效，但不能决定伤害。

---

## 7. UI 与 GameMode 边界问题

### 7.1 GameMode 创建 UI

涉及：

- FreeForAll 开始提示、比分、黑屏、结算。
- TeamBattle 开始提示、比分、黑屏、结算。
- MOBA 顶部状态、结算。
- Stage 单人模式也有 GameMode UI，但它不属于联网第一阶段。

联网规则：

- Dedicated Server 没有本地视口。
- GameMode 只在服务器存在。
- 联网 UI 应由 PlayerController 或 Widget 创建。
- UI 数据来自 PlayerState/GameState，不来自 GameMode。

### 7.2 TankPlayerController 部分逻辑读取 GameMode

位置：

- `Source/BattleBlaster/Shared/Controllers/TankPlayerController.cpp:81`
- `Source/BattleBlaster/Shared/Controllers/TankPlayerController.cpp:103`
- `Source/BattleBlaster/Shared/Controllers/TankPlayerController.cpp:104`
- `Source/BattleBlaster/Shared/Controllers/TankPlayerController.cpp:576`

现状：

- HUD 初始化时通过 `GetAuthGameMode()` 判断当前模式。
- KDA 颜色用 `LocalPlayerIndex` + GameMode 类型判断。
- 观战模式也通过 `GetAuthGameMode()` 判断是否 MOBA。

联网影响：

- 客户端上 `GetAuthGameMode()` 通常不可用。
- 联网 HUD 应使用 `PlayerState->TeamId`、`GameState`、Pawn 状态。

### 7.3 GetPlayerController(0) 的使用

涉及很多 UI 和菜单代码。联网对局内不能把 `GetPlayerController(0)` 理解成“玩家 0”，它只代表当前机器的第一个本地玩家。

联网模式里：

- 本地 HUD 可以用 Owning Player。
- 全局玩家数据用 `GameState->PlayerArray`。
- 指定玩家用 `PlayerState` 的 `SlotId` 或 `UniqueId`。

---

## 8. GameInstance 边界

位置：

- `Source/BattleBlaster/Core/BattleBlasterGameInstance.h:189`
- `Source/BattleBlaster/Core/BattleBlasterGameInstance.h:199`
- `Source/BattleBlaster/Core/BattleBlasterGameInstance.h:564`
- `Source/BattleBlaster/Core/BattleBlasterGameInstance.h:572`

现状：

- `TargetPlayerCount`
- `TargetMatchScore`
- `SelectedTankClasses`
- `ConnectedGamepadCount`
- `AIControlledPlayerIndices`
- `PlayerDeviceIdMap`

联网影响：

- GameInstance 只代表本机进程，不自动复制。
- Host 可以把 GameInstance 当作房间初始配置来源。
- Client 不能用本机 GameInstance 决定全局玩家数量或自己最终 TankClass。
- 客户端设置必须通过 RPC 交给服务器确认。

---

## 9. 可复用与不可复用边界

### 9.1 可以直接复用的设计

- `SlotId` / `TeamId` 概念。
- `ATank` 的基础移动、炮塔、开火配置。
- `AProjectile` 的碰撞规则和炮弹相撞特性。
- `UHealthComponent` 的血量/护盾计算思路。
- `UTankBuffComponent` 的 Buff 类型和效果逻辑。
- `AAIBotPlayerController` 的 `TeamId` 敌我判断。
- 现有 Tank 蓝图和输入资源。

### 9.2 可以参考但不直接复用的设计

- 本地多人选人菜单。
- GameMode 里的 `ActiveTanks` / `PlayerStarts` 数组。
- 本地复活流程。
- 本地结算 UI。
- MOBA 顶部 UI。

### 9.3 不建议在联网模式复用的设计

- GameMode 中 `CreatePlayer` / `RemovePlayer`。
- GameMode 中创建 UMG。
- 根据 `GetPlayerController(World, i)` 找全局玩家。
- 根据 `LocalPlayerIndex` 做队伍/颜色/身份判断。
- 用客户端 GameInstance 直接保存全局对局选择。
- 客户端本地直接开火、扣血、加分。

---

## 10. 第一版联网模式边界建议

为了可控，第一版只做：

- 局域网 Listen Server。
- 手动输入 IP Join。
- 2-4 人。
- 每台电脑 1 名玩家。
- 不做联网分屏。
- 不做 MOBA。
- 不做 LAN 房间搜索。
- 不做 Dedicated Server。
- 不做客户端预测。
- 不做复杂大厅，只做最简 Ready/选坦克。

第一版推荐模式：

```text
NetworkBattleMode = 简化 FFA
```

原因：

- FFA 的队伍逻辑最简单。
- 可以先验证连接、生成、移动、开火、伤害、死亡、复活、KDA、分数。
- TeamBattle 可以第二个迁移，因为 `TeamId` 已经很好用。
- MOBA 最后迁移，因为防御塔、淘汰、复活倒计时、顶部 UI 都需要大量复制状态。

---

## 11. 阶段 1 开发任务建议

阶段 1 只做最小 Host / Join，不碰完整战斗。

### 11.1 新增目录

```text
Source/BattleBlaster/Core/Networking/
Source/BattleBlaster/Modes/Network/
```

### 11.2 新增类

```text
UBattleBlasterSessionSubsystem
ANetworkBattleGameMode
ANetworkBattleGameState
ANetworkBattlePlayerState
ANetworkBattlePlayerController
```

### 11.3 最小功能

- 主菜单或临时测试入口调用 Host。
- Host 打开 `NetworkBattleMap?listen`。
- Client 输入 IP 后连接 Host。
- `ANetworkBattleGameMode::PostLogin` 打印玩家加入日志。
- `ANetworkBattleGameMode::Logout` 打印玩家离开日志。
- 暂时不生成 Tank。
- 暂时不做移动和战斗。

### 11.4 阶段 1 验收标准

- PIE 2 Players / Listen Server 能启动。
- 独立两个进程能连接同一个 Host。
- 两台电脑局域网能连接。
- Host 日志能看到 Client 加入。
- Client 断开时 Host 日志能看到离开。

---

## 12. 阶段 2 预告

阶段 2 再做服务器身份分配和 Tank 生成：

- `PostLogin` 分配 `SlotId` / `TeamId`。
- 写入 `NetworkBattlePlayerState`。
- `SlotId` / `TeamId` 加复制。
- 服务器 Spawn Tank。
- 服务器 Possess。
- Tank 开启复制。
- 所有人能看到彼此的 Tank。

阶段 2 之前不要急着改 `ATank::Fire()` 和 `Projectile`。先把连接和身份链路打通，地基稳了再进战斗。

---

## 13. 阶段 0 总结

当前代码不适合“原地联网化”，但非常适合“新增联网模式 + 逐步复用共享战斗类”。

核心策略：

```text
旧模式继续本地分屏
新模式走服务器权威
共享类逐步补 Replication / RPC / Authority Guard
```

下一步应该进入阶段 1：新增联网模块骨架和最小 Host / Join 流程。
