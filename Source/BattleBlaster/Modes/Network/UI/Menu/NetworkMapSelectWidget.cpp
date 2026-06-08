#include "Modes/Network/UI/Menu/NetworkMapSelectWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"

namespace
{
	constexpr int32 MapsPerPage = 4;
	constexpr float MapCardWidth = 258.0f;
	constexpr float MapCardHeight = 168.0f;

	const FLinearColor BackgroundColor(0.014f, 0.015f, 0.018f, 0.97f);
	const FLinearColor PanelColor(0.040f, 0.043f, 0.050f, 0.98f);
	const FLinearColor CardColor(0.075f, 0.082f, 0.094f, 1.0f);
	const FLinearColor CardHoverColor(0.100f, 0.112f, 0.130f, 1.0f);
	const FLinearColor SelectedBorderColor(0.98f, 0.86f, 0.18f, 1.0f);
	const FLinearColor HiddenBorderColor(0.0f, 0.0f, 0.0f, 0.0f);
	const FLinearColor TextColor(0.92f, 0.94f, 0.96f, 1.0f);
	const FLinearColor MutedTextColor(0.64f, 0.68f, 0.72f, 1.0f);
	const FLinearColor AccentColor(0.11f, 0.64f, 0.58f, 1.0f);
	const FLinearColor WarningColor(0.95f, 0.63f, 0.24f, 1.0f);

	void ApplyButtonTint(UButton* Button, const FLinearColor& NormalColor, const FLinearColor& HoverColor)
	{
		if (!Button)
		{
			return;
		}

		FButtonStyle Style = Button->GetStyle();
		Style.Normal.TintColor = FSlateColor(NormalColor);
		Style.Hovered.TintColor = FSlateColor(HoverColor);
		Style.Pressed.TintColor = FSlateColor(NormalColor * 0.75f);
		Style.SetNormalPadding(FMargin(0.0f));
		Style.SetPressedPadding(FMargin(1.0f, 1.0f, -1.0f, -1.0f));
		Button->SetStyle(Style);
	}
}

UNetworkMapSelectWidget::UNetworkMapSelectWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EnsureDefaultMaps();
}

void UNetworkMapSelectWidget::SetupForMode(
	ENetworkGameModeType InModeType,
	int32 InPlayerCount,
	const FNetworkMapInfo& InCurrentMap)
{
	CurrentModeType = InModeType;
	CurrentPlayerCount = FMath::Clamp(InPlayerCount, 1, 8);
	CurrentPageIndex = 0;
	SelectedGlobalMapIndex = FindMapIndexByLevelName(ResolveLevelName(InCurrentMap));

	if (SelectedGlobalMapIndex >= 0)
	{
		CurrentPageIndex = SelectedGlobalMapIndex / MapsPerPage;
	}

	UpdatePageDisplay();
}

const TArray<FNetworkMapInfo>& UNetworkMapSelectWidget::GetMapsForCurrentMode() const
{
	switch (CurrentModeType)
	{
	case ENetworkGameModeType::TeamDeathmatch:
		return TeamDeathmatchMaps;
	case ENetworkGameModeType::MOBA:
		return MOBAMaps;
	case ENetworkGameModeType::TeamMOBA:
		return TeamMOBAMaps;
	case ENetworkGameModeType::Deathmatch:
	default:
		return DeathmatchMaps;
	}
}

TSharedRef<SWidget> UNetworkMapSelectWidget::RebuildWidget()
{
	BuildDefaultWidgetTree();
	return Super::RebuildWidget();
}

void UNetworkMapSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	EnsureDefaultMaps();
	BindButtons();
	UpdatePageDisplay();
}

