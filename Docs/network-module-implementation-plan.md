# BattleBlaster 网络模块项目落地文档

> 版本：2026-06-05
> 目标：把网络模块从当前最小联机链路推进到可选择 LAN / Dedicated Server、可选择多种网络玩法模式的可落地方案。

---

## 1. 总体目标

网络模块最终分成两条轴：

```text
连接方式
├── 局域网游戏：Listen Server / LAN Join
└── 服务器游戏：Dedicated Server / 公网 IP / 后续服务器列表

玩法模式
├── 多人死斗
├── 团队死斗
├── MOBA
└── 团队 MOBA
```

核心原则：

- LAN 和 Dedicated Server 只是连接和部署方式不同，不写两套玩法规则。
- 所有网络玩法模式继承 `ANetworkGameModeBase`。
- `Modes/Network` 的对局内代码按 Dedicated Server 标准写，同时兼容 Listen Server。
- UMG 蓝图只负责 UI 展示和调用 C++ 暴露接口，不承载网络规则。
- Dedicated Server 没有本地玩家、没有 Viewport、没有 UMG，服务器规则不能依赖这些对象存在。

---

## 2. 目标用户流程

```text
主菜单
-> 网络游戏
   -> 局域网游戏
      -> 加入：输入 IP:Port
      -> 主持：进入游戏设置页
   -> 服务器游戏
      -> 加入：输入 IP:Port，后续可扩展服务器列表
      -> 主持：后续扩展为请求 Dedicated Server 开房

游戏设置页
-> 选择游戏模式
-> 选择人数、AI 数、地图、目标分数等
-> 开始游戏
```

当前阶段可以先做：

- LAN Host：本机 `OpenLevel(Map?listen)`。
- LAN Join：输入 `IP:Port` 后连接。
- Server Join：输入 Dedicated Server 的公网或内网穿透地址后连接。
- Server Host：先不做自动开房，后续接后端或手动启动 Dedicated Server。

---

## 3. 推荐代码结构

```text
Source/BattleBlaster/Core/Networking/
├── BattleBlasterNetworkTypes.h
└── BattleBlasterSessionSubsystem.h/.cpp

Source/BattleBlaster/Modes/Network/
├── NetworkGameModeBase.h/.cpp
├── NetworkGameStateBase.h/.cpp
├── NetworkPlayerStateBase.h/.cpp
├── NetworkPlayerControllerBase.h/.cpp
├── NetworkDeathmatchGameMode.h/.cpp
├── NetworkDeathmatchGameState.h/.cpp
├── NetworkTeamDeathmatchGameMode.h/.cpp
├── NetworkTeamDeathmatchGameState.h/.cpp
├── NetworkMOBAGameMode.h/.cpp
├── NetworkMOBAGameState.h/.cpp
├── NetworkTeamMOBAGameMode.h/.cpp
└── UI/
    ├── CppShowScoresWidget.h/.cpp
    ├── NetworkTeamScoresWidget.h/.cpp
    ├── NetworkMOBAStateWidget.h/.cpp
    ├── NetworkDeathmatchGameOverWidget.h/.cpp
    └── Menu/
        ├── NetworkMenuWidgetBase.h/.cpp
        ├── NetworkModeSelectWidget.h/.cpp
        ├── LANMenuWidget.h/.cpp
        ├── LANHostSettingsWidget.h/.cpp
        └── LANJoinWidget.h/.cpp
```

建议新增的类型：

```cpp
UENUM(BlueprintType)
enum class EBattleBlasterNetworkConnectionType : uint8
{
	LAN,
	DedicatedServer
};

UENUM(BlueprintType)
enum class ENetworkGameModeType : uint8
{
	Deathmatch,
	TeamDeathmatch,
	MOBA,
	TeamMOBA
};

USTRUCT(BlueprintType)
struct FNetworkMatchSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EBattleBlasterNetworkConnectionType ConnectionType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ENetworkGameModeType ModeType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName MapName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Port;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxPlayers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AICount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TargetScore;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TeamCount;
};
```

