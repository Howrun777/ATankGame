#include "Modes/Stage/UI/TankStageOverWidget.h"
#include "Core/BattleBlasterGameInstance.h"
#include "Modes/Stage/TankStageGameMode.h"
#include "Modes/MainMenu/MainMenuGameMode.h"
#include "Kismet/GameplayStatics.h"

void UTankStageOverWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Restart)
		Btn_Restart->OnClicked.AddDynamic(this, &UTankStageOverWidget::OnRestartClicked);
	if (Btn_ReturnMenu)
		Btn_ReturnMenu->OnClicked.AddDynamic(this, &UTankStageOverWidget::OnReturnMenuClicked);
}

void UTankStageOverWidget::NativeDestruct()
{
	// Unbind delegates to prevent dangling references during destruction
	if (Btn_Restart)
	{
		Btn_Restart->OnClicked.RemoveAll(this);
	}
	if (Btn_ReturnMenu)
	{
		Btn_ReturnMenu->OnClicked.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UTankStageOverWidget::RefreshDisplay(int32 CurrentLevel, int32 HighestLevel, float GameTimeSeconds, TSubclassOf<ATank> PlayerTankClass)
{
	if (Text_CurrentLevel)
		Text_CurrentLevel->SetText(FText::AsNumber(CurrentLevel));
	if (Text_HighestLevel)
		Text_HighestLevel->SetText(FText::AsNumber(HighestLevel));
	if (Text_MatchTime)
		Text_MatchTime->SetText(FText::FromString(FormatGameTime(GameTimeSeconds)));

	UTexture2D* Portrait = GetTankPortrait(PlayerTankClass);
	if (Img_TankPortrait && Portrait)
	{
		Img_TankPortrait->SetBrushFromTexture(Portrait);
		Img_TankPortrait->SetVisibility(ESlateVisibility::Visible);
	}
	else if (Img_TankPortrait)
	{
		Img_TankPortrait->SetVisibility(ESlateVisibility::Collapsed);
	}
}

FString UTankStageOverWidget::FormatGameTime(float TotalSeconds) const
{
	int32 Total = FMath::Max(0, FMath::FloorToInt(TotalSeconds));
	int32 Hours = Total / 3600;
	int32 Minutes = (Total % 3600) / 60;
	int32 Seconds = Total % 60;
	return FString::Printf(TEXT("%02d:%02d:%02d"), Hours, Minutes, Seconds);
}

UTexture2D* UTankStageOverWidget::GetTankPortrait(TSubclassOf<ATank> TankClass) const
{
	if (!TankClass) return nullptr;
	for (const FTankImageEntry& Entry : TankImageMap)
	{
		if (Entry.TankClass == TankClass && Entry.PortraitTexture)
			return Entry.PortraitTexture;
	}
	return nullptr;
}

void UTankStageOverWidget::OnRestartClicked()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// 先恢复输入模式，再移除菜单
	APlayerController* PC = World->GetFirstPlayerController();
	if (PC)
	{
		// 恢复游戏输入模式
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
	}

	// 取消暂停
	UGameplayStatics::SetGamePaused(World, false);

	// 移除菜单
	RemoveFromParent();

	UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(World->GetGameInstance());
	if (!GI) return;

	// 再来一局：重置当前关卡为1，用 RestartGame 加载第一关（带 TankStageGameMode）
	ATankStageGameMode* SPGM = World->GetAuthGameMode<ATankStageGameMode>();
	FString Options = SPGM
		? FString::Printf(TEXT("?game=%s"), *SPGM->GetClass()->GetPathName())
		: TEXT("");
	GI->RestartGame(Options);
}

void UTankStageOverWidget::OnReturnMenuClicked()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// 先恢复输入模式，再移除菜单
	APlayerController* PC = World->GetFirstPlayerController();
	if (PC)
	{
		// 恢复游戏输入模式
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
	}

	// 取消暂停
	UGameplayStatics::SetGamePaused(World, false);

	// 移除菜单
	RemoveFromParent();

	UGameInstance* GameInstance = World->GetGameInstance();
	if (GameInstance)
	{
		TArray<ULocalPlayer*> AllLocalPlayers = GameInstance->GetLocalPlayers();
		for (int32 i = AllLocalPlayers.Num() - 1; i > 0; i--)
		{
			if (IsValid(AllLocalPlayers[i]))
				GameInstance->RemoveLocalPlayer(AllLocalPlayers[i]);
		}
	}

	// 返回单人闯关选择界面 (TankStageStartWidget)
	UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(GameInstance);
	if (GI)
	{
		GI->SetReturnToMenuType(EReturnToMenuType::SinglePlayerMenu);
	}

	FString LoadOptions = FString::Printf(TEXT("?GameMode=%s"), *AMainMenuGameMode::StaticClass()->GetName());
	UGameplayStatics::OpenLevel(World, FName(TEXT("MainMenuLevel_1")), true, LoadOptions);
}
