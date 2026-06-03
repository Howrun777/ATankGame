#include "Modes/Network/NetworkPlayerControllerBase.h"

#include "Blueprint/UserWidget.h"
#include "Modes/Network/NetworkDeathmatchGameState.h"
#include "Modes/Network/NetworkPlayerStateBase.h"
#include "Modes/Network/UI/CppShowScoresWidget.h"
#include "Modes/Network/UI/NetworkDeathmatchGameOverWidget.h"
#include "Shared/UI/BulletsWidget.h"
#include "Shared/UI/HUDWidget.h"
#include "Shared/UI/KDAWidget.h"
#include "UObject/ConstructorHelpers.h"

ANetworkPlayerControllerBase::ANetworkPlayerControllerBase()
{
	// These C++ defaults are fallbacks only. The production network mode should
	// use BP_NetworkPlayerControllerBase to assign widgets per mode.
	static ConstructorHelpers::FClassFinder<UHUDWidget> DefaultHUDWidgetClass(TEXT("/Game/Blueprints/Controller/WBP_HUD"));
	if (DefaultHUDWidgetClass.Succeeded())
	{
		HUDWidgetClass = DefaultHUDWidgetClass.Class;
	}

	static ConstructorHelpers::FClassFinder<UBulletsWidget> DefaultAmmoWidgetClass(TEXT("/Game/Blueprints/Controller/BP_BulletsWidget"));
	if (DefaultAmmoWidgetClass.Succeeded())
	{
		AmmoWidgetClass = DefaultAmmoWidgetClass.Class;
	}

	static ConstructorHelpers::FClassFinder<UKDAWidget> DefaultKDAWidgetClass(TEXT("/Game/Blueprints/Controller/WBP_KDAWidget"));
	if (DefaultKDAWidgetClass.Succeeded())
	{
		KDAWidgetClass = DefaultKDAWidgetClass.Class;
	}

	ScoresWidgetClass = UCppShowScoresWidget::StaticClass();
}

void ANetworkPlayerControllerBase::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Display, TEXT("NetworkPlayerControllerBase BeginPlay: Local=%d, NetMode=%d"),
		IsLocalController() ? 1 : 0,
		static_cast<int32>(GetNetMode()));

	InitializeNetworkScoreUI();
}

void ANetworkPlayerControllerBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(NetworkScoreUIRetryTimerHandle);
	UnbindDeathmatchScoreState();

	if (IsValid(ScoresWidget))
	{
		ScoresWidget->RemoveFromParent();
		ScoresWidget = nullptr;
	}
	if (IsValid(DeathmatchGameOverWidget))
	{
		DeathmatchGameOverWidget->RemoveFromParent();
		DeathmatchGameOverWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ANetworkPlayerControllerBase::InitializeNetworkScoreUI()
{
	if (!IsLocalController())
	{
		return;
	}

	ANetworkDeathmatchGameState* DeathmatchGameState = GetWorld() ? GetWorld()->GetGameState<ANetworkDeathmatchGameState>() : nullptr;
	if (!DeathmatchGameState)
	{
		if (NetworkScoreUIRetryCount < 20)
		{
			++NetworkScoreUIRetryCount;
			GetWorldTimerManager().SetTimer(NetworkScoreUIRetryTimerHandle, this, &ANetworkPlayerControllerBase::InitializeNetworkScoreUI, 0.25f, false);
		}
		return;
	}

	TSubclassOf<UCppShowScoresWidget> WidgetClass = ScoresWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = UCppShowScoresWidget::StaticClass();
	}
	if (WidgetClass && !ScoresWidget)
	{
		ScoresWidget = CreateWidget<UCppShowScoresWidget>(this, WidgetClass);
		if (ScoresWidget)
		{
			ScoresWidget->AddToPlayerScreen();
		}
	}

	BindDeathmatchScoreState(DeathmatchGameState);
	RefreshNetworkScoreUI();
}

