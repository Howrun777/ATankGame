// Fill out your copyright notice in the Description page of Project Settings.


#include "Shared/UI/BulletsWidget.h"
#include "Components/TextBlock.h"

void UBulletsWidget::SetAmmoText(int32 CurrentAmmo, int32 MaxAmmo)
{
    // 设置当前弹药数
    if (CurrentAmmoText)
    {
        CurrentAmmoText->SetText(FText::AsNumber(CurrentAmmo));
    }

    // 设置最大弹药数
    if (MaxAmmoText)
    {
        MaxAmmoText->SetText(FText::AsNumber(MaxAmmo));
    }
}
