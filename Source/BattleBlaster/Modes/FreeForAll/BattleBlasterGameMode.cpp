#include "Modes/FreeForAll/BattleBlasterGameMode.h"





// 引入UE内置的游戏玩法静态工具类（提供Actor查找、玩家获取等通用功能）
#include "Kismet/GameplayStatics.h"
// 引入塔楼Actor的头文件（用于识别场景中的塔楼类）
#include "Particles/ParticleSystemComponent.h" // 【新增】粒子组件头文件
#include "Core/BattleBlasterGameInstance.h"
#include "Shared/Pawns/NPC/Tower.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/GameViewportClient.h"
//#include "BattleBlasterGameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Shared/AI/AIBotPlayerController.h"
#include "Shared/Controllers/TankPlayerController.h"
#include "Shared/State/TankPlayerState.h"

ABattleBlasterGameMode::ABattleBlasterGameMode()
{
	// 设置专用的 GameState 和 PlayerState 类
	GameStateClass = ATankBattleGameState::StaticClass();
	PlayerStateClass = ATankBattlePlayerState::StaticClass();

	WinnerIndex = -1; // 【极其重要】：确保开局时没有赢家！
	//禁止生成默认角色,所有角色生成逻辑由程序员严格控制
	DefaultPawnClass = nullptr;
}

ATankBattleGameState* ABattleBlasterGameMode::GetTankBattleGameState() const
{
	if (!GameState)
	{
		return nullptr;
	}
	return Cast<ATankBattleGameState>(GameState);
}


