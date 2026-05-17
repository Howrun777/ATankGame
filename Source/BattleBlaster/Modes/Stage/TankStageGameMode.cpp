#include "Modes/Stage/TankStageGameMode.h"
#include "Modes/Stage/UI/TankStageOverWidget.h"
#include "Modes/Stage/TankStagePlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Shared/Pawns/NPC/Tower.h"
#include "Shared/Pawns/Tank.h"
#include "Core/BattleBlasterGameInstance.h"
#include "GameFramework/PlayerStart.h"
#include "Modes/MainMenu/MainMenuGameMode.h"
#include "Widgets/SWidget.h"
#include "Shared/Combat/HealthComponent.h"
#include "Shared/Buffs/TankBuffComponent.h"

ATankStageGameMode::ATankStageGameMode()
{
	// 设置专用的 GameState 和 PlayerState 类
	GameStateClass = ATankStageGameState::StaticClass();
	PlayerStateClass = ATankStagePlayerState::StaticClass();

	DefaultPawnClass = nullptr;
}

void ATankStageGameMode::BeginPlay()
{
	Super::BeginPlay();

	GameStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	// 单人闯关总计时：进入关卡后记录本关起点（跨关卡累计）
	if (UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(GetGameInstance()))
	{
		// 如果外部没有显式启动计时（例如从菜单直接进入某关），这里兜底启动
		if (!GI->bCampaignTimerActive)
		{
			GI->ResetCampaignTimer();
		}
		GI->MarkCampaignLevelStart(GetWorld());
	}

	// Cache selected tank class from GameInstance
	CacheSelectedTankClass();

	// Select random player start
	CurrentPlayerStart = SelectRandomPlayerStart();

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);

	// Cleanup previous state
	if (PC)
	{
		UGameplayStatics::SetGamePaused(GetWorld(), false);
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
	}

	// Spawn player tank at selected start
	TSubclassOf<ATank> TankClassToUse = TankClass;

	UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(GetGameInstance());
	if (GI && GI->SelectedTankClasses.Num() > 0 && GI->SelectedTankClasses[0] != nullptr)
	{
		TankClassToUse = GI->SelectedTankClasses[0];
		UE_LOG(LogTemp, Display, TEXT("TankStageGameMode: Using tank from UI selection: %s"), *TankClassToUse->GetName());
	}

	if (TankClassToUse)
	{
		FVector SpawnLoc = FVector(0, 0, 100);
		FRotator SpawnRot = FRotator::ZeroRotator;

		if (CurrentPlayerStart)
		{
			SpawnLoc = CurrentPlayerStart->GetActorLocation();
			SpawnRot = CurrentPlayerStart->GetActorRotation();
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		_PlayerTank = GetWorld()->SpawnActor<ATank>(TankClassToUse, SpawnLoc, SpawnRot, SpawnParams);

		if (PC && _PlayerTank)
		{
			PC->Possess(_PlayerTank);
			_PlayerTank->SetSlotId(0);
			_PlayerTank->SetTeamId(0);

			// 应用玩家携带的状态（从上一关继承）
			ApplyPlayerCarryState();

			// === 绑定玩家坦克死亡事件（处理复活/游戏结束逻辑）===
			_PlayerTank->OnKilled.AddDynamic(this, &ATankStageGameMode::HandleTankKilled);
		}
	}

	// Count towers in level and bind their death events
	TArray<AActor*> Towers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATower::StaticClass(), Towers);
	TowerCount = Towers.Num();

	UE_LOG(LogTemp, Display, TEXT("Towers to destroy: %d"), TowerCount);

	// === 绑定所有塔的死亡事件 ===
	for (AActor* TowerActor : Towers)
	{
		if (ATower* Tower = Cast<ATower>(TowerActor))
		{
			if (UHealthComponent* TowerHealth = Tower->FindComponentByClass<UHealthComponent>())
			{
				TowerHealth->OnDeath.AddDynamic(this, &ATankStageGameMode::HandleTowerDestroyed);
			}
		}
	}

	// Apply difficulty to all towers
	ApplyDifficultyToTowers();

	// UI initialization
	if (PC && ScreenMessageClass)
	{
		ScreenMessageWidget = CreateWidget<UScreenMessage>(PC, ScreenMessageClass);
		if (ScreenMessageWidget)
		{
			ScreenMessageWidget->AddToViewport(999);
			ScreenMessageWidget->SetMessageText("READY?");
		}
	}

	// Start countdown
	CountdownSeconds = CountdownDelay;
	PlayerCanControl = false;
	HasShownControlMessage = false;

	GetWorldTimerManager().SetTimer(CountdownTimerHandle, this, &ATankStageGameMode::OnCountdownTimerTimeout, 1.0f, true);
}

