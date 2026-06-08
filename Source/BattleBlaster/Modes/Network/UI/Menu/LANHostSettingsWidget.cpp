#include "Modes/Network/UI/Menu/LANHostSettingsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/Networking/BattleBlasterSessionSubsystem.h"
#include "Modes/Network/UI/Menu/NetworkMapSelectWidget.h"

TSharedRef<SWidget> ULANHostSettingsWidget::RebuildWidget()
{
	UVerticalBox* ContentBox = nullptr;
	BuildMenuRoot(TEXT("Host LAN Game"), TEXT("Configure rules and open a listen server."), ContentBox);

	AddSectionHeader(ContentBox, TEXT("MATCH RULES"));
	ModeValueText = AddStepperRow(ContentBox, TEXT("Mode"), ModeMinusButton, ModePlusButton);
	PlayerCountValueText = AddStepperRow(ContentBox, TEXT("Max Players"), PlayerMinusButton, PlayerPlusButton);
	AICountValueText = AddStepperRow(ContentBox, TEXT("AI Players"), AIMinusButton, AIPlusButton);
	TargetScoreValueText = AddStepperRow(ContentBox, TEXT("Target Score"), ScoreMinusButton, ScorePlusButton);

	AddSectionHeader(ContentBox, TEXT("MAP"));
	USizeBox* MapCardBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	MapCardBox->SetWidthOverride(640.0f);
	MapCardBox->SetHeightOverride(104.0f);
	if (UVerticalBoxSlot* MapCardSlot = ContentBox->AddChildToVerticalBox(MapCardBox))
	{
		MapCardSlot->SetHorizontalAlignment(HAlign_Center);
		MapCardSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	UBorder* MapCardBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SelectedMapCard"));
	MapCardBorder->SetBrushColor(FLinearColor(0.075f, 0.082f, 0.094f, 1.0f));
	MapCardBorder->SetPadding(FMargin(12.0f));
	MapCardBox->AddChild(MapCardBorder);

	UHorizontalBox* MapCardRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SelectedMapRow"));
	MapCardBorder->SetContent(MapCardRow);

	USizeBox* PreviewBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SelectedMapPreviewBox"));
	PreviewBox->SetWidthOverride(132.0f);
	PreviewBox->SetHeightOverride(78.0f);
	SelectedMapPreviewImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SelectedMapPreviewImage"));
	SelectedMapPreviewImage->SetColorAndOpacity(FLinearColor(0.11f, 0.13f, 0.15f, 1.0f));
	PreviewBox->AddChild(SelectedMapPreviewImage);
	if (UHorizontalBoxSlot* PreviewSlot = MapCardRow->AddChildToHorizontalBox(PreviewBox))
	{
		PreviewSlot->SetVerticalAlignment(VAlign_Center);
		PreviewSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));
	}

	UVerticalBox* MapTextBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SelectedMapTextBox"));
	if (UHorizontalBoxSlot* MapTextSlot = MapCardRow->AddChildToHorizontalBox(MapTextBox))
	{
		MapTextSlot->SetHorizontalAlignment(HAlign_Fill);
		MapTextSlot->SetVerticalAlignment(VAlign_Center);
		MapTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		MapTextSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));
	}

	SelectedMapNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SelectedMapNameText"));
	SelectedMapNameText->SetText(FText::FromString(TEXT("Network Test Map")));
	{
		FSlateFontInfo Font = SelectedMapNameText->GetFont();
		Font.Size = 20;
		SelectedMapNameText->SetFont(Font);
		SelectedMapNameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.94f, 0.96f, 1.0f)));
	}
	MapTextBox->AddChildToVerticalBox(SelectedMapNameText);

	SelectedMapDetailsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SelectedMapDetailsText"));
	SelectedMapDetailsText->SetAutoWrapText(true);
	{
		FSlateFontInfo Font = SelectedMapDetailsText->GetFont();
		Font.Size = 14;
		SelectedMapDetailsText->SetFont(Font);
		SelectedMapDetailsText->SetColorAndOpacity(FSlateColor(FLinearColor(0.64f, 0.68f, 0.72f, 1.0f)));
	}
	MapTextBox->AddChildToVerticalBox(SelectedMapDetailsText);

	USizeBox* ChangeMapBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ChangeMapButtonBox"));
	ChangeMapBox->SetWidthOverride(132.0f);
	ChangeMapBox->SetHeightOverride(44.0f);
	ChangeMapButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ChangeMapButton"));
	UTextBlock* ChangeMapText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ChangeMapText->SetText(FText::FromString(TEXT("Change")));
	{
		FSlateFontInfo Font = ChangeMapText->GetFont();
		Font.Size = 16;
		ChangeMapText->SetFont(Font);
		ChangeMapText->SetJustification(ETextJustify::Center);
		ChangeMapText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.94f, 0.96f, 1.0f)));
	}
	ChangeMapButton->AddChild(ChangeMapText);
	ChangeMapBox->AddChild(ChangeMapButton);
	if (UHorizontalBoxSlot* ChangeSlot = MapCardRow->AddChildToHorizontalBox(ChangeMapBox))
	{
		ChangeSlot->SetVerticalAlignment(VAlign_Center);
	}

	AddSectionHeader(ContentBox, TEXT("CONNECTION"));
	PortTextBox = AddEditableTextBox(ContentBox, TEXT("Port"), TEXT("7777"));
	StatusText = AddNoticeText(ContentBox, TEXT(""), FLinearColor(0.08f, 0.74f, 0.64f, 1.0f), 18.0f);

	StartHostButton = AddMenuButton(ContentBox, TEXT("Start Host"), 14.0f);
	BackButton = AddMenuButton(ContentBox, TEXT("Back"));

	return Super::RebuildWidget();
}

void ULANHostSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ModeMinusButton) ModeMinusButton->OnClicked.AddDynamic(this, &ULANHostSettingsWidget::HandleModeMinusClicked);
	if (ModePlusButton) ModePlusButton->OnClicked.AddDynamic(this, &ULANHostSettingsWidget::HandleModePlusClicked);
	if (PlayerMinusButton) PlayerMinusButton->OnClicked.AddDynamic(this, &ULANHostSettingsWidget::HandlePlayerMinusClicked);
	if (PlayerPlusButton) PlayerPlusButton->OnClicked.AddDynamic(this, &ULANHostSettingsWidget::HandlePlayerPlusClicked);
	if (AIMinusButton) AIMinusButton->OnClicked.AddDynamic(this, &ULANHostSettingsWidget::HandleAIMinusClicked);
	if (AIPlusButton) AIPlusButton->OnClicked.AddDynamic(this, &ULANHostSettingsWidget::HandleAIPlusClicked);
	if (ScoreMinusButton) ScoreMinusButton->OnClicked.AddDynamic(this, &ULANHostSettingsWidget::HandleScoreMinusClicked);
	if (ScorePlusButton) ScorePlusButton->OnClicked.AddDynamic(this, &ULANHostSettingsWidget::HandleScorePlusClicked);
	if (ChangeMapButton) ChangeMapButton->OnClicked.AddDynamic(this, &ULANHostSettingsWidget::HandleChangeMapClicked);
	if (StartHostButton) StartHostButton->OnClicked.AddDynamic(this, &ULANHostSettingsWidget::HandleStartHostClicked);
	if (BackButton) BackButton->OnClicked.AddDynamic(this, &ULANHostSettingsWidget::HandleBackClicked);

	SelectDefaultMapForCurrentMode();
	RefreshValues();
}

void ULANHostSettingsWidget::HandleModeMinusClicked()
{
	ModeIndex = (ModeIndex + ModeNames.Num() - 1) % ModeNames.Num();
	SelectDefaultMapForCurrentMode();
	RefreshValues();
}

void ULANHostSettingsWidget::HandleModePlusClicked()
{
	ModeIndex = (ModeIndex + 1) % ModeNames.Num();
	SelectDefaultMapForCurrentMode();
	RefreshValues();
}

void ULANHostSettingsWidget::HandlePlayerMinusClicked()
{
	PlayerCount = FMath::Clamp(PlayerCount - 1, 1, 8);
	AICount = FMath::Clamp(AICount, 0, FMath::Max(0, PlayerCount - 1));
	RefreshValues();
}

void ULANHostSettingsWidget::HandlePlayerPlusClicked()
{
	PlayerCount = FMath::Clamp(PlayerCount + 1, 1, 8);
	AICount = FMath::Clamp(AICount, 0, FMath::Max(0, PlayerCount - 1));
	RefreshValues();
}

