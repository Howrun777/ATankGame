#include "Modes/Network/UI/NetworkTeamScoresWidget.h"

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

UNetworkTeamScoresWidget::UNetworkTeamScoresWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PanelColor = FLinearColor(0.015f, 0.017f, 0.022f, 0.78f);
	HeaderTextColor = FLinearColor(0.94f, 0.97f, 1.0f, 1.0f);
	OtherTeamTextColor = FLinearColor(0.72f, 0.76f, 0.80f, 1.0f);
	OtherTeamFillColor = FLinearColor(0.28f, 0.34f, 0.40f, 1.0f);
	LocalTeamTextColor = FLinearColor(1.0f, 0.96f, 0.90f, 1.0f);
	LocalTeamFillColor = FLinearColor(0.95f, 0.15f, 0.10f, 1.0f);
	ScoreTrackColor = FLinearColor(0.035f, 0.040f, 0.048f, 1.0f);
}

TSharedRef<SWidget> UNetworkTeamScoresWidget::RebuildWidget()
{
	BuildDefaultWidgetTree();
	return Super::RebuildWidget();
}

void UNetworkTeamScoresWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshDisplay();
}

void UNetworkTeamScoresWidget::UpdateTeamScoreboard(int32 InTargetScore, int32 InElapsedSeconds, const TArray<int32>& InTeamScores, int32 InLocalTeamId, int32 InWinningTeamId)
{
	TargetScore = FMath::Max(1, InTargetScore);
	ElapsedSeconds = FMath::Max(0, InElapsedSeconds);
	CurrentTeamScores = InTeamScores;
	LocalTeamId = InLocalTeamId;
	WinningTeamId = InWinningTeamId;
	RefreshDisplay();
}

void UNetworkTeamScoresWidget::RefreshDisplay()
{
	if (TargetAndTimeText)
	{
		TargetAndTimeText->SetText(FormatHeaderText());
	}

	if (TeamScoresBox)
	{
		RebuildGeneratedTeamBars();
		for (int32 TeamId = 0; TeamId < GeneratedTeamBars.Num(); ++TeamId)
		{
			ApplyTeamBarState(TeamId);
		}
	}
}

void UNetworkTeamScoresWidget::BuildDefaultWidgetTree()
{
	if (HasBlueprintLayout() || GeneratedRootCanvas)
	{
		return;
	}

	GeneratedRootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("TeamScoresRoot"));
	WidgetTree->RootWidget = GeneratedRootCanvas;

	GeneratedPanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TeamScoresPanel"));
	GeneratedPanelBorder->SetBrushColor(PanelColor);
	GeneratedPanelBorder->SetPadding(FMargin(20.0f, 12.0f, 20.0f, 12.0f));

	if (UCanvasPanelSlot* PanelSlot = GeneratedRootCanvas->AddChildToCanvas(GeneratedPanelBorder))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.0f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		PanelSlot->SetPosition(FVector2D(0.0f, 16.0f));
		PanelSlot->SetAutoSize(true);
	}

	GeneratedPanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TeamScoresPanelBox"));
	GeneratedPanelBorder->SetContent(GeneratedPanelBox);

	TargetAndTimeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TargetAndTimeText"));
	TargetAndTimeText->SetText(FormatHeaderText());
	ApplyTextStyle(TargetAndTimeText, HeaderFontSize, HeaderTextColor, ETextJustify::Center);
	if (UVerticalBoxSlot* HeaderSlot = GeneratedPanelBox->AddChildToVerticalBox(TargetAndTimeText))
	{
		HeaderSlot->SetHorizontalAlignment(HAlign_Fill);
		HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	TeamScoresBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TeamScoresBox"));
	if (UVerticalBoxSlot* ScoresSlot = GeneratedPanelBox->AddChildToVerticalBox(TeamScoresBox))
	{
		ScoresSlot->SetHorizontalAlignment(HAlign_Center);
	}
}

bool UNetworkTeamScoresWidget::HasBlueprintLayout() const
{
	return GeneratedRootCanvas == nullptr && WidgetTree && WidgetTree->RootWidget != nullptr;
}

void UNetworkTeamScoresWidget::RebuildGeneratedTeamBars()
{
	if (!TeamScoresBox)
	{
		return;
	}

	const int32 DesiredCount = FMath::Clamp(CurrentTeamScores.Num(), 0, 8);
	if (GeneratedTeamBars.Num() == DesiredCount)
	{
		return;
	}

	TeamScoresBox->ClearChildren();
	GeneratedTeamBars.Empty();

	for (int32 TeamId = 0; TeamId < DesiredCount; ++TeamId)
	{
		BuildGeneratedTeamBar(TeamId);
	}
}

