#include "Modes/Network/UI/CppShowScoresWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

UCppShowScoresWidget::UCppShowScoresWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PanelColor = FLinearColor(0.018f, 0.020f, 0.024f, 0.78f);
	HeaderTextColor = FLinearColor(0.94f, 0.96f, 0.98f, 1.0f);
	OtherPlayerTextColor = FLinearColor(0.66f, 0.68f, 0.72f, 1.0f);
	OtherPlayerFillColor = FLinearColor(0.18f, 0.19f, 0.21f, 1.0f);
	LocalPlayerTextColor = FLinearColor(1.0f, 0.92f, 0.90f, 1.0f);
	LocalPlayerFillColor = FLinearColor(0.92f, 0.12f, 0.10f, 1.0f);
	ScoreTrackColor = FLinearColor(0.055f, 0.060f, 0.068f, 1.0f);
}

TSharedRef<SWidget> UCppShowScoresWidget::RebuildWidget()
{
	BuildDefaultWidgetTree();
	return Super::RebuildWidget();
}

void UCppShowScoresWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshDisplay();
}

void UCppShowScoresWidget::SetTargetScore(int32 InTargetScore)
{
	TargetScore = FMath::Max(1, InTargetScore);
	RefreshDisplay();
}

void UCppShowScoresWidget::SetElapsedTime(int32 InElapsedSeconds)
{
	ElapsedSeconds = FMath::Max(0, InElapsedSeconds);
	RefreshDisplay();
}

void UCppShowScoresWidget::SetScores(const TArray<int32>& InScores)
{
	CurrentScores = InScores;
	RefreshDisplay();
}

void UCppShowScoresWidget::SetLocalPlayerSlotId(int32 InSlotId)
{
	LocalPlayerSlotId = InSlotId;
	RefreshDisplay();
}

void UCppShowScoresWidget::UpdateScoreboard(int32 InTargetScore, int32 InElapsedSeconds, const TArray<int32>& InScores, int32 InLocalSlotId)
{
	TargetScore = FMath::Max(1, InTargetScore);
	ElapsedSeconds = FMath::Max(0, InElapsedSeconds);
	CurrentScores = InScores;
	LocalPlayerSlotId = InLocalSlotId;
	RefreshDisplay();
}

void UCppShowScoresWidget::RefreshDisplay()
{
	if (TargetAndTimeText)
	{
		TargetAndTimeText->SetText(FormatHeaderText());
	}

	if (PlayerScoresBox)
	{
		RebuildGeneratedScoreBars();
		for (int32 SlotId = 0; SlotId < GeneratedScoreBars.Num(); ++SlotId)
		{
			ApplyScoreBarState(SlotId);
		}
	}
}

void UCppShowScoresWidget::BuildDefaultWidgetTree()
{
	if (HasBlueprintLayout() || GeneratedRootCanvas)
	{
		return;
	}

	GeneratedRootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CppScoresRoot"));
	WidgetTree->RootWidget = GeneratedRootCanvas;

	GeneratedPanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CppScoresPanel"));
	GeneratedPanelBorder->SetBrushColor(PanelColor);
	GeneratedPanelBorder->SetPadding(FMargin(18.0f, 12.0f, 18.0f, 12.0f));

	if (UCanvasPanelSlot* PanelSlot = GeneratedRootCanvas->AddChildToCanvas(GeneratedPanelBorder))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.0f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		PanelSlot->SetPosition(FVector2D(0.0f, 18.0f));
		PanelSlot->SetAutoSize(true);
	}

	GeneratedPanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CppScoresPanelBox"));
	GeneratedPanelBorder->SetContent(GeneratedPanelBox);

	TargetAndTimeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TargetAndTimeText"));
	TargetAndTimeText->SetText(FormatHeaderText());
	ApplyTextStyle(TargetAndTimeText, HeaderFontSize, HeaderTextColor, ETextJustify::Center);
	if (UVerticalBoxSlot* HeaderSlot = GeneratedPanelBox->AddChildToVerticalBox(TargetAndTimeText))
	{
		HeaderSlot->SetHorizontalAlignment(HAlign_Fill);
		HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 9.0f));
	}

	PlayerScoresBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PlayerScoresBox"));
	if (UVerticalBoxSlot* ScoresSlot = GeneratedPanelBox->AddChildToVerticalBox(PlayerScoresBox))
	{
		ScoresSlot->SetHorizontalAlignment(HAlign_Center);
	}
}

