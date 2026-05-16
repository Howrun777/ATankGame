#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BuffSlotWidget.generated.h"

// 前向声明，加快编译速度
class UImage;
class UTextBlock;
class UTexture2D;

UCLASS()
class BATTLEBLASTER_API UBuffSlotWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// 使用 BindWidget 绑定 UMG 里的控件
	// 名字必须和 UMG 里的完全一致
	UPROPERTY(meta = (BindWidget))
	UImage* IconImage;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TimeText;

public:
	// 核心函数：由列表容器调用，用来刷新这张卡片显示的内容
	void UpdateSlot(UTexture2D* InIcon, float RemainingTime);

private:
	UPROPERTY()
	UTexture2D* CachedIcon = nullptr;

	int32 CachedSecondsDisplay = INDEX_NONE;
	bool bCachedWarningColor = false;
};
