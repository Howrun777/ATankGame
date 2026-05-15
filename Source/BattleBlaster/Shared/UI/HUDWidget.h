// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "HUDWidget.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEBLASTER_API UHUDWidget : public UUserWidget
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, meta = (BindWidgetOptional))
	UProgressBar* HealthBar;

	/** 护盾条（在血量栏上方），显示护盾值，最大值等于最大生命值 */
	UPROPERTY(EditAnywhere, meta = (BindWidgetOptional))
	UProgressBar* ShieldBar;

	void SetHealthBarPercent(float NewPercent);
	/** 设置护盾条进度 [0,1]，0 表示无护盾，1 表示满护盾（= MaxHealth） */
	void SetShieldBarPercent(float NewPercent);
};