void UNetworkMapSelectWidget::BuildDefaultWidgetTree()
{
	if ((WidgetTree && WidgetTree->RootWidget) || GeneratedRootCanvas)
	{
		return;
	}

	GeneratedRootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("NetworkMapSelectRoot"));
	WidgetTree->RootWidget = GeneratedRootCanvas;

	UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Background"));
	Background->SetColorAndOpacity(BackgroundColor);
	if (UCanvasPanelSlot* BackgroundSlot = GeneratedRootCanvas->AddChildToCanvas(Background))
	{
		BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackgroundSlot->SetOffsets(FMargin(0.0f));
	}

	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MapSelectPanel"));
	PanelBorder->SetBrushColor(PanelColor);
	PanelBorder->SetPadding(FMargin(32.0f, 28.0f));
	if (UCanvasPanelSlot* PanelSlot = GeneratedRootCanvas->AddChildToCanvas(PanelBorder))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetSize(FVector2D(760.0f, 690.0f));
		PanelSlot->SetPosition(FVector2D(0.0f, 0.0f));
	}

	UVerticalBox* PanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelBox"));
	PanelBorder->SetContent(PanelBox);

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(FText::FromString(TEXT("Select Map")));
	ApplyTextStyle(TitleText, 32, TextColor, ETextJustify::Left);
	if (UVerticalBoxSlot* TitleSlot = PanelBox->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Fill);
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	Text_Status = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_Status"));
	Text_Status->SetText(FText::FromString(TEXT("Choose a map for the selected network mode.")));
	Text_Status->SetAutoWrapText(true);
	ApplyTextStyle(Text_Status, 15, MutedTextColor, ETextJustify::Left);
	if (UVerticalBoxSlot* StatusSlot = PanelBox->AddChildToVerticalBox(Text_Status))
	{
		StatusSlot->SetHorizontalAlignment(HAlign_Fill);
		StatusSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
	}

	UGridPanel* MapGrid = WidgetTree->ConstructWidget<UGridPanel>(UGridPanel::StaticClass(), TEXT("MapGrid"));
	if (UVerticalBoxSlot* GridSlot = PanelBox->AddChildToVerticalBox(MapGrid))
	{
		GridSlot->SetHorizontalAlignment(HAlign_Center);
		GridSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
	}

	UButton* MapButtons[MapsPerPage] = {};
	UBorder* MapBorders[MapsPerPage] = {};
	UTextBlock* MapTexts[MapsPerPage] = {};

	for (int32 Index = 0; Index < MapsPerPage; ++Index)
	{
		UBorder* OuterBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		OuterBorder->SetBrushColor(HiddenBorderColor);
		OuterBorder->SetPadding(FMargin(3.0f));
		MapBorders[Index] = OuterBorder;

		USizeBox* CardSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		CardSize->SetWidthOverride(MapCardWidth);
		CardSize->SetHeightOverride(MapCardHeight);
		OuterBorder->SetContent(CardSize);

		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		ApplyButtonTint(Button, CardColor, CardHoverColor);
		MapButtons[Index] = Button;
		CardSize->AddChild(Button);

		UOverlay* CardOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		Button->AddChild(CardOverlay);

		UImage* PreviewImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		PreviewImage->SetColorAndOpacity(FLinearColor(0.11f, 0.13f, 0.15f, 1.0f));
		GeneratedMapImages.Add(PreviewImage);
		if (UOverlaySlot* ImageSlot = CardOverlay->AddChildToOverlay(PreviewImage))
		{
			ImageSlot->SetHorizontalAlignment(HAlign_Fill);
			ImageSlot->SetVerticalAlignment(VAlign_Fill);
		}

		UBorder* LabelPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		LabelPanel->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.58f));
		LabelPanel->SetPadding(FMargin(10.0f, 6.0f));
		if (UOverlaySlot* LabelSlot = CardOverlay->AddChildToOverlay(LabelPanel))
		{
			LabelSlot->SetHorizontalAlignment(HAlign_Fill);
			LabelSlot->SetVerticalAlignment(VAlign_Bottom);
		}

		UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		NameText->SetAutoWrapText(true);
		ApplyTextStyle(NameText, 18, TextColor, ETextJustify::Center);
		LabelPanel->SetContent(NameText);
		MapTexts[Index] = NameText;

		if (UGridSlot* GridSlot = MapGrid->AddChildToGrid(OuterBorder, Index / 2, Index % 2))
		{
			GridSlot->SetPadding(FMargin(8.0f));
			GridSlot->SetHorizontalAlignment(HAlign_Center);
			GridSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	Btn_Map0 = MapButtons[0];
	Btn_Map1 = MapButtons[1];
	Btn_Map2 = MapButtons[2];
	Btn_Map3 = MapButtons[3];
	Border_Map0 = MapBorders[0];
	Border_Map1 = MapBorders[1];
	Border_Map2 = MapBorders[2];
	Border_Map3 = MapBorders[3];
	Text_MapName0 = MapTexts[0];
	Text_MapName1 = MapTexts[1];
	Text_MapName2 = MapTexts[2];
	Text_MapName3 = MapTexts[3];

	UHorizontalBox* PageRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PageRow"));
	if (UVerticalBoxSlot* PageRowSlot = PanelBox->AddChildToVerticalBox(PageRow))
	{
		PageRowSlot->SetHorizontalAlignment(HAlign_Center);
		PageRowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	}

	Btn_PrevPage = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Btn_PrevPage"));
	ApplyButtonTint(Btn_PrevPage, FLinearColor(0.12f, 0.13f, 0.15f, 1.0f), FLinearColor(0.16f, 0.17f, 0.20f, 1.0f));
	UTextBlock* PrevText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	PrevText->SetText(FText::FromString(TEXT("<")));
	ApplyTextStyle(PrevText, 20, TextColor, ETextJustify::Center);
	Btn_PrevPage->AddChild(PrevText);

	USizeBox* PrevBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	PrevBox->SetWidthOverride(72.0f);
	PrevBox->SetHeightOverride(42.0f);
	PrevBox->AddChild(Btn_PrevPage);
	PageRow->AddChildToHorizontalBox(PrevBox);

	Text_PageNumber = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_PageNumber"));
	Text_PageNumber->SetText(FText::FromString(TEXT("1 / 1")));
	ApplyTextStyle(Text_PageNumber, 18, TextColor, ETextJustify::Center);
	USizeBox* PageTextBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	PageTextBox->SetWidthOverride(120.0f);
	PageTextBox->AddChild(Text_PageNumber);
	if (UHorizontalBoxSlot* PageTextSlot = PageRow->AddChildToHorizontalBox(PageTextBox))
	{
		PageTextSlot->SetVerticalAlignment(VAlign_Center);
		PageTextSlot->SetPadding(FMargin(12.0f, 0.0f));
	}

	Btn_NextPage = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Btn_NextPage"));
	ApplyButtonTint(Btn_NextPage, FLinearColor(0.12f, 0.13f, 0.15f, 1.0f), FLinearColor(0.16f, 0.17f, 0.20f, 1.0f));
	UTextBlock* NextText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	NextText->SetText(FText::FromString(TEXT(">")));
	ApplyTextStyle(NextText, 20, TextColor, ETextJustify::Center);
	Btn_NextPage->AddChild(NextText);

	USizeBox* NextBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	NextBox->SetWidthOverride(72.0f);
	NextBox->SetHeightOverride(42.0f);
	NextBox->AddChild(Btn_NextPage);
	PageRow->AddChildToHorizontalBox(NextBox);

	Btn_Confirm = AddMenuButton(PanelBox, TEXT("Confirm Map"), 10.0f);
	Btn_Back = AddMenuButton(PanelBox, TEXT("Back"), 0.0f);
}

