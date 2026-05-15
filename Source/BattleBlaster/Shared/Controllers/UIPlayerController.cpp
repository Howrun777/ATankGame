#include "Shared/Controllers/UIPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

#include "Modes/FreeForAll/UI/MutiBattleMenuWidget.h"
#include "Modes/TeamBattle/UI/TeamBattleMenuWidget.h"
#include "Modes/MOBA/UI/MOBASetupWidget.h"
#include "Modes/Stage/UI/TankStageStartWidget.h"
#include "Core/BattleBlasterGameInstance.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Engine/LocalPlayer.h"

void AUIPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (World)
	{
		const int32 MaxSupportedPlayers = 4;
		const int32 CurrentPlayers = UGameplayStatics::GetNumLocalPlayerControllers(World);

		for (int32 i = CurrentPlayers; i < MaxSupportedPlayers; ++i)
		{
			UGameplayStatics::CreatePlayer(World, -1, true);
		}
	}

	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
		{
			if (MenuMappingContext)
			{
				Subsystem->AddMappingContext(MenuMappingContext, 100);
			}
		}
	}

	{
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(Mode);

		const int32 Id = UGameplayStatics::GetPlayerControllerID(this);
		bShowMouseCursor = (Id == 0);
	}

	// 进入菜单时注册 DeviceId → PlayerIndex 映射
	RegisterDeviceMappingToGameInstance();
}

void AUIPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC) return;

	if (IA_TankSelectAxis)
	{
		EIC->BindAction(IA_TankSelectAxis, ETriggerEvent::Triggered,
			this, &AUIPlayerController::HandleTankSelectAxis);
	}
}

void AUIPlayerController::HandleTankSelectAxis(const FInputActionValue& Value)
{
	const float Axis = Value.Get<float>();
	if (FMath::IsNearlyZero(Axis)) return;

	const int32 PlayerIndex = UGameplayStatics::GetPlayerControllerID(this);

	// 注册 DeviceId 映射（解决缺陷 3）
	RegisterDeviceMappingToGameInstance();

	// 依次查找并分发给四个 Widget（按优先级：MutiBattle → TeamBattle → MOBA → TankStage）
	if (UMutiBattleMenuWidget* Menu = FindWidget<UMutiBattleMenuWidget>())
	{
		Menu->OnTankSelectAxisInput(PlayerIndex, Axis);
		return;
	}
	if (UTeamBattleMenuWidget* Menu = FindWidget<UTeamBattleMenuWidget>())
	{
		Menu->OnTankSelectAxisInput(PlayerIndex, Axis);
		return;
	}
	if (UMOBASetupWidget* Menu = FindWidget<UMOBASetupWidget>())
	{
		Menu->OnTankSelectAxisInput(PlayerIndex, Axis);
		return;
	}
	// TankStageStartWidget 是单人的，不需要手柄分发，跳过
}

void AUIPlayerController::RegisterDeviceMappingToGameInstance()
{
	UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (!GI) return;

	const int32 PlayerIndex = UGameplayStatics::GetPlayerControllerID(this);
	ULocalPlayer* LP = GetLocalPlayer();
	if (!LP) return;

	FPlatformUserId UserId = LP->GetPlatformUserId();
	FInputDeviceId ActiveDevice = IPlatformInputDeviceMapper::Get().GetPrimaryInputDeviceForUser(UserId);

	GI->RegisterPlayerDeviceMapping(PlayerIndex, ActiveDevice);
}

UMutiBattleMenuWidget* AUIPlayerController::GetMenuWidgetCached() const
{
	if (CachedMenuWidget.IsValid())
	{
		return CachedMenuWidget.Get();
	}

	TArray<UUserWidget*> Found;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(
		this,
		Found,
		UMutiBattleMenuWidget::StaticClass(),
		false
	);

	if (Found.Num() > 0)
	{
		CachedMenuWidget = Cast<UMutiBattleMenuWidget>(Found[0]);
		return CachedMenuWidget.Get();
	}
	return nullptr;
}

// ================== 模板方法实现：查找当前可见的指定类型 Widget ======================

template<typename WidgetType>
WidgetType* AUIPlayerController::FindWidget() const
{
	TArray<UUserWidget*> Found;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(
		this,
		Found,
		WidgetType::StaticClass(),
		false
	);

	for (UUserWidget* Widget : Found)
	{
		// 只返回可见的 Widget（隐藏的菜单不响应输入）
		if (Widget && Widget->IsVisible())
		{
			return Cast<WidgetType>(Widget);
		}
	}
	return nullptr;
}

// 显式实例化模板（避免链接错误）
template UMutiBattleMenuWidget* AUIPlayerController::FindWidget<UMutiBattleMenuWidget>() const;
template UTeamBattleMenuWidget* AUIPlayerController::FindWidget<UTeamBattleMenuWidget>() const;
template UMOBASetupWidget* AUIPlayerController::FindWidget<UMOBASetupWidget>() const;