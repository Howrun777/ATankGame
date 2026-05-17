/**
 * @file BattleBlasterGameInstance.h
 * @brief 战斗Blaster游戏实例 - 管理游戏全局状态和数据
 * 
 * 主要功能:
 * - 单人闯关模式关卡管理(难度、进度、关卡切换)
 * - 玩家状态继承(生命值、子弹、Buff)
 * - 多人游戏设置(目标人数、坦克选择)
 * - 存档管理(游戏进度保存/加载)
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameFramework/SaveGame.h"
#include "Core/Persistence/BattleBlasterSaveGame.h"
#include "Shared/Buffs/BuffTypes.h"
#include "BattleBlasterGameInstance.generated.h"

/**
 * @enum EReturnToMenuType
 * @brief 单人闯关模式返回菜单时的目标UI类型
 */
UENUM(BlueprintType)
enum class EReturnToMenuType : uint8
{
	MainMenu         UMETA(DisplayName = "Main Menu"),          // 返回主菜单
	SinglePlayerMenu UMETA(DisplayName = "Single Player Menu"), // 返回单人模式菜单
	MOBASetupMenu    UMETA(DisplayName = "MOBA Setup Menu")     // 返回 MOBA 设置界面（如 WBP_MOBASetupWidget）
};

/**
 * @struct FPlayerCarryState
 * @brief 玩家状态结构体 - 用于关卡间继承
 * 
 * 包含:
 * - 玩家剩余生命值
 * - 玩家剩余子弹数
 * - 各种Buff的状态、持续时间和图标
 */
USTRUCT(BlueprintType)
struct FPlayerCarryState
{
	GENERATED_BODY()

	// ========== 基础资源 ==========
	
	/** 剩余生命值(过关时从上一关继承) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Health = 0.0f;

	/** 剩余子弹数(过关时从上一关继承) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Ammo = 0;

	// ========== Buff 状态标志 ==========
	
	/** 是否拥有无限子弹Buff */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasInfiniteAmmo = false;

	/** 是否拥有伤害提升Buff */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasDamageBoost = false;

	/** 是否拥有子弹穿透Buff */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasBulletPierce = false;

	/** 是否拥有双发弹道Buff */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasDoubleShot = false;

	/** 是否拥有穿墙模式Buff */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsGhostMode = false;

	/** 是否拥有移速提升Buff */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasSpeedBoost = false;

	/** 是否拥有护盾Buff */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasShield = false;

	// ========== Buff 持续时间 ==========
	
	/** 无限子弹Buff剩余时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float InfiniteAmmoDuration = 0.0f;

	/** 移速提升Buff剩余时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpeedBoostDuration = 0.0f;

	/** 伤害提升Buff剩余时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DamageBoostDuration = 0.0f;

	/** 子弹穿透Buff剩余时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BulletPierceDuration = 0.0f;

	/** 双发弹道Buff剩余时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DoubleShotDuration = 0.0f;

	/** 穿墙模式Buff剩余时间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GhostModeDuration = 0.0f;

	// ========== Buff 图标(不暴露给蓝图) ==========
	
	/** 无限子弹Buff图标 */
	UPROPERTY()
	UTexture2D* InfiniteAmmoIcon = nullptr;

	/** 移速提升Buff图标 */
	UPROPERTY()
	UTexture2D* SpeedBoostIcon = nullptr;

	/** 伤害提升Buff图标 */
	UPROPERTY()
	UTexture2D* DamageBoostIcon = nullptr;

	/** 子弹穿透Buff图标 */
	UPROPERTY()
	UTexture2D* BulletPierceIcon = nullptr;

	/** 双发弹道Buff图标 */
	UPROPERTY()
	UTexture2D* DoubleShotIcon = nullptr;

	/** 穿墙模式Buff图标 */
	UPROPERTY()
	UTexture2D* GhostModeIcon = nullptr;
};

/**
 * @struct FMultiBattleHistoryEntry
 * @brief 多人死斗历史记录条目（用于结算榜单）
 */