void UNetworkMapSelectWidget::BindButtons()
{
	if (Btn_Map0 && !Btn_Map0->OnClicked.IsAlreadyBound(this, &UNetworkMapSelectWidget::OnMap0Clicked))
	{
		Btn_Map0->OnClicked.AddDynamic(this, &UNetworkMapSelectWidget::OnMap0Clicked);
	}
	if (Btn_Map1 && !Btn_Map1->OnClicked.IsAlreadyBound(this, &UNetworkMapSelectWidget::OnMap1Clicked))
	{
		Btn_Map1->OnClicked.AddDynamic(this, &UNetworkMapSelectWidget::OnMap1Clicked);
	}
	if (Btn_Map2 && !Btn_Map2->OnClicked.IsAlreadyBound(this, &UNetworkMapSelectWidget::OnMap2Clicked))
	{
		Btn_Map2->OnClicked.AddDynamic(this, &UNetworkMapSelectWidget::OnMap2Clicked);
	}
	if (Btn_Map3 && !Btn_Map3->OnClicked.IsAlreadyBound(this, &UNetworkMapSelectWidget::OnMap3Clicked))
	{
		Btn_Map3->OnClicked.AddDynamic(this, &UNetworkMapSelectWidget::OnMap3Clicked);
	}
	if (Btn_PrevPage && !Btn_PrevPage->OnClicked.IsAlreadyBound(this, &UNetworkMapSelectWidget::OnPrevPageClicked))
	{
		Btn_PrevPage->OnClicked.AddDynamic(this, &UNetworkMapSelectWidget::OnPrevPageClicked);
	}
	if (Btn_NextPage && !Btn_NextPage->OnClicked.IsAlreadyBound(this, &UNetworkMapSelectWidget::OnNextPageClicked))
	{
		Btn_NextPage->OnClicked.AddDynamic(this, &UNetworkMapSelectWidget::OnNextPageClicked);
	}
	if (Btn_Confirm && !Btn_Confirm->OnClicked.IsAlreadyBound(this, &UNetworkMapSelectWidget::OnConfirmClicked))
	{
		Btn_Confirm->OnClicked.AddDynamic(this, &UNetworkMapSelectWidget::OnConfirmClicked);
	}
	if (Btn_Back && !Btn_Back->OnClicked.IsAlreadyBound(this, &UNetworkMapSelectWidget::HandleBackClicked))
	{
		Btn_Back->OnClicked.AddDynamic(this, &UNetworkMapSelectWidget::HandleBackClicked);
	}
}

