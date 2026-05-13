// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/PanelWidget.h"
#include "Tank.h"
#include "MOBAGameOverWidget.generated.h"

/**
 * MOBA 模式结算界面：从 ATankMOBAGameState 读取获胜阵营与各玩家 KDA，行为对齐 UMultiBattleGameOverWidget。
 */
UCLASS()
class BATTLEBLASTER_API UMOBAGameOverWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 从 GameState / GameInstance / PlayerState 拉取数据并刷新 UI */
	void InitResultData();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Restart;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_ReturnMenu;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_WinnerCamp;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* Img_TankPortrait;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* RedKDAText;
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* RedScoresText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* BlueKDAText;
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* BlueScoresText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* GreenKDAText;
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* GreenScoresText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* YellowKDAText;
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* YellowScoresText;

	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* RedRow;
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* BlueRow;
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* GreenRow;
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* YellowRow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result", meta = (AllowPrivateAccess = "true"))
	TMap<TSubclassOf<ATank>, UTexture2D*> TankPortraitMap;

	UPROPERTY(meta = (BindWidgetOptional))
	UScrollBox* HistoryScrollBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result", meta = (ClampMin = 12, ClampMax = 48))
	int32 HistoryListFontSize = 30;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* OutOfRangeText;

	/** 返回主菜单时展示的 MOBA 设置界面（如 WBP_MOBASetupWidget），在蓝图中指定 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	TSubclassOf<UUserWidget> MOBASetupMenuWidgetClass;

private:
	UFUNCTION()
	void HandleRestartClicked();

	UFUNCTION()
	void HandleReturnMenuClicked();

	void GetCampInfo(int32 CampIndex, FText& OutCampName, FLinearColor& OutColor) const;

	void GetRowWidgets(int32 PlayerIndex, UTextBlock*& OutKDAText, UTextBlock*& OutSkillScoreText, UPanelWidget*& OutRow) const;
};
