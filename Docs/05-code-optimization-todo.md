# BattleBlaster 代码待优化清单

更新日期：2026-05-16

本文只关注 C++ 代码层面的可维护性、性能、多人本地分屏稳定性和后续扩展成本。当前项目已经可以通过 `Development Editor|Win64` 编译，本清单不是必须马上修复的编译错误，而是后续迭代时建议逐步处理的技术债。

## 0. 当前处理进度

已完成：

- 身份语义拆分：C++ 中的 `PlayerIndex` 已硬改为 `SlotId`；`TeamId` 独立用于敌我/友伤判断；`LocalPlayerIndex` 只保留在输入设备映射语义中。`Development Editor|Win64` MSBuild 已通过，结果 `0 warning / 0 error`。

- P0-2 输入设备数量刷新：三个选人菜单已改为 0.5 秒低频刷新，不再每帧强制刷新手柄数量。
- P0-3 Timer Lambda 生命周期保护：`TankPlayerController` 和 `TankMOBAGameMode` 中的延迟 Timer 已避免裸 `this` / `PC` 捕获，并在 `EndPlay` 中清理关键 Timer。
- P2-2 空 Tick 清理：`Projectile`、`ExplosiveBarrel`、`BasePawn` 的无意义 Tick 已关闭；`Tower` 作为需要持续逻辑的 NPC 显式保留 Tick。
- P2-2 延伸项：`TankPlayerState` 攻击者记录清理已从 1 秒 Tick 改为按需启动/停止的 Timer。

部分完成：

- P0-1 Player 0 假设审计：可破坏物血条已改为遍历所有 PlayerController；其余主菜单、暂停、结算等全局 UI 的 Player 0 用法仍需逐处标注意图。
- P1-4 Buff UI 刷新：轻量版本已完成，`BuffListWidget` 已从每帧刷新改为 0.2 秒 Timer，`BuffSlotWidget` 已避免重复写入未变化的图标/秒数/颜色；完整事件驱动 Buff 系统仍保留为后续重构项。

暂缓：

- P1-3 选人菜单公共逻辑抽取：该功能适合组件化思路，但当前三套菜单和各自模式规则、人数规则、AI 填充、地图跳转、UMG 绑定存在耦合；项目已接近末期，当前阶段不建议为减少重复而冒回归风险。

最近一次验证：

- 以上已完成项均通过 `Development Editor|Win64` 的 Visual Studio/MSBuild 编译验证，结果为 `0 warning / 0 error`。

## 1. 总体判断

项目现在的功能已经比较丰富，但代码的主要压力集中在几个“大脑型”类上：

- `Shared/AI/AIBotPlayerController.cpp`：约 1300 行，AI 感知、目标选择、状态机、移动、闪避、瞄准、射击都在一个类里。
- `Modes/FreeForAll/BattleBlasterGameMode.cpp`、`Modes/TeamBattle/TeamBattleGameMode.cpp`、`Modes/MOBA/TankMOBAGameMode.cpp`：玩家创建、AI 创建、PlayerStart 匹配、复活、UI、结算等逻辑高度重复。
- `Modes/FreeForAll/UI/MutiBattleMenuWidget.cpp`、`Modes/TeamBattle/UI/TeamBattleMenuWidget.cpp`、`Modes/MOBA/UI/MOBASetupWidget.cpp`：选人、设备检测、坦克图片切换、地图跳转逻辑重复。
- `Core/BattleBlasterGameInstance.h/.cpp`：同时承担菜单设置、关卡进度、存档、历史战绩、设备映射、跨关卡玩家状态等职责。
- 多处 `NativeTick` / `Tick` 承担轮询逻辑，短期没问题，但后面玩家数、AI 数、UI 数增加后会让问题变得难排查。

建议按“先消除风险，再抽公共能力，最后做系统化重构”的顺序推进。

## 2. 优先级定义

- P0：可能引起多人本地分屏、输入设备、生命周期或稳定性问题，建议最近处理。
- P1：明显降低后续开发效率或增加 Bug 复发概率，建议分模块重构。
- P2：代码风格、日志、注释、数据配置等改进，适合穿插处理。

## 3. P0 待优化项

### P0-1：本地分屏下继续审计 Player 0 假设（部分完成）

相关文件：

- `Modes/Stage/TankStageGameMode.cpp`
- `Modes/Stage/UI/TankStageOverWidget.cpp`
- `Modes/Stage/UI/TankStageStartWidget.cpp`
- `Modes/MainMenu/MainMenuGameMode.cpp`
- `Modes/*/GameMode.cpp` 中创建全局 UI 的 `GetPlayerController(0)` 调用