void ATankStageGameMode::CacheSelectedTankClass()
{
	UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(GetGameInstance());
	if (GI && GI->SelectedTankClasses.Num() > 0 && GI->SelectedTankClasses[0] != nullptr)
	{
		CurrentTankClass = GI->SelectedTankClasses[0];
	}
	else
	{
		CurrentTankClass = TankClass;
	}
}

AActor* ATankStageGameMode::SelectRandomPlayerStart()
{
	TArray<AActor*> FoundStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundStarts);

	if (FoundStarts.Num() > 0)
	{
		int32 RandomIdx = FMath::RandRange(0, FoundStarts.Num() - 1);
		UE_LOG(LogTemp, Display, TEXT("Selected random PlayerStart: %s (index %d/%d)"), *FoundStarts[RandomIdx]->GetName(), RandomIdx, FoundStarts.Num());
		return FoundStarts[RandomIdx];
	}

	UE_LOG(LogTemp, Warning, TEXT("No PlayerStart found in level!"));
	return nullptr;
}

void ATankStageGameMode::CheckVictoryCondition()
{
	if (TowerCount <= 0)
	{
		IsVictory = true;
		if (ScreenMessageWidget)
		{
			ScreenMessageWidget->SetMessageText("VICTORY!");
			ScreenMessageWidget->SetVisibility(ESlateVisibility::Visible);
		}

		// 过关前保存玩家状态
		SavePlayerStateBeforeLevelEnd();

		// Start game over timer
		GetWorldTimerManager().SetTimer(EndTimerHandle, this, &ATankStageGameMode::OnGameOverTimerTimeOut, GameOverDelay, false);
	}
}