USTRUCT(BlueprintType)
struct FMultiBattleHistoryEntry
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Score = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Kills = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Deaths = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Assists = 0;

	// 0=红,1=蓝,2=绿,3=黄（对应玩家 Index）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 CampIndex = 0;

	// 用于同分时“后来者居上”的排序序号
	UPROPERTY()
	int32 SequenceId = 0;
};

/**
 * @class UBattleBlasterGameInstance
 * @brief 战斗Blaster游戏实例 - 管理游戏全局状态
 * 
 * 主要职责:
 * - 单人闯关模式:关卡管理、难度计算、进度追踪、玩家状态继承
 * - 多人对战模式:目标人数、坦克选择
 * - 存档管理:游戏进度保存和加载
 */
UCLASS()
class BATTLEBLASTER_API UBattleBlasterGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// ========== 多人对战设置 ==========
	
	/** 目标玩家数量 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Match Settings")
	int32 TargetPlayerCount = 2;

	/** 目标胜利分数 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Match Settings")
	int32 TargetMatchScore = 7;

	// ========== 坦克选择 ==========
	
	/** 玩家选择的坦克类(按玩家索引) */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "TankSelect")
	TArray<TSubclassOf<APawn>> SelectedTankClasses;

	// ========== 手柄连接信息 ==========
	
	/** 已连接手柄数量 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GamepadInfo")
	int32 ConnectedGamepadCount = 1;

	/** 需要AI控制的玩家索引 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "GamepadInfo")
	TArray<int32> AIControlledPlayerIndices;

	// ========== 单人闯关模式 ==========
	
	/** 关卡名称列表(在蓝图中配置) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign")
	TArray<FName> CampaignLevelNames;

	/** 当前关卡名称 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Campaign")
	FName CurrentLevelName;

	/** 当前关卡索引(从1开始) */
	UPROPERTY(VisibleAnywhere, Category = "Campaign")
	int32 CurrentLevelIndex = 1;

	/** 最高达成关卡(本次运行期间) */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Campaign")
	int32 BestLevelRecord = 1;

	/** 难度增长系数(k=1.2表示每关增加20%难度) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign")
	float DifficultyCoefficientK = 1.2f;

	/** 玩家死亡次数 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Campaign")
	int32 PlayerDeathCount = 0;

	/** 最大允许死亡次数 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Campaign")
	int32 MaxDeathCount = 3;

	/** 玩家携带状态(跨关卡继承) */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Campaign")
	FPlayerCarryState PlayerCarryState;

	/** 单人闯关开始时间(用于计算总游戏时间) */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Campaign")
	float CampaignStartTime = 0.0f;

	/**
	 * 单人闯关累计用时（跨关卡累加，单位：秒）
	 * 说明：不要用 World->GetTimeSeconds() 做“跨关卡起点”，因为切关会重置 World 时间；
	 * 这里采用“每关开始记一次起点 + 离开关卡时把差值累加”的方式，确保总时间正确。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Campaign")
	float CampaignAccumulatedTime = 0.0f;

	/** 当前关卡计时起点（World->GetTimeSeconds()，仅在本关有效） */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Campaign")
	float CampaignLevelStartTime = -1.0f;

	/** 是否正在进行单人闯关计时 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Campaign")
	bool bCampaignTimerActive = false;

	// ========== 多人死斗历史榜单 ==========

	/** 历史记录（最多保留50条），按分数降序，同分按 SequenceId 降序（后来者在前） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MultiBattle")
	TArray<FMultiBattleHistoryEntry> MultiBattleHistory;

	/** 递增的序号，用于实现“同分后来者居上” */
	UPROPERTY()
	int32 MultiBattleHistorySequence = 0;

