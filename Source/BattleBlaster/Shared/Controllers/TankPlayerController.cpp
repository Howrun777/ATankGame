#include "Shared/Controllers/TankPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/LatentActionManager.h"
#include "Shared/Combat/HealthComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Shared/UI/BulletsWidget.h"
#include "Shared/UI/BuffListWidget.h"
#include "Shared/Pawns/Tank.h"
#include "Modes/Stage/TankStageGameMode.h"
#include "Shared/UI/KDAWidget.h"
#include "Modes/FreeForAll/BattleBlasterGameMode.h"
#include "Modes/TeamBattle/TeamBattleGameMode.h"
#include "Modes/MOBA/TankMOBAGameMode.h"
#include "Modes/MOBA/UI/DeathScreenWidget.h"
#include "Modes/MOBA/UI/EliminatedScreenWidget.h"
#include "Shared/UI/ReturnToSpawnWidget.h"
#include "Shared/State/TankPlayerState.h"

void ATankPlayerController::InitializeHUD()
{
	// 【修复】：先检查 Controller 自身是否有效（防止在蓝图构造脚本中被调用时崩溃）
	if (!this || !IsValid(this)) return;
	// 再次检查防止中途对象失效
	if (!HUDWidgetClass || !GetLocalPlayer()) return;

	// 如果这个 Controller 手里没有坦克，直接返回
	APawn* HavePawn = GetPawn();
	if (HavePawn == nullptr)
	{
		return;
	}

	// --- 创建血量 UI (HUD) ---
	if (!HUDWidget)
	{
		HUDWidget = CreateWidget<UHUDWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToPlayerScreen();

			// 刷新血量
			UHealthComponent* HealthComp = HavePawn->FindComponentByClass<UHealthComponent>();
			if (HealthComp)
			{
				HealthComp->UpdateHUD();
			}
		}
	}

	// --- 创建弹药 UI (Ammo) ---
	if (AmmoWidgetClass && !AmmoWidget)
	{
		AmmoWidget = CreateWidget<UBulletsWidget>(this, AmmoWidgetClass);
		if (AmmoWidget)
		{
			AmmoWidget->AddToPlayerScreen();

			// 刷新弹药
			if (ATank* TankPawn = Cast<ATank>(GetPawn()))
			{
				SetHUDAmmo(TankPawn->CurrentAmmo, TankPawn->MaxAmmo);
			}
		}
	}

	// --- Buff列表UI创建代码 ---
	if (BuffListWidgetClass && !BuffListUI)
	{
		BuffListUI = CreateWidget<UBuffListWidget>(this, BuffListWidgetClass);
		if (BuffListUI)
		{
			BuffListUI->InitBuffUI(this);
			BuffListUI->AddToPlayerScreen();
		}
	}

	// --- 单人闯关模式：右上角当前关卡 + 最高历史记录 ---
	if (PassWidgetClass && !PassWidget)
	{
		AGameModeBase* GM = GetWorld()->GetAuthGameMode();
		if (GM && GM->IsA(ATankStageGameMode::StaticClass()))
		{
			PassWidget = CreateWidget<UPassWidget>(this, PassWidgetClass);
			if (PassWidget)
			{
				PassWidget->AddToPlayerScreen();
			}
		}
	}

	// --- 多人战斗模式：KDA 显示 ---
	if (KDAWidgetClass && !KDAWidget)
	{
		KDAWidget = CreateWidget<UKDAWidget>(this, KDAWidgetClass);
		if (KDAWidget)
		{
			KDAWidget->AddToPlayerScreen();
			// 初始为 0-0-0
			KDAWidget->UpdateKDA(0, 0, 0);

			// 根据游戏模式和玩家索引设置KDA颜色
			int32 PlayerIndex = UGameplayStatics::GetPlayerControllerID(this);
			AGameModeBase* GM = GetWorld()->GetAuthGameMode();

			FLinearColor KDAColor = FLinearColor::White;

			if (GM && GM->IsA(ABattleBlasterGameMode::StaticClass()))
			{
				// 多人死斗模式：玩家0-红色，玩家1-蓝色，玩家2-绿色，玩家3-黄色
				switch (PlayerIndex)
				{
				case 0: KDAColor = FLinearColor::Red; break;
				case 1: KDAColor = FLinearColor::Blue; break;
				case 2: KDAColor = FLinearColor::Green; break;
				case 3: KDAColor = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f); break; // 黄色
				}
			}
			else if (GM && GM->IsA(ATeamBattleGameMode::StaticClass()))
			{
				// 团队模式：玩家0-红色，玩家2-红色，玩家1-蓝色，玩家3-蓝色
				if (PlayerIndex == 0 || PlayerIndex == 2)
				{
					KDAColor = FLinearColor::Red;
				}
				else
				{
					KDAColor = FLinearColor::Blue;
				}
			}
			else if (GM && GM->IsA(ATankMOBAGameMode::StaticClass()))
			{
				// MOBA 模式：玩家0-红色，玩家1-蓝色，玩家2-绿色，玩家3-黄色
				switch (PlayerIndex)
				{
				case 0: KDAColor = FLinearColor::Red; break;
				case 1: KDAColor = FLinearColor::Blue; break;
				case 2: KDAColor = FLinearColor::Green; break;
				case 3: KDAColor = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f); break; // 黄色
				}
			}

			KDAWidget->SetColor(KDAColor);
		}
	}
}

