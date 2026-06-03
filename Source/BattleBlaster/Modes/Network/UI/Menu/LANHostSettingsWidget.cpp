#include "Modes/Network/UI/Menu/LANHostSettingsWidget.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Core/Networking/BattleBlasterSessionSubsystem.h"

TSharedRef<SWidget> ULANHostSettingsWidget::RebuildWidget()
{
	UVerticalBox* ContentBox = nullptr;
	BuildMenuRoot(TEXT("Host LAN Game"), TEXT("Configure a listen server session for players on the same network."), ContentBox);

	ModeValueText = AddStepperRow(ContentBox, TEXT("Mode"), ModeMinusButton, ModePlusButton);
	PlayerCountValueText = AddStepperRow(ContentBox, TEXT("Max Players"), PlayerMinusButton, PlayerPlusButton);
	AICountValueText = AddStepperRow(ContentBox, TEXT("AI Players"), AIMinusButton, AIPlusButton);
	TargetScoreValueText = AddStepperRow(ContentBox, TEXT("Target Score"), ScoreMinusButton, ScorePlusButton);

	MapNameTextBox = AddEditableTextBox(ContentBox, TEXT("Map"), TEXT("NetworkBattleTestMap"));
	PortTextBox = AddEditableTextBox(ContentBox, TEXT("Port"), TEXT("7777"));
	StatusText = AddMenuText(ContentBox, TEXT(""), 17, FLinearColor(0.62f, 0.88f, 0.82f, 1.0f), 16.0f);

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
	if (StartHostButton) StartHostButton->OnClicked.AddDynamic(this, &ULANHostSettingsWidget::HandleStartHostClicked);
	if (BackButton) BackButton->OnClicked.AddDynamic(this, &ULANHostSettingsWidget::HandleBackClicked);

	RefreshValues();
}

void ULANHostSettingsWidget::HandleModeMinusClicked()
{
	ModeIndex = (ModeIndex + ModeNames.Num() - 1) % ModeNames.Num();
	RefreshValues();
}

void ULANHostSettingsWidget::HandleModePlusClicked()
{
	ModeIndex = (ModeIndex + 1) % ModeNames.Num();
	RefreshValues();
}

void ULANHostSettingsWidget::HandlePlayerMinusClicked()
{
	PlayerCount = FMath::Clamp(PlayerCount - 1, 1, 8);
	RefreshValues();
}

void ULANHostSettingsWidget::HandlePlayerPlusClicked()
{
	PlayerCount = FMath::Clamp(PlayerCount + 1, 1, 8);
	RefreshValues();
}

void ULANHostSettingsWidget::HandleAIMinusClicked()
{
	AICount = FMath::Clamp(AICount - 1, 0, 16);
	RefreshValues();
}

void ULANHostSettingsWidget::HandleAIPlusClicked()
{
	AICount = FMath::Clamp(AICount + 1, 0, 16);
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

void ULANHostSettingsWidget::HandleStartHostClicked()
{
	const FString MapName = MapNameTextBox ? MapNameTextBox->GetText().ToString() : TEXT("NetworkBattleTestMap");
	if (MapName.IsEmpty())
	{
		if (StatusText)
		{
			StatusText->SetText(FText::FromString(TEXT("Map name is empty.")));
		}
		return;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UBattleBlasterSessionSubsystem* SessionSubsystem = GI->GetSubsystem<UBattleBlasterSessionSubsystem>())
		{
			SessionSubsystem->HostListenServerWithOptions(FName(*MapName), BuildOptionsString());
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
	if (StatusText)
	{
		StatusText->SetText(ModeIndex == 0
			? FText::FromString(TEXT("Ready: LAN Deathmatch will open as a listen server."))
			: FText::FromString(TEXT("This mode is selectable for UI flow only right now.")));
	}
}

FString ULANHostSettingsWidget::BuildOptionsString() const
{
	const FString Port = PortTextBox ? PortTextBox->GetText().ToString() : TEXT("7777");
	const FString ModeName = ModeNames.IsValidIndex(ModeIndex) ? ModeNames[ModeIndex] : TEXT("Deathmatch");

	return FString::Printf(TEXT("listen?Port=%s?NetworkMode=%s?MaxPlayers=%d?AICount=%d?TargetScore=%d"),
		*Port,
		*ModeName,
		PlayerCount,
		AICount,
		TargetScore);
}
