// Fill out your copyright notice in the Description page of Project Settings.

#include "Modes/TeamBattle/TeamBattleGameMode.h"

#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Core/BattleBlasterGameInstance.h"
#include "Shared/Pawns/NPC/Tower.h"
#include "Shared/Pawns/Tank.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Shared/AI/AIBotPlayerController.h"
#include "Shared/Controllers/TankPlayerController.h"
#include "Modes/TeamBattle/UI/TeamBattleGameOverWidget.h"
#include "Modes/TeamBattle/TeamBattlePlayerState.h"

ATeamBattleGameMode::ATeamBattleGameMode()
{
	// 设置专用的 GameState 和 PlayerState 类
	GameStateClass = ATeamBattleGameState::StaticClass();
	PlayerStateClass = ATeamBattlePlayerState::StaticClass();

	// 禁止生成默认角色，所有角色生成逻辑由程序员严格控制
	DefaultPawnClass = nullptr;
}

ATeamBattleGameState* ATeamBattleGameMode::GetTeamBattleGameState() const
{
	if (!GameState)
	{
		return nullptr;
	}
	return Cast<ATeamBattleGameState>(GameState);
}

// ================= 阵营相关函数 =================

ATeamBattleGameMode::ETeamCamp ATeamBattleGameMode::GetPlayerCamp(int32 PlayerIndex) const
{
	// 玩家0和2为红色阵营，玩家1和3为蓝色阵营
	if (PlayerIndex == 0 || PlayerIndex == 2)
	{
		return ETeamCamp::Red;
	}
	return ETeamCamp::Blue;
}

TArray<int32> ATeamBattleGameMode::GetPlayersInCamp(ETeamCamp Camp) const
{
	TArray<int32> Players;
	for (int32 i = 0; i < TeamBattlePlayerCount; i++)
	{
		if (GetPlayerCamp(i) == Camp)
		{
			Players.Add(i);
		}
	}
	return Players;
}

bool ATeamBattleGameMode::IsSameCamp(int32 PlayerIndexA, int32 PlayerIndexB) const
{
	return GetPlayerCamp(PlayerIndexA) == GetPlayerCamp(PlayerIndexB);
}

void ATeamBattleGameMode::AddTeamScore(int32 CampIndex, int32 Amount)
{
	if (TeamScores.IsValidIndex(CampIndex))
	{
		TeamScores[CampIndex] += Amount;
		UpdateTeamScoresDisplay();
	}
}

void ATeamBattleGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 游戏阶段：允许分屏
	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		Viewport->SetForceDisableSplitscreen(false);
	}

	// 1) 读取 GameInstance 设置
	if (UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(GetGameInstance()))
	{
		TargetScore = GI->TargetMatchScore;
		// 团队模式固定4人
		TargetPlayerCount = 4;
		// 读取实际连接的手柄数量
		ConnectedGamepadCount = FMath::Clamp(GI->ConnectedGamepadCount, 1, 4);
	}
	else
	{
		TargetPlayerCount = 4;
		TargetScore = 7;
		ConnectedGamepadCount = 1;
	}

	// 2) 分屏机制：根据手柄数量动态决定
	// 1个手柄→不分屏，2个手柄→左右分屏，3-4个手柄→四宫格
	if (ConnectedGamepadCount <= 1)
	{
		ViewportPlayerCount = 1;
	}
	else if (ConnectedGamepadCount == 2)
	{
		ViewportPlayerCount = 2;
	}
	else
	{
		ViewportPlayerCount = 4;
	}

	// 3) 根据 ViewportPlayerCount 调整本地玩家数量
	UWorld* World = GetWorld();
	if (World)
	{
		const int32 CurrentPlayers = UGameplayStatics::GetNumLocalPlayerControllers(World);

		for (int32 i = CurrentPlayers; i < ViewportPlayerCount; ++i)
		{
			UGameplayStatics::CreatePlayer(World, -1, true);
		}

		while (UGameplayStatics::GetNumLocalPlayerControllers(World) > ViewportPlayerCount)
		{
			const int32 LastPlayerIndex = UGameplayStatics::GetNumLocalPlayerControllers(World) - 1;
			APlayerController* PlayerToRemove = UGameplayStatics::GetPlayerController(World, LastPlayerIndex);
			if (!PlayerToRemove) break;
			// 【核心修复】：在移除 Controller 前，先把它对应的 PlayerState 扬了
			if (APlayerState* PSToRemove = PlayerToRemove->PlayerState)
			{
				PSToRemove->Destroy();
			}
			UGameplayStatics::RemovePlayer(PlayerToRemove, true);
		}
	}

	// ================= 计算AI控制信息 =================
	// 团队模式固定4人，前ConnectedGamepadCount个为真人，其余为AI
	bIsPlayerAIControlled.SetNum(TeamBattlePlayerCount);
	for (int32 i = 0; i < TeamBattlePlayerCount; i++)
	{
		bIsPlayerAIControlled[i] = (i >= ConnectedGamepadCount);
		UE_LOG(LogTemp, Display, TEXT("Player %d: Camp=%s, AIControlled=%s"), i,
			(GetPlayerCamp(i) == ETeamCamp::Red ? TEXT("Red") : TEXT("Blue")),
			bIsPlayerAIControlled[i] ? TEXT("True") : TEXT("False"));
	}

	// ================= 初始化所有数组 =================
	PlayerStarts.SetNum(TeamBattlePlayerCount);
	ActiveTanks.SetNum(TeamBattlePlayerCount);
	TeamScores.Init(0, 2); // 红色=0，蓝色=1
	PlayerSavedBuffs.SetNum(TeamBattlePlayerCount);

	// 初始化 GameState 中的玩家KDA数据
	if (ATeamBattleGameState* TBGameState = GetTeamBattleGameState())
	{
		TBGameState->InitializePlayerData(TeamBattlePlayerCount);
	}

	// 2. 查找并排序所有 PlayerStart
	TArray<AActor*> FoundStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundStarts);

	for (AActor* Start : FoundStarts)
	{
		for (int32 i = 0; i < TeamBattlePlayerCount; i++)
		{
			FString TagName = FString::Printf(TEXT("P%d"), i);
			APlayerStart* PStart = Cast<APlayerStart>(Start);
			if (PStart && PStart->PlayerStartTag == FName(*TagName))
			{
				PlayerStarts[i] = Start;
				break;
			}
		}
	}

	// 3. 循环生成玩家（玩家 + AI 共用 Tank 蓝图）
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (int32 i = 0; i < TeamBattlePlayerCount; i++)
	{
		// 检查出生点是否存在
		if (!PlayerStarts[i])
		{
			UE_LOG(LogTemp, Error, TEXT("Missing PlayerStart with Tag P%d"), i);
			continue;
		}

		// 统一根据菜单选择 / 默认 TankClass 选出要生成的 Tank 蓝图
		TSubclassOf<ATank> TankClassToUse = TankClass;

		if (UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(GetGameInstance()))
		{
			if (GI->SelectedTankClasses.IsValidIndex(i) &&
				GI->SelectedTankClasses[i] != nullptr)
			{
				TankClassToUse = GI->SelectedTankClasses[i];
			}
		}

		if (!TankClassToUse)
		{
			UE_LOG(LogTemp, Error, TEXT("No TankClassToUse for slot %d"), i);
			continue;
		}

		// 生成坦克实例
		ATank* NewTank = GetWorld()->SpawnActor<ATank>(
			TankClassToUse,
			PlayerStarts[i]->GetActorLocation(),
			PlayerStarts[i]->GetActorRotation(),
			SpawnParams
		);

		if (!NewTank)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to spawn tank for slot %d"), i);
			continue;
		}

		// 按槽位决定挂人类控制器还是AI控制器
		const bool bIsAI =
			bIsPlayerAIControlled.IsValidIndex(i) &&
			bIsPlayerAIControlled[i];

		if (bIsAI)
		{
			FActorSpawnParameters AIControllerSpawnParams;
			AIControllerSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AAIBotPlayerController* AIPC = GetWorld()->SpawnActor<AAIBotPlayerController>(
				AAIBotPlayerController::StaticClass(),
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				AIControllerSpawnParams
			);

			if (AIPC)
			{
				AIPC->Possess(NewTank);

				// 【核心修复】：AI Controller 已开启 bWantsPlayerState=true，引擎已自动生成 PlayerState！
				// 我们只需提取它并改写编号和阵营，不要手动 Spawn！
				if (ATeamBattlePlayerState* AIPlayerState = AIPC->GetPlayerState<ATeamBattlePlayerState>())
				{
					AIPlayerState->PlayerIndex = i;
					AIPlayerState->TeamID = static_cast<int32>(GetPlayerCamp(i));
					AIPlayerState->SetPlayerName(FString::Printf(TEXT("AI_P%d"), i));
				}

				UE_LOG(LogTemp, Display, TEXT("Slot %d (Camp=%s) controlled by AI"), i,
					(GetPlayerCamp(i) == ETeamCamp::Red ? TEXT("Red") : TEXT("Blue")));
			}
		}
		else
		{
			// 真人槽位：使用本地 PlayerController（分屏）
			APlayerController* PC = nullptr;

			if (i == 0)
			{
				PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
			}
			else
			{
				if (UGameplayStatics::GetNumLocalPlayerControllers(GetWorld()) <= i)
				{
					PC = UGameplayStatics::CreatePlayer(GetWorld(), -1, true);
				}
				else
				{
					PC = UGameplayStatics::GetPlayerController(GetWorld(), i);
				}
			}

			if (PC)
			{
				PC->Possess(NewTank);

				// 【核心修复】：确保真人玩家拿到属于自己的槽位编号！
				if (ATeamBattlePlayerState* HumanPS = PC->GetPlayerState<ATeamBattlePlayerState>())
				{
					HumanPS->PlayerIndex = i;
					HumanPS->TeamID = static_cast<int32>(GetPlayerCamp(i));
				}
			}
		}

		// 开局先禁用输入（等倒计时GO!再启用）
		NewTank->SetPlayerEnabled(false);
		ActiveTanks[i] = NewTank;

		// === 绑定 Tank 死亡事件（GameMode 监听此委托处理胜负/计分/复活） ===
		NewTank->OnKilled.AddDynamic(this, &ATeamBattleGameMode::HandleTankKilled);

		// 设置玩家索引，用于团队模式判断阵营
		NewTank->SetPlayerIndex(i);
	}

	// 为"额外的视口玩家"设置纯黑画面
	GetWorldTimerManager().SetTimer(
		ExtraViewportBlackTimerHandle,
		this,
		&ATeamBattleGameMode::ApplyBlackScreenToExtraViewports,
		0.2f,
		false
	);

	// ================= UI相关的逻辑 =================
	// 1. 创建旧的ScreenMessage (用于显示 GO 和 Victory)
	APlayerController* PC0 = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC0 && ScreenMessageClass)
	{
		ScreenMessageWidget = CreateWidget<UScreenMessage>(PC0, ScreenMessageClass);
		if (ScreenMessageWidget)
		{
			ScreenMessageWidget->AddToViewport(1000);
			ScreenMessageWidget->SetMessageText("Get Ready!");
		}
	}

	// 2. 创建ScoreBoard (用于显示比分和时间)
	// 团队模式显示红色vs蓝色
	if (PC0 && ScoresWidgetClass)
	{
		ScoresWidgetInstance = CreateWidget<UScoresDisplayWidget>(PC0, ScoresWidgetClass);
		if (ScoresWidgetInstance)
		{
			ScoresWidgetInstance->AddToViewport(10);
			ScoresWidgetInstance->InitTargetScore(TargetScore);
			// 团队模式固定4人，但UI只显示两个阵营分数
			ScoresWidgetInstance->SetVisiblePlayerCount(2);
			ScoresWidgetInstance->UpdateTeamScores(0, 0); // 初始红色0，蓝色0
			ScoresWidgetInstance->UpdateMatchTimer(0);
		}
	}

	// 初始化倒计时数值
	CountdownSeconds = CountdownDelay;
	GetWorldTimerManager().SetTimer(
		CountdownTimerHandle,
		this,
		&ATeamBattleGameMode::OnCountdownTimerTimeout,
		1.0f,
		true
	);
}