void ATankStageGameMode::OnCountdownTimerTimeout()
{
	CountdownSeconds--;

	if (CountdownSeconds == 2 || CountdownSeconds == 1)
	{
		if (ScreenMessageWidget)
			ScreenMessageWidget->SetMessageText("READY?");
		PlayerCanControl = false;
	}
	else if (CountdownSeconds == 0)
	{
		if (ScreenMessageWidget)
			ScreenMessageWidget->SetMessageText("GO!");

		PlayerCanControl = true;
		HasShownControlMessage = true;

		if (_PlayerTank)
			_PlayerTank->SetPlayerEnabled(true);
	}
	else
	{
		GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
		if (ScreenMessageWidget)
			ScreenMessageWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void ATankStageGameMode::HandleTankKilled(ATank* DeadTank, ATank* KillerTank)
{
	// ATank::HandleDeath 已完成 HandleDestruction + ProcessDeath
	// 这里只负责 PVE 复活流程管理

	UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(GetGameInstance());
	if (!GI)
	{
		UE_LOG(LogTemp, Error, TEXT("HandleTankKilled: Failed to get GameInstance!"));
		return;
	}

	GI->IncrementPlayerDeathCount();
	int32 RemainingLives = GI->GetRemainingLives();

	UE_LOG(LogTemp, Warning, TEXT("Player Died! Remaining lives: %d"), RemainingLives);

	if (RemainingLives <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Player exceeded max death count! Game Over!"));
		GI->ResetPlayerCarryState();
		HandleGameOver();
		return;
	}

	SavePlayerStateBeforeLevelEnd();
	GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &ATankStageGameMode::OnRespawnTimerTimeout, RespawnDelay, false);
}

void ATankStageGameMode::HandleTowerDestroyed(UHealthComponent* InHealthComp, AController* InstigatedBy, AActor* DamageCauser)
{
	// 从 HealthComp 反向查找对应的 Tower Actor
	AActor* TowerOwner = InHealthComp ? InHealthComp->GetOwner() : nullptr;
	ATower* DestroyedTower = Cast<ATower>(TowerOwner);

	if (DestroyedTower)
	{
		DestroyedTower->HandleDestruction();
	}

	TowerCount--;
	UE_LOG(LogTemp, Display, TEXT("Tower destroyed! Remaining: %d"), TowerCount);

	if (TowerCount <= 0)
	{
		CheckVictoryCondition();
	}
}

void ATankStageGameMode::OnRespawnTimerTimeout()
{
	RespawnPlayer();
}

void ATankStageGameMode::RespawnPlayer()
{
	if (!CurrentTankClass)
	{
		UE_LOG(LogTemp, Error, TEXT("No tank class for respawn!"));
		return;
	}

	// =========================================================================
	// 【核心修复：不要Destroy旧坦克！】
	// HandleDestruction 已经把坦克隐藏/关闭了，Destroy() 会连带把 Controller 的 Possess 关系也清掉，
	// 导致后续需要重新做大量修复工作。改成直接废弃旧引用，Spawn 新躯壳。
	// =========================================================================
	ATank* OldTank = _PlayerTank;
	_PlayerTank = nullptr;

	// =========================================================================
	// 【灵魂附体】单人模式也是一视同仁！
	// 通过 PlayerState 的 SlotId 找到原来的 Controller（0号玩家），让它附身新躯壳。
	// =========================================================================
	AController* PlayerController = nullptr;
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		AController* C = It->Get();
		if (ATankStagePlayerState* PS = C->GetPlayerState<ATankStagePlayerState>())
		{
			if (PS->SlotId == 0)
			{
				PlayerController = C;
				break;
			}
		}
	}

	if (!PlayerController)
	{
		PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	}

	// 获取出生位置
	FVector SpawnLoc = FVector(0, 0, 100);
	FRotator SpawnRot = FRotator::ZeroRotator;

	if (CurrentPlayerStart)
	{
		SpawnLoc = CurrentPlayerStart->GetActorLocation();
		SpawnRot = CurrentPlayerStart->GetActorRotation();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	_PlayerTank = GetWorld()->SpawnActor<ATank>(CurrentTankClass, SpawnLoc, SpawnRot, SpawnParams);

	if (_PlayerTank)
	{
		// 灵魂附体：让原来的 Controller 附身新躯壳
		if (PlayerController)
		{
			PlayerController->Possess(_PlayerTank);
			_PlayerTank->SetSlotId(0);
			_PlayerTank->SetTeamId(0);
		}

		// 销毁旧 Tank（释放内存，不再保留隐藏的尸体）
		if (OldTank && OldTank != _PlayerTank)
		{
			OldTank->Destroy();
		}

		// 应用玩家携带的状态（从上一关继承）
		ApplyPlayerCarryState();

		// === 刷新血量 UI ===
		if (UHealthComponent* HC = _PlayerTank->FindComponentByClass<UHealthComponent>())
		{
			HC->UpdateHUD();
		}

		// === 重新绑定死亡事件 ===
		_PlayerTank->OnKilled.AddDynamic(this, &ATankStageGameMode::HandleTankKilled);

		// 启用无敌并播放复活特效
		EnableInvincibilityAndVFX(_PlayerTank);

		// Start countdown for player control
		CountdownSeconds = CountdownDelay;
		PlayerCanControl = false;
		HasShownControlMessage = false;

		if (ScreenMessageWidget)
		{
			ScreenMessageWidget->SetMessageText("READY?");
			ScreenMessageWidget->SetVisibility(ESlateVisibility::Visible);
		}

		GetWorldTimerManager().SetTimer(CountdownTimerHandle, this, &ATankStageGameMode::OnCountdownTimerTimeout, 1.0f, true);

		UE_LOG(LogTemp, Display, TEXT("Player respawned at %s"), *SpawnLoc.ToString());
	}
}

void ATankStageGameMode::OnGameOverTimerTimeOut()
{
	if (!IsVictory) return;

	UBattleBlasterGameInstance* GameInstance = Cast<UBattleBlasterGameInstance>(GetGameInstance());
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get BattleBlasterGameInstance!"));
		return;
	}

	// 离开本关前：把本关用时累加进总计时
	GameInstance->MarkCampaignLevelEnd(GetWorld());

	FString LevelOptions = FString::Printf(TEXT("?game=%s"), *GetClass()->GetPathName());

	// Victory! Load next level
	GameInstance->LoadNextLevel(LevelOptions);
}

