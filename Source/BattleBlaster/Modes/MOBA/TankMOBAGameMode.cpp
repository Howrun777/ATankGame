// Fill out your copyright notice in the Description page of Project Settings.

#include "Modes/MOBA/TankMOBAGameMode.h"
#include "Modes/MOBA/TankMOBAGameState.h"
#include "Modes/MOBA/TankMOBAPlayerState.h"
#include "Shared/Pawns/Tank.h"
#include "Shared/State/TankPlayerState.h"
#include "Shared/Controllers/TankPlayerController.h"
#include "Shared/AI/AIBotPlayerController.h"
#include "Modes/MOBA/Turret.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Core/BattleBlasterGameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Modes/MOBA/UI/MOBATopStateUI.h"
#include "Modes/MOBA/UI/MOBAGameOverWidget.h"

ATankMOBAGameMode::ATankMOBAGameMode()
{
	PlayerCount = 0;
	MatchTime = 0.0f;

	// 禁止生成默认角色,所有角色生成逻辑由程序员严格控制
	DefaultPawnClass = nullptr;

	// 设置专用的 GameState 和 PlayerState 类
	GameStateClass = ATankMOBAGameState::StaticClass();
	PlayerStateClass = ATankMOBAPlayerState::StaticClass();

	// 设置 PlayerController 类（用于创建 HUD 等 UI）
	PlayerControllerClass = ATankPlayerController::StaticClass();

	// 默认值
	InitialRespawnDelay = 2.0f;
	MaxRespawnDelay = 10.0f;
	RespawnDelayGrowthInterval = 30.0f;  // 每30秒
	RespawnDelayGrowthAmount = 1.0f;     // 增长1秒
	RespawnEffectHeight = 0.0f;
	RespawnHealthPercent = 0.5f;  // 复活后50%血量
	RespawnAmmoPercent = 0.5f;   // 复活后50%弹药
	TowerDamage = 25.0f;
	TowerHealPercent = 0.5f;

	// 使用 Tick
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
}

void ATankMOBAGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 游戏阶段：允许分屏
	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		Viewport->SetForceDisableSplitscreen(false);
	}

	// 获取游戏状态
	MOBAGameState = GetGameState<ATankMOBAGameState>();

	// 1) 从GameInstance获取玩家数量
	UBattleBlasterGameInstance* GameInst = Cast<UBattleBlasterGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (GameInst)
	{
		TargetPlayerCount = FMath::Clamp(GameInst->TargetPlayerCount, 2, 4);
		ConnectedGamepadCount = FMath::Clamp(GameInst->ConnectedGamepadCount, 1, 4);
		PlayerCount = TargetPlayerCount;

		UE_LOG(LogTemp, Display, TEXT("MOBA GameMode: TargetPlayerCount = %d, ConnectedGamepadCount = %d"), TargetPlayerCount, ConnectedGamepadCount);
	}
	else
	{
		TargetPlayerCount = 2;
		ConnectedGamepadCount = 1;
		PlayerCount = 2;
	}

	// 2) 分屏机制：只根据"真实玩家（手柄）数量"创建视口；AI 不创建视口
	const int32 HumanPlayerCount = FMath::Clamp(FMath::Min(ConnectedGamepadCount, TargetPlayerCount), 1, 4);
	ViewportPlayerCount = (HumanPlayerCount == 3) ? 4 : HumanPlayerCount;
	ConnectedGamepadCount = HumanPlayerCount;

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

	// 4) 计算AI控制信息
	bIsPlayerAIControlled.SetNum(TargetPlayerCount);
	for (int32 i = 0; i < TargetPlayerCount; i++)
	{
		bIsPlayerAIControlled[i] = (i >= ConnectedGamepadCount);
		UE_LOG(LogTemp, Display, TEXT("MOBA Player %d: AIControlled=%s"), i,
			bIsPlayerAIControlled[i] ? TEXT("True") : TEXT("False"));
	}

	// 5) 初始化数组大小
	PlayerStarts.SetNum(TargetPlayerCount);
	ActiveTanks.SetNum(TargetPlayerCount);

	// 6) 查找并排序所有 PlayerStart
	TArray<AActor*> FoundStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundStarts);

	for (AActor* Start : FoundStarts)
	{
		for (int32 i = 0; i < TargetPlayerCount; i++)
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

	// 7) 循环生成玩家（玩家 + AI 共用 Tank 蓝图）
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (int32 i = 0; i < TargetPlayerCount; i++)
	{
		// 检查出生点是否存在
		if (!PlayerStarts[i])
		{
			UE_LOG(LogTemp, Error, TEXT("MOBA: Missing PlayerStart with Tag P%d"), i);
			continue;
		}

		// 根据菜单选择 / 默认 TankClass 选出要生成的 Tank 蓝图
		TSubclassOf<ATank> TankClassToUse = TankClass;

		if (GameInst && GameInst->SelectedTankClasses.Num() > i)
		{
			if (GameInst->SelectedTankClasses[i])
			{
				TankClassToUse = GameInst->SelectedTankClasses[i];
			}
		}

		if (!TankClassToUse)
		{
			UE_LOG(LogTemp, Warning, TEXT("MOBA: TankClass is null for player %d, using default ATank"), i);
			TankClassToUse = ATank::StaticClass();
		}

		// 生成 Tank
		FVector SpawnLocation = PlayerStarts[i]->GetActorLocation();
		FRotator SpawnRotation = PlayerStarts[i]->GetActorRotation();

		ATank* NewTank = GetWorld()->SpawnActor<ATank>(TankClassToUse, SpawnLocation, SpawnRotation, SpawnParams);

		if (NewTank)
		{
			ActiveTanks[i] = NewTank;

			// 设置玩家索引（通过 SetPlayerIndex 同步到 PlayerState）
			NewTank->SetPlayerIndex(i);

			UE_LOG(LogTemp, Display, TEXT("MOBA: Spawned Tank %d at location %s"), i, *SpawnLocation.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("MOBA: Failed to spawn Tank for player %d"), i);
		}
	}

	// 8) 等待玩家控制器生成完成后，绑定 Pawn
	// 通过延迟几帧后手动设置每个 PlayerController 的 Pawn
	GetWorld()->GetTimerManager().SetTimer(BindPawnTimerHandle, [WeakThis = TWeakObjectPtr<ATankMOBAGameMode>(this)]()
	{
		ATankMOBAGameMode* GameMode = WeakThis.Get();
		if (!GameMode || !GameMode->GetWorld())
		{
			return;
		}

		for (int32 i = 0; i < GameMode->TargetPlayerCount; i++)
		{
			if (!GameMode->ActiveTanks.IsValidIndex(i) || !GameMode->ActiveTanks[i]) continue;

			// 判断这个槽位是否需要 AI 控制
			const bool bIsAI = GameMode->bIsPlayerAIControlled.IsValidIndex(i) && GameMode->bIsPlayerAIControlled[i];

			if (bIsAI)
			{
				// AI 槽位：创建 AIBotPlayerController 并占有坦克
				FActorSpawnParameters AIControllerSpawnParams;
				AIControllerSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

				AAIBotPlayerController* AIPC = GameMode->GetWorld()->SpawnActor<AAIBotPlayerController>(
					AAIBotPlayerController::StaticClass(),
					FVector::ZeroVector,
					FRotator::ZeroRotator,
					AIControllerSpawnParams
				);

				if (AIPC)
				{
					AIPC->Possess(GameMode->ActiveTanks[i]);

					// 【核心修复】：AIBotPlayerController 已开启 bWantsPlayerState=true，引擎已自动生成 PlayerState！
					// 我们只需提取它并初始化阵营信息，不要手动 Spawn！
					if (ATankMOBAPlayerState* AIPlayerState = AIPC->GetPlayerState<ATankMOBAPlayerState>())
					{
						AIPlayerState->InitializeMOBAState(i);
						AIPlayerState->SetPlayerName(FString::Printf(TEXT("AI_P%d"), i));
					}

					UE_LOG(LogTemp, Display, TEXT("MOBA: Slot %d controlled by AI"), i);
				}
			}
			else
			{
				// 人类玩家槽位：获取对应的 PlayerController 并 Possess
				APlayerController* PC = UGameplayStatics::GetPlayerController(GameMode->GetWorld(), i);
				if (!PC) continue;

				// 如果是本地控制器，绑定 Pawn 并初始化 UI
				if (PC->IsLocalController())
				{
					PC->Possess(GameMode->ActiveTanks[i]);
					UE_LOG(LogTemp, Display, TEXT("MOBA: PlayerController %d possessed Tank %d"), i, i);

					// 【核心修复】：在 Possess 完成后立即初始化 MOBAState
					// 不再依赖 HandleStartingNewPlayer（它调用时 GetPawn() 可能还是 nullptr）
					if (ATankMOBAPlayerState* MOBAState = PC->GetPlayerState<ATankMOBAPlayerState>())
					{
						MOBAState->InitializeMOBAState(GameMode->ActiveTanks[i]->GetPlayerIndex());
						UE_LOG(LogTemp, Display, TEXT("MOBA: InitializeMOBAState for player %d, PlayerIndex=%d"),
							i, GameMode->ActiveTanks[i]->GetPlayerIndex());
					}

					// 延迟一点时间后初始化 UI，确保 Possess 完成
					FTimerHandle InitHUDTimer;
					if (ATankPlayerController* TankPC = Cast<ATankPlayerController>(PC))
					{
						GameMode->GetWorld()->GetTimerManager().SetTimer(InitHUDTimer, TankPC, &ATankPlayerController::InitializeHUD, 0.1f, false);
					}
				}
			}

			// === 绑定 Tank 死亡事件（GameMode 监听此委托处理复活/胜负判定） ===
			GameMode->ActiveTanks[i]->OnKilled.AddDynamic(GameMode, &ATankMOBAGameMode::HandleTankKilled);
		}
	}, 0.5f, false);

	// 9) 开始游戏时间计时
	GetWorld()->GetTimerManager().SetTimer(
		GameTimerHandle,
		this,
		&ATankMOBAGameMode::UpdateGameTimer,
		1.0f,
		true
	);

	// 10) 创建 MOBA 顶部状态 UI
	if (TopStateUIClass && !TopStateUIInstance)
	{
		TopStateUIInstance = CreateWidget<UMOBATopStateUI>(GetWorld(), TopStateUIClass);
		if (TopStateUIInstance)
		{
			TopStateUIInstance->AddToViewport();
		}
	}
}

void ATankMOBAGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 更新复活倒计时
	UpdateRespawnTimers(DeltaTime);

	if (MOBAGameState && MOBAGameState->IsGameOver() && !bMOBAGameOverUIScheduled && MOBAGameOverWidgetClass)
	{
		bMOBAGameOverUIScheduled = true;
		GetWorldTimerManager().SetTimer(MOBAGameOverTimerHandle, this, &ATankMOBAGameMode::ShowMOBAGameOver, GameOverDelay, false);
	}
}

void ATankMOBAGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(BindPawnTimerHandle);
	GetWorldTimerManager().ClearTimer(MOBAGameOverTimerHandle);

	// 清理 TopStateUI
	if (TopStateUIInstance)
	{
		TopStateUIInstance->RemoveFromParent();
		TopStateUIInstance = nullptr;
	}

	if (IsValid(MOBAGameOverWidgetInstance))
	{
		MOBAGameOverWidgetInstance->RemoveFromParent();
		MOBAGameOverWidgetInstance = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ATankMOBAGameMode::HideCoreTurretImage(int32 CampIndex)
{
	if (TopStateUIInstance)
	{
		TopStateUIInstance->HideTurretImage(CampIndex);
	}
}

void ATankMOBAGameMode::UpdateGameTimer()
{
	MatchTime += 1.0f;
	
	if (MOBAGameState)
	{
		MOBAGameState->MatchTimeSeconds = FMath::FloorToInt(MatchTime);
	}
}

void ATankMOBAGameMode::CheckGameOver()
{
	if (!MOBAGameState || MOBAGameState->IsGameOver())
	{
		return;
	}

	// 【修改后的游戏结束判定逻辑】
	// 条件1：场上只剩1个核心塔
	// 条件2：无核心塔的阵营，所有玩家都已被淘汰（不是死亡状态）

	// 条件1：检查是否只剩一个核心塔存活
	int32 TotalCoreTurretCount = MOBAGameState->GetAliveCoreTurretCount();
	if (TotalCoreTurretCount != 1)
	{
		return; // 不满足条件，不结束游戏
	}

	// 获取唯一存活核心塔的阵营索引
	int32 WinnerCampIndex = MOBAGameState->GetAliveCampIndex();
	if (WinnerCampIndex < 0)
	{
		return; // 没有存活阵营
	}

	// 条件2：检查除了获胜阵营外，其他所有玩家是否都已被淘汰
	bool bAllOtherPlayersEliminated = true;

	for (APlayerState* PS : GameState->PlayerArray)
	{
		ATankMOBAPlayerState* MOBAState = Cast<ATankMOBAPlayerState>(PS);
		if (!MOBAState)
		{
			continue;
		}

		int32 PlayerCampIndex = MOBAState->GetCampIndex();

		// 跳过获胜阵营的玩家
		if (PlayerCampIndex == WinnerCampIndex)
		{
			continue;
		}

		// 检查该玩家是否已被淘汰
		if (!MOBAState->IsEliminated())
		{
			bAllOtherPlayersEliminated = false;
			break;
		}
	}

	// 两个条件都满足，游戏结束
	if (bAllOtherPlayersEliminated)
	{
		MOBAGameState->SetGameOver(true);
		MOBAGameState->SetWinningCampIndex(WinnerCampIndex);
		MOBAGameState->GameStatus = EGameStatus::Ended;

		UE_LOG(LogTemp, Display, TEXT("MOBA Game Over! Winner Camp: %d"), WinnerCampIndex);
	}
}

void ATankMOBAGameMode::HandleStartingNewPlayer(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer(NewPlayer);

	if (!NewPlayer) return;

	// 【核心修复】：MOBAState 的初始化已移至 BeginPlay 中的 Possess 之后（更可靠）
	// 此函数不再需要延迟1秒去获取 GetPawn()
	// 这里只做安全检查：如果 MOBAState 未初始化（PlayerIndex == -1），则通过遍历 ActiveTanks 找到匹配的 Tank 并初始化

	if (ATankMOBAPlayerState* MOBAState = NewPlayer->GetPlayerState<ATankMOBAPlayerState>())
	{
		// 如果还没有初始化（PlayerIndex == -1），则尝试初始化
		if (MOBAState->PlayerIndex == -1)
		{
			// 通过遍历 ActiveTanks 找到对应的 Tank
			for (int32 i = 0; i < ActiveTanks.Num(); ++i)
			{
				if (ActiveTanks[i] && ActiveTanks[i]->Controller == NewPlayer)
				{
					MOBAState->InitializeMOBAState(ActiveTanks[i]->GetPlayerIndex());
					UE_LOG(LogTemp, Display, TEXT("MOBA: HandleStartingNewPlayer - InitializeMOBAState for Tank[%d], PlayerIndex=%d"),
						i, ActiveTanks[i]->GetPlayerIndex());
					break;
				}
			}
		}
	}
}

AActor* ATankMOBAGameMode::GetPlayerStartForIndex(int32 TargetPlayerIndex)
{
	// 拼接我们要找的标签，例如 "P0", "P1", "P2"
	FName SearchTag = FName(*FString::Printf(TEXT("P%d"), TargetPlayerIndex));

	// 1. 优先从我们缓存的 PlayerStarts 数组中找（效率最高）
	if (PlayerStarts.IsValidIndex(TargetPlayerIndex) && IsValid(PlayerStarts[TargetPlayerIndex]))
	{
		APlayerStart* PStart = Cast<APlayerStart>(PlayerStarts[TargetPlayerIndex]);
		if (PStart && PStart->PlayerStartTag == SearchTag)
		{
			return PStart;
		}
	}

	// 2. 如果缓存失效或不匹配，全图硬搜，严格比对 PlayerStartTag！
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		APlayerStart* PStart = Cast<APlayerStart>(Actor);
		if (PStart && PStart->PlayerStartTag == SearchTag)
		{
			// 顺手更新一下缓存，下次复活就不用全图搜了
			if (PlayerStarts.IsValidIndex(TargetPlayerIndex))
			{
				PlayerStarts[TargetPlayerIndex] = PStart;
			}
			return PStart;
		}
	}

	// 3. 致命错误：地图里根本没有打对 PlayerStartTag！
	UE_LOG(LogTemp, Error, TEXT("MOBA CRITICAL ERROR: Map is missing PlayerStart with PlayerStartTag [%s]! Player %d will spawn at map origin!"), *SearchTag.ToString(), TargetPlayerIndex);

	return nullptr; // 如果找不到，返回 nullptr 避免随便乱飞
}