// -------------------------------------------------------------------------
// 计时器相关逻辑
// -------------------------------------------------------------------------

void ATeamBattleGameMode::OnCountdownTimerTimeout()
{
	CountdownSeconds -= 1;

	if (CountdownSeconds > 0)
	{
		if (ScreenMessageWidget)
			ScreenMessageWidget->SetMessageText(FString::FromInt(CountdownSeconds));
	}
	else if (CountdownSeconds == 0)
	{
		if (ScreenMessageWidget)
			ScreenMessageWidget->SetMessageText("Go!");

		// 启用所有玩家输入
		for (ATank* Tank : ActiveTanks)
		{
			if (Tank) Tank->SetPlayerEnabled(true);
		}

		// 开始比赛计时
		GetWorldTimerManager().SetTimer(
			MatchTimerHandle,
			this,
			&ATeamBattleGameMode::UpdateMatchTime,
			1.0f,
			true
		);
	}
	else
	{
		GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
		if (ScreenMessageWidget)
			ScreenMessageWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void ATeamBattleGameMode::UpdateMatchTime()
{
	MatchTimeSeconds++;
	if (ScoresWidgetInstance)
	{
		ScoresWidgetInstance->UpdateMatchTimer(MatchTimeSeconds);
	}
}

// ------------------------------------------------------------
// 将多余视口渲染为纯黑
// ------------------------------------------------------------
void ATeamBattleGameMode::ApplyBlackScreenToExtraViewports()
{
	APlayerController* PC0 = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PC = Iterator->Get();

		if (PC && PC->IsLocalController() && PC->GetPawn() == nullptr)
		{
			if (PC0 && BlackoutWidgetClass)
			{
				UUserWidget* MasterBlackout = CreateWidget<UUserWidget>(PC0, BlackoutWidgetClass);
				if (MasterBlackout)
				{
					MasterBlackout->AddToViewport(1);
					BlackoutWidgetInstances.Add(MasterBlackout);
					UE_LOG(LogTemp, Warning, TEXT("发现空闲屏幕！已贴上全屏遮挡黑布！"));
				}
			}

			PC->DisableInput(PC);
		}
	}
}

// ================== 内存泄漏终极修复 ==================
void ATeamBattleGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// 1. 清理所有黑屏UI
	for (UUserWidget* BlackoutWidget : BlackoutWidgetInstances)
	{
		if (IsValid(BlackoutWidget))
		{
			BlackoutWidget->RemoveFromParent();
		}
	}
	BlackoutWidgetInstances.Empty();

	// 2. 清理屏幕提示UI
	if (IsValid(ScreenMessageWidget))
	{
		ScreenMessageWidget->RemoveFromParent();
		ScreenMessageWidget = nullptr;
	}

	// 3. 清理计分板UI
	if (IsValid(ScoresWidgetInstance))
	{
		ScoresWidgetInstance->RemoveFromParent();
		ScoresWidgetInstance = nullptr;
	}

	// 4. 清理团队结算UI
	if (IsValid(TeamBattleGameOverWidgetInstance))
	{
		TeamBattleGameOverWidgetInstance->RemoveFromParent();
		TeamBattleGameOverWidgetInstance = nullptr;
	}

	// 5. 清理所有定时器
	GetWorldTimerManager().ClearAllTimersForObject(this);
}

