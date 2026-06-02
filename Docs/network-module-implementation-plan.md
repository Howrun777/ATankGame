# BattleBlaster 网络模块项目落地文档

> 版本：2026-05-27
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
├── NetworkTeamDeathmatchGameMode.h/.cpp
├── NetworkMOBAGameMode.h/.cpp
└── NetworkTeamMOBAGameMode.h/.cpp
```

建议新增的类型：

```cpp
UENUM(BlueprintType)
enum class ENetworkConnectionType : uint8
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
	ENetworkConnectionType ConnectionType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ENetworkGameModeType ModeType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName MapId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxPlayers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 AIBotCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TargetScore;
};
```

---

## 4. 阶段规划

### 阶段 1：稳定网络战斗基类

目标：确保 `ANetworkGameModeBase` 是所有网络玩法的公共基类。

我负责：

- 维护 `ANetworkGameModeBase` 的公共流程：连接、分配身份、Spawn、死亡、复活。
- 保持基类不写死具体玩法规则。
- 继续完善钩子：`ChooseTeamIdForSlot()`、`ShouldRespawnPlayer()`、`HandleNetworkTankKilled()`、`CheckNetworkGameOver()`。
- 保证基类代码 Dedicated Server 兼容。

你负责：

- 在编辑器中新建并维护 `BP_NetworkGameModeBase` / `BP_NetworkPlayerControllerBase` 等蓝图资产。
- 地图和网络模式蓝图确认切到新 BP 后，再删除旧的 `BP_NetworkBattleGameMode` / `BP_NetworkBattlePlayerController`。
- 测试现有网络地图在 PIE Listen Server 下是否仍能正常移动、开火、死亡、复活。

验收：

- 现有网络测试不回退。
- Host 和 Client 都能正常生成、移动、开火、死亡、复活。

### 阶段 2：实现网络多人死斗

目标：创建第一套具体网络玩法模式。

我负责：

- 新建 `ANetworkDeathmatchGameMode`。
- 新建 `ANetworkDeathmatchGameState`。
- 添加目标分数、击杀加分、环境死亡 / 自杀扣分。
- 复用 `ATankPlayerState::ProcessDeath()` 的 Killer / Assist 归因。
- 把 `PlayerScores`、`TargetScore`、`WinnerSlotId` 写入并复制到 `ANetworkDeathmatchGameState`。
- 在 `ANetworkPlayerControllerBase` 暴露 `ScoresWidgetClass`，复用 `UScoresDisplayWidget` 刷新网络死斗比分。
- 更新 API 和网络开发文档。

你负责：

- 在编辑器中创建或配置 `BP_NetworkDeathmatchGameMode`。
- 配置网络死斗地图使用正确 GameMode 蓝图。
- 在 `BP_NetworkPlayerControllerBase` 或其模式子类中把 `ScoresWidgetClass` 设置为 `WBP_ScoresDisplayWidget`。
- 在 UMG 中准备或调整网络死斗 HUD / 计分板 / 结算 UI。
- 测试目标分数、击杀、死亡、复活和结算表现。

验收：

- 两名玩家能完成完整死斗对局。
- KDA、分数、胜负在 Host 和 Client 上一致。

### 阶段 3：网络入口设置结构

目标：让 UMG 蓝图可以通过 C++ 接口发起 Host / Join。

我负责：

- 新建或完善 `BattleBlasterNetworkTypes.h`。
- 在 `UBattleBlasterSessionSubsystem` 中提供 `BlueprintCallable`：
  - `HostNetworkGame(const FNetworkMatchSettings& Settings)`
  - `JoinNetworkGame(const FString& Address, int32 Port)`
  - `BuildTravelURL(const FNetworkMatchSettings& Settings)`
- 设计 URL Options，让服务器进入地图后能读取模式、人数、AI 数、目标分数等。
- 保证 LAN / Dedicated Server 连接方式和玩法模式解耦。

你负责：

- 绘制网络游戏选择菜单 UMG。
- 绘制 LAN / Server 选择 UI。
- 绘制 Join IP:Port 输入 UI。
- 绘制 Host 游戏设置 UI。
- 在蓝图中调用 C++ 暴露的 Host / Join 接口。

验收：

- UMG 可以发起 LAN Host。
- UMG 可以输入 IP:Port 加入。
- 设置页能把模式、地图、人数、目标分数传给 C++。

### 阶段 4：Dedicated Server 目标

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

### 阶段 5：网络团队死斗

目标：实现团队分配和团队胜负。

我负责：

- 新建 `ANetworkTeamDeathmatchGameMode`。
- 覆盖 `ChooseTeamIdForSlot()`。
- 实现友伤过滤、团队分数、团队胜负。
- 同步团队分数到 GameState。

你负责：

- 配置团队死斗 GameMode 蓝图。
- 制作或调整团队分数 UI。
- 测试队伍颜色、友伤、团队结算。

验收：

- 玩家按队伍分配。
- 队友不能被误伤或按设计规则处理。
- 团队分数和胜负一致。

### 阶段 6：网络 MOBA / 团队 MOBA

目标：迁移最复杂的网络玩法。

我负责：

- 新建 `ANetworkMOBAGameMode` 和 `ANetworkTeamMOBAGameMode`。
- 梳理核心塔、外塔、复活、淘汰、胜负规则。
- 复用现有 Tower / Turret 网络同步能力。
- 保证 MOBA 规则不依赖本地分屏 PlayerIndex。

你负责：

- 配置 MOBA 网络地图。
- 配置 MOBA 网络 GameMode 蓝图。
- 调整 MOBA 顶部状态 UI、死亡 UI、淘汰 UI。
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

## 5. 近期建议执行顺序

1. 先实现 `ANetworkDeathmatchGameMode`。
2. 再补 `BattleBlasterNetworkTypes` 和 `FNetworkMatchSettings`。
3. 再完善 `BattleBlasterSessionSubsystem` 的蓝图 Host / Join 接口。
4. 然后你开始绘制网络菜单 UMG，并挂接 C++ 接口。
5. 最后再进入 Dedicated Server 编译和外网部署测试。

这样做的好处是：先有一个真实可玩的网络玩法模式，再让菜单选择它，不会出现 UI 做完但底层玩法还没定型的尴尬情况。