---

## 4. 当前实现快照

截至 2026-06-05，网络模块已经具备以下基础能力：

| 能力 | 状态 | 说明 |
| --- | --- | --- |
| 网络基础基类 | 已实现基础版 | `ANetworkGameModeBase`、`ANetworkGameStateBase`、`ANetworkPlayerStateBase`、`ANetworkPlayerControllerBase` 已落地 |
| LAN Host / Join | 已实现基础版 | C++ 网络菜单通过 `FNetworkMatchSettings` Host Listen Server、输入 IP:Port 加入 |
| 主菜单入口 | 已实现基础版 | `UMainMenuWidget::OpenNetworkMenu()` 进入网络菜单 |
| 网络个人死斗 | 已实现基础版 | 个人分数、目标分、复活、胜负、结算 UI |
| 网络团队死斗 | 已实现基础版 | 队伍分配、友伤过滤、团队分数、团队胜负、团队分数 UI |
| 网络 MOBA | 已实现基础版 | 核心塔注册/摧毁、核心塔倒后死亡淘汰、胜负、核心塔状态 UI |
| 网络团队 MOBA | 已实现基础版 | 复用网络 MOBA 规则，按 TeamId 分队 |
| 网络顶部 UI | 已实现基础版 | 个人死斗、团队死斗、MOBA 分别使用独立 C++ UI，可由蓝图子类替换 |
| AI 填充 | 已实现基础版 | `AICount` 由 Host 设置传入，服务器使用 `AAIBotPlayerController` 填充末尾槽位 |
| Dedicated Server | 未完成 | 代码按 Dedicated Server 兼容原则推进，但还未完成 Server Target、构建和部署测试 |

---

## 5. 阶段规划

### 阶段 1：稳定网络战斗基类（已实现基础版）

目标：确保 `ANetworkGameModeBase` 是所有网络玩法的公共基类。

我负责：

- 已维护 `ANetworkGameModeBase` 的公共流程：连接、分配身份、Spawn、死亡、复活。
- 已保持基类不写死具体玩法规则。
- 已提供并使用钩子：`ChooseTeamIdForSlot()`、`UsesTeamDamageRules()`、`AreTeamIdsHostile()`、`CanTankDamageTank()`、`ShouldRespawnPlayer()`、`HandleNetworkTankKilled()`、`CheckNetworkGameOver()`。
- 持续保证基类代码 Dedicated Server 兼容。

你负责：

- 在编辑器中新建并维护 `BP_NetworkGameModeBase` / `BP_NetworkPlayerControllerBase` 等蓝图资产。
- 地图和网络模式蓝图确认切到新 BP 后，再删除旧的 `BP_NetworkBattleGameMode` / `BP_NetworkBattlePlayerController`。
- 测试现有网络地图在 PIE Listen Server 下是否仍能正常移动、开火、死亡、复活。

验收：

- 现有网络测试不回退。
- Host 和 Client 都能正常生成、移动、开火、死亡、复活。

### 阶段 2：实现网络多人死斗（已实现基础版）

目标：创建第一套具体网络玩法模式。

我负责：

- 新建 `ANetworkDeathmatchGameMode`。
- 新建 `ANetworkDeathmatchGameState`。
- 添加目标分数、击杀加分、环境死亡 / 自杀扣分。
- 复用 `ATankPlayerState::ProcessDeath()` 的 Killer / Assist 归因。
- 把 `PlayerScores`、`TargetScore`、`WinnerSlotId` 写入并复制到 `ANetworkDeathmatchGameState`。
- 在 `ANetworkPlayerControllerBase` 暴露 `ScoresWidgetClass`，默认使用 `UCppShowScoresWidget` 刷新网络死斗比分。
- 新增 `UNetworkDeathmatchGameOverWidget`，由本地 `ANetworkPlayerControllerBase` 在 `WinnerSlotId` 同步后创建结算界面。
- 在 `ANetworkGameStateBase` 复制 `bIsMatchOver`，比赛结束后 Tank 移动和开火入口拒绝继续执行。
- 更新 API 和网络开发文档。

