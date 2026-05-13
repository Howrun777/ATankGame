#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Tank.h"
#include "TankStageOverWidget.generated.h"

// Tank 类型与显示图片的映射（方便在蓝图中注册）
USTRUCT(BlueprintType)
struct FTankImageEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TankImage")
	TSubclassOf<ATank> TankClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TankImage")
	UTexture2D* PortraitTexture = nullptr;
};


/**
 * 单人闯关模式游戏结束菜单 (WBP_TankStageOverWidget)
 */
UCLASS()
class BATTLEBLASTER_API UTankStageOverWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 刷新显示：当前关卡、历史最高、游戏时间、玩家坦克图片
	UFUNCTION(BlueprintCallable, Category = "GameOver")
	void RefreshDisplay(int32 CurrentLevel, int32 HighestLevel, float GameTimeSeconds, TSubclassOf<ATank> PlayerTankClass);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 与 WBP 命名一致
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Restart;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_ReturnMenu;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_CurrentLevel;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_HighestLevel;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_MatchTime;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* Img_TankPortrait;

	// Tank 类型 -> 图片映射表（在蓝图中配置）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TankImage")
	TArray<FTankImageEntry> TankImageMap;

private:
	UFUNCTION()
	void OnRestartClicked();

	UFUNCTION()
	void OnReturnMenuClicked();

	// 根据坦克类型从 Map 中取图片
	UTexture2D* GetTankPortrait(TSubclassOf<ATank> TankClass) const;

	// 将秒数格式化为 00:00:00
	FString FormatGameTime(float TotalSeconds) const;
};
