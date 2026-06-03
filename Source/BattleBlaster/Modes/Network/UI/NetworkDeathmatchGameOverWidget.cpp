#include "Modes/Network/UI/NetworkDeathmatchGameOverWidget.h"

#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Modes/MainMenu/MainMenuGameMode.h"
#include "Modes/Network/NetworkDeathmatchGameState.h"
#include "Shared/Pawns/Tank.h"
#include "Shared/State/TankPlayerState.h"
#include "Blueprint/WidgetTree.h"

TSharedRef<SWidget> UNetworkDeathmatchGameOverWidget::RebuildWidget()
{
	BuildTemporaryWidgetTree();

	return Super::RebuildWidget();
}

void UNetworkDeathmatchGameOverWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Restart)
	{
		Btn_Restart->OnClicked.AddDynamic(this, &UNetworkDeathmatchGameOverWidget::HandleRestartClicked);
	}

	if (Btn_ReturnMenu)
	{
		Btn_ReturnMenu->OnClicked.AddDynamic(this, &UNetworkDeathmatchGameOverWidget::HandleReturnMenuClicked);
	}

	if (TempReturnMenuButton)
	{
		TempReturnMenuButton->OnClicked.AddDynamic(this, &UNetworkDeathmatchGameOverWidget::HandleReturnMenuClicked);
	}
}