现状：

- 可破坏物血条已经修过一次“只检查 Player 0 导致其他分屏玩家看不到血条”的问题，目前 `DestructibleProp` 会遍历所有 PlayerController。
- 代码里仍有不少 `GetFirstPlayerController()` / `GetPlayerController(World, 0)`。
- 有些场景确实应该只由 Player 0 操作，例如主菜单、暂停菜单、全局结算 UI；但有些未来可能会变成本地玩家独立 UI 或独立反馈，需要明确标注意图。

建议：

- 建一个小工具函数或约定，例如 `GetPrimaryUIController()`、`GetOwningLocalController()`、`ForEachLocalPlayerController()`。
- 全局 UI 使用 `GetPrimaryUIController()`，玩家个人 UI 使用 owning controller，不再直接散落 `GetFirstPlayerController()`。
- 给每个保留 Player 0 的地方加短注释，说明这是“全局 UI 主控”，不是忘记兼容分屏。

验收：

- 2/3/4 人本地分屏下，HUD、Buff UI、死亡 UI、结算 UI、暂停菜单、回城 UI 都显示在预期玩家屏幕上。
- 搜索 `GetFirstPlayerController` 时，每处都有明确理由。

### P0-2：输入设备数量缓存逻辑需要修正（已完成）

相关文件：

- `Core/BattleBlasterGameInstance.cpp:517`
- `Core/BattleBlasterGameInstance.h:592`
- `Modes/FreeForAll/UI/MutiBattleMenuWidget.cpp:101`
- `Modes/TeamBattle/UI/TeamBattleMenuWidget.cpp:50`
- `Modes/MOBA/UI/MOBASetupWidget.cpp:95`

现状：

- 已按“尽可能简化逻辑”的方向处理。
- 三个菜单 Widget 改为每 0.5 秒刷新一次设备数量，不再在 `NativeTick` 里每帧 `GetConnectedGamepadCount(true)`。
- `GetConnectedGamepadCount(bool bForceRefresh)` 的伪缓存计时字段已移除，避免“缓存字段存在但时间不递增”的误导。

处理方式：

- 去掉容易误导的伪缓存计时字段。
- 三个菜单 Widget 使用 0.5 秒低频 Timer 刷新手柄数量。
- 暂不引入新的 Subsystem，保持当前项目结构简单。

验收：

- 已通过：插拔手柄后 UI 预期能在 0.5 秒内更新。
- 已通过：`GetConnectedGamepadCount(true)` 不再出现在每帧 Tick 路径中。
- 已通过：`Development Editor|Win64` MSBuild 编译通过，`0 warning / 0 error`。

### P0-3：Timer Lambda 捕获对象生命周期需要统一保护（已完成）

相关文件：

- `Shared/Controllers/TankPlayerController.cpp:171`
- `Modes/MOBA/TankMOBAGameMode.cpp:198`
- `Modes/MOBA/TankMOBAGameMode.cpp` 内部延迟初始化 HUD 的 lambda

现状：

- `TankPlayerController::BeginPlay()` 中的延迟 HUD 初始化已改成成员函数 Timer，不再捕获裸 `this`。
- `TankMOBAGameMode::BeginPlay()` 中延迟绑定 Pawn 的 Timer 已改为 `TWeakObjectPtr` 保护，并在 `EndPlay()` 清理。
- MOBA 延迟 HUD 初始化已改为绑定到 `ATankPlayerController` 对象的成员函数 Timer，不再捕获 `PC`。

处理方式：

- `TankPlayerController` 的延迟 HUD 初始化改为 `SetTimer(this, &ATankPlayerController::InitializeHUD)`。
- `TankMOBAGameMode` 的延迟 Pawn 绑定使用 `TWeakObjectPtr<ATankMOBAGameMode>` 保护。
- `TankMOBAGameMode` 增加 `BindPawnTimerHandle`，并在 `EndPlay()` 中清理。

验收：

- 已通过：静态搜索确认相关 `SetTimer` 不再捕获裸 `this` / `PC`。
- 已通过：`Development Editor|Win64` MSBuild 编译通过，`0 warning / 0 error`。

## 4. P1 待优化项

### P1-1：抽出多人模式公共 GameMode 基类或辅助服务

相关文件：

- `Modes/FreeForAll/BattleBlasterGameMode.cpp`
- `Modes/TeamBattle/TeamBattleGameMode.cpp`
- `Modes/MOBA/TankMOBAGameMode.cpp`

重复点：

