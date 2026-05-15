#include "Modes/MainMenu/MainMenuGameMode.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWidget.h"
#include "Core/BattleBlasterGameInstance.h"


void AMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Display, TEXT("AMainMenuGameMode::BeginPlay - Starting"));

	//PlayerControllerClass = AUIPlayerController::StaticClass();
	APlayerController* PC = GetWorld()->GetFirstPlayerController();

	UE_LOG(LogTemp, Display, TEXT("AMainMenuGameMode::BeginPlay - PlayerController: %s"), 
		PC ? TEXT("Valid") : TEXT("NULL"));

	// 菜单阶段：强制禁用分屏
	if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		// UE5.4+ 高版本正确API，等价于老版本的SetDisableSplitscreenOverride(true)
		Viewport->SetForceDisableSplitscreen(true);
	}

	// 2. （可选）在这里创建最多 4 个 LocalPlayer，供所有菜单复用
	UWorld* World = GetWorld();
	if (World)
	{
		const int32 MaxMenuPlayers = 4;
		int32 CurrentPlayers = UGameplayStatics::GetNumLocalPlayerControllers(World);
		for (int32 i = CurrentPlayers; i < MaxMenuPlayers && CurrentPlayers < MaxMenuPlayers; ++i)
		{
			APlayerController* NewPC = UGameplayStatics::CreatePlayer(World, -1, true);
			if (NewPC)
			{
				CurrentPlayers++;
			}
			else
			{
				break; // 达到最大玩家数
			}
		}
	}

	// 根据 GameInstance 的 ReturnToMenuType 决定打开哪个 UI
	if (PC)
	{
		UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(GetGameInstance());

		bool bReturnToSinglePlayer =
			GI && GI->GetReturnToMenuType() == EReturnToMenuType::SinglePlayerMenu &&
			SinglePlayerSelectWidgetClass != nullptr;

		const bool bReturnToMOBASetup =
			GI && GI->GetReturnToMenuType() == EReturnToMenuType::MOBASetupMenu &&
			GI->GetPendingMainMenuWidgetClass() != nullptr;

		// 选择要创建的 Widget 类：单人闯关选择 / MOBA 设置 / 主菜单
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

			// 如果是从单人闯关返回，使用完一次后重置为默认值
			if (bReturnToSinglePlayer && GI)
			{
				GI->SetReturnToMenuType(EReturnToMenuType::MainMenu);
			}
			if (bReturnToMOBASetup && GI)
			{
				GI->SetReturnToMenuType(EReturnToMenuType::MainMenu);
				GI->ClearPendingMainMenuWidgetClass();
			}

			// 1. 创建 Widget
			CurrentWidget = CreateWidget<UUserWidget>(GetWorld(), WidgetClassToUse);

			UE_LOG(LogTemp, Display, TEXT("AMainMenuGameMode::BeginPlay - CurrentWidget: %s"), 
				CurrentWidget ? TEXT("Valid") : TEXT("NULL"));

			if (CurrentWidget)
			{
				// 2. 添加到视口
				CurrentWidget->AddToViewport();

				// 3. 设置输入模式为 UI Only
				FInputModeUIOnly InputMode;
				TSharedRef<SWidget> FocusWidget = CurrentWidget->TakeWidget();
				InputMode.SetWidgetToFocus(FocusWidget);
				InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

				PC->SetInputMode(InputMode);

				// 4. 显示鼠标
				PC->bShowMouseCursor = true;
			}
		}
	}
}

void AMainMenuGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 在 GameMode 销毁前，从视口移除并清理 Widget
	if (CurrentWidget)
	{
		CurrentWidget->RemoveFromParent();
		CurrentWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}