void ATankPlayerController::BeginPlay()
{
	Super::BeginPlay();
	// 1. 获取增强输入子系统并添加上下文
	// 注意：这里的 Priority 设为 1，确保比坦克的默认 Priority (通常是0) 高一点点，或者一样都可以
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (InputMappingContext)
		{
			Subsystem->AddMappingContext(InputMappingContext, 1);
		}
	}
	if (IsLocalPlayerController())
	{
		// 如果 Pawn 已经存在，立即初始化 UI
		if (GetPawn() != nullptr)
		{
			InitializeHUD();
		}
		else
		{
			// 如果还没有 Pawn，延迟 0.1 秒后尝试初始化（用于 GameMode 延迟 Possess 的情况）
			FTimerHandle TimerHandle;
			GetWorldTimerManager().SetTimer(TimerHandle, this, &ATankPlayerController::InitializeHUD, 0.1f, false);
		}
	}
}
void ATankPlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);

	// 如果传入的 Pawn 是空的（比如角色刚死还没复活），直接返回
	if (InPawn == nullptr) return;

	// 情况A：如果是游戏刚开始，HUDWidget 可能还没创建（BeginPlay 还没跑），
	// 这时候不需要做任何事，因为 BeginPlay 会处理初始化。

	// 情况B：如果是重生，HUDWidget 已经存在了，我们需要手动强制刷新它。
	if (HUDWidget)
	{
		// 1. 刷新血量
		// 获取新角色的血量组件
		UHealthComponent* HealthComp = InPawn->FindComponentByClass<UHealthComponent>();
		if (HealthComp)
		{
			// 这里的关键：让新角色的组件去更新那个旧的 UI
			HealthComp->UpdateHUD();
		}

		// 2. 刷新弹药 (如果你有弹药逻辑)
		ATank* MyTank = Cast<ATank>(InPawn);
		if (MyTank && AmmoWidget)
		{
			// 假设你有 CurrentAmmo 和 MaxAmmo
			SetHUDAmmo(MyTank->CurrentAmmo, MyTank->MaxAmmo);
		}

		// 3. 刷新 KDA（复活后从 PlayerState 重新读取正确的 K/D/A）
		if (KDAWidget)
		{
			UpdateKDA();
		}

		// --- 重生时确保BuffUI存在（可选但推荐） ---
		// 因为BuffUI在PlayerController里，不会因为Pawn死亡而消失，
		// 但为了保险起见，如果UI意外丢失，可以在这里重新创建
		if (BuffListWidgetClass && !IsValid(BuffListUI))
		{
			BuffListUI = CreateWidget<UBuffListWidget>(this, BuffListWidgetClass);
			if (BuffListUI)
			{
				BuffListUI->InitBuffUI(this); // 【必须补上这一句】重新绑定所属的控制器！
				BuffListUI->AddToPlayerScreen();
			}
		}
	}
}

void ATankPlayerController::SetHUDAmmo(int32 Current, int32 Max)
{
	if (AmmoWidget)
	{
		AmmoWidget->SetAmmoText(Current, Max);
	}
}

void ATankPlayerController::UpdateHealthHUD(float HealthPercent, float ShieldPercent)
{
	// 确保 HUD 存在且已显示在屏幕上
	if (HUDWidget)
	{
		HUDWidget->SetHealthBarPercent(HealthPercent);
		HUDWidget->SetShieldBarPercent(ShieldPercent);
	}
}

void ATankPlayerController::UpdateKDA()
{
	if (!KDAWidget) return;

	if (ATankPlayerState* PS = GetPlayerState<ATankPlayerState>())
	{
		KDAWidget->UpdateKDA(PS->KillCount, PS->DeathCount, PS->AssistCount);
	}
}

void ATankPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// 1. 获取增强输入子系统并添加上下文
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (InputMappingContext)
		{
			// 检查是否已经添加过，避免重复添加和清除其他玩家的映射
			if (!Subsystem->HasMappingContext(InputMappingContext))
			{
				Subsystem->AddMappingContext(InputMappingContext, 1);
			}
		}
	}

	// 2. 强制转换为增强输入组件
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// 3. 绑定 IA_Pause 资产
		if (PauseAction)
		{
			EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this, &ATankPlayerController::TogglePauseMenu);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("PauseAction is not set in TankPlayerController! Please assign IA_Pause in blueprint."));
		}

		// 4. 绑定 IA_Spectator 资产（旁观者模式）
		if (SpectatorAction)
		{
			EnhancedInputComponent->BindAction(SpectatorAction, ETriggerEvent::Started, this, &ATankPlayerController::EnterSpectatorMode);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("SpectatorAction is not set in TankPlayerController! Please assign IA_Spectator in blueprint."));
		}

		// 5. 绑定回城功能：按住空格/手柄Y键7秒后回到出生点
		if (ReturnToSpawnAction)
		{
			EnhancedInputComponent->BindAction(ReturnToSpawnAction, ETriggerEvent::Started, this, &ATankPlayerController::OnReturnToSpawnStarted);
			EnhancedInputComponent->BindAction(ReturnToSpawnAction, ETriggerEvent::Completed, this, &ATankPlayerController::OnReturnToSpawnCompleted);
			EnhancedInputComponent->BindAction(ReturnToSpawnAction, ETriggerEvent::Canceled, this, &ATankPlayerController::OnReturnToSpawnCompleted);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ReturnToSpawnAction is not set in TankPlayerController blueprint."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to cast to EnhancedInputComponent! Input may not work properly."));
	}
}

void ATankPlayerController::TogglePauseMenu()
{
	// 检查是否是本地控制器
	if (!IsLocalController())
	{
		return;
	}

	// 只允许第一个玩家(索引0)打开暂停菜单
	int32 PlayerIndex = UGameplayStatics::GetPlayerControllerID(this);
	UE_LOG(LogTemp, Display, TEXT("TogglePauseMenu called by player %d"), PlayerIndex);
	
	if (PlayerIndex != 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Only player 0 can toggle pause menu!"));
		return;
	}

	if (PauseMenuClass)
	{
		// 检查暂停菜单是否已经存在
		if (!IsValid(PauseMenuInstance))
		{
			PauseMenuInstance = CreateWidget<UPauseMenuWidget>(this, PauseMenuClass);
			if (!PauseMenuInstance)
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to create pause menu widget!"));
				return;
			}
		}

		// 切换显示状态
		if (IsValid(PauseMenuInstance))
		{
			if (PauseMenuInstance->IsInViewport())
			{
				PauseMenuInstance->Teardown();
			}
			else
			{
				PauseMenuInstance->Setup();
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PauseMenuClass is not set in TankPlayerController!"));
	}
}

// --- 游戏结束时清理所有UI，避免内存泄漏 ---
void ATankPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// 清理暂停菜单
	if (IsValid(PauseMenuInstance))
	{
		PauseMenuInstance->RemoveFromParent();
		PauseMenuInstance = nullptr;
	}

	// 清理Buff列表UI
	if (IsValid(BuffListUI))
	{
		BuffListUI->RemoveFromParent();
		BuffListUI = nullptr;
	}

	// 清理弹药UI
	if (IsValid(AmmoWidget))
	{
		AmmoWidget->RemoveFromParent();
		AmmoWidget = nullptr;
	}

	// 清理血量UI
	if (IsValid(HUDWidget))
	{
		HUDWidget->RemoveFromParent();
		HUDWidget = nullptr;
	}
	// 清理单人闯关关卡信息UI
	if (IsValid(PassWidget))
	{
		PassWidget->RemoveFromParent();
		PassWidget = nullptr;
	}
	// 清理 KDA UI
	if (IsValid(KDAWidget))
	{
		KDAWidget->RemoveFromParent();
		KDAWidget = nullptr;
	}

	// 清理 MOBA 死亡界面
	if (IsValid(DeathScreenInstance))
	{
		DeathScreenInstance->RemoveFromParent();
		DeathScreenInstance = nullptr;
	}

	// 清理 MOBA 淘汰界面
	if (IsValid(EliminatedScreenInstance))
	{
		EliminatedScreenInstance->RemoveFromParent();
		EliminatedScreenInstance = nullptr;
	}

	// 清理回城进度 Widget
	if (IsValid(ReturnProgressWidgetInstance))
	{
		ReturnProgressWidgetInstance->RemoveFromParent();
		ReturnProgressWidgetInstance = nullptr;
	}

	// 清理手柄震动
	StopVibration();
}