bool UCppShowScoresWidget::HasBlueprintLayout() const
{
	return GeneratedRootCanvas == nullptr && WidgetTree && WidgetTree->RootWidget != nullptr;
}

void UCppShowScoresWidget::RebuildGeneratedScoreBars()
{
	if (!PlayerScoresBox)
	{
		return;
	}

	const int32 DesiredCount = FMath::Clamp(CurrentScores.Num(), 0, 8);
	if (GeneratedScoreBars.Num() == DesiredCount)
	{
		return;
	}

	PlayerScoresBox->ClearChildren();
	GeneratedScoreBars.Empty();

	for (int32 SlotId = 0; SlotId < DesiredCount; ++SlotId)
	{
		BuildGeneratedScoreBar(SlotId);
	}
}

void UCppShowScoresWidget::BuildGeneratedScoreBar(int32 SlotId)
{
	FNetworkScoreBarWidgets RowWidgets;

	UBorder* BarBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	BarBorder->SetBrushColor(FLinearColor(0.030f, 0.034f, 0.040f, 0.96f));
	BarBorder->SetPadding(FMargin(5.0f));
	RowWidgets.RootBorder = BarBorder;

	UVerticalBox* OuterBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	BarBorder->SetContent(OuterBox);

	UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	NameText->SetText(FText::FromString(FString::Printf(TEXT("P%d"), SlotId + 1)));
	ApplyTextStyle(NameText, 13, OtherPlayerTextColor, ETextJustify::Center);
	RowWidgets.NameText = NameText;
	if (UVerticalBoxSlot* NameSlot = OuterBox->AddChildToVerticalBox(NameText))
	{
		NameSlot->SetHorizontalAlignment(HAlign_Fill);
		NameSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 3.0f));
	}

	USizeBox* BarSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	BarSizeBox->SetWidthOverride(ScoreBarWidth);
	BarSizeBox->SetHeightOverride(ScoreBarHeight);
	if (UVerticalBoxSlot* BarSlot = OuterBox->AddChildToVerticalBox(BarSizeBox))
	{
		BarSlot->SetHorizontalAlignment(HAlign_Center);
	}

	UOverlay* BarOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	BarSizeBox->AddChild(BarOverlay);

	UProgressBar* ProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
	ProgressBar->SetBarFillType(EProgressBarFillType::LeftToRight);
	ProgressBar->SetPercent(0.0f);
	FProgressBarStyle ProgressStyle = ProgressBar->GetWidgetStyle();
	ProgressStyle.BackgroundImage.TintColor = FSlateColor(ScoreTrackColor);
	ProgressStyle.FillImage.TintColor = FSlateColor(OtherPlayerFillColor);
	ProgressBar->SetWidgetStyle(ProgressStyle);
	RowWidgets.ProgressBar = ProgressBar;
	if (UOverlaySlot* ProgressSlot = BarOverlay->AddChildToOverlay(ProgressBar))
	{
		ProgressSlot->SetHorizontalAlignment(HAlign_Fill);
		ProgressSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UTextBlock* ScoreText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ScoreText->SetText(FText::FromString(TEXT("0 / 0")));
	ApplyTextStyle(ScoreText, ScoreFontSize, OtherPlayerTextColor, ETextJustify::Center);
	RowWidgets.ScoreText = ScoreText;
	if (UOverlaySlot* ScoreSlot = BarOverlay->AddChildToOverlay(ScoreText))
	{
		ScoreSlot->SetHorizontalAlignment(HAlign_Fill);
		ScoreSlot->SetVerticalAlignment(VAlign_Center);
	}

	GeneratedScoreBars.Add(RowWidgets);

	if (UHorizontalBoxSlot* RootSlot = PlayerScoresBox->AddChildToHorizontalBox(BarBorder))
	{
		RootSlot->SetPadding(FMargin(SlotId == 0 ? 0.0f : 10.0f, 0.0f, 0.0f, 0.0f));
		RootSlot->SetVerticalAlignment(VAlign_Center);
	}
}

