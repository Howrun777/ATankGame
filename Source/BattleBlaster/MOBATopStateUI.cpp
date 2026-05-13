// Fill out your copyright notice in the Description page of Project Settings.

#include "MOBATopStateUI.h"
#include "TankMOBAGameMode.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"

void UMOBATopStateUI::NativeConstruct()
{
	Super::NativeConstruct();

	// 初始化防御塔图片数组
	TurretImages.Add(TurretImage_0);
	TurretImages.Add(TurretImage_1);
	TurretImages.Add(TurretImage_2);
	TurretImages.Add(TurretImage_3);

	// 获取当前阵营数量并设置可见图片
	int32 CampCount = 0;
	if (ATankMOBAGameMode* MOBAGameMode = GetWorld()->GetAuthGameMode<ATankMOBAGameMode>())
	{
		CampCount = MOBAGameMode->GetActiveCampCount();
	}
	SetupVisibleTurretCount(CampCount);

	// 启动时间刷新定时器
	GetWorld()->GetTimerManager().SetTimer(
		TimeRefreshTimerHandle,
		this,
		&UMOBATopStateUI::RefreshGameTime,
		TimeRefreshInterval,
		true
	);

	// 立即刷新一次时间
	RefreshGameTime();
}

void UMOBATopStateUI::NativeDestruct()
{
	// 清除定时器
	GetWorld()->GetTimerManager().ClearTimer(TimeRefreshTimerHandle);

	Super::NativeDestruct();
}

void UMOBATopStateUI::SetupVisibleTurretCount(int32 CampCount)
{
	// 确保 CampCount 在有效范围内 [0, 4]
	CampCount = FMath::Clamp(CampCount, 0, 4);

	for (int32 i = 0; i < TurretImages.Num(); ++i)
	{
		if (TurretImages[i])
		{
			TurretImages[i]->SetVisibility(i < CampCount ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		}
	}
}

void UMOBATopStateUI::RefreshGameTime()
{
	if (!TimeText)
	{
		return;
	}

	int32 MatchTime = 0;
	if (ATankMOBAGameMode* MOBAGameMode = GetWorld()->GetAuthGameMode<ATankMOBAGameMode>())
	{
		MatchTime = FMath::FloorToInt(MOBAGameMode->GetCurrentGameTime());
	}

	TimeText->SetText(FormatTime(MatchTime));
}

FText UMOBATopStateUI::FormatTime(int32 TotalSeconds) const
{
	int32 Minutes = TotalSeconds / 60;
	int32 Seconds = TotalSeconds % 60;

	// 格式化为 MM:SS
	return FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds));
}

void UMOBATopStateUI::HideTurretImage(int32 CampIndex)
{
	// 确保索引在有效范围内
	if (CampIndex < 0 || CampIndex >= TurretImages.Num())
	{
		return;
	}

	// 隐藏对应阵营的防御塔图片
	if (UImage* TurretImage = TurretImages[CampIndex])
	{
		TurretImage->SetVisibility(ESlateVisibility::Hidden);
	}
}
