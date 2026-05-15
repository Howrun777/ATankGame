// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameSettingsMenuWidget.generated.h"

// 前向声明
class UWidgetSwitcher;
class UButton;

UCLASS()
class BATTLEBLASTER_API UGameSettingsMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 记住是谁打开了我，以便返回
	UPROPERTY()
	UUserWidget* ParentUI = nullptr;

protected:
	virtual bool Initialize() override;

	// === 绑定 UMG 中的控件（名字必须严格一致） ===

	UPROPERTY(meta = (BindWidget))
	UWidgetSwitcher* ContentSwitcher;
	
	UPROPERTY(meta = (BindWidget))
	UButton* Btn_TabGamepad;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_TabKBM;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_TabCustom;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_Back;

	// === 按钮点击事件 ===
	UFUNCTION()
	void OnGamepadTabClicked();

	UFUNCTION()
	void OnKBMTabClicked();

	UFUNCTION()
	void OnCustomTabClicked();

	UFUNCTION()
	void OnBackClicked();

	// 用于更新按钮的视觉状态（高亮当前选中的按钮）
	void UpdateTabButtonStyles(int32 ActiveIndex);
};