void ATankMOBAGameMode::HandleTankKilled(ATank* DeadTank, ATank* KillerTank)
{
	if (!DeadTank) return;

	int32 VictimIndex = DeadTank->GetPlayerIndex();

	// === 保存死亡玩家的 Buff 信息到 PlayerState ===
	UTankBuffComponent* DeadBuffComp = DeadTank->FindComponentByClass<UTankBuffComponent>();
	if (DeadBuffComp)
	{
		TArray<FActiveBuffUIInfo> SavedBuffs = DeadBuffComp->GetAllActiveBuffs();
		// 通过 PlayerIndex 找到对应的 PlayerState 并保存
		for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
		{
			AController* C = It->Get();
			if (ATankMOBAPlayerState* PS = C->GetPlayerState<ATankMOBAPlayerState>())
			{
				if (PS->PlayerIndex == VictimIndex)
				{
					PS->SaveCurrentBuffs(SavedBuffs);
					break;
				}
			}
		}
	}

	// === 检查核心塔存活状态 ===
	bool bCoreAlive = true;
	if (MOBAGameState)
	{
		bCoreAlive = (MOBAGameState->GetAliveCoreTurretCountByCamp(VictimIndex) > 0);
	}

	// =========================================================================
	// 【核心修复 1】：坦克死亡时必定已经 Unpossess 解绑！不能用 DeadTank->GetController()
	// 必须遍历全局 Controller，通过永远不会变的 PlayerIndex 找回主人的灵魂 (包含AI和人类)
	// =========================================================================
	AController* DeadController = nullptr;
	ATankMOBAPlayerState* VictimMOBAState = nullptr;

	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		AController* C = It->Get();
		if (ATankMOBAPlayerState* PS = C->GetPlayerState<ATankMOBAPlayerState>())
		{
			if (PS->PlayerIndex == VictimIndex)
			{
				DeadController = C;
				VictimMOBAState = PS;
				break;
			}
		}
	}

	ATankPlayerController* DeadPC = Cast<ATankPlayerController>(DeadController);

	if (bCoreAlive)
	{
		// ====== 核心塔存活：进入统一复活流程（AI和真实玩家完美共用） ======
		if (VictimMOBAState)
		{
			VictimMOBAState->SetDead(true);
			float CurrentRespawnDelay = VictimMOBAState->CalculateRespawnDelay(
				MatchTime, InitialRespawnDelay, MaxRespawnDelay,
				RespawnDelayGrowthInterval, RespawnDelayGrowthAmount);

			VictimMOBAState->SetCurrentRespawnDelay(CurrentRespawnDelay);
			VictimMOBAState->SetWaitingForRespawn(true);
			VictimMOBAState->SetRespawnTimeRemaining(CurrentRespawnDelay);
		}

		// ====== 显示死亡倒计时 UI (只对本地真实玩家生效) ======
		if (DeadPC && DeadPC->IsLocalController())
		{
			float RespawnDelayToShow = VictimMOBAState ? VictimMOBAState->GetCurrentRespawnDelay() : InitialRespawnDelay;
			DeadPC->ShowDeathScreen(RespawnDelayToShow);
		}

		// 【新增】：核心塔存活时，玩家只是死亡等待复活，不触发游戏结束检查
	}
	else
	{
		// ====== 核心塔已摧毁：永久淘汰 ======
		if (VictimMOBAState)
		{
			VictimMOBAState->SetEliminated(true);
			VictimMOBAState->SetDead(true);
			VictimMOBAState->SetWaitingForRespawn(false);
			VictimMOBAState->SetRespawnTimeRemaining(0.0f);
		}

		if (DeadPC && DeadPC->IsLocalController())
		{
			DeadPC->HideDeathScreen();
			DeadPC->ShowEliminatedScreen();
		}

		NotifyAllPlayersCoreDestroyed(VictimIndex);

		// 【修改】：只有在玩家被淘汰时才检查游戏是否结束
		CheckGameOver();
	}
}

