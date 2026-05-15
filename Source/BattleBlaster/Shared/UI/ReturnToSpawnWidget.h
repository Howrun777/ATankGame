// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReturnToSpawnWidget.generated.h"

// 前向声明进度条组件
class UProgressBar;

UCLASS()
class BATTLEBLASTER_API UReturnToSpawnWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 更新进度条百分比的函数
	void UpdateProgress(float InProgress);

protected:
	// 核心魔法：BindWidget 要求 UMG 蓝图里必须有一个名字完全叫 "ReturnProgressBar" 的进度条组件！
	UPROPERTY(meta = (BindWidget))
	UProgressBar* ReturnProgressBar;
};