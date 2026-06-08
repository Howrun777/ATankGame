#include "Modes/Network/UI/NetworkMOBAStateWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

UNetworkMOBAStateWidget::UNetworkMOBAStateWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PanelColor = FLinearColor(0.014f, 0.017f, 0.020f, 0.78f);
	HeaderTextColor = FLinearColor(0.94f, 0.97f, 1.0f, 1.0f);
	AliveColor = FLinearColor(0.16f, 0.82f, 0.48f, 1.0f);
	DownColor = FLinearColor(0.96f, 0.70f, 0.22f, 1.0f);
	EliminatedColor = FLinearColor(0.44f, 0.47f, 0.52f, 1.0f);
	LocalTeamColor = FLinearColor(0.95f, 0.12f, 0.08f, 1.0f);
}

TSharedRef<SWidget> UNetworkMOBAStateWidget::RebuildWidget()
{
	BuildDefaultWidgetTree();
	return Super::RebuildWidget();
}

void UNetworkMOBAStateWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshDisplay();
}

void UNetworkMOBAStateWidget::UpdateMOBAState(int32 InElapsedSeconds, const TArray<int32>& InAliveCoreCountsByTeam, const TArray<bool>& InTeamEliminated, int32 InLocalTeamId, int32 InWinningTeamId)
{
	ElapsedSeconds = FMath::Max(0, InElapsedSeconds);
	AliveCoreCountsByTeam = InAliveCoreCountsByTeam;
	bTeamEliminated = InTeamEliminated;
	LocalTeamId = InLocalTeamId;
	WinningTeamId = InWinningTeamId;
	RefreshDisplay();
}

void UNetworkMOBAStateWidget::RefreshDisplay()
{
	if (HeaderText)
	{
		HeaderText->SetText(FormatHeaderText());
	}

	if (TeamStateBox)
	{
		RebuildGeneratedTeamStates();
		for (int32 TeamId = 0; TeamId < GeneratedTeamStates.Num(); ++TeamId)
		{
			ApplyTeamState(TeamId);
		}
	}
}

void UNetworkMOBAStateWidget::BuildDefaultWidgetTree()
{
	if (HasBlueprintLayout() || GeneratedRootCanvas)
	{
		return;
	}

	GeneratedRootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("NetworkMOBARoot"));
	WidgetTree->RootWidget = GeneratedRootCanvas;

	GeneratedPanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("NetworkMOBAPanel"));
	GeneratedPanelBorder->SetBrushColor(PanelColor);
	GeneratedPanelBorder->SetPadding(FMargin(18.0f, 12.0f, 18.0f, 12.0f));

	if (UCanvasPanelSlot* PanelSlot = GeneratedRootCanvas->AddChildToCanvas(GeneratedPanelBorder))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.0f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		PanelSlot->SetPosition(FVector2D(0.0f, 16.0f));
		PanelSlot->SetAutoSize(true);
	}

	GeneratedPanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("NetworkMOBAPanelBox"));
	GeneratedPanelBorder->SetContent(GeneratedPanelBox);

	HeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HeaderText"));
	HeaderText->SetText(FormatHeaderText());
	ApplyTextStyle(HeaderText, HeaderFontSize, HeaderTextColor, ETextJustify::Center);
	if (UVerticalBoxSlot* HeaderSlot = GeneratedPanelBox->AddChildToVerticalBox(HeaderText))
	{
		HeaderSlot->SetHorizontalAlignment(HAlign_Fill);
		HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	TeamStateBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TeamStateBox"));
	if (UVerticalBoxSlot* StateSlot = GeneratedPanelBox->AddChildToVerticalBox(TeamStateBox))
	{
		StateSlot->SetHorizontalAlignment(HAlign_Center);
	}
}

bool UNetworkMOBAStateWidget::HasBlueprintLayout() const
{
	return GeneratedRootCanvas == nullptr && WidgetTree && WidgetTree->RootWidget != nullptr;
}

void UNetworkMOBAStateWidget::RebuildGeneratedTeamStates()
{
	if (!TeamStateBox)
	{
		return;
	}

	const int32 DesiredCount = FMath::Clamp(AliveCoreCountsByTeam.Num(), 0, 8);
	if (GeneratedTeamStates.Num() == DesiredCount)
	{
		return;
	}

	TeamStateBox->ClearChildren();
	GeneratedTeamStates.Empty();

	for (int32 TeamId = 0; TeamId < DesiredCount; ++TeamId)
	{
		BuildGeneratedTeamState(TeamId);
	}
}