void ATankMOBAGameMode::StartPlayerRespawn(ATankMOBAPlayerState* MOBAState)
{
	if (!MOBAState)
	{
		return;
	}

	// 复活玩家
	RespawnPlayer(MOBAState);
}

void ATankMOBAGameMode::RespawnPlayer(ATankMOBAPlayerState* MOBAState)
{
	if (!MOBAState || MOBAState->IsEliminated()) return;
	if (!MOBAState->IsWaitingForRespawn()) return;

	// =========================================================================
	// 【核心修复：灵魂附体】
	// 玩家死亡后 Tank 被 HandleDestruction 隐藏，但我们必须 Spawn 新躯壳，
	// 再让原来持有击杀记录的老 Controller 附身新躯壳！
	// =========================================================================

	// 1. 通过 PlayerIndex 找到原来那个 Controller（不再用 Controller->GetPawn()）
	int32 CampIndex = MOBAState->PlayerIndex;
	if (CampIndex < 0 || CampIndex >= TargetPlayerCount) return;

	// 2. 获取 Tank 蓝图
	TSubclassOf<ATank> TankClassToUse = TankClass;
	if (UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(GetGameInstance()))
	{
		if (GI->SelectedTankClasses.IsValidIndex(CampIndex) && GI->SelectedTankClasses[CampIndex] != nullptr)
		{
			TankClassToUse = GI->SelectedTankClasses[CampIndex];
		}
	}
	if (!TankClassToUse) TankClassToUse = ATank::StaticClass();

	// 3. 获取出生点
	AActor* StartActor = GetPlayerStartForIndex(CampIndex);
	FVector SpawnLoc = FVector::ZeroVector;
	FRotator SpawnRot = FRotator::ZeroRotator;
	if (StartActor)
	{
		SpawnLoc = StartActor->GetActorLocation();
		SpawnRot = StartActor->GetActorRotation();
	}

	// 4. 保存旧 Tank 引用（用于复活后销毁）
	ATank* OldTank = ActiveTanks.IsValidIndex(CampIndex) ? ActiveTanks[CampIndex] : nullptr;

	// 5. 生成新坦克
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	ATank* NewTank = GetWorld()->SpawnActor<ATank>(TankClassToUse, SpawnLoc, SpawnRot, SpawnParams);
	if (!NewTank) return;

	// 6. 找到老 Controller 并附身新躯壳
	AController* TargetController = nullptr;
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		AController* C = It->Get();
		if (ATankMOBAPlayerState* PS = C->GetPlayerState<ATankMOBAPlayerState>())
		{
			if (PS->PlayerIndex == CampIndex)
			{
				TargetController = C;
				break;
			}
		}
	}

	if (TargetController)
	{
		TargetController->Possess(NewTank);

		// 唤醒 AI 大脑（如果是 AI 控制器）
		if (AAIBotPlayerController* AIPC = Cast<AAIBotPlayerController>(TargetController))
		{
			AIPC->ControlledTank = NewTank;
			AIPC->CurrentCombatState = EAICombatState::Idle;
			AIPC->CurrentTarget = nullptr;
		}
	}

	// 7. 销毁旧 Tank（释放内存，不再保留隐藏的尸体）
	if (OldTank && OldTank != NewTank)
	{
		OldTank->Destroy();
	}

	// 8. 更新 ActiveTanks 数组
	if (ActiveTanks.IsValidIndex(CampIndex))
	{
		ActiveTanks[CampIndex] = NewTank;
	}

	// 9. 设置玩家索引
	NewTank->SetPlayerIndex(CampIndex);

	// 10. 恢复状态
	UHealthComponent* HealthComp = NewTank->FindComponentByClass<UHealthComponent>();
	if (HealthComp)
	{
		HealthComp->CurrentHealth = HealthComp->MaxHealth * RespawnHealthPercent;
		HealthComp->UpdateHUD();
	}

	// 使用复活弹药比例，并从 PlayerState 读取已保存的弹药
	int32 RespawnAmmo = FMath::FloorToInt(NewTank->MaxAmmo * RespawnAmmoPercent);
	// 检查 PlayerState 中是否保存了更多弹药（因为可能在死前加过弹药）
	if (MOBAState)
	{
		int32 SavedAmmo = MOBAState->GetAmmo();
		if (SavedAmmo > RespawnAmmo)
		{
			RespawnAmmo = SavedAmmo;
		}
		// 更新 PlayerState 的存活状态
		MOBAState->SetAlive(true);
	}
	NewTank->CurrentAmmo = RespawnAmmo;
	NewTank->SetAmmo(RespawnAmmo);
	NewTank->IsAlive = true;
	NewTank->SetIsAlive(true);
	NewTank->SetActorHiddenInGame(false);
	NewTank->SetActorTickEnabled(true);
	NewTank->SetActorEnableCollision(true);
	NewTank->SetPlayerEnabled(true);

	// 9. 重新绑定死亡事件
	NewTank->OnKilled.AddDynamic(this, &ATankMOBAGameMode::HandleTankKilled);

	// 10. 恢复死亡前保存的 Buff（从 PlayerState 恢复）
	if (MOBAState)
	{
		const TArray<FActiveBuffUIInfo>& SavedBuffs = MOBAState->GetBuffs();
		if (SavedBuffs.Num() > 0)
		{
			if (UTankBuffComponent* NewBuffComp = NewTank->FindComponentByClass<UTankBuffComponent>())
			{
				NewBuffComp->RestoreBuffs(SavedBuffs);
				UE_LOG(LogTemp, Display, TEXT("[MOBA] Player %d restored %d buffs on respawn."), CampIndex, SavedBuffs.Num());
			}
		}
		// 清除已恢复的 Buff 记录
		MOBAState->ClearBuffs();
	}

	// 11. 更新玩家状态
	MOBAState->SetDead(false);
	MOBAState->SetWaitingForRespawn(false);
	MOBAState->SetRespawnTimeRemaining(0.0f);

	// 11. 复活特效
	if (RespawnEffect)
	{
		FVector VFXLoc = NewTank->GetActorLocation();
		VFXLoc.Z += RespawnEffectHeight;
		UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), RespawnEffect, VFXLoc);
		if (NiagaraComp && NewTank->GetRootComponent())
		{
			NiagaraComp->AttachToComponent(NewTank->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
		}
	}
	if (RespawnSound) UGameplayStatics::PlaySoundAtLocation(GetWorld(), RespawnSound, NewTank->GetActorLocation());

	// 12. 恢复屏幕 UI
	if (ATankPlayerController* TankPC = Cast<ATankPlayerController>(TargetController))
	{
		if (TankPC->IsLocalController())
		{
			TankPC->HideDeathScreen();
			TankPC->SetHUDAmmo(NewTank->CurrentAmmo, NewTank->MaxAmmo);
		}
	}
}