void UNetworkDeathmatchGameOverWidget::InitResultData(int32 InWinnerSlotId)
{
	const ANetworkDeathmatchGameState* DeathmatchGameState = GetDeathmatchGameState();
	const int32 PlayerCount = DeathmatchGameState ? FMath::Clamp(DeathmatchGameState->PlayerScores.Num(), 2, 4) : 2;
	const int32 TargetScore = DeathmatchGameState ? DeathmatchGameState->TargetScore : 7;

	if (Text_WinnerCamp && InWinnerSlotId >= 0)
	{
		FText SlotName;
		FLinearColor SlotColor;
		GetSlotInfo(InWinnerSlotId, SlotName, SlotColor);
		Text_WinnerCamp->SetText(SlotName);
		Text_WinnerCamp->SetColorAndOpacity(FSlateColor(SlotColor));
	}
	if (TempWinnerText && InWinnerSlotId >= 0)
	{
		FText SlotName;
		FLinearColor SlotColor;
		GetSlotInfo(InWinnerSlotId, SlotName, SlotColor);
		TempWinnerText->SetText(FText::Format(FText::FromString(TEXT("{0} Wins")), SlotName));
		TempWinnerText->SetColorAndOpacity(FSlateColor(SlotColor));
	}
	if (TempRowsText)
	{
		TempRowsText->SetText(FText::FromString(BuildResultRowsText(PlayerCount, TargetScore)));
	}

	if (Img_TankPortrait && InWinnerSlotId >= 0)
	{
		TArray<AActor*> AllTanks;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATank::StaticClass(), AllTanks);
		for (AActor* Actor : AllTanks)
		{
			const ATank* Tank = Cast<ATank>(Actor);
			if (Tank && Tank->GetSlotId() == InWinnerSlotId)
			{
				if (UTexture2D* const* FoundTexture = TankPortraitMap.Find(Tank->GetClass()))
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

	for (int32 SlotId = 0; SlotId < 4; ++SlotId)
	{
		UTextBlock* KDAText = nullptr;
		UTextBlock* ScoreText = nullptr;
		UPanelWidget* Row = nullptr;
		GetRowWidgets(SlotId, KDAText, ScoreText, Row);

		if (SlotId >= PlayerCount)
		{
			if (Row)
			{
				Row->SetVisibility(ESlateVisibility::Collapsed);
			}
			if (KDAText)
			{
				KDAText->SetText(FText::GetEmpty());
			}
			if (ScoreText)
			{
				ScoreText->SetText(FText::GetEmpty());
			}
			continue;
		}

		if (Row)
		{
			Row->SetVisibility(ESlateVisibility::Visible);
		}

		int32 Kills = 0;
		int32 Deaths = 0;
		int32 Assists = 0;
		GetKDAForSlot(SlotId, Kills, Deaths, Assists);

		if (KDAText)
		{
			KDAText->SetText(FText::FromString(FString::Printf(TEXT("%d-%d-%d"), Kills, Deaths, Assists)));
		}

		if (ScoreText)
		{
			ScoreText->SetText(FText::AsNumber(CalculateSkillScore(Kills, Deaths, Assists, TargetScore)));
		}
	}

	if (HistoryScrollBox)
	{
		HistoryScrollBox->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (OutOfRangeText)
	{
		OutOfRangeText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UNetworkDeathmatchGameOverWidget::BuildTemporaryWidgetTree()
{
	if (HasBlueprintBoundLayout() || TempRootCanvas)
	{
		return;
	}

	TempRootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Temp_NetworkDeathmatchRoot"));
	WidgetTree->RootWidget = TempRootCanvas;

	UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Temp_Background"));
	Background->SetColorAndOpacity(FLinearColor(0.02f, 0.02f, 0.025f, 0.88f));
	if (UCanvasPanelSlot* BackgroundSlot = TempRootCanvas->AddChildToCanvas(Background))
	{
		BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackgroundSlot->SetOffsets(FMargin(0.0f));
	}

	TempContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Temp_Content"));
	if (UCanvasPanelSlot* ContentSlot = TempRootCanvas->AddChildToCanvas(TempContentBox))
	{
		ContentSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		ContentSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		ContentSlot->SetPosition(FVector2D(0.0f, 0.0f));
		ContentSlot->SetSize(FVector2D(760.0f, 460.0f));
	}

	TempTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Temp_Title"));
	TempTitleText->SetText(FText::FromString(TEXT("Deathmatch Result")));
	TempTitleText->SetJustification(ETextJustify::Center);
	TempTitleText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo TitleFont = TempTitleText->GetFont();
	TitleFont.Size = 44;
	TempTitleText->SetFont(TitleFont);
	if (UVerticalBoxSlot* TitleSlot = TempContentBox->AddChildToVerticalBox(TempTitleText))
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 26.0f));
		TitleSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	TempWinnerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Temp_Winner"));
	TempWinnerText->SetText(FText::FromString(TEXT("Winner Pending")));
	TempWinnerText->SetJustification(ETextJustify::Center);
	FSlateFontInfo WinnerFont = TempWinnerText->GetFont();
	WinnerFont.Size = 34;
	TempWinnerText->SetFont(WinnerFont);
	if (UVerticalBoxSlot* WinnerSlot = TempContentBox->AddChildToVerticalBox(TempWinnerText))
	{
		WinnerSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));
		WinnerSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	TempRowsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Temp_Rows"));
	TempRowsText->SetText(FText::FromString(TEXT("Waiting for replicated results...")));
	TempRowsText->SetJustification(ETextJustify::Center);
	TempRowsText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.94f, 1.0f, 1.0f)));
	FSlateFontInfo RowsFont = TempRowsText->GetFont();
	RowsFont.Size = 24;
	TempRowsText->SetFont(RowsFont);
	if (UVerticalBoxSlot* RowsSlot = TempContentBox->AddChildToVerticalBox(TempRowsText))
	{
		RowsSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 34.0f));
		RowsSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	TempReturnMenuButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Temp_ReturnMenuButton"));
	TempReturnMenuText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Temp_ReturnMenuText"));
	TempReturnMenuText->SetText(FText::FromString(TEXT("Return Menu")));
	TempReturnMenuText->SetJustification(ETextJustify::Center);
	FSlateFontInfo ButtonFont = TempReturnMenuText->GetFont();
	ButtonFont.Size = 22;
	TempReturnMenuText->SetFont(ButtonFont);
	TempReturnMenuButton->AddChild(TempReturnMenuText);
	if (UVerticalBoxSlot* ButtonSlot = TempContentBox->AddChildToVerticalBox(TempReturnMenuButton))
	{
		ButtonSlot->SetHorizontalAlignment(HAlign_Center);
	}
}

bool UNetworkDeathmatchGameOverWidget::HasBlueprintBoundLayout() const
{
	return Text_WinnerCamp || RedKDAText || BlueKDAText || GreenKDAText || YellowKDAText || Btn_ReturnMenu;
}

