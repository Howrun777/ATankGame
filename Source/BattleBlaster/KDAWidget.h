// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "KDAWidget.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEBLASTER_API UKDAWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	// 更新 KDA 显示（击杀-死亡-助攻）
	UFUNCTION(BlueprintCallable, Category = "KDA")
	void UpdateKDA(int32 Kills, int32 Deaths, int32 Assists);

	// 设置KDA Widget的颜色
	UFUNCTION(BlueprintCallable, Category = "KDA")
	void SetColor(FLinearColor Color);

protected:
	// 分别显示 K / D / A 的文本（可选绑定）
	UPROPERTY(EditAnywhere, meta = (BindWidgetOptional))
	UTextBlock* KillsText;

	UPROPERTY(EditAnywhere, meta = (BindWidgetOptional))
	UTextBlock* DeathsText;

	UPROPERTY(EditAnywhere, meta = (BindWidgetOptional))
	UTextBlock* AssistsText;

	// 如果你在 UMG 里只做了一个总的文本（例如 "1-2-3"），可以把它绑到这里
	UPROPERTY(EditAnywhere, meta = (BindWidgetOptional))
	UTextBlock* SummaryText;
};