void ABattleBlasterGameMode::BeginPlay()
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
		TargetPlayerCount = FMath::Clamp(GI->TargetPlayerCount, 2, 4);

		// 读取实际连接的手柄数量（菜单页保存过）
		ConnectedGamepadCount = FMath::Clamp(GI->ConnectedGamepadCount, 1, 4);
	}
	else
	{
		TargetPlayerCount = 2;
		TargetScore = 7;
		ConnectedGamepadCount = 1;
	}

	// 2) 分屏机制：只根据“真实玩家（手柄）数量”创建视口；AI 不创建视口
	// - 视口数 = min(已连接手柄数, 选择人数)；三人时使用四宫格，第四块用 WBP_Blackout 遮挡
	const int32 HumanPlayerCount = FMath::Clamp(FMath::Min(ConnectedGamepadCount, TargetPlayerCount), 1, 4);
	ViewportPlayerCount = (HumanPlayerCount == 3) ? 4 : HumanPlayerCount;
	ConnectedGamepadCount = HumanPlayerCount; // 统一口径：前 N 个玩家为真人，其余为AI

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
	// 根据手柄数量和目标玩家数量，确定哪些玩家需要AI控制
	bIsPlayerAIControlled.SetNum(TargetPlayerCount);
	for (int32 i = 0; i < TargetPlayerCount; i++)
	{
		// 如果玩家索引 >= 手柄数量，则该玩家需要AI控制
		bIsPlayerAIControlled[i] = (i >= ConnectedGamepadCount);
		UE_LOG(LogTemp, Display, TEXT("Player %d: AIControlled=%s"), i,
			bIsPlayerAIControlled[i] ? TEXT("True") : TEXT("False"));
	}

	// 初始化 GameState 中的玩家数据
	if (ATankBattleGameState* BBGameState = GetTankBattleGameState())
	{
		BBGameState->InitializePlayerData(TargetPlayerCount, TargetScore);
	}

	// 【核心修改 1】：初始化所有数组的大小，并给分数清零！
	PlayerStarts.SetNum(TargetPlayerCount);
	ActiveTanks.SetNum(TargetPlayerCount);
	PlayerSavedBuffs.SetNum(TargetPlayerCount);

	// 2. 查找并排序所有 PlayerStart
	TArray<AActor*> FoundStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundStarts);

	for (AActor* Start : FoundStarts)
	{
		for (int32 i = 0; i < TargetPlayerCount; i++)
		{
			FString TagName = FString::Printf(TEXT("P%d"), i);
			// 将 AActor 转换为 APlayerStart
			APlayerStart* PStart = Cast<APlayerStart>(Start);
			// 检查 PlayerStartTag 属性是否匹配
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

	for (int32 i = 0; i < TargetPlayerCount; i++)
	{
		if (!PlayerStarts[i]) continue;

		TSubclassOf<ATank> TankClassToUse = TankClass;
		if (UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(GetGameInstance()))
		{
			if (GI->SelectedTankClasses.IsValidIndex(i) && GI->SelectedTankClasses[i] != nullptr)
			{
				TankClassToUse = GI->SelectedTankClasses[i];
			}
		}

		if (!TankClassToUse) continue;

		ATank* NewTank = GetWorld()->SpawnActor<ATank>(
			TankClassToUse,
			PlayerStarts[i]->GetActorLocation(),
			PlayerStarts[i]->GetActorRotation(),
			SpawnParams
		);
		if (!NewTank) continue;

		const bool bIsAI = bIsPlayerAIControlled.IsValidIndex(i) && bIsPlayerAIControlled[i];

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

				// 【核心修复 1】：绝不能手动 Spawn PlayerState！
				// 引擎已经自动生成了，我们只需要提取出来并改写编号即可！
				if (ATankBattlePlayerState* AIPlayerState = AIPC->GetPlayerState<ATankBattlePlayerState>())
				{
					AIPlayerState->PlayerIndex = i;
					AIPlayerState->SetPlayerName(FString::Printf(TEXT("AI_P%d"), i));
				}
			}
		}
		else
		{
			APlayerController* PC = nullptr;
			if (i == 0) PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
			else
			{
				if (UGameplayStatics::GetNumLocalPlayerControllers(GetWorld()) <= i)
					PC = UGameplayStatics::CreatePlayer(GetWorld(), -1, true);
				else
					PC = UGameplayStatics::GetPlayerController(GetWorld(), i);
			}

			if (PC)
			{
				PC->Possess(NewTank);

				// 【核心修复 2】：确保真人玩家拿到属于自己的槽位编号！
				if (ATankBattlePlayerState* HumanPS = PC->GetPlayerState<ATankBattlePlayerState>())
				{
					HumanPS->PlayerIndex = i;
				}
			}
		}

		NewTank->SetPlayerEnabled(false);
		NewTank->SetPlayerIndex(i);
		ActiveTanks[i] = NewTank;
		NewTank->OnKilled.AddDynamic(this, &ABattleBlasterGameMode::HandleTankKilled);
	}

	// 3.1 为“额外的视口玩家”（只在三人模式下的第 4 个 LocalPlayer）设置纯黑画面
	// 注意：此时 PlayerController 以及其 CameraManager 可能尚未完全初始化，
	// 因此这里使用一个短延迟的定时器来应用黑屏效果，避免初始化顺序问题。
	GetWorldTimerManager().SetTimer(
		ExtraViewportBlackTimerHandle,
		this,
		&ABattleBlasterGameMode::ApplyBlackScreenToExtraViewports,
		0.2f,   // 延迟 0.2 秒
		false
	);

	//UI相关的逻辑
	// 1. 创建旧的 ScreenMessage (用于显示 GO 和 Victory)
	APlayerController* PC0 = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC0 && ScreenMessageClass)
	{
		ScreenMessageWidget = CreateWidget<UScreenMessage>(PC0, ScreenMessageClass);
		if (ScreenMessageWidget)
		{
			ScreenMessageWidget->AddToViewport(1000); // 最高层级
			ScreenMessageWidget->SetMessageText("Get Ready!");
		}
	}
	// 2. 创建新的 ScoreBoard (用于显示 比分 和 时间)
	if (PC0 && ScoresWidgetClass)
	{
		ScoresWidgetInstance = CreateWidget<UScoresDisplayWidget>(PC0, ScoresWidgetClass);
		if (ScoresWidgetInstance)
		{
			//优先级不能比暂停菜单高
			ScoresWidgetInstance->AddToViewport(10);
			// 初始化 UI 显示：2 人显示一行分数，3–4 人显示两行（四格）
			ScoresWidgetInstance->InitTargetScore(TargetScore);
			ScoresWidgetInstance->SetVisiblePlayerCount(TargetPlayerCount);
			if (TargetPlayerCount > 2)
			{
				ScoresWidgetInstance->UpdateScoresFour(0, 0, 0, 0);
			}
			else
			{
				ScoresWidgetInstance->UpdateScores(0, 0);
			}
			ScoresWidgetInstance->UpdateMatchTimer(0);
		}
	}
	// 初始化倒计时数值：将倒计时初始值（CountdownDelay）赋值给当前倒计时秒数（CountdownSeconds）
	// 比如CountdownDelay是10，就表示从10开始倒计时
	CountdownSeconds = CountdownDelay;
	GetWorldTimerManager().SetTimer(
		CountdownTimerHandle,          // 【参数1】定时器句柄：唯一标识这个倒计时定时器，用于后续停止/清理定时器
		this,                          // 【参数2】回调函数所属对象：指向当前ABattleBlasterGameMode实例本身
		&ABattleBlasterGameMode::OnCountdownTimerTimeout, // 【参数3】定时器触发的回调函数：每次定时器到时间就执行这个函数
		1.0f,                          // 【参数4】触发间隔：1.0秒（每1秒执行一次回调函数）
		true                           // 【参数5】是否循环：true表示循环触发，直到手动调用ClearTimer停止
	);

}

