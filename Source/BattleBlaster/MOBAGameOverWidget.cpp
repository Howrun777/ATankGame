// Fill out your copyright notice in the Description page of Project Settings.

#include "MOBAGameOverWidget.h"
#include "TankMOBAGameState.h"
#include "TankMOBAPlayerState.h"
#include "BattleBlasterGameInstance.h"
#include "MainMenuGameMode.h"
#include "Kismet/GameplayStatics.h"

void UMOBAGameOverWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Restart)
	{
		Btn_Restart->OnClicked.AddDynamic(this, &UMOBAGameOverWidget::HandleRestartClicked);
	}

	if (Btn_ReturnMenu)
	{
		Btn_ReturnMenu->OnClicked.AddDynamic(this, &UMOBAGameOverWidget::HandleReturnMenuClicked);
	}
}

void UMOBAGameOverWidget::InitResultData()
{
	int32 LocalPlayerCount = 2;
	int32 LocalTargetScore = 7;

	if (UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		LocalPlayerCount = FMath::Clamp(GI->TargetPlayerCount, 2, 4);
		LocalTargetScore = GI->TargetMatchScore;
	}

	int32 WinningCampIndex = -1;
	if (UWorld* World = GetWorld())
	{
		if (ATankMOBAGameState* MGS = World->GetGameState<ATankMOBAGameState>())
		{
			WinningCampIndex = MGS->GetWinningCampIndex();
		}
	}

	UE_LOG(LogTemp, Display, TEXT("[MOBAGameOver] InitResultData: WinnerCamp=%d, PlayerCount=%d, TargetScore=%d"),
		WinningCampIndex, LocalPlayerCount, LocalTargetScore);

	if (Text_WinnerCamp && WinningCampIndex >= 0 && WinningCampIndex <= 3)
	{
		FText CampName;
		FLinearColor CampColor;
		GetCampInfo(WinningCampIndex, CampName, CampColor);
		Text_WinnerCamp->SetText(CampName);
		Text_WinnerCamp->SetColorAndOpacity(FSlateColor(CampColor));
	}

	if (WinningCampIndex >= 0)
	{
		TArray<AActor*> AllTanks;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATank::StaticClass(), AllTanks);

		for (AActor* Actor : AllTanks)
		{
			if (ATank* Tank = Cast<ATank>(Actor))
			{
				if (Tank->GetPlayerIndex() == WinningCampIndex && Img_TankPortrait)
				{
					TSubclassOf<ATank> WinnerClass = Tank->GetClass();
					if (UTexture2D** FoundTexture = TankPortraitMap.Find(WinnerClass))
					{
						if (*FoundTexture)
						{
							Img_TankPortrait->SetBrushFromTexture(*FoundTexture);
						}
					}
					break;
				}
			}
		}
	}

	TArray<int32> AllKills, AllDeaths, AllAssists;
	AllKills.Init(0, LocalPlayerCount);
	AllDeaths.Init(0, LocalPlayerCount);
	AllAssists.Init(0, LocalPlayerCount);

	TArray<AActor*> AllPlayerStates;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATankMOBAPlayerState::StaticClass(), AllPlayerStates);
	for (AActor* Actor : AllPlayerStates)
	{
		if (ATankMOBAPlayerState* PS = Cast<ATankMOBAPlayerState>(Actor))
		{
			const int32 PIndex = PS->PlayerIndex;
			if (PIndex >= 0 && PIndex < LocalPlayerCount)
			{
				AllKills[PIndex] = PS->KillCount;
				AllDeaths[PIndex] = PS->DeathCount;
				AllAssists[PIndex] = PS->AssistCount;
			}
		}
	}

	for (int32 Index = 0; Index < 4; ++Index)
	{
		UTextBlock* KDAText = nullptr;
		UTextBlock* SkillScoreText = nullptr;
		UPanelWidget* Row = nullptr;
		GetRowWidgets(Index, KDAText, SkillScoreText, Row);

		if (Index < LocalPlayerCount)
		{
			if (Row)
			{
				Row->SetVisibility(ESlateVisibility::Visible);
			}

			const int32 Kill = AllKills.IsValidIndex(Index) ? AllKills[Index] : 0;
			const int32 Death = AllDeaths.IsValidIndex(Index) ? AllDeaths[Index] : 0;
			const int32 Assist = AllAssists.IsValidIndex(Index) ? AllAssists[Index] : 0;

			if (KDAText)
			{
				KDAText->SetText(FText::FromString(FString::Printf(TEXT("%d-%d-%d"), Kill, Death, Assist)));
			}

			if (SkillScoreText && LocalTargetScore > 0)
			{
				const float Raw = (static_cast<float>(Kill) + static_cast<float>(Assist) * 0.5f - static_cast<float>(Death) * 0.3f) * 50.0f / static_cast<float>(LocalTargetScore);
				SkillScoreText->SetText(FText::AsNumber(FMath::Max(0, FMath::RoundToInt(Raw))));
			}
		}
		else
		{
			if (Row)
			{
				Row->SetVisibility(ESlateVisibility::Collapsed);
			}
			if (KDAText)
			{
				KDAText->SetText(FText::GetEmpty());
			}
			if (SkillScoreText)
			{
				SkillScoreText->SetText(FText::GetEmpty());
			}
		}
	}

	TArray<int32> RankIndices;
	RankIndices.Init(-1, LocalPlayerCount);

	if (UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		RankIndices = GI->AddMultiBattleHistoryRecordsFromMatch(
			LocalPlayerCount, AllKills, AllDeaths, AllAssists, LocalTargetScore);

		if (HistoryScrollBox)
		{
			HistoryScrollBox->ClearChildren();
			const int32 FontSize = FMath::Clamp(HistoryListFontSize, 12, 48);
			const TArray<FMultiBattleHistoryEntry>& History = GI->GetMultiBattleHistory();

			for (int32 Idx = 0; Idx < History.Num(); ++Idx)
			{
				const FMultiBattleHistoryEntry& E = History[Idx];

				UTextBlock* Line = NewObject<UTextBlock>(HistoryScrollBox);
				Line->SetText(FText::FromString(FString::Printf(TEXT("%2d | %3d | %2d - %2d - %2d"),
					Idx + 1, E.Score, E.Kills, E.Deaths, E.Assists)));

				FSlateFontInfo Fi = Line->GetFont();
				Fi.Size = FontSize;
				Line->SetFont(Fi);

				bool bFromThisRound = false;
				for (int32 R : RankIndices)
				{
					if (R == Idx)
					{
						bFromThisRound = true;
						break;
					}
				}

				if (bFromThisRound)
				{
					FLinearColor C;
					switch (E.CampIndex)
					{
					case 0: C = FLinearColor::Red; break;
					case 1: C = FLinearColor::Blue; break;
					case 2: C = FLinearColor::Green; break;
					default: C = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f); break;
					}
					Line->SetColorAndOpacity(FSlateColor(C));
				}
				else
				{
					Line->SetColorAndOpacity(FSlateColor(FLinearColor::White));
				}

				HistoryScrollBox->AddChild(Line);
			}
		}

		if (OutOfRangeText)
		{
			bool bAny = false;
			for (int32 R : RankIndices)
			{
				if (R >= 0)
				{
					bAny = true;
					break;
				}
			}
			OutOfRangeText->SetVisibility(bAny ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		}
	}
}

