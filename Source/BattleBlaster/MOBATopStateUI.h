// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MOBATopStateUI.generated.h"

class UImage;
class UTextBlock;

/**
 * MOBATopStateUI - MOBA 模式顶部状态 UI
 * 显示各阵营防御塔存活状态和游戏时间
 */
UCLASS()
class BATTLEBLASTER_API UMOBATopStateUI : public UUserWidget
{
	GENERATED_BODY()

public:
	// ================= UI 操作 =================

	// 隐藏指定阵营的防御塔图标
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	void HideTurretImage(int32 CampIndex);

protected:
	// ================= UUserWidget 生命周期 =================

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// ================= UI 初始化 =================

	// 根据阵营数量设置可见的防御塔图片
	void SetupVisibleTurretCount(int32 CampCount);

	// ================= 时间刷新 =================

	// 定时刷新游戏时间
	UFUNCTION()
	void RefreshGameTime();

	// 格式化时间显示 (MM:SS)
	FText FormatTime(int32 TotalSeconds) const;

	// ================= UI 绑定 =================

	// 防御塔状态图片 (最多4个阵营)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* TurretImage_0;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* TurretImage_1;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* TurretImage_2;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* TurretImage_3;

	// 游戏时间显示
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UTextBlock* TimeText;

	// ================= 缓存 =================

	// 防御塔图片数组 (方便批量操作)
	UPROPERTY()
	TArray<UImage*> TurretImages;

	// 时间刷新定时器句柄
	FTimerHandle TimeRefreshTimerHandle;

	// 时间刷新间隔 (秒)
	UPROPERTY(EditDefaultsOnly, Category = "MOBA")
	float TimeRefreshInterval = 1.0f;
};
