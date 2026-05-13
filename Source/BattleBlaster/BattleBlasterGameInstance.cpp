/**
 * @file BattleBlasterGameInstance.cpp
 * @brief 战斗Blaster游戏实例实现 - 管理游戏全局状态和数据
 */

#include "BattleBlasterGameInstance.h"
#include "BattleBlasterHistorySaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "InputCoreTypes.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"

/**
 * @brief 切换到指定关卡(内部核心函数)
 * @param Index 目标关卡索引(从1开始)
 * @param Options 关卡选项字符串(如指定GameMode)
 * 
 * 处理逻辑:
 * 1. 验证索引有效性(必须>0)
 * 2. 更新当前关卡索引和最高记录
 * 3. 从关卡列表随机选择(或使用旧命名方式Level_X)
 * 4. 调用OpenLevel切换关卡
 */
void UBattleBlasterGameInstance::ChangeLevel(int32 Index, const FString& Options)
{
	// 索引必须大于0
	if (Index > 0)
	{
		// 更新当前关卡的索引，记录玩家进度
		CurrentLevelIndex = Index;
		BestLevelRecord = FMath::Max(BestLevelRecord, CurrentLevelIndex);

		// 随机选择关卡（如果配置了关卡列表）
		FName TargetLevelName;
		if (CampaignLevelNames.Num() > 0)
		{
			int32 RandomIdx = FMath::RandRange(0, CampaignLevelNames.Num() - 1);
			TargetLevelName = CampaignLevelNames[RandomIdx];
		}
		else
		{
			// 兼容旧的命名方式：Level_X
			FString LevelNameString = FString::Printf(TEXT("Level_%d"), CurrentLevelIndex);
			TargetLevelName = FName(*LevelNameString);
		}

		// 保存当前关卡名称（用于重新进入同一关）
		CurrentLevelName = TargetLevelName;

		UE_LOG(LogTemp, Display, TEXT("Loading level: %s (Index: %d)"), *TargetLevelName.ToString(), Index);

		// 打开关卡
		UGameplayStatics::OpenLevel(GetWorld(), TargetLevelName, true, Options);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid Level Index: %d. Level change aborted."), Index);
	}
}

/**
 * @brief 获取随机关卡名称
 * @return 随机的关卡名称
 * 
 * 用于首次进入关卡时随机选择
 * 如果配置了CampaignLevelNames则从中随机选择
 * 否则使用旧命名方式Level_X
 */
FName UBattleBlasterGameInstance::GetRandomLevelName() const
{
	if (CampaignLevelNames.Num() > 0)
	{
		int32 RandomIdx = FMath::RandRange(0, CampaignLevelNames.Num() - 1);
		return CampaignLevelNames[RandomIdx];
	}
	// 兼容旧方式
	return FName(FString::Printf(TEXT("Level_%d"), CurrentLevelIndex));
}

/**
 * @brief 加载下一关
 * @param Options 关卡选项字符串
 * 
 * 处理逻辑:
 * 1. 当前关卡索引+1
 * 2. 保存游戏进度
 * 3. 切换到新关卡
 */
void UBattleBlasterGameInstance::LoadNextLevel(const FString& Options)
{
	// 无限闯关模式：每次加载下一关，索引+1
	ChangeLevel(CurrentLevelIndex + 1, Options);
	// 保存闯关进度
	SaveGameData();
}

/**
 * @brief 重新开始当前关卡
 * @param Options 关卡选项字符串
 * 
 * 处理逻辑:
 * 1. 重置玩家携带状态(生命值、子弹、Buff)
 * 2. 切换到当前关卡重新开始
 * 
 * 使用场景: 玩家死亡后选择重新开始当前关卡
 */
void UBattleBlasterGameInstance::RestartCurrentLevel(const FString& Options)
{
	// 重新进入当前关卡时，也需要重置玩家状态
	ResetPlayerCarryState();
	ChangeLevel(CurrentLevelIndex, Options);
}

/**
 * @brief 重新开始游戏(回到第一关)
 * @param Options 关卡选项字符串
 * 
 * 处理逻辑:
 * 1. 重置死亡次数为0
 * 2. 重置玩家携带状态(清空所有继承的数据)
 * 3. 切换到第一关
 * 
 * 使用场景: 结束菜单点击"再来一局"
 */