void ATankMOBAGameMode::EliminatePlayer(ATankMOBAPlayerState* MOBAState)
{
	if (!MOBAState)
	{
		return;
	}

	MOBAState->SetEliminated(true);
	MOBAState->SetDead(true);
	MOBAState->SetWaitingForRespawn(false);
	MOBAState->SetRespawnTimeRemaining(0.0f);

	// 通知玩家已被淘汰（显示永久灰色UI）
	// TODO: 调用UI显示
}

void ATankMOBAGameMode::UpdateRespawnTimers(float DeltaTime)
{
	// 遍历所有玩家状态
	for (APlayerState* PS : GameState->PlayerArray)
	{
		ATankMOBAPlayerState* MOBAState = Cast<ATankMOBAPlayerState>(PS);
		if (!MOBAState || !MOBAState->IsWaitingForRespawn())
		{
			continue;
		}

		// 更新倒计时
		float Remaining = MOBAState->GetRespawnTimeRemaining();
		Remaining -= DeltaTime;

		if (Remaining <= 0.0f)
		{
			// 倒计时结束，复活玩家
			RespawnPlayer(MOBAState);
		}
		else
		{
			MOBAState->SetRespawnTimeRemaining(Remaining);

			// =========================================================================
			// 【核心修复 2】：玩家死亡阶段是没有 Pawn 的，MOBAState->GetPawn() 会返回空！
			// PlayerState 的 Owner 永远是对应的 Controller，必须这样获取才能更新UI！
			// =========================================================================
			if (ATankPlayerController* TankPC = Cast<ATankPlayerController>(MOBAState->GetOwner()))
			{
				if (TankPC->IsLocalController())
				{
					TankPC->UpdateDeathScreenCountdown(Remaining);
				}
			}
		}
	}
}

