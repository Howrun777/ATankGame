#pragma once

#include "CoreMinimal.h"
#include "Modes/Network/UI/Menu/NetworkMenuWidgetBase.h"
#include "LANHostSettingsWidget.generated.h"

class UBattleBlasterSessionSubsystem;
class UButton;
class UEditableTextBox;
class UTextBlock;

UCLASS()
class BATTLEBLASTER_API ULANHostSettingsWidget : public UNetworkMenuWidgetBase
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

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
	UEditableTextBox* MapNameTextBox = nullptr;

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
	UButton* StartHostButton = nullptr;

	UPROPERTY()
	UButton* BackButton = nullptr;

	int32 ModeIndex = 0;
	int32 PlayerCount = 2;
	int32 AICount = 0;
	int32 TargetScore = 7;

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
	void HandleStartHostClicked();

	void RefreshValues();
	FString BuildOptionsString() const;
};