public:
	// ========== 存档管理 ==========
	
	/** 加载存档数据 */
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void LoadGameData();

	/** 保存游戏进度 */
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void SaveGameData();

	// ========== 关卡管理 ==========
	
	/** 获取随机关卡名称(用于首次进入) */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	FName GetRandomLevelName() const;

	/** 加载下一关 */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void LoadNextLevel(const FString& Options = TEXT(""));

	/** 重新开始当前关卡(会重置玩家状态) */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void RestartCurrentLevel(const FString& Options = TEXT(""));

	/** 重新开始游戏(回到第一关,会重置所有状态) */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void RestartGame(const FString& Options = TEXT(""));

	/** 重置当前关卡为1(保留历史记录) */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void ResetCurrentLevel();

	/** 获取当前关卡索引 */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	int32 GetCurrentLevelIndex() const { return CurrentLevelIndex; }

	/** 获取最高历史关卡 */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	int32 GetBestLevelRecord() const { return BestLevelRecord; }

	/** 获取单人闯关开始时间 */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	float GetCampaignStartTime() const { return CampaignStartTime; }

	/** 开始/重置一轮闯关计时（一般在“新开一局”时调用） */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void ResetCampaignTimer()
	{
		CampaignAccumulatedTime = 0.0f;
		CampaignLevelStartTime = -1.0f;
		bCampaignTimerActive = true;
	}

	/** 进入某一关卡后调用：记录本关计时起点（只记录一次） */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void MarkCampaignLevelStart(UWorld* World)
	{
		if (!bCampaignTimerActive || !World) return;
		// 注意：World->GetTimeSeconds() 在关卡刚开始时可能恰好为 0，所以用 -1 作为“未开始”的哨兵值
		if (CampaignLevelStartTime < 0.0f)
		{
			CampaignLevelStartTime = World->GetTimeSeconds();
		}
	}

	/** 离开某一关卡前调用：把本关用时累加进总时间 */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void MarkCampaignLevelEnd(UWorld* World)
	{
		if (!bCampaignTimerActive || !World) return;
		if (CampaignLevelStartTime >= 0.0f)
		{
			const float Delta = World->GetTimeSeconds() - CampaignLevelStartTime;
			CampaignAccumulatedTime += FMath::Max(0.0f, Delta);
			CampaignLevelStartTime = -1.0f;
		}
	}

	/** 获取当前总用时（累计 + 本关进行中） */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	float GetCampaignTotalTime(UWorld* World) const
	{
		if (!bCampaignTimerActive || !World) return 0.0f;
		float Total = CampaignAccumulatedTime;
		if (CampaignLevelStartTime >= 0.0f)
		{
			Total += FMath::Max(0.0f, World->GetTimeSeconds() - CampaignLevelStartTime);
		}
		return Total;
	}

	// ========== 多人死斗历史榜单接口 ==========

	/**
	 * @brief 新增一条多人死斗历史记录，并按规则维护前50名（保留用于兼容）
	 * @return 新记录在榜单中的名次索引（0为第一名，若未进入前50返回 -1）
	 */
	int32 AddMultiBattleHistoryRecord(int32 Score, int32 Kills, int32 Deaths, int32 Assists, int32 CampIndex);

	/**
	 * @brief 将本局所有玩家的成绩写入历史战绩池，排序后只保留前50，并持久化
	 * @param InPlayerCount 本局玩家数量
	 * @param InKills 各玩家击杀数
	 * @param InDeaths 各玩家死亡数
	 * @param InAssists 各玩家助攻数
	 * @param InTargetScore 本局目标分数（用于计算评分）
	 * @return 每个玩家在本局写入后在前50中的名次索引（0为第1名），未进前50为 -1
	 */
	TArray<int32> AddMultiBattleHistoryRecordsFromMatch(
		int32 InPlayerCount,
		const TArray<int32>& InKills,
		const TArray<int32>& InDeaths,
		const TArray<int32>& InAssists,
		int32 InTargetScore);

	/** 获取当前历史榜单（已按排序裁剪为前50） */
	const TArray<FMultiBattleHistoryEntry>& GetMultiBattleHistory() const { return MultiBattleHistory; }

	/** 游戏启动时加载历史战绩池（进程重启不丢失） */
	void LoadMultiBattleHistory();

	/** 将历史战绩池写入磁盘 */
	void SaveMultiBattleHistory();

	virtual void Init() override;

	// ========== 难度管理 ==========
	
	/**
	 * @brief 计算指定关卡的难度系数
	 * @param Level 关卡索引(从1开始)
	 * @return 难度乘数,公式: k^(Level-1)
	 * 
	 * 例如:k=1.2时
	 * - 第1关: 1.2^0 = 1.0
	 * - 第2关: 1.2^1 = 1.2
	 * - 第3关: 1.2^2 = 1.44
	 */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	float GetDifficultyMultiplier(int32 Level) const;

	/** 获取当前关卡的难度系数 */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	float GetCurrentDifficultyMultiplier() const;

	/** 设置难度系数k */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void SetDifficultyCoefficientK(float K) { DifficultyCoefficientK = K; }

	/** 获取难度系数k */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	float GetDifficultyCoefficientK() const { return DifficultyCoefficientK; }

	// ========== 玩家状态(生命次数) ==========
	
	/** 获取玩家死亡次数 */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	int32 GetPlayerDeathCount() const { return PlayerDeathCount; }

	/** 增加玩家死亡次数 */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void IncrementPlayerDeathCount() { PlayerDeathCount++; }

	/** 获取剩余生命次数 */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	int32 GetRemainingLives() const { return MaxDeathCount - PlayerDeathCount; }

	/** 获取最大死亡次数 */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	int32 GetMaxDeathCount() const { return MaxDeathCount; }

	/** 重置死亡次数(开始新游戏时调用) */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void ResetPlayerDeathCount() { PlayerDeathCount = 0; }

	// ========== 玩家状态继承(跨关卡) ==========
	
	/**
	 * @brief 保存玩家携带状态
	 * @param CurrentHealth 当前生命值
	 * @param CurrentAmmo 当前子弹数
	 * @param bInfiniteAmmo 是否有无限子弹Buff
	 * @param bSpeedBoost 是否有移速提升Buff
	 * @param bDamageBoost 是否有伤害提升Buff
	 * @param bBulletPierce 是否有子弹穿透Buff
	 * @param bDoubleShot 是否有双发弹道Buff
	 * @param bGhostMode 是否有穿墙模式Buff
	 * @param bShield 是否有护盾Buff
	 * @param InfiniteAmmoDur 无限子弹Buff剩余时间
	 * @param SpeedBoostDur 移速提升Buff剩余时间
	 * @param DamageBoostDur 伤害提升Buff剩余时间
	 * @param BulletPierceDur 子弹穿透Buff剩余时间
	 * @param DoubleShotDur 双发弹道Buff剩余时间
	 * @param GhostModeDur 穿墙模式Buff剩余时间
	 * @param InInfiniteAmmoIcon 无限子弹Buff图标
	 * @param InSpeedBoostIcon 移速提升Buff图标
	 * @param InDamageBoostIcon 伤害提升Buff图标
	 * @param InBulletPierceIcon 子弹穿透Buff图标
	 * @param InDoubleShotIcon 双发弹道Buff图标
	 * @param InGhostModeIcon 穿墙模式Buff图标
	 * 
	 * 在玩家过关或死亡时调用,保存当前状态以便下一关继承
	 */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void SavePlayerCarryState(float CurrentHealth, int32 CurrentAmmo,
		bool bInfiniteAmmo, bool bSpeedBoost, bool bDamageBoost, bool bBulletPierce,
		bool bDoubleShot, bool bGhostMode, bool bShield,
		float InfiniteAmmoDur, float SpeedBoostDur, float DamageBoostDur, float BulletPierceDur,
		float DoubleShotDur, float GhostModeDur,
		UTexture2D* InInfiniteAmmoIcon, UTexture2D* InSpeedBoostIcon,
		UTexture2D* InDamageBoostIcon, UTexture2D* InBulletPierceIcon,
		UTexture2D* InDoubleShotIcon, UTexture2D* InGhostModeIcon);

	/** 获取保存的玩家携带状态 */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	FPlayerCarryState GetPlayerCarryState() const { return PlayerCarryState; }

	/** 重置玩家携带状态(开始新游戏时调用) */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	void ResetPlayerCarryState() { PlayerCarryState = FPlayerCarryState(); }

	/**
	 * @brief 检查是否有可继承的玩家状态
	 * @return 如果有任何可继承的状态(生命值>0、子弹>0或有Buff)返回true
	 * 
	 * 用于判断是否是第一次进入关卡
	 */
	UFUNCTION(BlueprintCallable, Category = "Campaign")
	bool HasPlayerCarryState() const
	{
		return PlayerCarryState.Health > 0 || PlayerCarryState.Ammo > 0 ||
			PlayerCarryState.bHasInfiniteAmmo || PlayerCarryState.bHasSpeedBoost ||
			PlayerCarryState.bHasDamageBoost || PlayerCarryState.bHasBulletPierce ||
			PlayerCarryState.bHasDoubleShot || PlayerCarryState.bIsGhostMode ||
			PlayerCarryState.bHasShield;
	}

	// ========== UI相关 ==========
	
	/** 返回菜单类型 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "UI")
	EReturnToMenuType ReturnToMenuType = EReturnToMenuType::MainMenu;

	/** 设置返回菜单类型 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetReturnToMenuType(EReturnToMenuType Type) { ReturnToMenuType = Type; }

	/** 获取返回菜单类型 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	EReturnToMenuType GetReturnToMenuType() const { return ReturnToMenuType; }

	/**
	 * 与 ReturnToMenuType=MOBASetupMenu 配合：主菜单加载完成后要创建的 Widget 类（例如 WBP_MOBASetupWidget）。
	 * 由 MOBA 结算界面在 OpenLevel 前设置，MainMenuGameMode 消费一次后应清空。
	 */
	UPROPERTY()
	TSubclassOf<class UUserWidget> PendingMainMenuWidgetClass;

	void SetPendingMainMenuWidgetClass(TSubclassOf<class UUserWidget> WidgetClass) { PendingMainMenuWidgetClass = WidgetClass; }

	TSubclassOf<class UUserWidget> GetPendingMainMenuWidgetClass() const { return PendingMainMenuWidgetClass; }

	void ClearPendingMainMenuWidgetClass() { PendingMainMenuWidgetClass = nullptr; }

	// ========== 手柄管理 ==========

	/**
	 * @brief 获取当前已连接的手柄数量（精确过滤，仅统计 Gamepad 类型设备）
	 * @param OutDeviceIndices 输出：每个手柄对应的本地 LocalPlayerIndex（与 PlayerController 数组对应）
	 * @return 已连接的手柄数量（最大 4）
	 */
	UFUNCTION(BlueprintCallable, Category = "Gamepad|Input")
	int32 GetConnectedGamepadCountWithMapping(TArray<int32>& OutDeviceIndices);

	/**
	 * @brief 获取当前已连接的手柄数量（忽略 LocalPlayerIndex 输出）
	 * @param bForceRefresh 是否强制重新检测（绕过缓存）
	 * @return 已连接的手柄数量
	 */
	UFUNCTION(BlueprintCallable, Category = "Gamepad|Input")
	int32 GetConnectedGamepadCount(bool bForceRefresh = false);

	/**
	 * @brief 根据 LocalPlayerIndex 获取其对应的 FInputDeviceId（用于输入来源追踪）
	 * @param LocalPlayerIndex 本地玩家索引（0~3）
	 * @return 该玩家最近一次活动手柄的 DeviceId，无效时返回 Invalid
	 */
	UFUNCTION(BlueprintCallable, Category = "Gamepad|Input")
	FInputDeviceId GetPlayerDeviceId(int32 LocalPlayerIndex) const;

	/**
	 * @brief 注册/更新 LocalPlayerIndex 与 DeviceId 的映射（在输入发生时调用）
	 * @param LocalPlayerIndex 本地玩家索引
	 * @param DeviceId 活动设备 ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Gamepad|Input")
	void RegisterPlayerDeviceMapping(int32 LocalPlayerIndex, FInputDeviceId DeviceId);

	/**
	 * @brief 重置所有设备映射（进入菜单时调用）
	 */
	UFUNCTION(BlueprintCallable, Category = "Gamepad|Input")
	void ResetDeviceMappings();

private:
	/**
	 * @brief 切换到指定关卡(内部核心函数)
	 * @param Index 目标关卡索引
	 * @param Options 关卡选项字符串(如指定GameMode)
	 */
	void ChangeLevel(int32 Index, const FString& Options = TEXT(""));

	// 设备映射表：LocalPlayerIndex → FInputDeviceId
	TArray<FInputDeviceId> PlayerDeviceIdMap;

	// 缓存的手柄数量（避免每帧频繁查询）
	int32 CachedGamepadCount = 0;
};