// 处理死亡逻辑
// KDA 全部已在 ATank::HandleDeath → ATankPlayerState::ProcessDeath 内部完成，
// 这里只处理：GameState 死亡数更新 / Buff 保存 / 复活计时 / 胜负判定
// 阵营积分已在 ATankPlayerState::HandleKillConfirmed → ATeamBattlePlayerState::HandleKillConfirmed
//   → ATeamBattleGameMode::AddTeamScore 中完成
// 死亡事件由 ATank::OnKilled 委托触发（BeginPlay 中已绑定）
void ATeamBattleGameMode::HandleTankKilled(ATank* DeadTank, ATank* KillerTank)
{
	if (WinnerCampIndex != -1) return;
	if (!DeadTank) return;

	// === 保存受害者玩家索引 ===
	int32 VictimIndex = ActiveTanks.Find(DeadTank);
	if (VictimIndex == INDEX_NONE) return;

	// === 保存死亡玩家的 Buff 信息 ===
	UTankBuffComponent* DeadBuffComp = DeadTank->FindComponentByClass<UTankBuffComponent>();
	if (DeadBuffComp && PlayerSavedBuffs.IsValidIndex(VictimIndex))
	{
		PlayerSavedBuffs[VictimIndex] = DeadBuffComp->GetAllActiveBuffs();
	}

	// === 阵营积分规则 ===
	// TeamBattle: 玩家索引 0,2 = 阵营0(红)；1,3 = 阵营1(蓝)
	int32 VictimCamp = static_cast<int32>(GetPlayerCamp(VictimIndex));
	if (KillerTank)
	{
		int32 KillerIdx = ActiveTanks.Find(KillerTank);
		if (KillerIdx != INDEX_NONE && KillerIdx != VictimIndex)
		{
			int32 KillerCamp = static_cast<int32>(GetPlayerCamp(KillerIdx));
			AddTeamScore(KillerCamp, +1);
		}
	}
	else
	{
		// 无凶手（自伤/塔杀）：死亡者阵营扣 1，最低为 0
		int32 CurrentScore = TeamScores.IsValidIndex(VictimCamp) ? TeamScores[VictimCamp] : 0;
		if (CurrentScore > 0)
		{
			AddTeamScore(VictimCamp, -1);
		}
	}

	// === 胜负判定 ===
	for (int32 CampIdx = 0; CampIdx < 2; CampIdx++)
	{
		if (TeamScores[CampIdx] >= TargetScore)
		{
			WinnerCampIndex = CampIdx;
			break;
		}
	}

	if (WinnerCampIndex != -1)
	{
		FString WinMsg = (WinnerCampIndex == 0) ? TEXT("RED TEAM WINS!") : TEXT("BLUE TEAM WINS!");
		ETeamCamp WinnerCamp = (WinnerCampIndex == 0) ? ETeamCamp::Red : ETeamCamp::Blue;
		TArray<int32> WinnerPlayers = GetPlayersInCamp(WinnerCamp);
		AActor* WinnerActor = nullptr;
		for (int32 WinnerIdx : WinnerPlayers)
		{
			if (ActiveTanks.IsValidIndex(WinnerIdx) && ActiveTanks[WinnerIdx])
			{
				WinnerActor = ActiveTanks[WinnerIdx];
				break;
			}
		}
		if (WinnerActor && VictoryEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), VictoryEffect, WinnerActor->GetActorLocation());
		}
		if (ScreenMessageWidget)
		{
			ScreenMessageWidget->SetMessageText(WinMsg);
			FLinearColor Color = (WinnerCampIndex == 0) ? FLinearColor::Red : FLinearColor::Blue;
			ScreenMessageWidget->SetMessageColor(Color);
			ScreenMessageWidget->SetVisibility(ESlateVisibility::Visible);
		}
		GetWorldTimerManager().ClearTimer(MatchTimerHandle);
		FTimerHandle EndTimer;
		GetWorldTimerManager().SetTimer(EndTimer, this, &ATeamBattleGameMode::ShowTeamBattleGameOver, GameOverDelay, false);
	}
	else
	{
		FTimerDelegate RespawnDel;
		RespawnDel.BindUObject(this, &ATeamBattleGameMode::RespawnPlayer, VictimIndex);
		FTimerHandle RespawnTimer;
		GetWorldTimerManager().SetTimer(RespawnTimer, RespawnDel, RespawnDelay, false);
	}
}