// -------------------------------------------------------------------------
// 计时器相关逻辑
// -------------------------------------------------------------------------

/**
 * @brief 倒计时定时器超时后的回调函数
 * @note 该函数属于ABattleBlasterGameMode游戏模式类，每一次倒计时定时器触发时会执行此函数
 *       核心逻辑：递减倒计时秒数 → 根据秒数状态更新UI/启用玩家/清除定时器
 */
void ABattleBlasterGameMode::OnCountdownTimerTimeout()
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

		// 启用所有玩家输入（AI坦克不受影响，因为它们的Controller不是PlayerController）
		for (ATank* Tank : ActiveTanks)
		{
			if (Tank) Tank->SetPlayerEnabled(true);
		}

		// 开始比赛计时
		GetWorldTimerManager().SetTimer(
			MatchTimerHandle,
			this,
			&ABattleBlasterGameMode::UpdateMatchTime,
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

void ABattleBlasterGameMode::UpdateMatchTime()
{
	MatchTimeSeconds++;
	if (ScoresWidgetInstance)
	{
		ScoresWidgetInstance->UpdateMatchTimer(MatchTimeSeconds);
	}
}

// ------------------------------------------------------------
// 将多余视口（例如三人模式下的第4块）渲染为纯黑
// ------------------------------------------------------------
void ABattleBlasterGameMode::ApplyBlackScreenToExtraViewports()
{
	APlayerController* PC0 = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	// 遍历所有当前正在运行的玩家控制器
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PC = Iterator->Get();

		// 【终极判断】：只要这个控制器是本地的，并且它【没有控制任何坦克】
		// 那么它绝对就是一个多出来的空闲幽灵屏幕！立刻遮挡！
		if (PC && PC->IsLocalController() && PC->GetPawn() == nullptr)
		{
			// ==========================================
			// 杀招 1：让它的底层摄像机直接致盲（纯黑）
			// ==========================================
			/*if (PC->PlayerCameraManager)
			{
				PC->PlayerCameraManager->StartCameraFade(1.0f, 1.0f, 99999.0f, FLinearColor::Black, false, true);
			}*/

		// ==========================================
		// 杀招 2：把你做好的那块右下角黑布贴上去
		// ==========================================
		if (PC0 && BlackoutWidgetClass)
		{
			UUserWidget* MasterBlackout = CreateWidget<UUserWidget>(PC0, BlackoutWidgetClass);
			if (MasterBlackout)
			{
				// AddToViewport 会配合你 UI 里 0.5~1.0 的锚点，完美贴在右下角
				MasterBlackout->AddToViewport(1);
				BlackoutWidgetInstances.Add(MasterBlackout);
				UE_LOG(LogTemp, Warning, TEXT("发现空闲屏幕！已贴上全屏遮挡黑布！"));
			}
		}

			// 废掉这个屏幕的所有按键输入
			PC->DisableInput(PC);
		}
	}
}
// ================== 内存泄漏终极修复 ==================
void ABattleBlasterGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// 1. 清理所有黑屏 UI
	for (UUserWidget* BlackoutWidget : BlackoutWidgetInstances)
	{
		if (IsValid(BlackoutWidget))
		{
			BlackoutWidget->RemoveFromParent();
		}
	}
	BlackoutWidgetInstances.Empty();

	// 2. 清理屏幕提示 UI (Get Ready / GO / Victory)
	if (IsValid(ScreenMessageWidget))
	{
		ScreenMessageWidget->RemoveFromParent();
		ScreenMessageWidget = nullptr;
	}

	// 3. 清理计分板 UI
	if (IsValid(ScoresWidgetInstance))
	{
		ScoresWidgetInstance->RemoveFromParent();
		ScoresWidgetInstance = nullptr;
	}

	// 4. 清理多人结算 UI
	if (IsValid(MultiBattleGameOverWidgetInstance))
	{
		MultiBattleGameOverWidgetInstance->RemoveFromParent();
		MultiBattleGameOverWidgetInstance = nullptr;
	}

	// 5. 清理所有定时器（防患于未然）
	GetWorldTimerManager().ClearAllTimersForObject(this);
}
// -------------------------------------------------------------------------
// 死亡与复活逻辑
// -------------------------------------------------------------------------
// KDA 全部已在 ATank::HandleDeath → ATankPlayerState::ProcessDeath 内部完成，
// 这里只处理：GameState 分数更新 / UI 刷新 / 复活计时 / 胜负判定
// 死亡事件由 ATank::OnKilled 委托触发（BeginPlay 中已绑定）
void ABattleBlasterGameMode::HandleTankKilled(ATank* DeadTank, ATank* KillerTank)
{
	if (WinnerIndex != -1) return;
	if (!DeadTank) return;

	ATankBattleGameState* BBGameState = GetTankBattleGameState();
	if (!BBGameState) return;

	// === 找到死者的索引 ===
	int32 VictimIndex = ActiveTanks.Find(DeadTank);
	if (VictimIndex == INDEX_NONE) return;

	// === 保存死亡玩家的 Buff 信息 ===
	UTankBuffComponent* DeadBuffComp = DeadTank->FindComponentByClass<UTankBuffComponent>();
	if (DeadBuffComp && PlayerSavedBuffs.IsValidIndex(VictimIndex))
	{
		PlayerSavedBuffs[VictimIndex] = DeadBuffComp->GetAllActiveBuffs();
	}

	// =========================================================
	// 注意：KDA 的计算已经完全在 PlayerState 中完成了！
	// GameMode 这里只计算 "比赛分数" (Score)，用于决定胜负
	// =========================================================

	int32 KillerIndex = ActiveTanks.Find(KillerTank);
	if (KillerIndex != INDEX_NONE && KillerIndex != VictimIndex)
	{
		// 正常击杀：给击杀者所在阵营/槽位加 1 分
		BBGameState->AddPlayerScore(KillerIndex);
		UE_LOG(LogTemp, Display, TEXT("[MatchScore] Player %d scored!"), KillerIndex);
	}
	else
	{
		// 无凶手（自杀/摔死/塔杀）：倒扣自己 1 分
		int32 CurrentScore = BBGameState->GetPlayerScore(VictimIndex);
		if (CurrentScore > 0)
		{
			BBGameState->AddPlayerScore(VictimIndex, -1);
		}
	}

	// === 更新顶部中央的比分板 UI ===
	if (ScoresWidgetInstance && BBGameState->PlayerScores.Num() >= 2)
	{
		if (TargetPlayerCount > 2)
		{
			ScoresWidgetInstance->UpdateScoresFour(
				BBGameState->GetPlayerScore(0),
				BBGameState->GetPlayerScore(1),
				BBGameState->GetPlayerScore(2),
				BBGameState->GetPlayerScore(3));
		}
		else
		{
			ScoresWidgetInstance->UpdateScores(
				BBGameState->GetPlayerScore(0),
				BBGameState->GetPlayerScore(1));
		}
	}

	// === 胜负判定 ===
	for (int32 i = 0; i < BBGameState->PlayerScores.Num(); i++)
	{
		if (BBGameState->GetPlayerScore(i) >= TargetScore)
		{
			WinnerIndex = i;
			break;
		}
	}

	// === 胜利与复活倒计时逻辑 ===
	if (WinnerIndex != -1)
	{
		FString WinMsg = FString::Printf(TEXT("PLAYER %d WINS!"), WinnerIndex + 1);
		AActor* WinnerActor = ActiveTanks[WinnerIndex];
		if (WinnerActor && VictoryEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), VictoryEffect, WinnerActor->GetActorLocation());
		}
		if (ScreenMessageWidget)
		{
			ScreenMessageWidget->SetMessageText(WinMsg);
			FLinearColor Color;
			if (WinnerIndex == 0) Color = FLinearColor::Red;
			else if (WinnerIndex == 1) Color = FLinearColor::Blue;
			else if (WinnerIndex == 2) Color = FLinearColor::Green;
			else Color = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);
			ScreenMessageWidget->SetMessageColor(Color);
			ScreenMessageWidget->SetVisibility(ESlateVisibility::Visible);
		}

		GetWorldTimerManager().ClearTimer(MatchTimerHandle);
		FTimerHandle EndTimer;
		GetWorldTimerManager().SetTimer(EndTimer, this, &ABattleBlasterGameMode::ShowMultiBattleGameOver, GameOverDelay, false);
	}
	else
	{
		FTimerDelegate RespawnDel;
		RespawnDel.BindUObject(this, &ABattleBlasterGameMode::RespawnPlayer, VictimIndex);
		FTimerHandle RespawnTimer;
		GetWorldTimerManager().SetTimer(RespawnTimer, RespawnDel, RespawnDelay, false);
	}
}

