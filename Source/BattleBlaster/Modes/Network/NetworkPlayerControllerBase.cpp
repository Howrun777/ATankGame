#include "Modes/Network/NetworkPlayerControllerBase.h"

#include "Blueprint/UserWidget.h"
#include "Modes/Network/NetworkDeathmatchGameState.h"
#include "Modes/Network/NetworkMOBAGameState.h"
#include "Modes/Network/NetworkPlayerStateBase.h"
#include "Modes/Network/NetworkTeamDeathmatchGameState.h"
#include "Modes/Network/UI/CppShowScoresWidget.h"
#include "Modes/Network/UI/NetworkDeathmatchGameOverWidget.h"
#include "Modes/Network/UI/NetworkMOBAStateWidget.h"
#include "Modes/Network/UI/NetworkTeamScoresWidget.h"
#include "Shared/UI/BulletsWidget.h"
#include "Shared/UI/HUDWidget.h"
#include "Shared/UI/KDAWidget.h"
#include "InputAction.h"
#include "InputMappingContext.h"
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

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> DefaultInputMappingContext(TEXT("/Game/Input/IMC_Default"));
	if (DefaultInputMappingContext.Succeeded())
	{
		InputMappingContext = DefaultInputMappingContext.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> DefaultSystemInputMappingContext(TEXT("/Game/Input/IMC_System"));
	if (DefaultSystemInputMappingContext.Succeeded())
	{
		SystemInputMappingContext = DefaultSystemInputMappingContext.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultPauseAction(TEXT("/Game/Input/IA_Pause"));
	if (DefaultPauseAction.Succeeded())
	{
		PauseAction = DefaultPauseAction.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultSpectatorAction(TEXT("/Game/Input/IA_Spectator"));
	if (DefaultSpectatorAction.Succeeded())
	{
		SpectatorAction = DefaultSpectatorAction.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultReturnToSpawnAction(TEXT("/Game/Input/IA_ReturnToSpawn"));
	if (DefaultReturnToSpawnAction.Succeeded())
	{
		ReturnToSpawnAction = DefaultReturnToSpawnAction.Object;
	}

	ScoresWidgetClass = UCppShowScoresWidget::StaticClass();
	TeamScoresWidgetClass = UNetworkTeamScoresWidget::StaticClass();
	MOBAStateWidgetClass = UNetworkMOBAStateWidget::StaticClass();
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
	UnbindMOBAState();
	RemoveScoreWidgets();

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

	ANetworkTeamDeathmatchGameState* TeamDeathmatchGameState = GetWorld() ? GetWorld()->GetGameState<ANetworkTeamDeathmatchGameState>() : nullptr;
	ANetworkMOBAGameState* MOBAGameState = GetWorld() ? GetWorld()->GetGameState<ANetworkMOBAGameState>() : nullptr;
	ANetworkDeathmatchGameState* DeathmatchGameState = GetWorld() ? GetWorld()->GetGameState<ANetworkDeathmatchGameState>() : nullptr;

	if (!TeamDeathmatchGameState && !MOBAGameState && !DeathmatchGameState)
	{
		if (NetworkScoreUIRetryCount < 20)
		{
			++NetworkScoreUIRetryCount;
			GetWorldTimerManager().SetTimer(NetworkScoreUIRetryTimerHandle, this, &ANetworkPlayerControllerBase::InitializeNetworkScoreUI, 0.25f, false);
		}
		return;
	}

	if (TeamDeathmatchGameState)
	{
		if (!TeamScoresWidget)
		{
			TSubclassOf<UNetworkTeamScoresWidget> WidgetClass = TeamScoresWidgetClass;
			if (!WidgetClass)
			{
				WidgetClass = UNetworkTeamScoresWidget::StaticClass();
			}

			TeamScoresWidget = CreateWidget<UNetworkTeamScoresWidget>(this, WidgetClass);
			if (TeamScoresWidget)
			{
				TeamScoresWidget->AddToPlayerScreen();
			}
		}

		BindDeathmatchScoreState(TeamDeathmatchGameState);
		RefreshNetworkScoreUI();
		return;
	}

	if (MOBAGameState)
	{
		if (!MOBAStateWidget)
		{
			TSubclassOf<UNetworkMOBAStateWidget> WidgetClass = MOBAStateWidgetClass;
			if (!WidgetClass)
			{
				WidgetClass = UNetworkMOBAStateWidget::StaticClass();
			}

			MOBAStateWidget = CreateWidget<UNetworkMOBAStateWidget>(this, WidgetClass);
			if (MOBAStateWidget)
			{
				MOBAStateWidget->AddToPlayerScreen();
			}
		}

		BindMOBAState(MOBAGameState);
		RefreshNetworkScoreUI();
		return;
	}

	if (DeathmatchGameState)
	{
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
	}

	RefreshNetworkScoreUI();
}

void ANetworkPlayerControllerBase::RefreshNetworkScoreUI()
{
	if (!IsLocalController())
	{
		return;
	}

	const ANetworkPlayerStateBase* NetworkPS = GetPlayerState<ANetworkPlayerStateBase>();
	const int32 LocalSlotId = NetworkPS ? NetworkPS->GetSlotId() : INDEX_NONE;
	const int32 LocalTeamId = NetworkPS ? NetworkPS->GetTeamId() : INDEX_NONE;

	if (const ANetworkTeamDeathmatchGameState* TeamDeathmatchGameState = GetWorld() ? GetWorld()->GetGameState<ANetworkTeamDeathmatchGameState>() : nullptr)
	{
		if (TeamScoresWidget)
		{
			TeamScoresWidget->UpdateTeamScoreboard(
				TeamDeathmatchGameState->TargetScore,
				TeamDeathmatchGameState->MatchElapsedSeconds,
				TeamDeathmatchGameState->TeamScores,
				LocalTeamId,
				TeamDeathmatchGameState->WinningTeamId);
		}

		if (TeamDeathmatchGameState->IsMatchOver())
		{
			ShowNetworkDeathmatchGameOver(TeamDeathmatchGameState->WinningTeamId);
		}
		return;
	}

	if (const ANetworkMOBAGameState* MOBAGameState = GetWorld() ? GetWorld()->GetGameState<ANetworkMOBAGameState>() : nullptr)
	{
		if (MOBAStateWidget)
		{
			MOBAStateWidget->UpdateMOBAState(
				MOBAGameState->MatchElapsedSeconds,
				MOBAGameState->AliveCoreCountsByTeam,
				MOBAGameState->bTeamEliminated,
				LocalTeamId,
				MOBAGameState->WinningTeamId);
		}
		return;
	}

	if (const ANetworkDeathmatchGameState* DeathmatchGameState = GetWorld() ? GetWorld()->GetGameState<ANetworkDeathmatchGameState>() : nullptr)
	{
		if (ScoresWidget)
		{
			ScoresWidget->UpdateScoreboard(
				DeathmatchGameState->TargetScore,
				DeathmatchGameState->MatchElapsedSeconds,
				DeathmatchGameState->PlayerScores,
				LocalSlotId);
		}

		if (DeathmatchGameState->IsMatchOver())
		{
			ShowNetworkDeathmatchGameOver(DeathmatchGameState->WinnerSlotId);
		}
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

void ANetworkPlayerControllerBase::HandleMOBAStateChanged()
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

void ANetworkPlayerControllerBase::BindMOBAState(ANetworkMOBAGameState* MOBAGameState)
{
	if (BoundMOBAGameState == MOBAGameState)
	{
		return;
	}

	UnbindMOBAState();

	BoundMOBAGameState = MOBAGameState;
	if (BoundMOBAGameState)
	{
		BoundMOBAGameState->OnMOBAStateChanged.AddDynamic(this, &ANetworkPlayerControllerBase::HandleMOBAStateChanged);
	}
}

void ANetworkPlayerControllerBase::UnbindMOBAState()
{
	if (BoundMOBAGameState)
	{
		BoundMOBAGameState->OnMOBAStateChanged.RemoveDynamic(this, &ANetworkPlayerControllerBase::HandleMOBAStateChanged);
		BoundMOBAGameState = nullptr;
	}
}

void ANetworkPlayerControllerBase::RemoveScoreWidgets()
{
	if (IsValid(ScoresWidget))
	{
		ScoresWidget->RemoveFromParent();
		ScoresWidget = nullptr;
	}
	if (IsValid(TeamScoresWidget))
	{
		TeamScoresWidget->RemoveFromParent();
		TeamScoresWidget = nullptr;
	}
	if (IsValid(MOBAStateWidget))
	{
		MOBAStateWidget->RemoveFromParent();
		MOBAStateWidget = nullptr;
	}
}