void UNetworkTeamScoresWidget::BuildGeneratedTeamBar(int32 TeamId)
{
	FNetworkTeamScoreBarWidgets RowWidgets;

	UBorder* BarBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	BarBorder->SetBrushColor(GetPanelColorForTeam(TeamId, false));
	BarBorder->SetPadding(FMargin(6.0f));
	RowWidgets.RootBorder = BarBorder;

	UVerticalBox* OuterBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	BarBorder->SetContent(OuterBox);

	UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	NameText->SetText(FText::FromString(FString::Printf(TEXT("TEAM %d"), TeamId + 1)));
	ApplyTextStyle(NameText, 13, OtherTeamTextColor, ETextJustify::Center);
	RowWidgets.NameText = NameText;
	if (UVerticalBoxSlot* NameSlot = OuterBox->AddChildToVerticalBox(NameText))
	{
		NameSlot->SetHorizontalAlignment(HAlign_Fill);
		NameSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
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
	FProgressBarStyle ProgressStyle = ProgressBar->GetWidgetStyle();
	ProgressStyle.BackgroundImage.TintColor = FSlateColor(ScoreTrackColor);
	ProgressStyle.FillImage.TintColor = FSlateColor(GetFillColorForTeam(TeamId, false));
	ProgressBar->SetWidgetStyle(ProgressStyle);
	RowWidgets.ProgressBar = ProgressBar;
	if (UOverlaySlot* ProgressSlot = BarOverlay->AddChildToOverlay(ProgressBar))
	{
		ProgressSlot->SetHorizontalAlignment(HAlign_Fill);
		ProgressSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UTextBlock* ScoreText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ScoreText->SetText(FText::FromString(TEXT("0 / 0")));
	ApplyTextStyle(ScoreText, ScoreFontSize, OtherTeamTextColor, ETextJustify::Center);
	RowWidgets.ScoreText = ScoreText;
	if (UOverlaySlot* ScoreSlot = BarOverlay->AddChildToOverlay(ScoreText))
	{
		ScoreSlot->SetHorizontalAlignment(HAlign_Fill);
		ScoreSlot->SetVerticalAlignment(VAlign_Center);
	}

	GeneratedTeamBars.Add(RowWidgets);

	if (UHorizontalBoxSlot* RootSlot = TeamScoresBox->AddChildToHorizontalBox(BarBorder))
	{
		RootSlot->SetPadding(FMargin(TeamId == 0 ? 0.0f : 12.0f, 0.0f, 0.0f, 0.0f));
		RootSlot->SetVerticalAlignment(VAlign_Center);
	}
}

void UNetworkTeamScoresWidget::ApplyTeamBarState(int32 TeamId)
{
	if (!GeneratedTeamBars.IsValidIndex(TeamId))
	{
		return;
	}

	FNetworkTeamScoreBarWidgets& Widgets = GeneratedTeamBars[TeamId];
	const int32 Score = CurrentTeamScores.IsValidIndex(TeamId) ? CurrentTeamScores[TeamId] : 0;
	const bool bIsLocalTeam = TeamId == LocalTeamId;
	const bool bIsWinner = TeamId == WinningTeamId;
	const FLinearColor FillColor = GetFillColorForTeam(TeamId, bIsLocalTeam);
	const FLinearColor TeamBarTextColor = bIsLocalTeam || bIsWinner ? LocalTeamTextColor : OtherTeamTextColor;
	const float Percent = TargetScore > 0
		? FMath::Clamp(static_cast<float>(Score) / static_cast<float>(TargetScore), 0.0f, 1.0f)
		: 0.0f;

	if (Widgets.RootBorder)
	{
		Widgets.RootBorder->SetBrushColor(GetPanelColorForTeam(TeamId, bIsLocalTeam || bIsWinner));
	}
	if (Widgets.NameText)
	{
		const FString Prefix = bIsLocalTeam ? TEXT("YOUR TEAM") : FString::Printf(TEXT("TEAM %d"), TeamId + 1);
		Widgets.NameText->SetText(FText::FromString(bIsWinner ? FString::Printf(TEXT("%s WINS"), *Prefix) : Prefix));
		ApplyTextStyle(Widgets.NameText, 13, TeamBarTextColor, ETextJustify::Center);
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
		Widgets.ScoreText->SetText(FormatScoreText(TeamId, Score));
		ApplyTextStyle(Widgets.ScoreText, ScoreFontSize, TeamBarTextColor, ETextJustify::Center);
	}
}

FLinearColor UNetworkTeamScoresWidget::GetFillColorForTeam(int32 TeamId, bool bIsLocalTeam) const
{
	return bIsLocalTeam ? LocalTeamFillColor : OtherTeamFillColor;
}

FLinearColor UNetworkTeamScoresWidget::GetPanelColorForTeam(int32 TeamId, bool bIsLocalTeam) const
{
	return bIsLocalTeam
		? FLinearColor(0.18f, 0.035f, 0.026f, 0.92f)
		: FLinearColor(0.034f, 0.040f, 0.048f, 0.84f);
}

FString UNetworkTeamScoresWidget::FormatElapsedTime() const
{
	const int32 Minutes = ElapsedSeconds / 60;
	const int32 Seconds = ElapsedSeconds % 60;
	return FString::Printf(TEXT("%d:%02d"), Minutes, Seconds);
}

FText UNetworkTeamScoresWidget::FormatHeaderText() const
{
	return FText::FromString(FString::Printf(TEXT("Target: %d  -  Time: %s"), TargetScore, *FormatElapsedTime()));
}

FText UNetworkTeamScoresWidget::FormatScoreText(int32 TeamId, int32 Score) const
{
	return FText::FromString(FString::Printf(TEXT("%d / %d"), Score, TargetScore));
}

void UNetworkTeamScoresWidget::ApplyTextStyle(UTextBlock* TextBlock, int32 FontSize, const FLinearColor& Color, ETextJustify::Type Justification)
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