void ATankStageGameMode::HandleGameOver()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;

	// 游戏结束结算前：把本关用时累加进总计时
	if (UBattleBlasterGameInstance* GIForTime = Cast<UBattleBlasterGameInstance>(GetGameInstance()))
	{
		GIForTime->MarkCampaignLevelEnd(World);
	}

	// BUG1修复: 游戏结束时解除玩家对Tank的控制
	if (ATank* Tank = Cast<ATank>(PC->GetPawn()))
	{
		Tank->SetPlayerEnabled(false);
	}

	if (PC && GameOverWidgetClass)
	{
		GameOverWidget = CreateWidget<UTankStageOverWidget>(PC, GameOverWidgetClass);
		if (GameOverWidget)
		{
			// 使用跨关卡累计的总游戏时间
			float GameTime = 0.0f;
			if (UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(GetGameInstance()))
			{
				GameTime = GI->GetCampaignTotalTime(World);
			}
			else if (World)
			{
				GameTime = World->GetTimeSeconds() - GameStartTime;
			}
			
			int32 CurrentLevel = 1;
			int32 BestLevel = 1;

			if (UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(GetGameInstance()))
			{
				CurrentLevel = GI->GetCurrentLevelIndex();
				BestLevel = GI->GetBestLevelRecord();
			}

			GameOverWidget->RefreshDisplay(CurrentLevel, BestLevel, GameTime, CurrentTankClass);
			GameOverWidget->AddToViewport(1000);

			FInputModeUIOnly InputMode;
			TSharedRef<SWidget> FocusWidget = GameOverWidget->TakeWidget();
			InputMode.SetWidgetToFocus(FocusWidget);
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = true;
		}
	}
	else if (ScreenMessageWidget)
	{
		ScreenMessageWidget->SetMessageText("GAME OVER");
		ScreenMessageWidget->SetVisibility(ESlateVisibility::Visible);
		FTimerHandle ReturnMenuTimerHandle;
		GetWorldTimerManager().SetTimer(ReturnMenuTimerHandle, this, &ATankStageGameMode::ReturnToSinglePlayerMenu, 2.0f, false);
	}
}

void ATankStageGameMode::ReturnToSinglePlayerMenu()
{
	// Unpause first
	UGameplayStatics::SetGamePaused(GetWorld(), false);

	UWorld* World = GetWorld();
	if (!World) return;

	// Clean up player controller
	APlayerController* PC = World->GetFirstPlayerController();
	if (PC)
	{
		// Clear input and reset state
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (GameInstance)
	{
		// Clean up extra local players (remove splitscreen)
		TArray<ULocalPlayer*> AllLocalPlayers = GameInstance->GetLocalPlayers();
		for (int32 i = AllLocalPlayers.Num() - 1; i > 0; i--)
		{
			if (IsValid(AllLocalPlayers[i]))
			{
				GameInstance->RemoveLocalPlayer(AllLocalPlayers[i]);
			}
		}
	}

	FString MainMenuLevel = TEXT("MainMenuLevel_1");
	FString LoadOptions;

	// 检查返回类型
	UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(GameInstance);
	if (GI && GI->GetReturnToMenuType() == EReturnToMenuType::SinglePlayerMenu)
	{
		// 返回单人闯关选择界面 - 重置死亡次数（新游戏）
		if (GI)
		{
			GI->ResetPlayerDeathCount();
		}

		// 返回单人闯关选择界面
		LoadOptions = FString::Printf(TEXT("?GameMode=%s"), *AMainMenuGameMode::StaticClass()->GetName());
		// 重置为默认的主菜单类型
		GI->SetReturnToMenuType(EReturnToMenuType::MainMenu);
		UE_LOG(LogTemp, Display, TEXT("Returning to SinglePlayer menu: %s with options: %s"), *MainMenuLevel, *LoadOptions);
	}
	else
	{
		// 返回主菜单
		LoadOptions = FString::Printf(TEXT("?GameMode=%s"), *AMainMenuGameMode::StaticClass()->GetName());
		UE_LOG(LogTemp, Display, TEXT("Returning to main menu: %s with options: %s"), *MainMenuLevel, *LoadOptions);
	}

	// Use absolute=true to ensure our options are used
	UGameplayStatics::OpenLevel(World, FName(*MainMenuLevel), true, LoadOptions);
}

void ATankStageGameMode::ApplyDifficultyToTowers()
{
	UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(GetGameInstance());
	if (!GI)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to get GameInstance for difficulty"));
		return;
	}

	float DifficultyMultiplier = GI->GetCurrentDifficultyMultiplier();
	UE_LOG(LogTemp, Display, TEXT("Applying difficulty: Level=%d, Multiplier=%.2f"),
		GI->GetCurrentLevelIndex(), DifficultyMultiplier);

	TArray<AActor*> Towers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATower::StaticClass(), Towers);

	for (AActor* TowerActor : Towers)
	{
		ATower* Tower = Cast<ATower>(TowerActor);
		if (Tower)
		{
			Tower->ApplyDifficultyMultiplier(DifficultyMultiplier);
		}
	}

	UE_LOG(LogTemp, Display, TEXT("Applied difficulty to %d towers"), Towers.Num());
}