void ULANHostSettingsWidget::HandleAIMinusClicked()
{
	AICount = FMath::Clamp(AICount - 1, 0, FMath::Max(0, PlayerCount - 1));
	RefreshValues();
}

void ULANHostSettingsWidget::HandleAIPlusClicked()
{
	AICount = FMath::Clamp(AICount + 1, 0, FMath::Max(0, PlayerCount - 1));
	RefreshValues();
}

void ULANHostSettingsWidget::HandleScoreMinusClicked()
{
	TargetScore = FMath::Clamp(TargetScore - 1, 1, 99);
	RefreshValues();
}

void ULANHostSettingsWidget::HandleScorePlusClicked()
{
	TargetScore = FMath::Clamp(TargetScore + 1, 1, 99);
	RefreshValues();
}

void ULANHostSettingsWidget::HandleChangeMapClicked()
{
	TSubclassOf<UNetworkMapSelectWidget> ClassToSpawn = MapSelectWidgetClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = UNetworkMapSelectWidget::StaticClass();
	}

	UNetworkMapSelectWidget* MapSelectWidget = CreateWidget<UNetworkMapSelectWidget>(GetOwningPlayer(), ClassToSpawn);
	if (!MapSelectWidget)
	{
		return;
	}

	MapSelectWidget->SetupForMode(GetSelectedModeType(), PlayerCount, SelectedMap);
	MapSelectWidget->OnNetworkMapSelected.AddDynamic(this, &ULANHostSettingsWidget::HandleNetworkMapSelected);
	OpenChildMenu(MapSelectWidget);
}

void ULANHostSettingsWidget::HandleNetworkMapSelected(const FNetworkMapInfo& InSelectedMap)
{
	SelectedMap = InSelectedMap;
	RefreshValues();
}

void ULANHostSettingsWidget::HandleStartHostClicked()
{
	const FName MapName = ResolveLevelName(SelectedMap);
	if (MapName.IsNone())
	{
		if (StatusText)
		{
			StatusText->SetText(FText::FromString(TEXT("Map is not set.")));
		}
		return;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UBattleBlasterSessionSubsystem* SessionSubsystem = GI->GetSubsystem<UBattleBlasterSessionSubsystem>())
		{
			SessionSubsystem->HostNetworkGame(BuildMatchSettings());
		}
	}
}

void ULANHostSettingsWidget::RefreshValues()
{
	if (ModeValueText)
	{
		ModeValueText->SetText(FText::FromString(ModeNames.IsValidIndex(ModeIndex) ? ModeNames[ModeIndex] : TEXT("Deathmatch")));
	}
	if (PlayerCountValueText)
	{
		PlayerCountValueText->SetText(FText::AsNumber(PlayerCount));
	}
	if (AICountValueText)
	{
		AICountValueText->SetText(FText::AsNumber(AICount));
	}
	if (TargetScoreValueText)
	{
		TargetScoreValueText->SetText(FText::AsNumber(TargetScore));
	}
	if (SelectedMapNameText)
	{
		SelectedMapNameText->SetText(FText::FromString(SelectedMap.MapDisplayName));
	}
	if (SelectedMapDetailsText)
	{
		SelectedMapDetailsText->SetText(FormatSelectedMapDetails());
	}
	if (SelectedMapPreviewImage)
	{
		if (SelectedMap.MapThumbnail)
		{
			SelectedMapPreviewImage->SetBrushFromTexture(SelectedMap.MapThumbnail, true);
			SelectedMapPreviewImage->SetColorAndOpacity(FLinearColor::White);
		}
		else
		{
			SelectedMapPreviewImage->SetBrushFromTexture(nullptr);
			SelectedMapPreviewImage->SetColorAndOpacity(FLinearColor(0.11f, 0.13f, 0.15f, 1.0f));
		}
	}
	if (StatusText)
	{
		StatusText->SetText(FText::Format(
			FText::FromString(TEXT("Ready: LAN {0} on {1} will open as a listen server.")),
			FText::FromString(ModeNames.IsValidIndex(ModeIndex) ? ModeNames[ModeIndex] : TEXT("Deathmatch")),
			FText::FromString(SelectedMap.MapDisplayName)));
	}
}