void ANetworkPlayerControllerBase::RefreshNetworkScoreUI()
{
	if (!IsLocalController() || !ScoresWidget)
	{
		return;
	}

	const ANetworkDeathmatchGameState* DeathmatchGameState = GetWorld() ? GetWorld()->GetGameState<ANetworkDeathmatchGameState>() : nullptr;
	if (!DeathmatchGameState)
	{
		return;
	}

	const int32 LocalSlotId = GetPlayerState<ANetworkPlayerStateBase>()
		? GetPlayerState<ANetworkPlayerStateBase>()->GetSlotId()
		: INDEX_NONE;

	const int32 VisibleScoreCount = FMath::Clamp(
		DeathmatchGameState->ConnectedPlayerCount > 0 ? DeathmatchGameState->ConnectedPlayerCount : DeathmatchGameState->PlayerScores.Num(),
		0,
		DeathmatchGameState->PlayerScores.Num());
	TArray<int32> VisibleScores;
	VisibleScores.Reserve(VisibleScoreCount);
	for (int32 SlotId = 0; SlotId < VisibleScoreCount; ++SlotId)
	{
		VisibleScores.Add(DeathmatchGameState->GetPlayerScore(SlotId));
	}

	ScoresWidget->UpdateScoreboard(
		DeathmatchGameState->TargetScore,
		DeathmatchGameState->MatchElapsedSeconds,
		VisibleScores,
		LocalSlotId);

	if (DeathmatchGameState->IsMatchOver())
	{
		ShowNetworkDeathmatchGameOver(DeathmatchGameState->WinnerSlotId);
	}
}

void ANetworkPlayerControllerBase::ShowNetworkDeathmatchGameOver(int32 WinnerSlotId)
{
	if (!IsLocalController() || WinnerSlotId < 0)
	{
		return;
	}

	if (!DeathmatchGameOverWidget)
	{
		TSubclassOf<UNetworkDeathmatchGameOverWidget> WidgetClass = DeathmatchGameOverWidgetClass;
		if (!WidgetClass)
		{
			WidgetClass = UNetworkDeathmatchGameOverWidget::StaticClass();
		}

		DeathmatchGameOverWidget = CreateWidget<UNetworkDeathmatchGameOverWidget>(this, WidgetClass);
		if (DeathmatchGameOverWidget)
		{
			DeathmatchGameOverWidget->AddToViewport(1000);
		}
	}

	if (DeathmatchGameOverWidget)
	{
		DeathmatchGameOverWidget->InitResultData(WinnerSlotId);
		if (DeathmatchGameOverWidget->IsInViewport())
		{
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(DeathmatchGameOverWidget->TakeWidget());
			SetInputMode(InputMode);
			bShowMouseCursor = true;
		}
	}
}

void ANetworkPlayerControllerBase::HandleDeathmatchScoreStateChanged()
{
	RefreshNetworkScoreUI();
}

void ANetworkPlayerControllerBase::BindDeathmatchScoreState(ANetworkDeathmatchGameState* DeathmatchGameState)
{
	if (BoundDeathmatchGameState == DeathmatchGameState)
	{
		return;
	}

	UnbindDeathmatchScoreState();

	BoundDeathmatchGameState = DeathmatchGameState;
	if (BoundDeathmatchGameState)
	{
		BoundDeathmatchGameState->OnScoreStateChanged.AddDynamic(this, &ANetworkPlayerControllerBase::HandleDeathmatchScoreStateChanged);
	}
}

void ANetworkPlayerControllerBase::UnbindDeathmatchScoreState()
{
	if (BoundDeathmatchGameState)
	{
		BoundDeathmatchGameState->OnScoreStateChanged.RemoveDynamic(this, &ANetworkPlayerControllerBase::HandleDeathmatchScoreStateChanged);
		BoundDeathmatchGameState = nullptr;
	}
}
