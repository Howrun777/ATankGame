// Fill out your copyright notice in the Description page of Project Settings.

#include "Modes/FreeForAll/UI/MultiBattleGameOverWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Modes/MainMenu/MainMenuGameMode.h"
#include "Core/BattleBlasterGameInstance.h"
#include "Modes/FreeForAll/TankBattlePlayerState.h"

void UMultiBattleGameOverWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UE_LOG(LogTemp, Display, TEXT("[GameOverUI] NativeConstruct called"));

	if (Btn_Restart)
	{
		Btn_Restart->OnClicked.AddDynamic(this, &UMultiBattleGameOverWidget::HandleRestartClicked);
	}

	if (Btn_ReturnMenu)
	{
		Btn_ReturnMenu->OnClicked.AddDynamic(this, &UMultiBattleGameOverWidget::HandleReturnMenuClicked);
	}
}

void UMultiBattleGameOverWidget::InitResultData(int32 InWinnerIndex)
{
	// ---- 从 GameInstance 获取玩家数量和目标分数 ----
	int32 LocalPlayerCount = 2;
	int32 LocalTargetScore = 7;

	if (UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		LocalPlayerCount = FMath::Clamp(GI->TargetPlayerCount, 2, 4);
		LocalTargetScore = GI->TargetMatchScore;
	}

	UE_LOG(LogTemp, Error, TEXT("[GameOverUI] InitResultData: WinnerIndex=%d, PlayerCount=%d, TargetScore=%d"),
		InWinnerIndex, LocalPlayerCount, LocalTargetScore);

	// ---- 1. 胜利阵营文字和颜色 ----
	if (Text_WinnerCamp && InWinnerIndex >= 0)
	{
		FText CampName;
		FLinearColor CampColor;
		//根据传进来的玩家索引确定阵营名字和颜色
		GetCampInfo(InWinnerIndex, CampName, CampColor);
		Text_WinnerCamp->SetText(CampName);
		Text_WinnerCamp->SetColorAndOpacity(FSlateColor(CampColor));
	}

	// ---- 2. 胜利坦克头像 ----
	if (InWinnerIndex >= 0)
	{
		TArray<AActor*> AllTanks;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATank::StaticClass(), AllTanks);

		for (AActor* Actor : AllTanks)
		{
			if (ATank* Tank = Cast<ATank>(Actor))
			{
				if (Tank->GetSlotId() == InWinnerIndex && Img_TankPortrait)
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

	// ---- 3. 批量收集所有玩家的 KDA ----
	TArray<int32> AllKills, AllDeaths, AllAssists;
	AllKills.Init(0, LocalPlayerCount);
	AllDeaths.Init(0, LocalPlayerCount);
	AllAssists.Init(0, LocalPlayerCount);

	{
		// 1. 定义一个动态数组，用于临时存储从场景中找到的所有"玩家状态对象"
		TArray<AActor*> AllPlayerStates;
		// 2. 调用UE工具类函数，从当前世界中"抓出"所有属于我们自定义玩家状态类的对象
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATankBattlePlayerState::StaticClass(), AllPlayerStates);
		// 3. 遍历数组：把刚才抓到的每个对象，依次拿出来处理
		for (AActor* Actor : AllPlayerStates)
		{
			// 4. 【关键步骤】安全类型转换：把基类AActor*，转回我们的具体类ATankBattlePlayerState*
			if (ATankBattlePlayerState* TankPS = Cast<ATankBattlePlayerState>(Actor))
			{
				// 5. 从玩家状态里，读取"玩家索引"（用来区分不同玩家，比如0=玩家1，1=玩家2）
				const int32 PIndex = TankPS->SlotId;
				// 6. 【安全检查】验证索引是否合法：防止数组越界崩溃
				if (PIndex >= 0 && PIndex < LocalPlayerCount)
				{
					// 7. 把战绩数据，按玩家索引"对号入座"存进数组（这些数组是你在类里提前定义好的）
					AllKills[PIndex]   = TankPS->KillCount;
					AllDeaths[PIndex]  = TankPS->DeathCount;
					AllAssists[PIndex] = TankPS->AssistCount;
					UE_LOG(LogTemp, Display, TEXT("[GameOverUI] Read PlayerState[%d]: K=%d D=%d A=%d"),
						PIndex, TankPS->KillCount, TankPS->DeathCount, TankPS->AssistCount);
				}
			}
		}
	}

	// ---- 4. 更新左侧战绩行：KDA + SkillScore ----
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

			const int32 Kill   = AllKills.IsValidIndex(Index)   ? AllKills[Index]   : 0;
			const int32 Death  = AllDeaths.IsValidIndex(Index)  ? AllDeaths[Index]  : 0;
			const int32 Assist = AllAssists.IsValidIndex(Index) ? AllAssists[Index] : 0;

			UE_LOG(LogTemp, Display, TEXT("[GameOverUI] Player %d KDA: K=%d D=%d A=%d"), Index, Kill, Death, Assist);

			if (KDAText)
			{
				KDAText->SetText(FText::FromString(FString::Printf(TEXT("%d-%d-%d"), Kill, Death, Assist)));
			}

			// SkillScore = (Kill + Assist/2 - Death) * 50 / TargetScore
			if (SkillScoreText && LocalTargetScore > 0)
			{
				const float Raw = (static_cast<float>(Kill) + static_cast<float>(Assist) * 0.5f - static_cast<float>(Death)*0.3f) * 50.0f / static_cast<float>(LocalTargetScore);
				SkillScoreText->SetText(FText::AsNumber(FMath::Max(0, FMath::RoundToInt(Raw))));
			}
		}
		else
		{
			if (Row)
			{
				Row->SetVisibility(ESlateVisibility::Collapsed);
			}
			if (KDAText)     KDAText->SetText(FText::GetEmpty());
			if (SkillScoreText) SkillScoreText->SetText(FText::GetEmpty());
		}
	}

	// ---- 5. 写入历史战绩并获取本局排名 ----
	TArray<int32> RankIndices;
	RankIndices.Init(-1, LocalPlayerCount);

	if (UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		RankIndices = GI->AddMultiBattleHistoryRecordsFromMatch(
			LocalPlayerCount, AllKills, AllDeaths, AllAssists, LocalTargetScore);

		UE_LOG(LogTemp, Display, TEXT("[GameOverUI] History written, ranks: %s"),
			*FString::JoinBy(RankIndices, TEXT(","), [](int32 R) { return FString::FromInt(R); }));

		// ---- 6. 构建历史榜单 ----
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
					if (R == Idx) { bFromThisRound = true; break; }
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

		// 本局无人进入前 50 时显示提示
		if (OutOfRangeText)
		{
			bool bAny = false;
			for (int32 R : RankIndices) { if (R >= 0) { bAny = true; break; } }
			OutOfRangeText->SetVisibility(bAny ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		}
	}
}

