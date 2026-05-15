#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/PanelWidget.h"
#include "ScoresDisplayWidget.generated.h"

// 前置声明
class UTextBlock;

UCLASS()
class BATTLEBLASTER_API UScoresDisplayWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// --- 功能函数 (供 GameMode 调用) ---

	// 初始化目标分数 (显示 "/ 13")
	UFUNCTION(BlueprintCallable, Category = "UI")
	void InitTargetScore(int32 TargetScore);

	// 设置可见玩家数：2 时只显示一行分数，3 或 4 时显示两行（四格分数）
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetVisiblePlayerCount(int32 PlayerCount);

	// 更新比分（2 人时只更新 P0/P1；3–4 人时更新全部）
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateScores(int32 ScoreP0, int32 ScoreP1);

	// 更新比分（四格，供 3–4 人使用）
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateScoresFour(int32 ScoreP0, int32 ScoreP1, int32 ScoreP2, int32 ScoreP3);

	// 更新团队比分（红色 vs 蓝色阵营，供团队模式使用）
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateTeamScores(int32 RedScore, int32 BlueScore);

	/**
	 * 更新比赛倒计时/时间
	 * @param TimeInSeconds 剩余秒数或经过的秒数
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateMatchTimer(int32 TimeInSeconds);

protected:
	// --- UI 组件绑定 ---
	// 必须在 UMG 编辑器中将控件名字改成一模一样！

	// [第一行左侧] 玩家0 分数
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ScoreText_0;

	// [第一行左侧] 玩家0 目标 (例如 "/13")
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TargetText_0;

	// [第一行右侧] 玩家1 分数
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ScoreText_1;

	// [第一行右侧] 玩家1 目标
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TargetText_1;

	// [第二行] 三/四人时显示，二人时隐藏；UMG 中命名为 HorizontalBox_2
	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* HorizontalBox_2;

	// [第二行左侧] 玩家2 分数
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* ScoreText_2;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TargetText_2;

	// [第二行右侧] 玩家3 分数
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* ScoreText_3;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TargetText_3;

	// [中间下方] 比赛时间 (例如 "02:30")
	UPROPERTY(meta = (BindWidget))
	UTextBlock* MatchTimerText;
};