void ATankMOBAGameMode::NotifyAllPlayersTowerDestroyed(int32 CampIndex, bool bIsCoreTurret)
{
	// 遍历所有玩家，通知他们防御塔被摧毁
	for (APlayerState* PS : GameState->PlayerArray)
	{
		ATankMOBAPlayerState* MOBAState = Cast<ATankMOBAPlayerState>(PS);
		if (!MOBAState)
		{
			continue;
		}

		// 如果是同阵营，增加摧毁计数
		if (MOBAState->GetCampIndex() == CampIndex)
		{
			MOBAState->AddTurretDestroyed();
		}
	}
}

void ATankMOBAGameMode::NotifyAllPlayersCoreDestroyed(int32 CampIndex)
{
	// 遍历所有玩家，通知他们某阵营主防御塔被摧毁
	for (APlayerState* PS : GameState->PlayerArray)
	{
		ATankMOBAPlayerState* MOBAState = Cast<ATankMOBAPlayerState>(PS);
		if (!MOBAState)
		{
			continue;
		}

		// 如果是同阵营，玩家被淘汰
		if (MOBAState->GetCampIndex() == CampIndex)
		{
			MOBAState->SetEliminated(true);
		}
	}

	// 检查是否有更多玩家被淘汰
	CheckAllPlayersEliminated();
}