void UBattleBlasterGameInstance::RestartGame(const FString& Options)
{
	PlayerDeathCount = 0; // 重置死亡次数
	ResetPlayerCarryState(); // 重置玩家携带状态

	// 开始新一轮单人闯关计时（跨关卡累计）
	ResetCampaignTimer();
	
	ChangeLevel(1, Options);
}

void UBattleBlasterGameInstance::Init()
{
	Super::Init();
	LoadMultiBattleHistory();
}

/**
 * @brief 加载历史战绩池（进程重启不丢失）
 */
void UBattleBlasterGameInstance::LoadMultiBattleHistory()
{
	if (UGameplayStatics::DoesSaveGameExist(UBattleBlasterHistorySaveGame::SaveSlotName, UBattleBlasterHistorySaveGame::UserIndex))
	{
		UBattleBlasterHistorySaveGame* Loaded = Cast<UBattleBlasterHistorySaveGame>(
			UGameplayStatics::LoadGameFromSlot(UBattleBlasterHistorySaveGame::SaveSlotName, UBattleBlasterHistorySaveGame::UserIndex));
		if (Loaded)
		{
			MultiBattleHistory = Loaded->MultiBattleHistory;
			MultiBattleHistorySequence = Loaded->NextSequenceId;
			UE_LOG(LogTemp, Display, TEXT("MultiBattle history loaded: %d entries"), MultiBattleHistory.Num());
		}
	}
	else
	{
		MultiBattleHistory.Empty();
		MultiBattleHistorySequence = 0;
		UE_LOG(LogTemp, Display, TEXT("No MultiBattle history file, starting empty"));
	}
}

/**
 * @brief 将历史战绩池写入磁盘
 */
void UBattleBlasterGameInstance::SaveMultiBattleHistory()
{
	UBattleBlasterHistorySaveGame* SaveObj = Cast<UBattleBlasterHistorySaveGame>(
		UGameplayStatics::CreateSaveGameObject(UBattleBlasterHistorySaveGame::StaticClass()));
	if (SaveObj)
	{
		SaveObj->MultiBattleHistory = MultiBattleHistory;
		SaveObj->NextSequenceId = MultiBattleHistorySequence;
		if (UGameplayStatics::SaveGameToSlot(SaveObj, UBattleBlasterHistorySaveGame::SaveSlotName, UBattleBlasterHistorySaveGame::UserIndex))
		{
			UE_LOG(LogTemp, Display, TEXT("MultiBattle history saved: %d entries"), MultiBattleHistory.Num());
		}
	}
}

/**
 * @brief 加载存档数据
 * 
 * 从本地存储加载游戏进度
 * 包括: 当前关卡索引、最高记录关卡
 */
void UBattleBlasterGameInstance::LoadGameData()
{
	if (UGameplayStatics::DoesSaveGameExist(UBattleBlasterSaveGame::SaveSlotName, UBattleBlasterSaveGame::UserIndex))
	{
		UBattleBlasterSaveGame* SaveGameInstance = Cast<UBattleBlasterSaveGame>(
			UGameplayStatics::LoadGameFromSlot(UBattleBlasterSaveGame::SaveSlotName, UBattleBlasterSaveGame::UserIndex));

		if (SaveGameInstance)
		{
			CurrentLevelIndex = SaveGameInstance->CurrentLevelIndex;
			BestLevelRecord = SaveGameInstance->BestLevelRecord;
			UE_LOG(LogTemp, Display, TEXT("Game data loaded: CurrentLevelIndex=%d, BestLevelRecord=%d"),
				CurrentLevelIndex, BestLevelRecord);
		}
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("No save game found, using defaults"));
	}
}

/**
 * @brief 保存游戏数据
 * 
 * 将当前游戏进度保存到本地存储
 * 包括: 当前关卡索引、最高记录关卡
 */
void UBattleBlasterGameInstance::SaveGameData()
{
	UBattleBlasterSaveGame* SaveGameInstance = Cast<UBattleBlasterSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UBattleBlasterSaveGame::StaticClass()));

	if (SaveGameInstance)
	{
		SaveGameInstance->CurrentLevelIndex = CurrentLevelIndex;
		SaveGameInstance->BestLevelRecord = BestLevelRecord;

		if (UGameplayStatics::SaveGameToSlot(SaveGameInstance, UBattleBlasterSaveGame::SaveSlotName, UBattleBlasterSaveGame::UserIndex))
		{
			UE_LOG(LogTemp, Display, TEXT("Game data saved: CurrentLevelIndex=%d, BestLevelRecord=%d"),
				CurrentLevelIndex, BestLevelRecord);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to save game data"));
		}
	}
}