// --- 手柄震动反馈实现 ---

void ATankPlayerController::TriggerFireVibration()
{
	if (!IsLocalController()) return;

#if WITH_EDITOR
	UE_LOG(LogTemp, Display, TEXT("Fire Vibration Triggered: Intensity=%f, Duration=%f"), FireVibrationIntensity, FireVibrationDuration);
#endif

	// 触发发射震动
	if (FireVibrationIntensity > 0.f)
	{
		// 先停止之前的震动
		StopVibration();

		// 使用 PlayDynamicForceFeedback 实现手柄震动
		// 参数: Intensity, Duration, bAffectsLeftLarge, bAffectsRightLarge, Action, LatentInfo
		FLatentActionInfo LatentInfo;
		LatentInfo.UUID = GetUniqueID();
		LatentInfo.Linkage = 0;
		LatentInfo.CallbackTarget = this;

		PlayDynamicForceFeedback(FireVibrationIntensity, FireVibrationDuration, true, true, true, true, EDynamicForceFeedbackAction::Start, LatentInfo);
	}
}

void ATankPlayerController::TriggerDamageVibration()
{
	if (!IsLocalController()) return;

#if WITH_EDITOR
	UE_LOG(LogTemp, Display, TEXT("Damage Vibration Triggered: Intensity=%f, Duration=%f"), DamageVibrationIntensity, DamageVibrationDuration);
#endif

	// 触发受伤震动
	if (DamageVibrationIntensity > 0.f)
	{
		// 先停止之前的震动
		StopVibration();

		// 使用 PlayDynamicForceFeedback 实现手柄震动
		FLatentActionInfo LatentInfo;
		LatentInfo.UUID = GetUniqueID();
		LatentInfo.Linkage = 0;
		LatentInfo.CallbackTarget = this;

		PlayDynamicForceFeedback(DamageVibrationIntensity, DamageVibrationDuration, true, true, true, true, EDynamicForceFeedbackAction::Start, LatentInfo);
	}
}

void ATankPlayerController::StopVibration()
{
	if (!IsLocalController()) return;

	// 停止当前的震动效果
	FLatentActionInfo LatentInfo;
	LatentInfo.UUID = GetUniqueID();
	LatentInfo.Linkage = 0;
	LatentInfo.CallbackTarget = this;

	PlayDynamicForceFeedback(0.f, 0.f, true, true, true, true, EDynamicForceFeedbackAction::Stop, LatentInfo);
}

// ================= MOBA 模式 UI 实现 =================

void ATankPlayerController::ShowDeathScreen(float RespawnTime)
{
	if (!IsLocalController()) return;

	// 创建或显示死亡界面
	if (DeathScreenClass && !DeathScreenInstance)
	{
		DeathScreenInstance = CreateWidget<UDeathScreenWidget>(this, DeathScreenClass);
	}

	if (DeathScreenInstance)
	{
		if (!DeathScreenInstance->IsInViewport())
		{
			DeathScreenInstance->AddToPlayerScreen();
		}
		DeathScreenInstance->Show();
		DeathScreenInstance->UpdateRespawnCountdown(RespawnTime);
	}
}

void ATankPlayerController::HideDeathScreen()
{
	if (!IsLocalController()) return;

	if (DeathScreenInstance)
	{
		DeathScreenInstance->Hide();
	}
}

void ATankPlayerController::UpdateDeathScreenCountdown(float TimeRemaining)
{
	if (!IsLocalController()) return;

	if (DeathScreenInstance)
	{
		DeathScreenInstance->UpdateRespawnCountdown(TimeRemaining);
	}
}

void ATankPlayerController::ShowEliminatedScreen()
{
	if (!IsLocalController()) return;

	// 先隐藏死亡界面
	HideDeathScreen();

	// 创建或显示淘汰界面
	if (EliminatedScreenClass && !EliminatedScreenInstance)
	{
		EliminatedScreenInstance = CreateWidget<UEliminatedScreenWidget>(this, EliminatedScreenClass);
	}

	if (EliminatedScreenInstance)
	{
		if (!EliminatedScreenInstance->IsInViewport())
		{
			EliminatedScreenInstance->AddToPlayerScreen();
		}
		EliminatedScreenInstance->Show();
	}
}

void ATankPlayerController::HideEliminatedScreen()
{
	if (!IsLocalController()) return;

	if (EliminatedScreenInstance)
	{
		EliminatedScreenInstance->Hide();
	}
}