void UNetworkMOBAStateWidget::BuildGeneratedTeamState(int32 TeamId)
{
	FNetworkMOBATeamStatusWidgets StatusWidgets;

	UBorder* CardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	CardBorder->SetBrushColor(GetTeamPanelColor(TeamId));
	CardBorder->SetPadding(FMargin(8.0f, 7.0f));
	StatusWidgets.RootBorder = CardBorder;

	USizeBox* CardSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	CardSizeBox->SetWidthOverride(TeamCardWidth);
	CardBorder->SetContent(CardSizeBox);

	UVerticalBox* CardBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	CardSizeBox->AddChild(CardBox);

	UTextBlock* TeamText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TeamText->SetText(FText::FromString(FString::Printf(TEXT("TEAM %d"), TeamId + 1)));
	ApplyTextStyle(TeamText, TeamFontSize, HeaderTextColor, ETextJustify::Center);
	StatusWidgets.TeamText = TeamText;
	if (UVerticalBoxSlot* TeamSlot = CardBox->AddChildToVerticalBox(TeamText))
	{
		TeamSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	UTextBlock* CoreText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CoreText->SetText(FormatCoreText(TeamId));
	ApplyTextStyle(CoreText, 13, HeaderTextColor, ETextJustify::Center);
	StatusWidgets.CoreText = CoreText;
	if (UVerticalBoxSlot* CoreSlot = CardBox->AddChildToVerticalBox(CoreText))
	{
		CoreSlot->SetHorizontalAlignment(HAlign_Fill);
		CoreSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 2.0f));
	}

	UTextBlock* StateText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	StateText->SetText(FormatStateText(TeamId));
	ApplyTextStyle(StateText, StateFontSize, GetTeamStateColor(TeamId), ETextJustify::Center);
	StatusWidgets.StateText = StateText;
	if (UVerticalBoxSlot* StateSlot = CardBox->AddChildToVerticalBox(StateText))
	{
		StateSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	GeneratedTeamStates.Add(StatusWidgets);

	if (UHorizontalBoxSlot* RootSlot = TeamStateBox->AddChildToHorizontalBox(CardBorder))
	{
		RootSlot->SetPadding(FMargin(TeamId == 0 ? 0.0f : 10.0f, 0.0f, 0.0f, 0.0f));
		RootSlot->SetVerticalAlignment(VAlign_Center);
	}
}

void UNetworkMOBAStateWidget::ApplyTeamState(int32 TeamId)
{
	if (!GeneratedTeamStates.IsValidIndex(TeamId))
	{
		return;
	}

	FNetworkMOBATeamStatusWidgets& Widgets = GeneratedTeamStates[TeamId];
	const FLinearColor StateColor = GetTeamStateColor(TeamId);

	if (Widgets.RootBorder)
	{
		Widgets.RootBorder->SetBrushColor(GetTeamPanelColor(TeamId));
	}
	if (Widgets.TeamText)
	{
		const FString TeamName = TeamId == LocalTeamId ? TEXT("YOUR TEAM") : FString::Printf(TEXT("TEAM %d"), TeamId + 1);
		Widgets.TeamText->SetText(FText::FromString(TeamId == WinningTeamId ? FString::Printf(TEXT("%s WINS"), *TeamName) : TeamName));
		ApplyTextStyle(Widgets.TeamText, TeamFontSize, TeamId == LocalTeamId ? LocalTeamColor : HeaderTextColor, ETextJustify::Center);
	}
	if (Widgets.CoreText)
	{
		Widgets.CoreText->SetText(FormatCoreText(TeamId));
		ApplyTextStyle(Widgets.CoreText, 13, HeaderTextColor, ETextJustify::Center);
	}
	if (Widgets.StateText)
	{
		Widgets.StateText->SetText(FormatStateText(TeamId));
		ApplyTextStyle(Widgets.StateText, StateFontSize, StateColor, ETextJustify::Center);
	}
}

FString UNetworkMOBAStateWidget::FormatElapsedTime() const
{
	const int32 Minutes = ElapsedSeconds / 60;
	const int32 Seconds = ElapsedSeconds % 60;
	return FString::Printf(TEXT("%d:%02d"), Minutes, Seconds);
}

FText UNetworkMOBAStateWidget::FormatHeaderText() const
{
	return FText::FromString(FString::Printf(TEXT("Core Status  -  Time: %s"), *FormatElapsedTime()));
}

FText UNetworkMOBAStateWidget::FormatCoreText(int32 TeamId) const
{
	const int32 CoreCount = AliveCoreCountsByTeam.IsValidIndex(TeamId) ? AliveCoreCountsByTeam[TeamId] : 0;
	return FText::FromString(FString::Printf(TEXT("Cores: %d"), CoreCount));
}

FText UNetworkMOBAStateWidget::FormatStateText(int32 TeamId) const
{
	if (bTeamEliminated.IsValidIndex(TeamId) && bTeamEliminated[TeamId])
	{
		return FText::FromString(TEXT("ELIMINATED"));
	}

	const int32 CoreCount = AliveCoreCountsByTeam.IsValidIndex(TeamId) ? AliveCoreCountsByTeam[TeamId] : 0;
	return FText::FromString(CoreCount > 0 ? TEXT("ALIVE") : TEXT("CORE DOWN"));
}

FLinearColor UNetworkMOBAStateWidget::GetTeamStateColor(int32 TeamId) const
{
	if (bTeamEliminated.IsValidIndex(TeamId) && bTeamEliminated[TeamId])
	{
		return EliminatedColor;
	}

	const int32 CoreCount = AliveCoreCountsByTeam.IsValidIndex(TeamId) ? AliveCoreCountsByTeam[TeamId] : 0;
	return CoreCount > 0 ? AliveColor : DownColor;
}

FLinearColor UNetworkMOBAStateWidget::GetTeamPanelColor(int32 TeamId) const
{
	if (TeamId == LocalTeamId)
	{
		return FLinearColor(0.16f, 0.030f, 0.025f, 0.92f);
	}

	if (bTeamEliminated.IsValidIndex(TeamId) && bTeamEliminated[TeamId])
	{
		return FLinearColor(0.030f, 0.034f, 0.040f, 0.72f);
	}

	return FLinearColor(0.034f, 0.040f, 0.048f, 0.84f);
}

void UNetworkMOBAStateWidget::ApplyTextStyle(UTextBlock* TextBlock, int32 FontSize, const FLinearColor& Color, ETextJustify::Type Justification)
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