/**
 * @brief 保存玩家状态 - 用于关卡切换或玩家死亡时
 * 
 * 保存当前玩家的:
 * - 生命值(剩余生命)
 * - 子弹数(剩余子弹)
 * - 所有激活的Buff状态、持续时间和图标
 * 
 * 这些数据会被保存到GameInstance中,在以下情况被调用:
 * 1. 玩家过关时(CheckVictoryCondition中调用)
 * 2. 玩家死亡但还有剩余生命时(HandleTankKilled中调用)
 * 
 * 注意: 游戏结束时不会调用此函数,而是调用ResetPlayerCarryState清除状态
 */
void ATankStageGameMode::SavePlayerStateBeforeLevelEnd()
{
	if (!_PlayerTank) return;

	UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(GetGameInstance());
	if (!GI) return;

	// 获取当前生命值
	UHealthComponent* HealthComp = _PlayerTank->FindComponentByClass<UHealthComponent>();
	float CurrentHealth = HealthComp ? HealthComp->CurrentHealth : 0.0f;

	// 获取当前子弹数量
	int32 CurrentAmmo = _PlayerTank->CurrentAmmo;

	// 获取 Buff 状态
	UTankBuffComponent* BuffComp = _PlayerTank->GetBuffComponent();
	bool bInfiniteAmmo = _PlayerTank->bHasInfiniteAmmo;
	bool bSpeedBoost = false; // 需要检查 Tank 是否有这个变量
	bool bDamageBoost = _PlayerTank->bHasDamageBoost;
	bool bBulletPierce = _PlayerTank->bHasBulletPierce;
	bool bDoubleShot = _PlayerTank->bHasDoubleShot;
	bool bGhostMode = _PlayerTank->bIsGhostMode;
	bool bShield = false; // 需要检查是否在 ActiveBuffs 中

	// Buff 图标
	UTexture2D* InfiniteAmmoIcon = nullptr;
	UTexture2D* SpeedBoostIcon = nullptr;
	UTexture2D* DamageBoostIcon = nullptr;
	UTexture2D* BulletPierceIcon = nullptr;
	UTexture2D* DoubleShotIcon = nullptr;
	UTexture2D* GhostModeIcon = nullptr;

	// 获取 Buff 持续时间
	float InfiniteAmmoDur = 0.0f, SpeedBoostDur = 0.0f, DamageBoostDur = 0.0f, BulletPierceDur = 0.0f;
	float DoubleShotDur = 0.0f, GhostModeDur = 0.0f;

	if (BuffComp)
	{
		TArray<FActiveBuffUIInfo> ActiveBuffs = BuffComp->GetActiveBuffsForUI();
		for (const FActiveBuffUIInfo& Buff : ActiveBuffs)
		{
			switch (Buff.Type)
			{
			case EBuffType::Ammo:
				InfiniteAmmoDur = Buff.RemainingTime;
				bInfiniteAmmo = true;
				InfiniteAmmoIcon = Buff.Icon;
				break;
			case EBuffType::Speed:
				SpeedBoostDur = Buff.RemainingTime;
				bSpeedBoost = true;
				SpeedBoostIcon = Buff.Icon;
				break;
			case EBuffType::Damage:
				DamageBoostDur = Buff.RemainingTime;
				bDamageBoost = true;
				DamageBoostIcon = Buff.Icon;
				break;
			case EBuffType::Pierce:
				BulletPierceDur = Buff.RemainingTime;
				bBulletPierce = true;
				BulletPierceIcon = Buff.Icon;
				break;
			case EBuffType::DoubleShot:
				DoubleShotDur = Buff.RemainingTime;
				bDoubleShot = true;
				DoubleShotIcon = Buff.Icon;
				break;
			case EBuffType::Ghost:
				GhostModeDur = Buff.RemainingTime;
				bGhostMode = true;
				GhostModeIcon = Buff.Icon;
				break;
			case EBuffType::Shield:
				bShield = true;
				break;
			default: break;
			}
		}
	}

	// 保存到 GameInstance
	GI->SavePlayerCarryState(CurrentHealth, CurrentAmmo,
		bInfiniteAmmo, bSpeedBoost, bDamageBoost, bBulletPierce, bDoubleShot, bGhostMode, bShield,
		InfiniteAmmoDur, SpeedBoostDur, DamageBoostDur, BulletPierceDur, DoubleShotDur, GhostModeDur,
		InfiniteAmmoIcon, SpeedBoostIcon, DamageBoostIcon, BulletPierceIcon, DoubleShotIcon, GhostModeIcon);

	UE_LOG(LogTemp, Display, TEXT("Player state saved: Health=%.1f, Ammo=%d"), CurrentHealth, CurrentAmmo);
}