void ATeamBattleGameMode::RespawnPlayer(int32 PlayerIndex)
{
	if (WinnerCampIndex != -1) return;
	if (!PlayerStarts.IsValidIndex(PlayerIndex) || !PlayerStarts[PlayerIndex]) return;

	TSubclassOf<ATank> TankClassToUse = TankClass;

	if (UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(GetGameInstance()))
	{
		if (GI->SelectedTankClasses.IsValidIndex(PlayerIndex) &&
			GI->SelectedTankClasses[PlayerIndex] != nullptr)
		{
			TankClassToUse = GI->SelectedTankClasses[PlayerIndex];
		}
	}

	if (!TankClassToUse) return;

	// 保存旧 Tank 引用（用于复活后销毁）
	ATank* OldTank = ActiveTanks.IsValidIndex(PlayerIndex) ? ActiveTanks[PlayerIndex] : nullptr;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// 1. 生成新的坦克躯体
	ATank* NewTank = GetWorld()->SpawnActor<ATank>(
		TankClassToUse,
		PlayerStarts[PlayerIndex]->GetActorLocation(),
		PlayerStarts[PlayerIndex]->GetActorRotation(),
		SpawnParams
	);

	if (!NewTank) return;

	// =========================================================================
	// 【核心修复：灵魂附体】不要生成新 Controller 和 PlayerState！
	// 去世界里找到原来那个存有击杀记录的老 Controller，让它直接附身新躯壳！
	// =========================================================================
	AController* TargetController = nullptr;
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		AController* C = It->Get();
		if (ATeamBattlePlayerState* PS = C->GetPlayerState<ATeamBattlePlayerState>())
		{
			if (PS->PlayerIndex == PlayerIndex)
			{
				TargetController = C;
				break;
			}
		}
	}

	if (TargetController)
	{
		TargetController->Possess(NewTank);

		// 【修复：唤醒 AI 大脑】
		if (AAIBotPlayerController* AIPC = Cast<AAIBotPlayerController>(TargetController))
		{
			AIPC->ControlledTank = NewTank;
			AIPC->ResetAIState();  // 重置追击状态 + 战斗状态
		}
	}

	// 【修复：复活后 IsAlive 必须为 true，否则 AI Tick 中的战斗逻辑会被 IsAlive 检查阻断】
	NewTank->IsAlive = true;

	// 销毁旧 Tank（释放内存，不再保留隐藏的尸体）
	if (OldTank && OldTank != NewTank)
	{
		OldTank->Destroy();
	}

	// 2. 设置复活后的生命值和弹药
	if (NewTank->HealthComp)
	{
		NewTank->HealthComp->CurrentHealth = NewTank->HealthComp->MaxHealth * RespawnHealthPercent;
		NewTank->HealthComp->UpdateHUD();
	}
	NewTank->CurrentAmmo = NewTank->MaxAmmo * RespawnAmmoPercent;

	// 使用复活弹药比例，并从 PlayerState 读取已保存的弹药
	int32 RespawnAmmo = FMath::FloorToInt(NewTank->MaxAmmo * RespawnAmmoPercent);
	// 通过 PlayerIndex 找到 PlayerState
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		AController* C = It->Get();
		if (ATeamBattlePlayerState* PS = C->GetPlayerState<ATeamBattlePlayerState>())
		{
			if (PS->PlayerIndex == PlayerIndex)
			{
				int32 SavedAmmo = PS->CurrentAmmo;
				if (SavedAmmo > RespawnAmmo)
				{
					RespawnAmmo = SavedAmmo;
				}
				PS->SetAlive(true);
				break;
			}
		}
	}
	NewTank->CurrentAmmo = RespawnAmmo;
	NewTank->SetAmmo(RespawnAmmo);
	NewTank->IsAlive = true;
	NewTank->SetIsAlive(true);

	// 更新数组引用
	if (ActiveTanks.IsValidIndex(PlayerIndex))
	{
		ActiveTanks[PlayerIndex] = NewTank;
	}

	// 3. 设置玩家索引（用于阵营判断）
	NewTank->SetPlayerIndex(PlayerIndex);

	// 4. 重新绑定 Tank 死亡事件（确保复活后依然触发 GameMode 逻辑）
	NewTank->OnKilled.AddDynamic(this, &ATeamBattleGameMode::HandleTankKilled);

	// 5. 复活时恢复死亡前保存的 Buff
	if (PlayerSavedBuffs.IsValidIndex(PlayerIndex) && PlayerSavedBuffs[PlayerIndex].Num() > 0)
	{
		UTankBuffComponent* NewBuffComp = NewTank->FindComponentByClass<UTankBuffComponent>();
		if (NewBuffComp)
		{
			NewBuffComp->RestoreBuffs(PlayerSavedBuffs[PlayerIndex]);
			UE_LOG(LogTemp, Display, TEXT("Player %d restored %d buffs on respawn."), PlayerIndex, PlayerSavedBuffs[PlayerIndex].Num());
		}
	}

	// 6. 无敌与复活特效
	NewTank->SetCanBeDamaged(false);

	UNiagaraComponent* VFXComp = nullptr;
	if (RespawnNiagaraVFX)
	{
		VFXComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
			RespawnNiagaraVFX,
			NewTank->GetRootComponent(),
			NAME_None,
			FVector(0, 0, -50),
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true
		);
	}

	FTimerHandle InvincibleTimer;
	FTimerDelegate InvincibleDel;
	InvincibleDel.BindUObject(this, &ATeamBattleGameMode::EndInvincibility, NewTank, VFXComp);

	GetWorldTimerManager().SetTimer(InvincibleTimer, InvincibleDel, InvincibleTime, false);
}