/**
 * @brief 重置当前关卡为1
 * 
 * 保留最高记录但将当前关卡重置为1
 * 用于开始新一轮闯关
 */
void UBattleBlasterGameInstance::ResetCurrentLevel()
{
	CurrentLevelIndex = 1;
	UE_LOG(LogTemp, Display, TEXT("Current level reset to 1 (BestRecord=%d preserved)"), BestLevelRecord);
}

/**
 * @brief 计算指定关卡的难度系数
 * @param Level 关卡索引(从1开始)
 * @return 难度乘数
 * 
 * 难度公式: k^(Level-1)
 * 例如k=1.2时:
 * - 第1关: 1.2^0 = 1.0
 * - 第2关: 1.2^1 = 1.2
 * - 第3关: 1.2^2 = 1.44
 */
float UBattleBlasterGameInstance::GetDifficultyMultiplier(int32 Level) const
{
	// 难度公式：k=1.2 时，第1关=1.0，第2关=1.2，第3关=1.44，第4关=1.728...
	// 公式：k^(Level-1)
	if (Level <= 1) return 1.0f;
	return FMath::Pow(DifficultyCoefficientK, Level - 1);
}

/**
 * @brief 获取当前关卡的难度系数
 * @return 当前关卡的难度乘数
 */
float UBattleBlasterGameInstance::GetCurrentDifficultyMultiplier() const
{
	return GetDifficultyMultiplier(CurrentLevelIndex);
}

/**
 * @brief 保存玩家携带状态(过关或死亡时调用)
 * 
 * 保存玩家当前的生命值、子弹数、Buff状态、持续时间和图标
 * 以便在下一关继承这些状态
 * 
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
 */
void UBattleBlasterGameInstance::SavePlayerCarryState(float CurrentHealth, int32 CurrentAmmo,
	bool bInfiniteAmmo, bool bSpeedBoost, bool bDamageBoost, bool bBulletPierce,
	bool bDoubleShot, bool bGhostMode, bool bShield,
	float InfiniteAmmoDur, float SpeedBoostDur, float DamageBoostDur, float BulletPierceDur,
	float DoubleShotDur, float GhostModeDur,
	UTexture2D* InInfiniteAmmoIcon, UTexture2D* InSpeedBoostIcon,
	UTexture2D* InDamageBoostIcon, UTexture2D* InBulletPierceIcon,
	UTexture2D* InDoubleShotIcon, UTexture2D* InGhostModeIcon)
{
	// 保存基础资源
	PlayerCarryState.Health = FMath::Max(0.0f, CurrentHealth);
	PlayerCarryState.Ammo = FMath::Max(0, CurrentAmmo);

	// 保存Buff状态标志
	PlayerCarryState.bHasInfiniteAmmo = bInfiniteAmmo;
	PlayerCarryState.bHasSpeedBoost = bSpeedBoost;
	PlayerCarryState.bHasDamageBoost = bDamageBoost;
	PlayerCarryState.bHasBulletPierce = bBulletPierce;
	PlayerCarryState.bHasDoubleShot = bDoubleShot;
	PlayerCarryState.bIsGhostMode = bGhostMode;
	PlayerCarryState.bHasShield = bShield;

	// 保存Buff持续时间
	PlayerCarryState.InfiniteAmmoDuration = InfiniteAmmoDur;
	PlayerCarryState.SpeedBoostDuration = SpeedBoostDur;
	PlayerCarryState.DamageBoostDuration = DamageBoostDur;
	PlayerCarryState.BulletPierceDuration = BulletPierceDur;
	PlayerCarryState.DoubleShotDuration = DoubleShotDur;
	PlayerCarryState.GhostModeDuration = GhostModeDur;

	// 保存Buff图标(用于UI显示)
	PlayerCarryState.InfiniteAmmoIcon = InInfiniteAmmoIcon;
	PlayerCarryState.SpeedBoostIcon = InSpeedBoostIcon;
	PlayerCarryState.DamageBoostIcon = InDamageBoostIcon;
	PlayerCarryState.BulletPierceIcon = InBulletPierceIcon;
	PlayerCarryState.DoubleShotIcon = InDoubleShotIcon;
	PlayerCarryState.GhostModeIcon = InGhostModeIcon;

	// 输出日志
	UE_LOG(LogTemp, Display, TEXT("Player Carry State Saved: Health=%.1f, Ammo=%d, InfiniteAmmo=%d, Speed=%d, Damage=%d, Pierce=%d, Double=%d, Ghost=%d, Shield=%d"),
		PlayerCarryState.Health, PlayerCarryState.Ammo,
		PlayerCarryState.bHasInfiniteAmmo, PlayerCarryState.bHasSpeedBoost,
		PlayerCarryState.bHasDamageBoost, PlayerCarryState.bHasBulletPierce,
		PlayerCarryState.bHasDoubleShot, PlayerCarryState.bIsGhostMode,
		PlayerCarryState.bHasShield);
}