你负责：

- 在编辑器中创建或配置 `BP_NetworkDeathmatchGameMode`。
- 配置网络死斗地图使用正确 GameMode 蓝图。
- 如需正式表现，在 `BP_NetworkPlayerControllerBase` 或其模式子类中把 `ScoresWidgetClass` 设置为继承 `UCppShowScoresWidget` 的蓝图子类；不设置时使用 C++ 默认 UI。
- 在 `BP_NetworkPlayerControllerBase` 或其模式子类中把 `DeathmatchGameOverWidgetClass` 设置为网络死斗结算 Widget 蓝图。
- 在 UMG 中准备或调整网络死斗 HUD / 计分板 / 结算 UI。
- 测试目标分数、击杀、死亡、复活和结算表现。

验收：

- 两名玩家能完成完整死斗对局。
- KDA、分数、胜负在 Host 和 Client 上一致。

### 阶段 3：网络入口设置结构（已实现基础版）

目标：让 UMG 蓝图可以通过 C++ 接口发起 Host / Join。

当前状态：

- 已新增正式 C++ 菜单入口：`UNetworkModeSelectWidget`、`ULANMenuWidget`、`ULANHostSettingsWidget`、`ULANJoinWidget`。
- `UMainMenuWidget::OpenNetworkMenu()` 已作为主菜单进入网络菜单的入口。
- `BattleBlasterNetworkTypes.h` 已提供 `EBattleBlasterNetworkConnectionType`、`ENetworkGameModeType`、`FNetworkMatchSettings`。
- `UBattleBlasterSessionSubsystem` 已提供 `HostNetworkGame()`、`JoinNetworkGame()`、`BuildTravelOptions()`，并保留 `HostListenServerWithOptions()` / `JoinByIpAndPort()` 兼容旧入口。
- `ANetworkGameModeBase` 已解析 `MaxPlayers` URL Option。
- `ANetworkGameModeBase` 已解析 `AICount` URL Option，并在服务器生成网络 AI 玩家。
- `ANetworkDeathmatchGameMode` 已解析 `TargetScore` URL Option。
- `ULANHostSettingsWidget` 已改为构造 `FNetworkMatchSettings`，具体网络 GameMode 选择和 URL Options 拼接由 `UBattleBlasterSessionSubsystem` 负责。
- `ULANHostSettingsWidget` 已移除手填地图名，改为“当前地图卡片 + Change Map”流程。
- `UNetworkMapSelectWidget` 已提供网络地图选择页，内部按模式拆分为 `DeathmatchMaps`、`TeamDeathmatchMaps`、`MOBAMaps`、`TeamMOBAMaps` 四个专属地图容器。
- `TargetScore`、`TeamCount`、`MaxPlayers` 已作为 URL Options 传入对应网络 GameMode。

我负责：

- 已新建并接入 `BattleBlasterNetworkTypes.h`。
- 已在 `UBattleBlasterSessionSubsystem` 中提供 `BlueprintCallable` / `BlueprintPure`：
  - `HostNetworkGame(const FNetworkMatchSettings& Settings)`
  - `JoinNetworkGame(const FString& Address, int32 Port)`
  - `BuildTravelOptions(const FNetworkMatchSettings& Settings)`
- 已设计 URL Options，让服务器进入地图后能读取模式、人数、AI 数、目标分数等。
- 已把当前 LAN Host 菜单从零散 URL 字符串升级为 `FNetworkMatchSettings` 结构化入口。
- 后续继续保证 LAN / Dedicated Server 连接方式和玩法模式解耦。

你负责：

- 在 `MainMenuWidget` 蓝图中把网络游戏按钮绑定到 `OpenNetworkMenu()`。
- 在 `MainMenuWidget` 蓝图中设置 `NetworkMenuClass`。
- 后续如果需要正式美术表现，可以创建继承当前 C++ 菜单类的 UMG 蓝图，替换外观而不重写 Host / Join 规则。
- 后续如果你创建 UMG 蓝图子类，优先调用结构化 Host / Join 接口，不要在蓝图里手写 URL Options。