- 读取 `GameInstance` 的人数、手柄数、目标分数、坦克选择。
- 创建/移除 LocalPlayer。
- 根据 `P0/P1/P2/P3` 找 `PlayerStart`。
- 生成玩家 Tank / AI Controller。
- 写入 SlotId / TeamId / PlayerState。
- 处理三人模式第四屏黑屏。
- 复活时重建 Tank、恢复 Buff、重新绑定 `OnKilled`。

建议：

- 新增 `Modes/Common` 或 `Shared/Match`：
  - `UMatchPlayerSlotService`：负责人数、真人/AI slot、PlayerStart 查找。
  - `ULocalSplitScreenService`：负责 LocalPlayer 数量和三人黑屏策略。
  - `UTankSpawnService`：负责按 slot 生成 Tank、Possess、PlayerState 初始化。
- GameMode 保留各模式独有规则：自由混战计分、团队阵营、MOBA 核心塔/复活时间。

验收：

- 新增或修改一个模式时，不需要复制另一套 GameMode 的 100 行以上样板代码。
- `PlayerStartTag=P%d`、三人黑屏、AI slot 规则只在一个地方维护。

### P1-2：把 AI 控制器拆成感知、决策、行动三层

相关文件：

- `Shared/AI/AIBotPlayerController.cpp`
- `Shared/AI/AIBotPlayerController.h`

现状：

- 单文件包含目标列表、敌友判断、难度参数、状态切换、MoveTo、闪避、预测瞄准、开火路径检测。
- `GetAllActorsOfClass` 在 AI 初始化和兜底搜索中多次使用。
- 当前还能工作，但后续加 MOBA 小兵、防御模式 NPC、更多塔类型时会变得很难扩展。

建议：

- 拆出：
  - `UAITargetRegistry`：维护可攻击目标注册表，Tank/Tower/Turret 出生和死亡时注册/注销。
  - `UAITargetSelector`：只负责从目标列表里选最近、低血量、高威胁目标。
  - `UAITacticalBrain`：负责状态机、追击/逃跑/侧移/闪避。
  - `UAIAimingComponent`：负责预测瞄准、视线检测、开火节奏。
- AI Controller 只保留 Possess、Tick 调度、和 Tank 的接口调用。

验收：

- AI 目标选择不再依赖频繁全图扫描。
- 新增一个可攻击 NPC 类型时，只需要注册到 TargetRegistry，不需要改多处搜索逻辑。
- AI 文件长度明显下降，单个类职责清楚。

### P1-3：把选人菜单公共逻辑抽成基类（暂缓，不建议当前阶段做）

相关文件：

- `Modes/FreeForAll/UI/MutiBattleMenuWidget.cpp`
- `Modes/TeamBattle/UI/TeamBattleMenuWidget.cpp`
- `Modes/MOBA/UI/MOBASetupWidget.cpp`

重复点：

- 每帧刷新设备图标。
- HoverFrame 显隐。
- 坦克图片数组、PlayerTankIndices、LastSwitchTimestamp。
- 鼠标滚轮切换坦克。
- 手柄方向输入切换坦克。
- 写入 `GameInstance->SelectedTankClasses`。

建议：

- 当前阶段不建议实施。
- 如果未来新增第四套选人菜单，可以优先考虑组合式 helper/service，而不是直接改 UMG 继承结构。
- 若后续确实要做，建议只抽纯 C++ 公共工具，不改变现有 Widget Blueprint 父类。

暂缓原因：

- 该功能表面上是通用 UI 部件，但实际和具体模式强耦合：自由混战/MOBA 人数可变，TeamBattle 固定 4 人；AI 填充、目标分数、地图跳转、保存配置也不同。
- 三套菜单已有蓝图绑定和已有流程，项目末期强行抽基类或组件容易制造回归。
- 当前重复代码虽然不好看，但可控；重构收益不足以覆盖蓝图和流程验证成本。

验收：

- 暂不验收实现。
- 后续如重启该项，再以“不改蓝图父类、先抽纯 C++ helper”为第一约束。

### P1-4：Buff 系统从“每帧轮询”改成“状态变化事件 + 必要 Tick”（部分完成）

相关文件：

- `Shared/Buffs/TankBuffComponent.cpp`
- `Shared/UI/BuffListWidget.cpp`

现状：

- `TankBuffComponent` 每帧遍历 ActiveBuffs 并处理倒计时。
- `BuffListWidget` 已取消 `NativeTick`，改为 0.2 秒 Timer 刷新 Buff UI。
- `BuffSlotWidget` 已缓存上一次显示的图标、秒数和警告颜色，避免重复写入未变化的 UMG 属性。
- 速度 Buff 的衰减和 Ghost 窒息确实需要时间推进，但 UI 不一定需要每帧重建/刷新所有内容。

