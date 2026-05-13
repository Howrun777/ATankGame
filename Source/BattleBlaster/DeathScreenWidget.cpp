// Fill out your copyright notice in the Description page of Project Settings.

#include "DeathScreenWidget.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"

void UDeathScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 默认隐藏
	Hide();
}

void UDeathScreenWidget::UpdateRespawnCountdown(float TimeRemaining)
{
	if (Text_RespawnCountdown)
	{
		Text_RespawnCountdown->SetText(FormatCountdown(TimeRemaining));
	}
}

void UDeathScreenWidget::Show()
{
	if (BackgroundBorder)
	{
		BackgroundBorder->SetVisibility(ESlateVisibility::Visible);
	}
}

void UDeathScreenWidget::Hide()
{
	if (BackgroundBorder)
	{
		BackgroundBorder->SetVisibility(ESlateVisibility::Hidden);
	}
}

FText UDeathScreenWidget::FormatCountdown(float TimeSeconds) const
{
	TimeSeconds = FMath::Max(0.0f, TimeSeconds);

	int32 WholeSeconds = FMath::FloorToInt(TimeSeconds);
	int32 TenthSeconds = FMath::FloorToInt((TimeSeconds - WholeSeconds) * 100);

	FString TextString = FString::Printf(TEXT("%02d.%02d"), WholeSeconds, TenthSeconds);
	return FText::FromString(TextString);
}