void ATankMOBAGameMode::CheckAllPlayersEliminated()
{
	if (!MOBAGameState)
	{
		return;
	}

	// 检查每个阵营
	for (int32 CampIndex = 0; CampIndex < 4; CampIndex++)
	{
		int32 CoreCount = MOBAGameState->GetAliveCoreTurretCountByCamp(CampIndex);
		int32 OuterCount = MOBAGameState->GetAliveOuterTurretCountByCamp(CampIndex);

		// 如果主防御塔已摧毁
		if (CoreCount == 0)
		{
			// 标记该阵营所有玩家为被淘汰
			for (APlayerState* PS : GameState->PlayerArray)
			{
				ATankMOBAPlayerState* MOBAState = Cast<ATankMOBAPlayerState>(PS);
				if (MOBAState && MOBAState->GetCampIndex() == CampIndex && !MOBAState->IsEliminated())
				{
					MOBAState->SetEliminated(true);
				}
			}
		}
	}
}

void ATankMOBAGameMode::ShowMOBAGameOver()
{
	if (!MOBAGameState || !MOBAGameState->IsGameOver() || !MOBAGameOverWidgetClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PC0 = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC0)
	{
		return;
	}

	if (TopStateUIInstance)
	{
		TopStateUIInstance->RemoveFromParent();
		TopStateUIInstance = nullptr;
	}

	MOBAGameOverWidgetInstance = CreateWidget<UMOBAGameOverWidget>(PC0, MOBAGameOverWidgetClass);
	if (!MOBAGameOverWidgetInstance)
	{
		return;
	}

	MOBAGameOverWidgetInstance->InitResultData();
	MOBAGameOverWidgetInstance->AddToViewport(1000);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MOBAGameOverWidgetInstance->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC0->SetInputMode(InputMode);
	PC0->bShowMouseCursor = true;
}