建议：

- `TankBuffComponent` 增加委托：
  - `OnBuffAdded`
  - `OnBuffRemoved`
  - `OnBuffTimeChanged`
  - `OnSuffocationStateChanged`
- Buff UI 只在 Buff 增删时调整 Slot 数量，剩余时间显示可用 0.1 秒或 0.2 秒节流刷新。
- 对不需要连续曲线的 Buff，使用 Timer 到期，而不是每帧遍历。

验收：

- 已通过：`BuffListWidget::NativeTick` 已移除。
- 已通过：Buff UI 刷新频率从每帧降为默认 0.2 秒一次。
- 已通过：`Development Editor|Win64` MSBuild 编译通过，`0 warning / 0 error`。
- 待验证：进 PIE 后确认 Buff 图标增删响应及时，4 人分屏下倒计时显示无明显迟钝。

### P1-5：把伤害/阵营/友伤判断抽成统一 Combat Policy

相关文件：

- `Shared/Combat/Projectile.cpp`
- `Modes/MOBA/TurretProjectile.cpp`
- `Modes/TeamBattle/TeamBattleGameMode.cpp`
- `Shared/AI/AIBotPlayerController.cpp`

现状：

- Projectile 自己判断 MOBA、TeamBattle、FreeForAll。
- AI 自己判断敌友。
- TurretProjectile 也有自己的阵营逻辑。
- TeamBattle 又有 `GetPlayerCamp` / `IsSameCamp`。

建议：

- 新增 `UCombatRuleLibrary` 或 `IMatchCombatRuleProvider`：
  - `CanDamage(Attacker, Victim)`
  - `AreEnemies(ActorA, ActorB)`
  - `GetCampId(Actor)`
- 各模式 GameMode 提供规则，Projectile、AI、Turret 都调用同一个入口。

验收：

- 改团队规则、友伤规则、MOBA 阵营规则时，不需要同时改 Projectile、AI、Turret。
- “炮弹相撞抵消”这类特色保留在 Projectile 碰撞层，不和伤害规则混在一起。

### P1-6：GameInstance 拆分职责，减少全局状态膨胀

相关文件：

- `Core/BattleBlasterGameInstance.h`
- `Core/BattleBlasterGameInstance.cpp`

现状：

- GameInstance 同时存菜单设置、关卡进度、跨关卡携带状态、多人历史榜、设备映射、返回菜单类型。
- Header 很长，任何模块都容易 include 它，导致耦合增加。

建议：

- 保留 GameInstance 作为总入口，但把职责拆到：
  - `UGameSettingsSubsystem`：人数、目标分数、坦克选择。
  - `UCampaignProgressSubsystem`：关卡、难度、跨关卡携带状态。
  - `UMatchHistorySubsystem`：多人历史榜。
  - `UInputDeviceMappingSubsystem`：手柄检测和 LocalPlayerIndex 映射。
- 如果暂时不想引入 Subsystem，也可以先拆成 `Core/Settings`、`Core/Persistence` 下的小类/结构体。

验收：

- 菜单模块不需要知道单人闯关存档细节。
- 战斗模块不直接读写 UI 返回菜单状态。

## 5. P2 待优化项

### P2-1：修复源码注释乱码并删掉过期大段注释

相关文件：

- 多数 `.cpp/.h`，尤其是 `AIBotPlayerController.cpp`、`Projectile.cpp`、`TankBuffComponent.cpp`、`BasePawn.cpp`

现状：

- 大量中文注释显示为乱码，例如 `銆愭牳蹇...`。
- 一些函数内保留了很长的旧逻辑注释或已注释代码块，降低阅读效率。

建议：

- 统一源码文件为 UTF-8。
- 保留解释“为什么这样做”的注释，删除逐行解释 API 的注释。
- 已废弃的大段代码块移动到 Git 历史，不长期留在源码里。

验收：

- 新人打开核心文件可以正常阅读中文注释。
- 单个函数内没有超过 20 行的废弃注释代码块。

### P2-2：禁用空 Tick，保留必要 Tick（已完成）

相关文件：

- `Shared/Combat/Projectile.cpp:25`
- `Shared/Combat/Projectile.cpp:75`
- `Shared/World/ExplosiveBarrel.cpp`
- `Shared/Pawns/BasePawn.cpp`
- `Shared/Pawns/NPC/Tower.cpp`
- `Shared/State/TankPlayerState.cpp`

现状：