void UNetworkMapSelectWidget::EnsureDefaultMaps()
{
	if (DeathmatchMaps.Num() == 0)
	{
		FNetworkMapInfo DefaultMap;
		DefaultMap.MapDisplayName = TEXT("Network Test Map");
		DefaultMap.LevelName = FName(TEXT("/Game/Maps/NetworkBattle/NetworkBattleTestMap"));
		DefaultMap.MinPlayers = 1;
		DefaultMap.MaxPlayers = 8;
		DeathmatchMaps.Add(DefaultMap);

		FNetworkMapInfo ClassicMap;
		ClassicMap.MapDisplayName = TEXT("Network Classic");
		ClassicMap.LevelName = FName(TEXT("/Game/Maps/NetworkBattle/Map_Classic"));
		ClassicMap.MinPlayers = 1;
		ClassicMap.MaxPlayers = 8;
		DeathmatchMaps.Add(ClassicMap);
	}

	if (TeamDeathmatchMaps.Num() == 0)
	{
		TeamDeathmatchMaps = DeathmatchMaps;
	}

	if (MOBAMaps.Num() == 0)
	{
		FNetworkMapInfo MOBAMap;
		MOBAMap.MapDisplayName = TEXT("MOBA Test Map");
		MOBAMap.LevelName = FName(TEXT("/Game/Maps/TestMobaMap_2p"));
		MOBAMap.MinPlayers = 1;
		MOBAMap.MaxPlayers = 8;
		MOBAMaps.Add(MOBAMap);
	}

	if (TeamMOBAMaps.Num() == 0)
	{
		TeamMOBAMaps = MOBAMaps;
	}
}

void UNetworkMapSelectWidget::UpdatePageDisplay()
{
	const TArray<FNetworkMapInfo>& Maps = GetMapsForCurrentMode();
	const int32 TotalPages = FMath::Max(1, FMath::CeilToInt(static_cast<float>(Maps.Num()) / static_cast<float>(MapsPerPage)));
	CurrentPageIndex = FMath::Clamp(CurrentPageIndex, 0, TotalPages - 1);

	const int32 StartIndex = CurrentPageIndex * MapsPerPage;
	for (int32 ButtonIndex = 0; ButtonIndex < MapsPerPage; ++ButtonIndex)
	{
		const int32 GlobalIndex = StartIndex + ButtonIndex;
		if (Maps.IsValidIndex(GlobalIndex))
		{
			SetMapButtonVisible(ButtonIndex, true);
			ApplyMapButtonVisual(ButtonIndex, Maps[GlobalIndex]);
		}
		else
		{
			SetMapButtonVisible(ButtonIndex, false);
		}
	}

	if (Text_PageNumber)
	{
		Text_PageNumber->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), CurrentPageIndex + 1, TotalPages)));
	}

	if (Text_Status)
	{
		const FString ModeLabel = StaticEnum<ENetworkGameModeType>()
			? StaticEnum<ENetworkGameModeType>()->GetDisplayNameTextByValue(static_cast<int64>(CurrentModeType)).ToString()
			: TEXT("Network Mode");
		Text_Status->SetText(FText::FromString(FString::Printf(TEXT("%s maps available: %d"), *ModeLabel, Maps.Num())));
		ApplyTextStyle(Text_Status, 15, Maps.Num() > 0 ? MutedTextColor : WarningColor, ETextJustify::Left);
	}

	HighlightSelectedMap();
}

