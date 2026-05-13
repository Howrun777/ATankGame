// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DeathScreenWidget.generated.h"

UCLASS()
class BATTLEBLASTER_API UDeathScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 更新倒计时显示（精确到0.1秒）
	UFUNCTION(BlueprintCallable, Category = "UI")
	void UpdateRespawnCountdown(float TimeRemaining);

	// 显示界面
	UFUNCTION(BlueprintCallable, Category = "UI")
	void Show();

	// 隐藏界面
	UFUNCTION(BlueprintCallable, Category = "UI")
	void Hide();

protected:
	// 绑定倒计时文本控件
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Text_RespawnCountdown;

	// 绑定背景面板
	UPROPERTY(meta = (BindWidget))
	class UBorder* BackgroundBorder;

	// 初始化
	virtual void NativeConstruct() override;

	// 格式化倒计时文本
	FText FormatCountdown(float TimeSeconds) const;
};