void ABattleBlasterGameMode::RespawnPlayer(int32 PlayerIndex)
{
	if (WinnerIndex != -1) return;
	if (!PlayerStarts.IsValidIndex(PlayerIndex) || !PlayerStarts[PlayerIndex]) return;

	TSubclassOf<ATank> TankClassToUse = TankClass;
	if (UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(GetGameInstance()))
	{
		if (GI->SelectedTankClasses.IsValidIndex(PlayerIndex) && GI->SelectedTankClasses[PlayerIndex] != nullptr)
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
	// 【核心修复 3：灵魂附体】不要生成新 Controller！
	// 去世界里找到原来那个存有击杀记录的老 Controller，让它直接附身新躯壳！
	// =========================================================================
	AController* TargetController = nullptr;
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		AController* C = It->Get();
		if (ATankBattlePlayerState* PS = C->GetPlayerState<ATankBattlePlayerState>())
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

		// 【修复 4：唤醒 AI 大脑】
		// 如果是 AI 控制器，必须强制清除它脑海里关于“死掉老躯体”的记忆，否则它会原地罚站！
		if (AAIBotPlayerController* AIPC = Cast<AAIBotPlayerController>(TargetController))
		{
			AIPC->ControlledTank = NewTank;
			AIPC->CurrentCombatState = EAICombatState::Idle;
			AIPC->CurrentTarget = nullptr;
		}
	}

	// 销毁旧 Tank（释放内存，不再保留隐藏的尸体）
	if (OldTank && OldTank != NewTank)
	{
		OldTank->Destroy();
	}

	// 2. 恢复初始状态并立刻可控
	NewTank->SetPlayerIndex(PlayerIndex);
	NewTank->SetPlayerEnabled(true);

	if (NewTank->HealthComp)
	{
		NewTank->HealthComp->CurrentHealth = NewTank->HealthComp->MaxHealth * RespawnHealthPercent;
		NewTank->HealthComp->UpdateHUD();
	}

	// 使用复活弹药比例，并从 PlayerState 读取已保存的弹药
	int32 RespawnAmmo = FMath::FloorToInt(NewTank->MaxAmmo * RespawnAmmoPercent);
	// 通过 PlayerIndex 找到 PlayerState
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		AController* C = It->Get();
		if (ATankBattlePlayerState* PS = C->GetPlayerState<ATankBattlePlayerState>())
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

	if (ActiveTanks.IsValidIndex(PlayerIndex))
	{
		ActiveTanks[PlayerIndex] = NewTank;
	}

	// 3. 重新挂载死亡监听
	NewTank->OnKilled.AddDynamic(this, &ABattleBlasterGameMode::HandleTankKilled);

	// 4. 恢复 Buff（从 GameMode 保存的数组恢复）
	if (PlayerSavedBuffs.IsValidIndex(PlayerIndex) && PlayerSavedBuffs[PlayerIndex].Num() > 0)
	{
		if (UTankBuffComponent* NewBuffComp = NewTank->FindComponentByClass<UTankBuffComponent>())
		{
			NewBuffComp->RestoreBuffs(PlayerSavedBuffs[PlayerIndex]);
		}
	}

	// 5. 无敌与特效
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
	InvincibleDel.BindUObject(this, &ABattleBlasterGameMode::EndInvincibility, NewTank, VFXComp);
	GetWorldTimerManager().SetTimer(InvincibleTimer, InvincibleDel, InvincibleTime, false);
}