美术 / UMG 待办：

- 新建 `BP_NetworkMapSelectWidget`，父类选择 `UNetworkMapSelectWidget`。
- 可以复制旧 `WBP_SelectMapWidget` 的布局风格，控件绑定名尽量保持一致：`Btn_Map0..Btn_Map3`、`Border_Map0..Border_Map3`、`Text_MapName0..Text_MapName3`、`Btn_Confirm`、`Btn_Back`、`Btn_PrevPage`、`Btn_NextPage`、`Text_PageNumber`。
- 在 `BP_NetworkMapSelectWidget` 默认值中维护四个专属地图容器：`DeathmatchMaps`、`TeamDeathmatchMaps`、`MOBAMaps`、`TeamMOBAMaps`。
- 每个地图项填写 `MapDisplayName`、`LevelAsset`、`MapThumbnail`、`MinPlayers`、`MaxPlayers`。优先设置 `LevelAsset`，`LevelName` 只作为手动兜底或特殊覆盖。
- 新建或配置 `BP_LANHostSettingsWidget`，父类为 `ULANHostSettingsWidget`，设置 `MapSelectWidgetClass = BP_NetworkMapSelectWidget`。
- 新建或配置 `BP_LANMenuWidget`，设置 `HostSettingsWidgetClass = BP_LANHostSettingsWidget`。
- 新建或配置 `BP_NetworkModeSelectWidget`，设置 `LANMenuWidgetClass = BP_LANMenuWidget`。
- 在主菜单 `MainMenuWidget` 中设置 `NetworkMenuClass = BP_NetworkModeSelectWidget`。

美术验收：

- Host 设置页显示当前地图卡片，而不是手填地图文本框。
- 点击 Change Map 后进入网络地图选择页。
- 切换不同网络模式时，地图选择页只展示该模式容器里的地图。
- 确认地图后返回 Host 设置页，并更新当前地图卡片。
- 点击 Start Host 后进入所选地图。

验收：

- C++ 默认菜单可以发起 LAN Host。
- C++ 默认菜单可以输入 IP:Port 加入。
- 设置页能把地图、人数、目标分数传给 C++，其中人数和目标分数已被网络 GameMode 解析。

### 阶段 4：网络 AI 填充（已实现基础版）

目标：让 Host 设置中的 `AICount` 真的生成服务器权威 AI 玩家。

当前状态：

- `MaxPlayers` 表示总槽位数，`AICount` 表示其中由 AI 填充的槽位数，不额外扩容。
- Host 菜单限制 `AICount <= MaxPlayers - 1`，至少保留 1 个真人槽位。
- AI 默认占用末尾 Slot，例如 `MaxPlayers=4, AICount=2` 时使用 Slot 2 和 Slot 3。
- AI 使用 `AAIBotPlayerController`，并拥有 `ANetworkPlayerStateBase`。
- AI 的 `SlotId`、`TeamId`、`bIsAIPlayer`、PlayerName 由服务器初始化并复制。
- AI 与真人共用 Tank Spawn、死亡、复活、KDA 和计分路径。
- AI 在网络模式下通过 `ANetworkGameModeBase::AreTeamIdsHostile()` 判断敌我。

你负责：

- 在 Host 菜单中选择 AI 数量并测试。
- 如果需要正式表现，可以在 UI 中根据 `ANetworkPlayerStateBase::IsAIPlayer()` 显示 AI 名称或图标。
- 如果 AI 行为强度不合适，可以在蓝图 GameMode 子类中调整 `NetworkAIDifficulty` 或替换 `AIControllerClass`。

验收：

- Host 设置 `AICount > 0` 后，服务器生成对应数量的 AI Tank。
- Client 能看到 AI 移动、开火、死亡和复活。
- AI 击杀玩家、玩家击杀 AI 后，KDA/比分/团队分数能同步。
- 团队死斗、MOBA、团队 MOBA 中 AI 不攻击友军。