int32 UBattleBlasterGameInstance::AddMultiBattleHistoryRecord(int32 Score, int32 Kills, int32 Deaths, int32 Assists, int32 CampIndex)
{
	FMultiBattleHistoryEntry NewEntry;
	NewEntry.Score = Score;
	NewEntry.Kills = Kills;
	NewEntry.Deaths = Deaths;
	NewEntry.Assists = Assists;
	NewEntry.CampIndex = CampIndex;
	NewEntry.SequenceId = MultiBattleHistorySequence++;

	MultiBattleHistory.Add(NewEntry);

	MultiBattleHistory.Sort(
		[](const FMultiBattleHistoryEntry& A, const FMultiBattleHistoryEntry& B)
		{
			if (A.Score != B.Score) return A.Score > B.Score;
			return A.SequenceId > B.SequenceId;
		});

	const int32 MaxCount = 50;
	int32 NewIndex = MultiBattleHistory.IndexOfByPredicate(
		[Seq = NewEntry.SequenceId](const FMultiBattleHistoryEntry& E) { return E.SequenceId == Seq; });

	if (MultiBattleHistory.Num() > MaxCount)
	{
		MultiBattleHistory.SetNum(MaxCount);
	}
	if (NewIndex >= MaxCount || NewIndex == INDEX_NONE)
	{
		NewIndex = -1;
	}

	SaveMultiBattleHistory();
	return NewIndex;
}

TArray<int32> UBattleBlasterGameInstance::AddMultiBattleHistoryRecordsFromMatch(
	int32 InPlayerCount,
	const TArray<int32>& InKills,
	const TArray<int32>& InDeaths,
	const TArray<int32>& InAssists,
	int32 InTargetScore)
{
	TArray<int32> RankIndices;
	RankIndices.SetNum(InPlayerCount);
	for (int32 i = 0; i < InPlayerCount; ++i)
	{
		RankIndices[i] = -1;
	}

	if (InTargetScore <= 0)
	{
		return RankIndices;
	}

	// 本局添加的记录的 SequenceId 列表，用于之后查找名次
	TArray<int32> AddedSequenceIds;
	AddedSequenceIds.Reserve(InPlayerCount);

	for (int32 i = 0; i < InPlayerCount; ++i)
	{
		const int32 K = InKills.IsValidIndex(i)   ? InKills[i]   : 0;
		const int32 D = InDeaths.IsValidIndex(i)  ? InDeaths[i]  : 0;
		const int32 A = InAssists.IsValidIndex(i) ? InAssists[i] : 0;
		const float ScoreRaw = (static_cast<float>(K) + static_cast<float>(A) * 0.5f - static_cast<float>(D) * 0.3f) * 50.0f / static_cast<float>(InTargetScore);
		const int32 ScoreInt = FMath::Max(0, FMath::RoundToInt(ScoreRaw));

		FMultiBattleHistoryEntry Entry;
		Entry.Score = ScoreInt;
		Entry.Kills = K;
		Entry.Deaths = D;
		Entry.Assists = A;
		Entry.CampIndex = i;
		Entry.SequenceId = MultiBattleHistorySequence++;

		AddedSequenceIds.Add(Entry.SequenceId);
		MultiBattleHistory.Add(Entry);
	}

	// 按评分降序，同分按 SequenceId 降序（后来者居上）
	MultiBattleHistory.Sort(
		[](const FMultiBattleHistoryEntry& A, const FMultiBattleHistoryEntry& B)
		{
			if (A.Score != B.Score)
			{
				return A.Score > B.Score;
			}
			return A.SequenceId > B.SequenceId;
		});

	const int32 MaxCount = 50;
	if (MultiBattleHistory.Num() > MaxCount)
	{
		MultiBattleHistory.SetNum(MaxCount);
	}

	// 查找本局每条记录在榜单中的名次（0 为第 1 名）
	for (int32 PlayerIdx = 0; PlayerIdx < AddedSequenceIds.Num() && PlayerIdx < InPlayerCount; ++PlayerIdx)
	{
		const int32 SeqId = AddedSequenceIds[PlayerIdx];
		const int32 FoundIndex = MultiBattleHistory.IndexOfByPredicate(
			[SeqId](const FMultiBattleHistoryEntry& E) { return E.SequenceId == SeqId; });
		if (FoundIndex != INDEX_NONE && FoundIndex < MaxCount)
		{
			RankIndices[PlayerIdx] = FoundIndex;
		}
	}

	SaveMultiBattleHistory();
	return RankIndices;
}

