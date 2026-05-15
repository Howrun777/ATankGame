// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BulletsWidget.generated.h"

class UTextBlock; // 前置声明

UCLASS()
class BATTLEBLASTER_API UBulletsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    // 更新弹药显示的函数，供外部调用
    void SetAmmoText(int32 CurrentAmmo, int32 MaxAmmo);

protected:
    // meta=(BindWidget) 意味着：UE会自动寻找在UMG编辑器里名字完全一样的控件并绑定
    // 这样你就不用去写 GetWidgetFromName 了
    UPROPERTY(meta = (BindWidget))
    UTextBlock* CurrentAmmoText;

    UPROPERTY(meta = (BindWidget))

    UTextBlock* MaxAmmoText;
};
