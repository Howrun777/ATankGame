#pragma once

#include "CoreMinimal.h"
#include "Core/Networking/BattleBlasterNetworkTypes.h"
#include "Modes/Network/UI/Menu/NetworkMenuWidgetBase.h"
#include "LANHostSettingsWidget.generated.h"

class UBattleBlasterSessionSubsystem;
class UButton;
class UEditableTextBox;
class UImage;
class UNetworkMapSelectWidget;
class UTextBlock;

UCLASS()
class BATTLEBLASTER_API ULANHostSettingsWidget : public UNetworkMenuWidgetBase
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Maps")
	TSubclassOf<UNetworkMapSelectWidget> MapSelectWidgetClass;

private:
	UPROPERTY()
	UTextBlock* ModeValueText = nullptr;

	UPROPERTY()
	UTextBlock* PlayerCountValueText = nullptr;

	UPROPERTY()
	UTextBlock* AICountValueText = nullptr;

	UPROPERTY()
	UTextBlock* TargetScoreValueText = nullptr;

	UPROPERTY()
	UTextBlock* SelectedMapNameText = nullptr;

	UPROPERTY()
	UTextBlock* SelectedMapDetailsText = nullptr;

	UPROPERTY()
	UImage* SelectedMapPreviewImage = nullptr;

	UPROPERTY()
	UEditableTextBox* PortTextBox = nullptr;

	UPROPERTY()
	UTextBlock* StatusText = nullptr;

	UPROPERTY()
	UButton* ModeMinusButton = nullptr;

	UPROPERTY()
	UButton* ModePlusButton = nullptr;

	UPROPERTY()
	UButton* PlayerMinusButton = nullptr;

	UPROPERTY()
	UButton* PlayerPlusButton = nullptr;

	UPROPERTY()
	UButton* AIMinusButton = nullptr;

	UPROPERTY()
	UButton* AIPlusButton = nullptr;

	UPROPERTY()
	UButton* ScoreMinusButton = nullptr;

	UPROPERTY()
	UButton* ScorePlusButton = nullptr;

	UPROPERTY()
	UButton* ChangeMapButton = nullptr;

	UPROPERTY()
	UButton* StartHostButton = nullptr;

	UPROPERTY()
	UButton* BackButton = nullptr;

	int32 ModeIndex = 0;
	int32 PlayerCount = 2;
	int32 AICount = 0;
	int32 TargetScore = 7;
	FNetworkMapInfo SelectedMap;

	const TArray<FString> ModeNames = { TEXT("Deathmatch"), TEXT("TeamDeathmatch"), TEXT("MOBA"), TEXT("TeamMOBA") };

	UFUNCTION()
	void HandleModeMinusClicked();

	UFUNCTION()
	void HandleModePlusClicked();

	UFUNCTION()
	void HandlePlayerMinusClicked();

	UFUNCTION()
	void HandlePlayerPlusClicked();

	UFUNCTION()
	void HandleAIMinusClicked();

	UFUNCTION()
	void HandleAIPlusClicked();

	UFUNCTION()
	void HandleScoreMinusClicked();

	UFUNCTION()
	void HandleScorePlusClicked();

	UFUNCTION()
	void HandleChangeMapClicked();

	UFUNCTION()
	void HandleNetworkMapSelected(const FNetworkMapInfo& InSelectedMap);

	UFUNCTION()
	void HandleStartHostClicked();

	void RefreshValues();
	void SelectDefaultMapForCurrentMode();
	ENetworkGameModeType GetSelectedModeType() const;
	FText FormatSelectedMapDetails() const;
	FNetworkMatchSettings BuildMatchSettings() const;
	static FName ResolveLevelName(const FNetworkMapInfo& MapInfo);
};