void UMultiBattleGameOverWidget::HandleRestartClicked()
{
	if (UWorld* World = GetWorld())
	{
		const FName CurrentLevelName(*World->GetName());
		UGameplayStatics::OpenLevel(World, CurrentLevelName, false);
	}
}

void UMultiBattleGameOverWidget::HandleReturnMenuClicked()
{
	if (UWorld* World = GetWorld())
	{
		FString LoadOptions = FString::Printf(TEXT("?GameMode=%s"), *AMainMenuGameMode::StaticClass()->GetName());
		UGameplayStatics::OpenLevel(World, FName(TEXT("MainMenuLevel_1")), true, LoadOptions);
	}
}

void UMultiBattleGameOverWidget::GetCampInfo(int32 SlotId, FText& OutCampName, FLinearColor& OutColor) const
{
	switch (SlotId)
	{
	case 0: OutCampName = FText::FromString(TEXT("红色")); OutColor = FLinearColor::Red; break;
	case 1: OutCampName = FText::FromString(TEXT("蓝色")); OutColor = FLinearColor::Blue; break;
	case 2: OutCampName = FText::FromString(TEXT("绿色")); OutColor = FLinearColor::Green; break;
	default: OutCampName = FText::FromString(TEXT("黄色")); OutColor = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f); break;
	}
}

void UMultiBattleGameOverWidget::GetRowWidgets(int32 SlotId, UTextBlock*& OutKDAText, UTextBlock*& OutSkillScoreText, UPanelWidget*& OutRow) const
{
	OutKDAText = nullptr;
	OutSkillScoreText = nullptr;
	OutRow = nullptr;

	switch (SlotId)
	{
	case 0: OutKDAText = RedKDAText; OutSkillScoreText = RedScoresText; OutRow = RedRow; break;
	case 1: OutKDAText = BlueKDAText; OutSkillScoreText = BlueScoresText; OutRow = BlueRow; break;
	case 2: OutKDAText = GreenKDAText; OutSkillScoreText = GreenScoresText; OutRow = GreenRow; break;
	default: OutKDAText = YellowKDAText; OutSkillScoreText = YellowScoresText; OutRow = YellowRow; break;
	}
}