void ABattleBlasterGameMode::EndInvincibility(ATank* Tank, UNiagaraComponent* VFXComp)
{
	// 1. 恢复受伤 (取消无敌)
	if (Tank && IsValid(Tank))
	{
		Tank->SetCanBeDamaged(true);

		// 调试日志
		// UE_LOG(LogTemp, Display, TEXT("Invincibility Ended for %s"), *Tank->GetName());
	}

	// 如果 Niagara 特效是循环的，需要手动停止
	if (VFXComp && VFXComp->IsActive())
	{
		VFXComp->Deactivate();
		// 或者使用 VFXComp->DestroyComponent();
	}
}

void ABattleBlasterGameMode::ShowMultiBattleGameOver()
{
	if (WinnerIndex < 0 || !MultiBattleGameOverWidgetClass) return;

	APlayerController* PC0 = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC0) return;

	MultiBattleGameOverWidgetInstance = CreateWidget<UMultiBattleGameOverWidget>(PC0, MultiBattleGameOverWidgetClass);
	if (!MultiBattleGameOverWidgetInstance) return;

	UE_LOG(LogTemp, Display, TEXT("[GameMode] ShowMultiBattleGameOver: WinnerIndex=%d, PlayerCount=%d"),
		WinnerIndex, TargetPlayerCount);

	// 只传胜利者索引，Widget 内部自己从 PlayerState 读 KDA / 算 SkillScore / 写历史战绩
	MultiBattleGameOverWidgetInstance->InitResultData(WinnerIndex);

	MultiBattleGameOverWidgetInstance->AddToViewport(1000);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MultiBattleGameOverWidgetInstance->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC0->SetInputMode(InputMode);
	PC0->bShowMouseCursor = true;
}

void ABattleBlasterGameMode::OnGameOverTimerTimeOut()
{

	UE_LOG(LogTemp, Warning, TEXT("=== GAME OVER ==="));


	// 你可以在这里选择：
	// 1. 什么都不做，让玩家看着 "Victory" 界面
	// 2. 重新加载当前关卡 (重置比赛)

	// 如果想自动重开一局，取消下面这行的注释：
	// UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()), false);
}

/*Checklist
Tag 设置：请务必确保你的场景中有 4 个 PlayerStart，并且它们的 Tags 属性分别设置为 P0, P1, P2, P3。
如果只有 P0 和 P1，当你设置玩家数为 3 时，控制台会报错并可能无法生成第 3 个玩家。
Project Settings：在 3-4 人模式下，确保 Project Settings -> Maps & Modes -> Local Multiplayer
-> Use Splitscreen 是勾选的，这样屏幕会自动分割成 3 或 4 块。
*/