void UNetworkMapSelectWidget::HighlightSelectedMap()
{
	UBorder* Borders[MapsPerPage] = { Border_Map0, Border_Map1, Border_Map2, Border_Map3 };
	const int32 StartIndex = CurrentPageIndex * MapsPerPage;

	for (int32 ButtonIndex = 0; ButtonIndex < MapsPerPage; ++ButtonIndex)
	{
		if (!Borders[ButtonIndex])
		{
			continue;
		}

		const int32 GlobalIndex = StartIndex + ButtonIndex;
		Borders[ButtonIndex]->SetBrushColor(GlobalIndex == SelectedGlobalMapIndex ? SelectedBorderColor : HiddenBorderColor);
	}
}

void UNetworkMapSelectWidget::SelectMapAtButtonIndex(int32 ButtonIndex)
{
	const int32 GlobalIndex = CurrentPageIndex * MapsPerPage + ButtonIndex;
	if (!GetMapsForCurrentMode().IsValidIndex(GlobalIndex))
	{
		return;
	}

	SelectedGlobalMapIndex = GlobalIndex;
	HighlightSelectedMap();
}

void UNetworkMapSelectWidget::ConfirmSelectedMap()
{
	const TArray<FNetworkMapInfo>& Maps = GetMapsForCurrentMode();
	if (!Maps.IsValidIndex(SelectedGlobalMapIndex))
	{
		if (Text_Status)
		{
			Text_Status->SetText(FText::FromString(TEXT("Please select a map first.")));
			ApplyTextStyle(Text_Status, 15, WarningColor, ETextJustify::Left);
		}
		return;
	}

	const FNetworkMapInfo& SelectedMap = Maps[SelectedGlobalMapIndex];
	if (!IsMapAllowedForPlayerCount(SelectedMap) && Text_Status)
	{
		Text_Status->SetText(FText::Format(
			FText::FromString(TEXT("This map supports {0}-{1} players.")),
			FText::AsNumber(SelectedMap.MinPlayers),
			FText::AsNumber(SelectedMap.MaxPlayers)));
		ApplyTextStyle(Text_Status, 15, WarningColor, ETextJustify::Left);
		return;
	}

	OnNetworkMapSelected.Broadcast(SelectedMap);
	HandleBackClicked();
}

void UNetworkMapSelectWidget::ApplyMapButtonVisual(int32 ButtonIndex, const FNetworkMapInfo& MapInfo)
{
	UButton* Buttons[MapsPerPage] = { Btn_Map0, Btn_Map1, Btn_Map2, Btn_Map3 };
	UTextBlock* Texts[MapsPerPage] = { Text_MapName0, Text_MapName1, Text_MapName2, Text_MapName3 };

	if (Texts[ButtonIndex])
	{
		Texts[ButtonIndex]->SetText(FText::FromString(MapInfo.MapDisplayName));
	}

	if (GeneratedMapImages.IsValidIndex(ButtonIndex) && GeneratedMapImages[ButtonIndex])
	{
		if (MapInfo.MapThumbnail)
		{
			GeneratedMapImages[ButtonIndex]->SetBrushFromTexture(MapInfo.MapThumbnail, true);
			GeneratedMapImages[ButtonIndex]->SetColorAndOpacity(FLinearColor::White);
		}
		else
		{
			GeneratedMapImages[ButtonIndex]->SetBrushFromTexture(nullptr);
			GeneratedMapImages[ButtonIndex]->SetColorAndOpacity(FLinearColor(0.11f, 0.13f, 0.15f, 1.0f));
		}
	}

	if (Buttons[ButtonIndex] && MapInfo.MapThumbnail)
	{
		FButtonStyle Style = Buttons[ButtonIndex]->GetStyle();
		FSlateBrush PreviewBrush;
		PreviewBrush.SetResourceObject(MapInfo.MapThumbnail);
		PreviewBrush.DrawAs = ESlateBrushDrawType::Image;
		PreviewBrush.ImageSize = FVector2D(256.0f, 144.0f);
		Style.SetNormal(PreviewBrush);
		Style.SetHovered(PreviewBrush);
		Style.SetPressed(PreviewBrush);
		Style.Hovered.TintColor = FSlateColor(FLinearColor(0.82f, 0.82f, 0.82f, 1.0f));
		Style.Pressed.TintColor = FSlateColor(FLinearColor(0.62f, 0.62f, 0.62f, 1.0f));
		Buttons[ButtonIndex]->SetStyle(Style);
	}
	else if (Buttons[ButtonIndex])
	{
		ApplyButtonTint(Buttons[ButtonIndex], CardColor, CardHoverColor);
	}
}