void ATankPlayerController::EnterSpectatorMode()
{
	if (!IsLocalController()) return;

	AGameModeBase* GM = GetWorld()->GetAuthGameMode();
	if (!GM || !GM->IsA(ATankMOBAGameMode::StaticClass())) return;

	if (EliminatedScreenInstance && EliminatedScreenInstance->IsInViewport())
	{
		EliminatedScreenInstance->OnSwitchSpectateClicked();
	}
}

// ================= 回城系统实现 =================

// 获取关联的 Tank Pawn
ATank* ATankPlayerController::GetControlledTank() const
{
	return Cast<ATank>(GetPawn());
}

// 获取 Tank 是否存活
bool ATankPlayerController::IsTankAlive() const
{
	if (ATank* Tank = GetControlledTank())
	{
		return Tank->GetIsAlive();
	}
	return false;
}

// 检查 Tank 是否有出生点
bool ATankPlayerController::HasTankSpawnLocation() const
{
	if (ATank* Tank = GetControlledTank())
	{
		return Tank->HasSpawnLocation();
	}
	return false;
}

// PlayerTick：用于更新回城进度
void ATankPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// Tank 死亡时自动取消回城状态
	if (!IsTankAlive() && bIsHoldingReturnToSpawn)
	{
		bIsHoldingReturnToSpawn = false;
		CurrentHoldTime = 0.0f;
		if (ReturnProgressWidgetInstance && ReturnProgressWidgetInstance->IsInViewport())
		{
			ReturnProgressWidgetInstance->RemoveFromParent();
			ReturnProgressWidgetInstance = nullptr;
		}
	}

	// 回城功能：按住时累加时间
	if (bIsHoldingReturnToSpawn && IsTankAlive())
	{
		CurrentHoldTime += DeltaTime;

		// 实时更新UI进度
		if (ReturnProgressWidgetInstance && ReturnToSpawnHoldTime > 0.0f)
		{
			float ProgressPct = FMath::Clamp(CurrentHoldTime / ReturnToSpawnHoldTime, 0.0f, 1.0f);
			ReturnProgressWidgetInstance->UpdateProgress(ProgressPct);
		}

		// 读条满了！
		if (CurrentHoldTime >= ReturnToSpawnHoldTime)
		{
			ExecuteReturnToSpawn();

			bIsHoldingReturnToSpawn = false;
			CurrentHoldTime = 0.0f;

			// 传送成功，移除屏幕上的 UI
			if (ReturnProgressWidgetInstance && ReturnProgressWidgetInstance->IsInViewport())
			{
				ReturnProgressWidgetInstance->RemoveFromParent();
				ReturnProgressWidgetInstance = nullptr;
			}
		}
	}
}

// 回城功能：开始按住
void ATankPlayerController::OnReturnToSpawnStarted(const FInputActionValue& Value)
{
	if (!IsLocalController() || !IsTankAlive() || !HasTankSpawnLocation()) return;

	bIsHoldingReturnToSpawn = true;
	CurrentHoldTime = 0.0f;

	// 创建并显示回城进度 Widget
	if (ReturnProgressWidgetClass)
	{
		if (!ReturnProgressWidgetInstance)
		{
			ReturnProgressWidgetInstance = CreateWidget<UReturnToSpawnWidget>(this, ReturnProgressWidgetClass);
		}

		if (ReturnProgressWidgetInstance && !ReturnProgressWidgetInstance->IsInViewport())
		{
			ReturnProgressWidgetInstance->AddToPlayerScreen();
			ReturnProgressWidgetInstance->UpdateProgress(0.0f);
		}
	}
}

// 回城功能：结束按住/取消
void ATankPlayerController::OnReturnToSpawnCompleted(const FInputActionValue& Value)
{
	if (!IsLocalController()) return;

	bIsHoldingReturnToSpawn = false;
	CurrentHoldTime = 0.0f;

	// 玩家中途松手，立刻把屏幕上的进度条 UI 删掉
	if (ReturnProgressWidgetInstance && ReturnProgressWidgetInstance->IsInViewport())
	{
		ReturnProgressWidgetInstance->RemoveFromParent();
		ReturnProgressWidgetInstance = nullptr;
	}
}

// 执行回城传送
void ATankPlayerController::ExecuteReturnToSpawn()
{
	if (!IsLocalController()) return;

	ATank* Tank = GetControlledTank();
	if (!Tank || !Tank->GetIsAlive()) return;
	if (!Tank->HasSpawnLocation()) return;

	Tank->SetActorLocation(Tank->GetHomeSpawnLocation());
	Tank->SetActorRotation(Tank->GetHomeSpawnRotation());
}
