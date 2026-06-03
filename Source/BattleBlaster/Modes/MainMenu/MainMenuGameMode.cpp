#include "Modes/MainMenu/MainMenuGameMode.h"

#include "Blueprint/UserWidget.h"
#include "Core/BattleBlasterGameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Shared/Controllers/UIPlayerController.h"
#include "Widgets/SWidget.h"

AMainMenuGameMode::AMainMenuGameMode()
{
	PlayerControllerClass = AUIPlayerController::StaticClass();
}

void AMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Display, TEXT("AMainMenuGameMode::BeginPlay - Starting"));

	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;

	UE_LOG(LogTemp, Display, TEXT("AMainMenuGameMode::BeginPlay - PlayerController: %s"),
		PC ? TEXT("Valid") : TEXT("NULL"));

	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->SetForceDisableSplitscreen(true);
	}

	// Main menu is a shared screen. Local split-screen players are created only by local setup widgets.
	if (PC)
	{
		UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(GetGameInstance());

		const bool bReturnToSinglePlayer =
			GI && GI->GetReturnToMenuType() == EReturnToMenuType::SinglePlayerMenu &&
			SinglePlayerSelectWidgetClass != nullptr;

		const bool bReturnToMOBASetup =
			GI && GI->GetReturnToMenuType() == EReturnToMenuType::MOBASetupMenu &&
			GI->GetPendingMainMenuWidgetClass() != nullptr;

		TSubclassOf<UUserWidget> WidgetClassToUse = MainMenuWidgetClass;
		if (bReturnToSinglePlayer)
		{
			WidgetClassToUse = SinglePlayerSelectWidgetClass;
		}
		else if (bReturnToMOBASetup)
		{
			WidgetClassToUse = GI->GetPendingMainMenuWidgetClass();
		}

		if (WidgetClassToUse)
		{
			UE_LOG(LogTemp, Display, TEXT("AMainMenuGameMode::BeginPlay - WidgetClassToUse: %s"), *WidgetClassToUse->GetName());

			if (bReturnToSinglePlayer && GI)
			{
				GI->SetReturnToMenuType(EReturnToMenuType::MainMenu);
			}

			if (bReturnToMOBASetup && GI)
			{
				GI->SetReturnToMenuType(EReturnToMenuType::MainMenu);
				GI->ClearPendingMainMenuWidgetClass();
			}

			CurrentWidget = CreateWidget<UUserWidget>(GetWorld(), WidgetClassToUse);

			UE_LOG(LogTemp, Display, TEXT("AMainMenuGameMode::BeginPlay - CurrentWidget: %s"),
				CurrentWidget ? TEXT("Valid") : TEXT("NULL"));

			if (CurrentWidget)
			{
				CurrentWidget->AddToViewport();

				FInputModeUIOnly InputMode;
				TSharedRef<SWidget> FocusWidget = CurrentWidget->TakeWidget();
				InputMode.SetWidgetToFocus(FocusWidget);
				InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

				PC->SetInputMode(InputMode);
				PC->bShowMouseCursor = true;
			}
		}
	}
}

void AMainMenuGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CurrentWidget)
	{
		CurrentWidget->RemoveFromParent();
		CurrentWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}