void ULANHostSettingsWidget::SelectDefaultMapForCurrentMode()
{
	UNetworkMapSelectWidget* ClassDefault = nullptr;
	TSubclassOf<UNetworkMapSelectWidget> ClassToRead = MapSelectWidgetClass;
	if (!ClassToRead)
	{
		ClassToRead = UNetworkMapSelectWidget::StaticClass();
	}

	if (ClassToRead)
	{
		ClassDefault = ClassToRead->GetDefaultObject<UNetworkMapSelectWidget>();
	}

	const TArray<FNetworkMapInfo>* Maps = nullptr;
	if (ClassDefault)
	{
		switch (GetSelectedModeType())
		{
		case ENetworkGameModeType::TeamDeathmatch:
			Maps = &ClassDefault->TeamDeathmatchMaps;
			break;
		case ENetworkGameModeType::MOBA:
			Maps = &ClassDefault->MOBAMaps;
			break;
		case ENetworkGameModeType::TeamMOBA:
			Maps = &ClassDefault->TeamMOBAMaps;
			break;
		case ENetworkGameModeType::Deathmatch:
		default:
			Maps = &ClassDefault->DeathmatchMaps;
			break;
		}
	}

	if (Maps && Maps->Num() > 0)
	{
		SelectedMap = (*Maps)[0];
		return;
	}

	SelectedMap = FNetworkMapInfo();
	SelectedMap.LevelName = FName(TEXT("/Game/Maps/NetworkBattle/NetworkBattleTestMap"));
}

ENetworkGameModeType ULANHostSettingsWidget::GetSelectedModeType() const
{
	switch (ModeIndex)
	{
	case 1:
		return ENetworkGameModeType::TeamDeathmatch;
	case 2:
		return ENetworkGameModeType::MOBA;
	case 3:
		return ENetworkGameModeType::TeamMOBA;
	case 0:
	default:
		return ENetworkGameModeType::Deathmatch;
	}
}

FText ULANHostSettingsWidget::FormatSelectedMapDetails() const
{
	return FText::Format(
		FText::FromString(TEXT("{0}  |  {1}-{2} players")),
		FText::FromName(ResolveLevelName(SelectedMap)),
		FText::AsNumber(SelectedMap.MinPlayers),
		FText::AsNumber(SelectedMap.MaxPlayers));
}

FNetworkMatchSettings ULANHostSettingsWidget::BuildMatchSettings() const
{
	FNetworkMatchSettings Settings;
	Settings.ConnectionType = EBattleBlasterNetworkConnectionType::LAN;
	Settings.MapName = ResolveLevelName(SelectedMap);
	Settings.Port = 7777;
	Settings.MaxPlayers = PlayerCount;
	Settings.AICount = FMath::Clamp(AICount, 0, FMath::Max(0, PlayerCount - 1));
	Settings.TargetScore = TargetScore;

	const FString PortText = PortTextBox ? PortTextBox->GetText().ToString().TrimStartAndEnd() : TEXT("7777");
	const int32 ParsedPort = FCString::Atoi(*PortText);
	if (ParsedPort > 0)
	{
		Settings.Port = FMath::Clamp(ParsedPort, 1, 65535);
	}

	Settings.ModeType = GetSelectedModeType();
	switch (Settings.ModeType)
	{
	case ENetworkGameModeType::TeamDeathmatch:
		Settings.TeamCount = 2;
		break;
	case ENetworkGameModeType::MOBA:
		Settings.TeamCount = FMath::Max(1, PlayerCount);
		break;
	case ENetworkGameModeType::TeamMOBA:
		Settings.TeamCount = 2;
		break;
	case ENetworkGameModeType::Deathmatch:
	default:
		Settings.TeamCount = 1;
		break;
	}

	return Settings;
}

FName ULANHostSettingsWidget::ResolveLevelName(const FNetworkMapInfo& MapInfo)
{
	if (!MapInfo.LevelName.IsNone())
	{
		return MapInfo.LevelName;
	}

	const FString LongPackageName = MapInfo.LevelAsset.ToSoftObjectPath().GetLongPackageName();
	if (!LongPackageName.IsEmpty())
	{
		return FName(*LongPackageName);
	}

	const FString AssetName = MapInfo.LevelAsset.GetAssetName();
	return AssetName.IsEmpty() ? NAME_None : FName(*AssetName);
}