### 阶段 5：Dedicated Server 目标

目标：让项目具备专用服务器运行能力。

我负责：

- 检查并补齐 Server Target。
- 编译 Dedicated Server。
- 提供本地启动命令示例。
- 确保网络玩法 GameMode 不创建 UMG、不依赖本地玩家。
- 测试客户端连接独立 Server 进程。

你负责：

- 准备测试地图和蓝图配置。
- 按文档启动 Dedicated Server 或提供服务器机器环境。
- 在客户端输入服务器地址测试连接。

验收：

- 独立 Server 进程能启动网络地图。
- 两个客户端能连接同一 Dedicated Server。
- 死斗模式核心流程正常。

### 阶段 5：网络团队死斗（已实现基础版）

目标：实现团队分配和团队胜负。

我负责：

- 已新建 `ANetworkTeamDeathmatchGameMode`。
- 已覆盖 `ChooseTeamIdForSlot()`。
- 已实现友伤过滤、团队分数、团队胜负。
- 已同步团队分数到 `ANetworkTeamDeathmatchGameState`。
- 已新增 `UNetworkTeamScoresWidget`，由 `ANetworkPlayerControllerBase` 自动创建。

你负责：

- 配置团队死斗 GameMode 蓝图。
- 后续可制作 `UNetworkTeamScoresWidget` 的蓝图子类，替换 C++ 默认表现。
- 测试队伍颜色、友伤、团队结算。

验收：

- 玩家按队伍分配。
- 队友不能被误伤或按设计规则处理。
- 团队分数和胜负一致。

### 阶段 6：网络 MOBA / 团队 MOBA（已实现基础版）

目标：迁移最复杂的网络玩法。

我负责：

- 已新建 `ANetworkMOBAGameMode` 和 `ANetworkTeamMOBAGameMode`。
- 已梳理核心塔、复活、淘汰、胜负基础规则。
- 已让核心 `ATurret` 在网络 MOBA 中注册核心塔并通知核心塔摧毁。
- 已复用 Turret / TurretProjectile 的 TeamId 敌我规则。
- 已保证 MOBA 规则不依赖本地分屏 PlayerIndex。
- 已新增 `UNetworkMOBAStateWidget`，显示各队核心塔和淘汰状态。

你负责：

- 配置 MOBA 网络地图。
- 配置 MOBA 网络 GameMode 蓝图。
- 后续可制作 `UNetworkMOBAStateWidget` 的蓝图子类，替换 C++ 默认表现。
- 大量进行编辑器内多人测试。

验收：

- 核心塔被摧毁后，玩家后续死亡才淘汰。
- 淘汰规则、复活规则、胜负判定符合原设计。
- Tower / Turret 不攻击友方玩家。

### 阶段 7：服务器部署和内网穿透

目标：验证公网或准公网联机。

我负责：

- 提供 Dedicated Server 启动命令、端口说明、日志检查方法。
- 提供云服务器 / 内网穿透连接方案建议。
- 记录测试步骤和常见问题。

你负责：

- 提供云服务器、内网主机或穿透工具环境。
- 配置端口开放和 UDP 转发。
- 进行真实外网连接测试。

验收：

- 外部客户端可以连接服务器。
- 延迟、丢包、命中同步在可接受范围。

---

## 6. 近期建议执行顺序

1. 给 Server Game 分支接入 Dedicated Server / 公网地址的最小 Join 流程。
2. 你按当前 C++ 菜单类和 C++ 网络 UI 类创建正式 UMG 表现层，或者先继续使用 C++ 默认菜单和默认 UI 测试。
3. 编译 Dedicated Server，做本机双客户端、AI 填充和真实外网/内网穿透测试。

当前已经有真实可玩的网络玩法原型，后续重点不再是“有没有网络模式”，而是把设置入口、AI、服务器部署和表现层打磨成可长期维护的系统。