- `Projectile` 的空 Tick 已删除，并关闭 Actor Tick。
- `ExplosiveBarrel` 的空 Tick 已删除。
- `BasePawn` 默认关闭 Tick；需要持续逻辑的 `Tower` 显式开启 Tick。
- `ATank` 的空 Tick 已删除。
- `TankPlayerState` 的攻击者记录清理已从 1 秒 Tick 改为按需 Timer：有攻击者记录时启动，队列清空、死亡结算、重置或 `EndPlay` 时停止。

处理方式：

- 空 Tick 的 Actor 直接关闭 `PrimaryActorTick.bCanEverTick` 或删除空 Tick override。
- `BasePawn` 默认关闭 Tick；需要 Tick 的子类自己开启。
- `Projectile` 继续依赖 `UProjectileMovementComponent` 和命中事件，不保留 Actor Tick。
- `TankPlayerState` 的 1 秒清理逻辑改为按需 Timer，不再占用 PlayerState Tick。

验收：

- 已通过：搜索空 `Tick` 时没有发现本轮涉及类仍保留无意义 Tick。
- 已通过：`Development Editor|Win64` MSBuild 编译通过，`0 warning / 0 error`。

### P2-3：日志系统从 LogTemp 迁移到模块日志分类

相关文件：

- 全项目大量 `UE_LOG(LogTemp, ...)`

建议：

- 新增日志分类，例如：
  - `LogBattleBlaster`
  - `LogBBCombat`
  - `LogBBAI`
  - `LogBBUI`
  - `LogBBSave`
- 高频日志降级为 `Verbose` 或加调试开关。

验收：

- 打包或长时间游玩时日志不被普通 Display/Warning 淹没。
- 调 AI 时可以只打开 `LogBBAI`。

### P2-4：把关卡名、菜单名、PlayerStart Tag、模式参数数据化

相关文件：

- `Core/BattleBlasterGameInstance.cpp`
- `Modes/Stage/TankStageGameMode.cpp`
- `Modes/*/UI/*Widget.cpp`
- `Modes/*/*GameMode.cpp`

现状：

- 代码里有 `Level_%d`、`MainMenuLevel_1`、`P%d`、目标分数、复活延迟等硬编码。

建议：

- 关卡、地图、模式参数使用 DataAsset 或项目设置类。
- PlayerStart Tag 可以封装成工具函数，至少不要每个模式自己拼 `P%d`。
- 菜单跳转目标也统一配置，避免以后改关卡名漏改。

验收：

- 新增地图/模式时主要改 DataAsset，不需要进 C++ 文件找字符串。

### P2-5：补一组自动化/半自动化验收场景

建议先覆盖这些高风险路径：

- 1/2/3/4 人本地分屏进入 FreeForAll、TeamBattle、MOBA。
- 3 人模式第四屏黑屏。
- AI slot 能正确生成、拥有 PlayerState、死亡后能复活。
- GhostMode + 子弹穿墙 + 炮弹相撞抵消。
- Buff 获得、持续、恢复、死亡继承。
- 返回主菜单后 LocalPlayer 清理干净。

验收：

- 每次重构 GameMode、输入设备、Buff 或 Projectile 后，用固定清单手动跑一遍。
- 后续可以逐步迁移到 Unreal Automation Test 或 Functional Test。

## 6. 推荐执行顺序

1. 已完成 P0-2、P0-3、P2-2 和 P1-4 轻量版本：输入设备刷新、Timer 生命周期保护、空 Tick/低价值 Tick 清理、Buff UI 低频刷新。
2. P1-3 选人菜单公共逻辑抽取暂缓：项目末期不建议为了减少重复而触碰模式选择流程和 UMG 结构。
3. 下一步更适合做 P1-5 的轻量版本：统一 Combat Policy 的小范围封装，降低 Projectile、AI、Turret 之间伤害规则不一致的概率。
4. 然后再考虑 P1-1：抽多人 GameMode 公共服务。这个收益最大，但要分小步做，每步都编译和进游戏验证。
5. 最后做 P1-2、P1-4 完整事件驱动版和 P1-6：AI、Buff 系统和 GameInstance 的大拆分。它们影响面较大，适合在功能相对稳定时做。

## 7. 不建议现在做的事

- 不建议一次性把所有类改名。UE 蓝图引用、资产引用、反射名都会增加风险。
- 不建议一次性拆多个 GameMode。先抽一个公共服务，再让 FreeForAll 接入，确认稳定后再迁 TeamBattle/MOBA。
- 不建议马上把所有 Tick 都删掉。像 Buff 窒息、滑轨、陷阱动画、炮塔追踪确实需要时间推进，要逐个判断。
