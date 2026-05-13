// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Tank.h"
#include "MultiBattleGameOverWidget.generated.h"

/**
 *
 */
UCLASS()
class BATTLEBLASTER_API UMultiBattleGameOverWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	// GameMode 判定胜负后调用，只传入胜利者索引。
	// Widget 内部从 PlayerState 读取各玩家的 KDA，自己计算 SkillScore。
	void InitResultData(int32 InWinnerIndex);

protected:
	virtual void NativeConstruct() override;

	// ==== 绑定到 UMG 的控件 ====
	// 按钮
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Restart;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_ReturnMenu;

	// 显示胜利阵营文字（例如: "红色"），颜色也在这里设置
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_WinnerCamp;

	// 胜利坦克头像
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* Img_TankPortrait;

	// 每个玩家的 KDA 和评分文本（左侧四行）
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

	// 可选: 每一行的容器, 用于根据玩家数量隐藏多余行
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* RedRow;
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* BlueRow;
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* GreenRow;
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* YellowRow;

	// Tank 类型 -> 显示图片 的映射, 允许在蓝图里配置
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result", meta = (AllowPrivateAccess = "true"))
	TMap<TSubclassOf<ATank>, UTexture2D*> TankPortraitMap;

	// 历史榜单 ScrollBox（右侧列表）
	UPROPERTY(meta = (BindWidgetOptional))
	UScrollBox* HistoryScrollBox;

	/** 历史榜单每行字体大小（默认 32） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result", meta = (ClampMin = 12, ClampMax = 48))
	int32 HistoryListFontSize = 30;

	// 当本局成绩未进入前 50 名时，在列表下方额外显示 "[* 分数  K-D-A]" 文本
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* OutOfRangeText;

private:
	// 按钮回调
	UFUNCTION()
	void HandleRestartClicked();

	UFUNCTION()
	void HandleReturnMenuClicked();

	// 工具函数: 根据玩家索引返回阵营名字和颜色
	void GetCampInfo(int32 PlayerIndex, FText& OutCampName, FLinearColor& OutColor) const;

	// 工具函数: 根据索引拿到一行的 KDA / SkillScore 文本和行容器
	void GetRowWidgets(int32 PlayerIndex, UTextBlock*& OutKDAText, UTextBlock*& OutSkillScoreText, UPanelWidget*& OutRow) const;
};
