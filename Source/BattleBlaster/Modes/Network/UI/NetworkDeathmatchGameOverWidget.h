#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NetworkDeathmatchGameOverWidget.generated.h"

class ANetworkDeathmatchGameState;
class UButton;
class UCanvasPanel;
class UImage;
class UPanelWidget;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class ATank;

UCLASS()
class BATTLEBLASTER_API UNetworkDeathmatchGameOverWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Network|Deathmatch")
	void InitResultData(int32 InWinnerSlotId);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Restart = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_ReturnMenu = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_WinnerCamp = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* Img_TankPortrait = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* RedKDAText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* RedScoresText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* BlueKDAText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* BlueScoresText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* GreenKDAText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* GreenScoresText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* YellowKDAText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* YellowScoresText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* RedRow = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* BlueRow = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* GreenRow = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* YellowRow = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Deathmatch")
	TMap<TSubclassOf<ATank>, UTexture2D*> TankPortraitMap;

	UPROPERTY(meta = (BindWidgetOptional))
	UScrollBox* HistoryScrollBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* OutOfRangeText = nullptr;

private:
	UPROPERTY()
	UCanvasPanel* TempRootCanvas = nullptr;

	UPROPERTY()
	UVerticalBox* TempContentBox = nullptr;

	UPROPERTY()
	UTextBlock* TempTitleText = nullptr;

	UPROPERTY()
	UTextBlock* TempWinnerText = nullptr;

	UPROPERTY()
	UTextBlock* TempRowsText = nullptr;

	UPROPERTY()
	UButton* TempReturnMenuButton = nullptr;

	UPROPERTY()
	UTextBlock* TempReturnMenuText = nullptr;

	UFUNCTION()
	void HandleRestartClicked();

	UFUNCTION()
	void HandleReturnMenuClicked();

	void BuildTemporaryWidgetTree();
	bool HasBlueprintBoundLayout() const;
	FString BuildResultRowsText(int32 PlayerCount, int32 TargetScore) const;
	void GetSlotInfo(int32 SlotId, FText& OutName, FLinearColor& OutColor) const;
	void GetRowWidgets(int32 SlotId, UTextBlock*& OutKDAText, UTextBlock*& OutScoreText, UPanelWidget*& OutRow) const;
	int32 CalculateSkillScore(int32 Kills, int32 Deaths, int32 Assists, int32 TargetScore) const;
	void GetKDAForSlot(int32 SlotId, int32& OutKills, int32& OutDeaths, int32& OutAssists) const;
	const ANetworkDeathmatchGameState* GetDeathmatchGameState() const;
};
