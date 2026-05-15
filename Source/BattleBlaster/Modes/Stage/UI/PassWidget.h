#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "PassWidget.generated.h"

/**
 * 单人闯关模式右上角（WBP_PassWidget）：当前关卡 + 最高历史记录 + 机会次数（爱心）
 * 仅在 TankStageGameMode 下显示
 * n 颗爱心 (n=玩家最大死亡次数)，每死亡一次替换一张为破碎爱心
 */
UCLASS()
class BATTLEBLASTER_API UPassWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 与 WBP_PassWidget 中命名一致
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_CurrentPass;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_Record;

	// 爱心图片（机会次数），与 WBP_PassWidget 中命名一致
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* HeartImage_0;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* HeartImage_1;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* HeartImage_2;

	// 在蓝图中指定：满爱心、破碎爱心贴图
	UPROPERTY(EditAnywhere, Category = "Hearts")
	UTexture2D* FullHeartTexture;

	UPROPERTY(EditAnywhere, Category = "Hearts")
	UTexture2D* BrokenHeartTexture;

	// 【新增】：绑定游戏时间文本框（注意：你的蓝图截图里叫 Text_GameTime）
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_GameTime;
private:
	void RefreshDisplay();
	void RefreshHearts(int32 MaxLives, int32 CurrentLives);
	float RefreshTimer = 0.0f;
	// 将秒数格式化为 00:00:00 的辅助函数（和结算界面的那个一样）
	FString FormatGameTime(float TotalSeconds) const;
};