FString UNetworkDeathmatchGameOverWidget::BuildResultRowsText(int32 PlayerCount, int32 TargetScore) const
{
	TArray<FString> Lines;
	for (int32 SlotId = 0; SlotId < PlayerCount; ++SlotId)
	{
		int32 Kills = 0;
		int32 Deaths = 0;
		int32 Assists = 0;
		GetKDAForSlot(SlotId, Kills, Deaths, Assists);

		FText SlotName;
		FLinearColor UnusedColor;
		GetSlotInfo(SlotId, SlotName, UnusedColor);

		const int32 SkillScore = CalculateSkillScore(Kills, Deaths, Assists, TargetScore);
		Lines.Add(FString::Printf(TEXT("%s     KDA %d-%d-%d     Score %d"),
			*SlotName.ToString(),
			Kills,
			Deaths,
			Assists,
			SkillScore));
	}

	return FString::Join(Lines, TEXT("\n"));
}

void UNetworkDeathmatchGameOverWidget::HandleRestartClicked()
{
	// Network restart should be routed through a future session/menu flow.
}

void UNetworkDeathmatchGameOverWidget::HandleReturnMenuClicked()
{
	if (UWorld* World = GetWorld())
	{
		const FString LoadOptions = FString::Printf(TEXT("?GameMode=%s"), *AMainMenuGameMode::StaticClass()->GetName());
		UGameplayStatics::OpenLevel(World, FName(TEXT("MainMenuLevel_1")), true, LoadOptions);
	}
}

void UNetworkDeathmatchGameOverWidget::GetSlotInfo(int32 SlotId, FText& OutName, FLinearColor& OutColor) const
{
	switch (SlotId)
	{
	case 0:
		OutName = FText::FromString(TEXT("Player 1"));
		OutColor = FLinearColor::Red;
		break;
	case 1:
		OutName = FText::FromString(TEXT("Player 2"));
		OutColor = FLinearColor::Blue;
		break;
	case 2:
		OutName = FText::FromString(TEXT("Player 3"));
		OutColor = FLinearColor::Green;
		break;
	default:
		OutName = FText::FromString(TEXT("Player 4"));
		OutColor = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);
		break;
	}
}

void UNetworkDeathmatchGameOverWidget::GetRowWidgets(int32 SlotId, UTextBlock*& OutKDAText, UTextBlock*& OutScoreText, UPanelWidget*& OutRow) const
{
	OutKDAText = nullptr;
	OutScoreText = nullptr;
	OutRow = nullptr;

	switch (SlotId)
	{
	case 0:
		OutKDAText = RedKDAText;
		OutScoreText = RedScoresText;
		OutRow = RedRow;
		break;
	case 1:
		OutKDAText = BlueKDAText;
		OutScoreText = BlueScoresText;
		OutRow = BlueRow;
		break;
	case 2:
		OutKDAText = GreenKDAText;
		OutScoreText = GreenScoresText;
		OutRow = GreenRow;
		break;
	default:
		OutKDAText = YellowKDAText;
		OutScoreText = YellowScoresText;
		OutRow = YellowRow;
		break;
	}
}

int32 UNetworkDeathmatchGameOverWidget::CalculateSkillScore(int32 Kills, int32 Deaths, int32 Assists, int32 TargetScore) const
{
	if (TargetScore <= 0)
	{
		return 0;
	}

	const float RawScore = (static_cast<float>(Kills) + static_cast<float>(Assists) * 0.5f - static_cast<float>(Deaths) * 0.3f) * 50.0f / static_cast<float>(TargetScore);
	return FMath::Max(0, FMath::RoundToInt(RawScore));
}

void UNetworkDeathmatchGameOverWidget::GetKDAForSlot(int32 SlotId, int32& OutKills, int32& OutDeaths, int32& OutAssists) const
{
	OutKills = 0;
	OutDeaths = 0;
	OutAssists = 0;

	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GameState)
	{
		return;
	}

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		const ATankPlayerState* TankPS = Cast<ATankPlayerState>(PlayerState);
		if (TankPS && TankPS->GetSlotId() == SlotId)
		{
			OutKills = TankPS->KillCount;
			OutDeaths = TankPS->DeathCount;
			OutAssists = TankPS->AssistCount;
			return;
		}
	}
}

const ANetworkDeathmatchGameState* UNetworkDeathmatchGameOverWidget::GetDeathmatchGameState() const
{
	return GetWorld() ? GetWorld()->GetGameState<ANetworkDeathmatchGameState>() : nullptr;
}
