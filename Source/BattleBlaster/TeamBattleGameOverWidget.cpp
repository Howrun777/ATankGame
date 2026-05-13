// Fill out your copyright notice in the Description page of Project Settings.

#include "TeamBattleGameOverWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Kismet/GameplayStatics.h"
#include "BattleBlasterGameInstance.h"
#include "MainMenuGameMode.h"
#include "TeamBattlePlayerState.h"
#include "TeamBattleGameState.h"

void UTeamBattleGameOverWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Restart)
	{
		Btn_Restart->OnClicked.AddDynamic(this, &UTeamBattleGameOverWidget::HandleRestartClicked);
	}

	if (Btn_ReturnMenu)
	{
		Btn_ReturnMenu->OnClicked.AddDynamic(this, &UTeamBattleGameOverWidget::HandleReturnMenuClicked);
	}
}

void UTeamBattleGameOverWidget::InitResultData(int32 InWinnerCampIndex)
{
	// ---- 从 GameInstance 获取配置 ----
	int32 LocalPlayerCount = 4;
	int32 LocalTargetScore = 7;
	int32 LocalRedScore = 0;
	int32 LocalBlueScore = 0;

	if (UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		LocalTargetScore = GI->TargetMatchScore;
		// 团队模式固定 4 人（2红+2蓝）
		LocalPlayerCount = 4;
	}

	// ---- 从 GameState 读取阵营分数 ----
	if (UWorld* World = GetWorld())
	{
		if (ATeamBattleGameState* TBGS = Cast<ATeamBattleGameState>(World->GetGameState()))
		{
			LocalRedScore = TBGS->GetRedTeamScore();
			LocalBlueScore = TBGS->GetBlueTeamScore();
		}
	}

	UE_LOG(LogTemp, Display, TEXT("[TeamBattleGameOver] WinnerCamp=%d, PlayerCount=%d, TargetScore=%d, Red=%d, Blue=%d"),
		InWinnerCampIndex, LocalPlayerCount, LocalTargetScore, LocalRedScore, LocalBlueScore);

	// ---- 1. 胜利阵营文字和颜色 ----
	if (Text_WinnerCamp)
	{
		FText WinnerText;
		FLinearColor WinnerColor;
		if (InWinnerCampIndex == 0)
		{
			WinnerText = FText::FromString(TEXT("红色"));
			WinnerColor = FLinearColor::Red;
		}
		else
		{
			WinnerText = FText::FromString(TEXT("蓝色"));
			WinnerColor = FLinearColor::Blue;
		}
		Text_WinnerCamp->SetText(WinnerText);
		Text_WinnerCamp->SetColorAndOpacity(FSlateColor(WinnerColor));
	}

	// ---- 2. 显示阵营总分 ----
	if (Text_RedTeamScore)
	{
		Text_RedTeamScore->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), LocalRedScore, LocalTargetScore)));
	}
	if (Text_BlueTeamScore)
	{
		Text_BlueTeamScore->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), LocalBlueScore, LocalTargetScore)));
	}

	// ---- 3. 从 PlayerState 读取所有玩家的 KDA ----
	TArray<int32> AllKills, AllDeaths, AllAssists;
	AllKills.Init(0, LocalPlayerCount);
	AllDeaths.Init(0, LocalPlayerCount);
	AllAssists.Init(0, LocalPlayerCount);

	TArray<AActor*> AllPSActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerState::StaticClass(), AllPSActors);
	for (AActor* Actor : AllPSActors)
	{
		if (ATeamBattlePlayerState* PS = Cast<ATeamBattlePlayerState>(Actor))
		{
			const int32 PIndex = PS->PlayerIndex;
			if (PIndex >= 0 && PIndex < LocalPlayerCount)
			{
				AllKills[PIndex]   = PS->KillCount;
				AllDeaths[PIndex]  = PS->DeathCount;
				AllAssists[PIndex] = PS->AssistCount;
				UE_LOG(LogTemp, Display, TEXT("[TeamBattleGameOver] PlayerState[%d]: K=%d D=%d A=%d"),
					PIndex, PS->KillCount, PS->DeathCount, PS->AssistCount);
			}
		}
	}

	// ---- 4. 显示战绩行：按 PlayerIndex (0,1,2,3) 排列 ----
	for (int32 Idx = 0; Idx < 4; ++Idx)
	{
		UTextBlock* KDAText = nullptr;
		UTextBlock* SkillScoreText = nullptr;
		UPanelWidget* Row = nullptr;
		GetRowWidgets(Idx, KDAText, SkillScoreText, Row);

		if (Idx < LocalPlayerCount)
		{
			if (Row)
			{
				Row->SetVisibility(ESlateVisibility::Visible);
			}

			const int32 Kill   = AllKills.IsValidIndex(Idx)   ? AllKills[Idx]   : 0;
			const int32 Death  = AllDeaths.IsValidIndex(Idx)  ? AllDeaths[Idx]  : 0;
			const int32 Assist = AllAssists.IsValidIndex(Idx) ? AllAssists[Idx] : 0;

			FText CampName;
			FLinearColor CampColor;
			GetCampInfo(Idx, CampName, CampColor);

			if (KDAText)
			{
				KDAText->SetText(FText::FromString(FString::Printf(TEXT("%d-%d-%d"), Kill, Death, Assist)));
				KDAText->SetColorAndOpacity(FSlateColor(CampColor));
			}

			// SkillScore = (Kill + Assist/2 - Death) * 50 / TargetScore
			if (SkillScoreText && LocalTargetScore > 0)
			{
				const float Raw = (static_cast<float>(Kill) + static_cast<float>(Assist) * 0.5f - static_cast<float>(Death) * 0.3f) * 50.0f / static_cast<float>(LocalTargetScore);
				SkillScoreText->SetText(FText::AsNumber(FMath::Max(0, FMath::RoundToInt(Raw))));
				SkillScoreText->SetColorAndOpacity(FSlateColor(CampColor));
			}
		}
		else
		{
			if (Row)
			{
				Row->SetVisibility(ESlateVisibility::Collapsed);
			}
			if (KDAText)         KDAText->SetText(FText::GetEmpty());
			if (SkillScoreText)   SkillScoreText->SetText(FText::GetEmpty());
		}
	}

	// ---- 5. 找出评分最高的坦克，显示头像 ----
	if (Img_TankPortrait && TankPortraitMap.Num() > 0 && LocalTargetScore > 0)
	{
		TArray<AActor*> AllTanks;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATank::StaticClass(), AllTanks);

		int32 BestPlayerIndex = 0;
		int32 BestScore = -1;
		ATank* BestTank = nullptr;

		for (AActor* Actor : AllTanks)
		{
			if (ATank* Tank = Cast<ATank>(Actor))
			{
				const int32 Idx = Tank->GetPlayerIndex();
				if (Idx < 0 || Idx >= LocalPlayerCount) continue;

				const int32 K = AllKills.IsValidIndex(Idx) ? AllKills[Idx] : 0;
				const int32 D = AllDeaths.IsValidIndex(Idx) ? AllDeaths[Idx] : 0;
				const int32 A = AllAssists.IsValidIndex(Idx) ? AllAssists[Idx] : 0;
				const int32 S = FMath::Max(0, FMath::RoundToInt(
					(static_cast<float>(K) + static_cast<float>(A) * 0.5f - static_cast<float>(D)) * 50.0f / static_cast<float>(LocalTargetScore)));

				if (S > BestScore)
				{
					BestScore = S;
					BestPlayerIndex = Idx;
					BestTank = Tank;
				}
			}
		}

		if (BestTank)
		{
			if (UTexture2D* const* Found = TankPortraitMap.Find(BestTank->GetClass()))
			{
				if (Found && *Found)
				{
					Img_TankPortrait->SetBrushFromTexture(*Found);
				}
			}
		}
	}

	// ---- 6. 历史榜单 ----
	if (LocalTargetScore > 0)
	{
		if (UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(UGameplayStatics::GetGameInstance(this)))
		{
			const TArray<int32> RankIndices = GI->AddMultiBattleHistoryRecordsFromMatch(
				LocalPlayerCount, AllKills, AllDeaths, AllAssists, LocalTargetScore);

			const TArray<FMultiBattleHistoryEntry>& History = GI->GetMultiBattleHistory();

			if (HistoryScrollBox)
			{
				HistoryScrollBox->ClearChildren();
				const int32 HistoryFontSize = FMath::Clamp(HistoryListFontSize, 12, 48);

				for (int32 HIdx = 0; HIdx < History.Num(); ++HIdx)
				{
					const FMultiBattleHistoryEntry& Entry = History[HIdx];
					const FString LineText = FString::Printf(TEXT("%2d | %3d | %2d - %2d - %2d"),
						HIdx + 1, Entry.Score, Entry.Kills, Entry.Deaths, Entry.Assists);

					UTextBlock* LineWidget = NewObject<UTextBlock>(HistoryScrollBox);
					LineWidget->SetText(FText::FromString(LineText));
					FSlateFontInfo FontInfo = LineWidget->GetFont();
					FontInfo.Size = HistoryFontSize;
					LineWidget->SetFont(FontInfo);

					// 本局成绩高亮：阵营颜色（Entry.CampIndex 在历史记录中已保存阵营）
					bool bFromThisRound = false;
					for (int32 R : RankIndices)
					{
						if (R == HIdx) { bFromThisRound = true; break; }
					}

					if (bFromThisRound)
					{
						FLinearColor CampColor = (Entry.CampIndex == 0 || Entry.CampIndex == 2)
							? FLinearColor::Red : FLinearColor::Blue;
						LineWidget->SetColorAndOpacity(FSlateColor(CampColor));
					}
					else
					{
						LineWidget->SetColorAndOpacity(FSlateColor(FLinearColor::White));
					}

					HistoryScrollBox->AddChild(LineWidget);
				}
			}

			if (OutOfRangeText)
			{
				bool bAnyInTop50 = false;
				for (int32 R : RankIndices)
				{
					if (R >= 0) { bAnyInTop50 = true; break; }
				}
				if (!bAnyInTop50)
				{
					OutOfRangeText->SetText(FText::FromString(TEXT("未进入历史榜单")));
					OutOfRangeText->SetVisibility(ESlateVisibility::Visible);
				}
				else
				{
					OutOfRangeText->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}
	}
}

void UTeamBattleGameOverWidget::GetCampInfo(int32 PlayerIndex, FText& OutCampName, FLinearColor& OutColor) const
{
	// 团队模式：0和2=红，1和3=蓝
	if (PlayerIndex == 0 || PlayerIndex == 2)
	{
		OutCampName = FText::FromString(PlayerIndex == 0 ? TEXT("红1") : TEXT("红2"));
		OutColor = FLinearColor::Red;
	}
	else
	{
		OutCampName = FText::FromString(PlayerIndex == 1 ? TEXT("蓝1") : TEXT("蓝2"));
		OutColor = FLinearColor::Blue;
	}
}

void UTeamBattleGameOverWidget::GetRowWidgets(int32 PlayerIndex,
	UTextBlock*& OutKDAText,
	UTextBlock*& OutSkillScoreText,
	UPanelWidget*& OutRow) const
{
	OutKDAText = nullptr;
	OutSkillScoreText = nullptr;
	OutRow = nullptr;

	switch (PlayerIndex)
	{
	case 0:
		OutKDAText = RedKDAText_1;
		OutSkillScoreText = RedSkillScoreText_1;
		OutRow = RedRow_1;
		break;
	case 2:
		OutKDAText = RedKDAText_2;
		OutSkillScoreText = RedSkillScoreText_2;
		OutRow = RedRow_2;
		break;
	case 1:
		OutKDAText = BlueKDAText_1;
		OutSkillScoreText = BlueSkillScoreText_1;
		OutRow = BlueRow_1;
		break;
	case 3:
	default:
		OutKDAText = BlueKDAText_2;
		OutSkillScoreText = BlueSkillScoreText_2;
		OutRow = BlueRow_2;
		break;
	}
}

void UTeamBattleGameOverWidget::HandleRestartClicked()
{
	UWorld* World = GetWorld();
	if (!World) return;
	FString CurrentLevel = World->GetName();
	UGameplayStatics::OpenLevel(World, FName(*CurrentLevel), false);
}

void UTeamBattleGameOverWidget::HandleReturnMenuClicked()
{
	if (UWorld* World = GetWorld())
	{
		FString LoadOptions = FString::Printf(TEXT("?GameMode=%s"), *AMainMenuGameMode::StaticClass()->GetName());
		UGameplayStatics::OpenLevel(World, FName(TEXT("MainMenuLevel_1")), true, LoadOptions);
	}
}