// ================== 手柄管理实现 ======================

int32 UBattleBlasterGameInstance::GetConnectedGamepadCountWithMapping(TArray<int32>& OutDeviceIndices)
{
	OutDeviceIndices.Empty();
	UWorld* World = GetWorld();

	IPlatformInputDeviceMapper& Mapper = IPlatformInputDeviceMapper::Get();
	TArray<FInputDeviceId> ConnectedDevices;
	Mapper.GetAllConnectedInputDevices(ConnectedDevices);

	const int32 RawDeviceCount = ConnectedDevices.Num();


	int32 GamepadCount = 0;

	if (RawDeviceCount >= 2)
	{
		// 设备 >= 2：可确定存在键盘/鼠标组合。
		// 用 "总设备数 - 1" 估算手柄数（原始 Widget 策略，最可靠）。
		// 场景：键鼠(1) + 手柄(N) = N 个手柄。
		GamepadCount = FMath::Max(0, RawDeviceCount - 1);
	}
	else if (RawDeviceCount == 1)
	{
		// 设备 = 1：需要判断是单键盘还是单手柄。
		// 用 ID > 1 经验规则区分（适用于 Windows 平台）。
		const FInputDeviceId& Dev = ConnectedDevices[0];
		if (Dev.IsValid() && Dev.GetId() > 1)
			GamepadCount = 1;  // 单手柄
		else
			GamepadCount = 0;  // 单键盘
	}

	// 保证至少有 1 个玩家（键鼠玩家），最多 4 人
	int32 EffectivePlayerCount = FMath::Max(1, FMath::Clamp(GamepadCount, 0, 4));


	for (int32 i = 0; i < EffectivePlayerCount; ++i)
	{
		OutDeviceIndices.Add(i);
	}

	CachedGamepadCount = EffectivePlayerCount;
	return EffectivePlayerCount;
}

int32 UBattleBlasterGameInstance::GetConnectedGamepadCount(bool bForceRefresh)
{
	if (bForceRefresh || TimeSinceLastCacheRefresh >= CacheRefreshInterval)
	{
		TArray<int32> Dummy;
		GetConnectedGamepadCountWithMapping(Dummy);
		TimeSinceLastCacheRefresh = 0.0f;
	}
	return CachedGamepadCount;
}

FInputDeviceId UBattleBlasterGameInstance::GetPlayerDeviceId(int32 PlayerIndex) const
{
	if (!PlayerDeviceIdMap.IsValidIndex(PlayerIndex))
	{
		return FInputDeviceId();
	}
	return PlayerDeviceIdMap[PlayerIndex];
}

void UBattleBlasterGameInstance::RegisterPlayerDeviceMapping(int32 PlayerIndex, FInputDeviceId DeviceId)
{
	while (PlayerDeviceIdMap.Num() <= PlayerIndex)
	{
		PlayerDeviceIdMap.Add(FInputDeviceId());
	}
	PlayerDeviceIdMap[PlayerIndex] = DeviceId;
}

void UBattleBlasterGameInstance::ResetDeviceMappings()
{
	PlayerDeviceIdMap.Empty();
	UE_LOG(LogTemp, Display, TEXT("Device mappings reset."));
}