/**
 * @brief 应用玩家携带状态 - 玩家复活或进入新关卡时调用
 * 
 * 从GameInstance中读取之前保存的玩家状态并应用到当前玩家:
 * 1. 生命值: 复活基础奖励 + 携带的剩余生命
 * 2. 子弹数: 复活基础子弹 + 携带的剩余子弹
 * 3. Buff状态: 清除所有现有Buff后重新应用保存的Buff
 * 
 * 使用场景:
 * 1. 玩家死亡后复活(RespawnPlayer中调用)
 * 2. 玩家过关后进入下一关(BeginPlay中调用)
 */
void ATankStageGameMode::ApplyPlayerCarryState()
{
	if (!_PlayerTank) return;

	UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(GetGameInstance());
	if (!GI) return;

	FPlayerCarryState CarryState;
	bool bHasCarryState = GI->HasPlayerCarryState();
	if (bHasCarryState)
	{
		CarryState = GI->GetPlayerCarryState();
	}

	// 1. 应用基础复活奖励 + 携带的生命值
	UHealthComponent* HealthComp = _PlayerTank->FindComponentByClass<UHealthComponent>();
	if (HealthComp)
	{
		float MaxHealth = HealthComp->MaxHealth;
		float RespawnHealth = MaxHealth * RespawnHealthPercent; // 50% 最大生命
		float AdditionalHealth = bHasCarryState ? CarryState.Health : 0.0f; // 上一关剩余的生命

		float TotalHealth = RespawnHealth + AdditionalHealth;
		HealthComp->CurrentHealth = FMath::Clamp(TotalHealth, 0.0f, MaxHealth);

		UE_LOG(LogTemp, Display, TEXT("Applied health: Respawn=%.1f + Carry=%.1f = Total=%.1f (Max=%.1f)"),
			RespawnHealth, AdditionalHealth, HealthComp->CurrentHealth, MaxHealth);
	}

	// 2. 应用子弹 + 携带的子弹
	int32 RespawnAmmo = FMath::FloorToInt(_PlayerTank->MaxAmmo * RespawnAmmoPercent); // MaxAmmo * 0.5
	int32 AdditionalAmmo = bHasCarryState ? CarryState.Ammo : 0; // 上一关剩余的子弹

	_PlayerTank->CurrentAmmo = FMath::Clamp(RespawnAmmo + AdditionalAmmo, 0, _PlayerTank->MaxAmmo);

	UE_LOG(LogTemp, Display, TEXT("Applied ammo: Respawn=%d + Carry=%d = Total=%d (Max=%d)"),
		RespawnAmmo, AdditionalAmmo, _PlayerTank->CurrentAmmo, _PlayerTank->MaxAmmo);

	// 3. 应用 Buff 状态
	UTankBuffComponent* BuffComp = _PlayerTank->GetBuffComponent();
	if (BuffComp)
	{
		// 先确保 BaseSpeed 已初始化（防止移速 Buff 出问题）
		if (_PlayerTank->BaseSpeed <= 0)
		{
			_PlayerTank->BaseSpeed = _PlayerTank->Speed;
		}

		// 清除所有现有 Buff，避免重复添加导致时间叠加
		BuffComp->ClearAllBuffs();

		// 无限子弹 - 使用保存的图标
		if (CarryState.bHasInfiniteAmmo && CarryState.InfiniteAmmoDuration > 0)
		{
			_PlayerTank->bHasInfiniteAmmo = true;
			_PlayerTank->CachedAmmo = _PlayerTank->CurrentAmmo;
			BuffComp->AddBuff(EBuffType::Ammo, CarryState.InfiniteAmmoDuration, CarryState.InfiniteAmmoIcon);
		}

		// 移速提升 - 使用保存的图标
		if (CarryState.bHasSpeedBoost && CarryState.SpeedBoostDuration > 0)
		{
			BuffComp->AddBuff(EBuffType::Speed, CarryState.SpeedBoostDuration, CarryState.SpeedBoostIcon);
		}

		// 伤害翻倍 - 使用保存的图标
		if (CarryState.bHasDamageBoost && CarryState.DamageBoostDuration > 0)
		{
			_PlayerTank->bHasDamageBoost = true;
			BuffComp->AddBuff(EBuffType::Damage, CarryState.DamageBoostDuration, CarryState.DamageBoostIcon);
		}

		// 子弹穿透 - 使用保存的图标
		if (CarryState.bHasBulletPierce && CarryState.BulletPierceDuration > 0)
		{
			_PlayerTank->bHasBulletPierce = true;
			BuffComp->AddBuff(EBuffType::Pierce, CarryState.BulletPierceDuration, CarryState.BulletPierceIcon);
		}

		// 双发 - 使用保存的图标
		if (CarryState.bHasDoubleShot && CarryState.DoubleShotDuration > 0)
		{
			_PlayerTank->bHasDoubleShot = true;
			BuffComp->AddBuff(EBuffType::DoubleShot, CarryState.DoubleShotDuration, CarryState.DoubleShotIcon);
		}

		// 穿墙 - 使用保存的图标
		if (CarryState.bIsGhostMode && CarryState.GhostModeDuration > 0)
		{
			_PlayerTank->bIsGhostMode = true;
			BuffComp->AddBuff(EBuffType::Ghost, CarryState.GhostModeDuration, CarryState.GhostModeIcon);
		}

		// 护盾（一次性 Buff）
		if (CarryState.bHasShield)
		{
			BuffComp->AddBuff(EBuffType::Shield, 0.0f, nullptr);
		}
	}

	UE_LOG(LogTemp, Display, TEXT("Applied carry state to player"));
}