void UNetworkMapSelectWidget::SetMapButtonVisible(int32 ButtonIndex, bool bVisible)
{
	UButton* Buttons[MapsPerPage] = { Btn_Map0, Btn_Map1, Btn_Map2, Btn_Map3 };
	UBorder* Borders[MapsPerPage] = { Border_Map0, Border_Map1, Border_Map2, Border_Map3 };
	UTextBlock* Texts[MapsPerPage] = { Text_MapName0, Text_MapName1, Text_MapName2, Text_MapName3 };

	const ESlateVisibility ButtonVisibility = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
	if (Buttons[ButtonIndex])
	{
		Buttons[ButtonIndex]->SetVisibility(ButtonVisibility);
	}
	if (Borders[ButtonIndex])
	{
		Borders[ButtonIndex]->SetVisibility(ButtonVisibility);
	}
	if (Texts[ButtonIndex])
	{
		Texts[ButtonIndex]->SetVisibility(ButtonVisibility);
	}
	if (GeneratedMapImages.IsValidIndex(ButtonIndex) && GeneratedMapImages[ButtonIndex])
	{
		GeneratedMapImages[ButtonIndex]->SetVisibility(ButtonVisibility);
	}
}

bool UNetworkMapSelectWidget::IsMapAllowedForPlayerCount(const FNetworkMapInfo& MapInfo) const
{
	return CurrentPlayerCount >= MapInfo.MinPlayers && CurrentPlayerCount <= MapInfo.MaxPlayers;
}

int32 UNetworkMapSelectWidget::FindMapIndexByLevelName(const FName& LevelName) const
{
	if (LevelName.IsNone())
	{
		return INDEX_NONE;
	}

	const TArray<FNetworkMapInfo>& Maps = GetMapsForCurrentMode();
	for (int32 Index = 0; Index < Maps.Num(); ++Index)
	{
		if (ResolveLevelName(Maps[Index]) == LevelName)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

FText UNetworkMapSelectWidget::BuildMapSubtitle(const FNetworkMapInfo& MapInfo) const
{
	return FText::Format(
		FText::FromString(TEXT("{0}-{1} Players")),
		FText::AsNumber(MapInfo.MinPlayers),
		FText::AsNumber(MapInfo.MaxPlayers));
}

FName UNetworkMapSelectWidget::ResolveLevelName(const FNetworkMapInfo& MapInfo)
{
	if (!MapInfo.LevelName.IsNone())
	{
		return MapInfo.LevelName;
	}

	const FString AssetName = MapInfo.LevelAsset.GetAssetName();
	const FString LongPackageName = MapInfo.LevelAsset.ToSoftObjectPath().GetLongPackageName();
	if (!LongPackageName.IsEmpty())
	{
		return FName(*LongPackageName);
	}

	return AssetName.IsEmpty() ? NAME_None : FName(*AssetName);
}

void UNetworkMapSelectWidget::ApplyTextStyle(UTextBlock* TextBlock, int32 FontSize, const FLinearColor& Color, ETextJustify::Type Justification)
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
	TextBlock->SetLineHeightPercentage(1.0f);
}

void UNetworkMapSelectWidget::OnMap0Clicked()
{
	SelectMapAtButtonIndex(0);
}

void UNetworkMapSelectWidget::OnMap1Clicked()
{
	SelectMapAtButtonIndex(1);
}

void UNetworkMapSelectWidget::OnMap2Clicked()
{
	SelectMapAtButtonIndex(2);
}

void UNetworkMapSelectWidget::OnMap3Clicked()
{
	SelectMapAtButtonIndex(3);
}

void UNetworkMapSelectWidget::OnPrevPageClicked()
{
	if (CurrentPageIndex > 0)
	{
		--CurrentPageIndex;
		UpdatePageDisplay();
	}
}

void UNetworkMapSelectWidget::OnNextPageClicked()
{
	const int32 TotalPages = FMath::Max(1, FMath::CeilToInt(static_cast<float>(GetMapsForCurrentMode().Num()) / static_cast<float>(MapsPerPage)));
	if (CurrentPageIndex < TotalPages - 1)
	{
		++CurrentPageIndex;
		UpdatePageDisplay();
	}
}

void UNetworkMapSelectWidget::OnConfirmClicked()
{
	ConfirmSelectedMap();
}
