// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/PanelWidget.h"
#include "Shared/Pawns/Tank.h"
#include "TeamBattleGameOverWidget.generated.h"

/**
 * 团队战斗结束结算界面
 * 模仿 MultiBattleGameOverWidget 的架构：
 * - InitResultData 只接收获胜阵营索引
 * - Widget 内部自行从 PlayerState 读取 KDA
 * - 从 GameInstance 读取玩家数量，按人数显示战绩行
 * - SkillScore 根据 KDA 计算，不从外部传入
 */
UCLASS()
class BATTLEBLASTER_API UTeamBattleGameOverWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// GameMode 判定胜负后调用，只传入获胜阵营索引 (0=红色, 1=蓝色)
	// Widget 内部从 PlayerState 读取各玩家的 KDA，自己计算 SkillScore
	void InitResultData(int32 InWinnerCampIndex);

protected:
	virtual void NativeConstruct() override;

	// ==== 绑定到 UMG 的控件 ====
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Restart;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_ReturnMenu;

	// 胜利阵营文字
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_WinnerCamp;

	// 阵营总分显示
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_RedTeamScore;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_BlueTeamScore;

	// 每个玩家的 KDA 和评分文本（最多4行，按 SlotId 排列：0,1,2,3）
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* RedKDAText_1;
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* RedSkillScoreText_1;
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* RedRow_1;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* RedKDAText_2;
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* RedSkillScoreText_2;
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* RedRow_2;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* BlueKDAText_1;
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* BlueSkillScoreText_1;
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* BlueRow_1;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* BlueKDAText_2;
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* BlueSkillScoreText_2;
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* BlueRow_2;

	// 胜利坦克头像
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* Img_TankPortrait;

	// Tank 类型 -> 显示图片 的映射
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result", meta = (AllowPrivateAccess = "true"))
	TMap<TSubclassOf<ATank>, UTexture2D*> TankPortraitMap;

	// 历史榜单 ScrollBox
	UPROPERTY(meta = (BindWidgetOptional))
	UScrollBox* HistoryScrollBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result", meta = (ClampMin = 12, ClampMax = 48))
	int32 HistoryListFontSize = 30;

	// 本局未进入前50名时的提示
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* OutOfRangeText;

private:
	UFUNCTION()
	void HandleRestartClicked();

	UFUNCTION()
	void HandleReturnMenuClicked();

	// 工具函数：根据 SlotId 返回阵营名字和颜色（0/2=红，1/3=蓝）
	void GetCampInfo(int32 SlotId, FText& OutCampName, FLinearColor& OutColor) const;

	// 工具函数：根据 SlotId 拿到对应行的控件
	void GetRowWidgets(int32 SlotId,
		UTextBlock*& OutKDAText,
		UTextBlock*& OutSkillScoreText,
		UPanelWidget*& OutRow) const;
};