int32 ATankStageGameMode::GetMaxDeathCount() const
{
	UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(GetGameInstance());
	if (GI)
	{
		return GI->GetMaxDeathCount();
	}
	return 3; // 默认值
}

int32 ATankStageGameMode::GetRemainingLives() const
{
	UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(GetGameInstance());
	if (GI)
	{
		return GI->GetRemainingLives();
	}
	return 3; // 默认值
}

void ATankStageGameMode::EnableInvincibilityAndVFX(ATank* Tank)
{
	if (!Tank || !IsValid(Tank)) return;

	// 1. 设置无敌
	Tank->SetCanBeDamaged(false);

	// 2. 播放 Niagara 特效
	UNiagaraComponent* VFXComp = nullptr;
	if (RespawnNiagaraVFX)
	{
		VFXComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
			RespawnNiagaraVFX,
			Tank->GetRootComponent(),
			NAME_None,
			FVector(0, 0, -50),
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true
		);
	}

	// 3. 设置定时器，3秒后结束无敌
	FTimerHandle InvincibleTimer;
	FTimerDelegate InvincibleDel;
	InvincibleDel.BindUObject(this, &ATankStageGameMode::EndInvincibility, Tank, VFXComp);

	GetWorldTimerManager().SetTimer(InvincibleTimer, InvincibleDel, InvincibleTime, false);
}

void ATankStageGameMode::EndInvincibility(ATank* Tank, UNiagaraComponent* VFXComp)
{
	// 1. 恢复受伤 (取消无敌)
	if (Tank && IsValid(Tank))
	{
		Tank->SetCanBeDamaged(true);
	}

	// 2. 停止 Niagara 特效
	if (VFXComp && IsValid(VFXComp))
	{
		VFXComp->DestroyComponent();
	}
}
