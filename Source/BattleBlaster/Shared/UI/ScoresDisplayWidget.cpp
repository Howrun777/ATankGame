#include "Shared/UI/ScoresDisplayWidget.h"
#include "Components/TextBlock.h"

void UScoresDisplayWidget::InitTargetScore(int32 TargetScore)
{
	// 格式化为 "/ 13"
	FString TargetStr = FString::Printf(TEXT("/ %d"), TargetScore);
	FText TargetTxt = FText::FromString(TargetStr);

	if (TargetText_0)
	{
		TargetText_0->SetText(TargetTxt);
	}

	if (TargetText_1)
	{
		TargetText_1->SetText(TargetTxt);
	}

	if (TargetText_2)
	{
		TargetText_2->SetText(TargetTxt);
	}

	if (TargetText_3)
	{
		TargetText_3->SetText(TargetTxt);
	}
}

void UScoresDisplayWidget::SetVisiblePlayerCount(int32 PlayerCount)
{
	if (HorizontalBox_2)
	{
		HorizontalBox_2->SetVisibility(PlayerCount > 2 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UScoresDisplayWidget::UpdateScores(int32 ScoreP0, int32 ScoreP1)
{
	if (ScoreText_0)
	{
		ScoreText_0->SetText(FText::AsNumber(ScoreP0));
	}

	if (ScoreText_1)
	{
		ScoreText_1->SetText(FText::AsNumber(ScoreP1));
	}
}

void UScoresDisplayWidget::UpdateScoresFour(int32 ScoreP0, int32 ScoreP1, int32 ScoreP2, int32 ScoreP3)
{
	if (ScoreText_0)
	{
		ScoreText_0->SetText(FText::AsNumber(ScoreP0));
	}
	if (ScoreText_1)
	{
		ScoreText_1->SetText(FText::AsNumber(ScoreP1));
	}
	if (ScoreText_2)
	{
		ScoreText_2->SetText(FText::AsNumber(ScoreP2));
	}
	if (ScoreText_3)
	{
		ScoreText_3->SetText(FText::AsNumber(ScoreP3));
	}
}

void UScoresDisplayWidget::UpdateTeamScores(int32 RedScore, int32 BlueScore)
{
	// 团队模式：ScoreText_0显示红色阵营分数，ScoreText_1显示蓝色阵营分数
	if (ScoreText_0)
	{
		ScoreText_0->SetText(FText::AsNumber(RedScore));
	}
	if (ScoreText_1)
	{
		ScoreText_1->SetText(FText::AsNumber(BlueScore));
	}
}

void UScoresDisplayWidget::UpdateMatchTimer(int32 TimeInSeconds)
{
	if (MatchTimerText)
	{
		// 计算分钟和秒
		// 假如 TimeInSeconds = 125
		// Minutes = 2
		// Seconds = 5
		int32 Minutes = TimeInSeconds / 60;
		int32 Seconds = TimeInSeconds % 60;

		// 格式化字符串: %02d 表示如果数字小于10，前面补0 (例如 5 变成 05)
		FString TimeStr = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);

		MatchTimerText->SetText(FText::FromString(TimeStr));
	}
}