void UMOBAGameOverWidget::HandleRestartClicked()
{
	if (UWorld* World = GetWorld())
	{
		const FName CurrentLevelName(*World->GetName());
		UGameplayStatics::OpenLevel(World, CurrentLevelName, false);
	}
}

void UMOBAGameOverWidget::HandleReturnMenuClicked()
{
	if (UWorld* World = GetWorld())
	{
		if (UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(UGameplayStatics::GetGameInstance(World)))
		{
			GI->SetReturnToMenuType(EReturnToMenuType::MOBASetupMenu);
			GI->SetPendingMainMenuWidgetClass(MOBASetupMenuWidgetClass);
			if (!MOBASetupMenuWidgetClass)
			{
				UE_LOG(LogTemp, Warning, TEXT("[MOBAGameOver] MOBASetupMenuWidgetClass 未在蓝图中指定，主菜单将退回默认界面"));
			}
		}

		FString LoadOptions = FString::Printf(TEXT("?GameMode=%s"), *AMainMenuGameMode::StaticClass()->GetName());
		UGameplayStatics::OpenLevel(World, FName(TEXT("MainMenuLevel_1")), true, LoadOptions);
	}
}

void UMOBAGameOverWidget::GetCampInfo(int32 CampIndex, FText& OutCampName, FLinearColor& OutColor) const
{
	switch (CampIndex)
	{
	case 0:
		OutCampName = FText::FromString(TEXT("红色"));
		OutColor = FLinearColor::Red;
		break;
	case 1:
		OutCampName = FText::FromString(TEXT("蓝色"));
		OutColor = FLinearColor::Blue;
		break;
	case 2:
		OutCampName = FText::FromString(TEXT("绿色"));
		OutColor = FLinearColor::Green;
		break;
	case 3:
	default:
		OutCampName = FText::FromString(TEXT("黄色"));
		OutColor = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);
		break;
	}
}

void UMOBAGameOverWidget::GetRowWidgets(int32 PlayerIndex, UTextBlock*& OutKDAText, UTextBlock*& OutSkillScoreText, UPanelWidget*& OutRow) const
{
	OutKDAText = nullptr;
	OutSkillScoreText = nullptr;
	OutRow = nullptr;

	switch (PlayerIndex)
	{
	case 0: OutKDAText = RedKDAText; OutSkillScoreText = RedScoresText; OutRow = RedRow; break;
	case 1: OutKDAText = BlueKDAText; OutSkillScoreText = BlueScoresText; OutRow = BlueRow; break;
	case 2: OutKDAText = GreenKDAText; OutSkillScoreText = GreenScoresText; OutRow = GreenRow; break;
	default: OutKDAText = YellowKDAText; OutSkillScoreText = YellowScoresText; OutRow = YellowRow; break;
	}
}
