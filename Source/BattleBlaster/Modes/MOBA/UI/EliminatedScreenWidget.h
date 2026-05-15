// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EliminatedScreenWidget.generated.h"

UCLASS()
class BATTLEBLASTER_API UEliminatedScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 显示界面
	UFUNCTION(BlueprintCallable, Category = "UI")
	void Show();

	// 隐藏界面
	UFUNCTION(BlueprintCallable, Category = "UI")
	void Hide();

	// 切换观战视角按钮点击事件
	UFUNCTION()
	void OnSwitchSpectateClicked();

protected:
	// 绑定背景面板
	UPROPERTY(meta = (BindWidget))
	class UBorder* BackgroundBorder;

	// 绑定主文本
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Text_Message;

	// 绑定切换观战按钮
	UPROPERTY(meta = (BindWidget))
	class UButton* Btn_SwitchSpectate;

	// 初始化
	virtual void NativeConstruct() override;

	// 按钮点击事件
	UFUNCTION()
	void OnSwitchSpectateClickedInternal();
};