void UCppShowScoresWidget::ApplyScoreBarState(int32 SlotId)
{
	if (!GeneratedScoreBars.IsValidIndex(SlotId))
	{
		return;
	}

	FNetworkScoreBarWidgets& Widgets = GeneratedScoreBars[SlotId];
	const int32 Score = CurrentScores.IsValidIndex(SlotId) ? CurrentScores[SlotId] : 0;
	const bool bIsLocalPlayer = SlotId == LocalPlayerSlotId;
	const FLinearColor FillColor = bIsLocalPlayer ? LocalPlayerFillColor : OtherPlayerFillColor;
	const FLinearColor TextColor = bIsLocalPlayer ? LocalPlayerTextColor : OtherPlayerTextColor;
	const float Percent = TargetScore > 0
		? FMath::Clamp(static_cast<float>(Score) / static_cast<float>(TargetScore), 0.0f, 1.0f)
		: 0.0f;

	if (Widgets.RootBorder)
	{
		Widgets.RootBorder->SetBrushColor(bIsLocalPlayer
			? FLinearColor(0.16f, 0.035f, 0.035f, 0.94f)
			: FLinearColor(0.030f, 0.034f, 0.040f, 0.94f));
	}
	if (Widgets.NameText)
	{
		Widgets.NameText->SetText(FText::FromString(FString::Printf(TEXT("P%d"), SlotId + 1)));
		ApplyTextStyle(Widgets.NameText, 13, TextColor, ETextJustify::Center);
	}
	if (Widgets.ProgressBar)
	{
		FProgressBarStyle ProgressStyle = Widgets.ProgressBar->GetWidgetStyle();
		ProgressStyle.BackgroundImage.TintColor = FSlateColor(ScoreTrackColor);
		ProgressStyle.FillImage.TintColor = FSlateColor(FillColor);
		Widgets.ProgressBar->SetWidgetStyle(ProgressStyle);
		Widgets.ProgressBar->SetPercent(Percent);
		Widgets.ProgressBar->SetFillColorAndOpacity(FillColor);
	}
	if (Widgets.ScoreText)
	{
		Widgets.ScoreText->SetText(FormatScoreText(SlotId, Score));
		ApplyTextStyle(Widgets.ScoreText, ScoreFontSize, TextColor, ETextJustify::Center);
	}
}

FString UCppShowScoresWidget::FormatElapsedTime() const
{
	const int32 Minutes = ElapsedSeconds / 60;
	const int32 Seconds = ElapsedSeconds % 60;
	return FString::Printf(TEXT("%d:%02d"), Minutes, Seconds);
}

FText UCppShowScoresWidget::FormatHeaderText() const
{
	return FText::FromString(FString::Printf(TEXT("\u76ee\u6807: %d  -  \u65f6\u95f4: %s"), TargetScore, *FormatElapsedTime()));
}

FText UCppShowScoresWidget::FormatScoreText(int32 SlotId, int32 Score) const
{
	return FText::FromString(FString::Printf(TEXT("P%d: %d / %d"), SlotId + 1, Score, TargetScore));
}

void UCppShowScoresWidget::ApplyTextStyle(UTextBlock* TextBlock, int32 FontSize, const FLinearColor& Color, ETextJustify::Type Justification)
{
	if (!TextBlock)
	{
		return;
	}

	FSlateFontInfo Font = TextBlock->GetFont();
	Font.Size = FontSize;
	TextBlock->SetFont(Font);
	TextBlock->SetJustification(Justification);
	TextBlock->SetColorAndOpacity(FSlateColor(Color));
}