void ATeamBattleGameMode::EndInvincibility(ATank* Tank, UNiagaraComponent* VFXComp)
{
	// 1. 恢复受伤 (取消无敌)
	if (Tank && IsValid(Tank))
	{
		Tank->SetCanBeDamaged(true);
	}

	// 如果Niagara特效是循环的，需要手动停止
	if (VFXComp && VFXComp->IsActive())
	{
		VFXComp->Deactivate();
	}
}

void ATeamBattleGameMode::UpdateTeamScoresDisplay()
{
	if (ScoresWidgetInstance)
	{
		// 团队模式显示红色vs蓝色阵营分数
		ScoresWidgetInstance->UpdateTeamScores(TeamScores[0], TeamScores[1]);
	}
}

void ATeamBattleGameMode::ShowTeamBattleGameOver()
{
	// 游戏结束时弹出团队战斗结算界面
	if (WinnerCampIndex < 0 || !TeamBattleGameOverWidgetClass)
	{
		return;
	}

	APlayerController* PC0 = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC0)
	{
		return;
	}

	TeamBattleGameOverWidgetInstance = CreateWidget<UTeamBattleGameOverWidget>(PC0, TeamBattleGameOverWidgetClass);
	if (!TeamBattleGameOverWidgetInstance)
	{
		return;
	}

	// 只传获胜阵营索引，Widget 内部自己从 PlayerState 读 KDA / 从 GameState 读阵营分数 / 计算 SkillScore
	TeamBattleGameOverWidgetInstance->InitResultData(WinnerCampIndex);

	TeamBattleGameOverWidgetInstance->AddToViewport(1000);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(TeamBattleGameOverWidgetInstance->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC0->SetInputMode(InputMode);
	PC0->bShowMouseCursor = true;
}

void ATeamBattleGameMode::OnGameOverTimerTimeOut()
{
	UE_LOG(LogTemp, Warning, TEXT("=== TEAM GAME OVER ==="));
}

bool ATeamBattleGameMode::CanDealDamage(AController* DamageCauser, AActor* DamageVictim) const
{
	if (!DamageCauser || !DamageVictim) return true;

	// 尝试从 DamageCauser 获取攻击者的 Tank
	ATank* AttackerTank = Cast<ATank>(DamageCauser->GetPawn());
	if (!AttackerTank) return true;

	// 尝试从 DamageVictim 获取受害者的 Tank
	ATank* VictimTank = Cast<ATank>(DamageVictim);
	if (!VictimTank) return true;

	// 禁止同阵营互相伤害
	if (IsSameCamp(AttackerTank->GetPlayerIndex(), VictimTank->GetPlayerIndex()))
	{
		return false;
	}

	return true;
}
