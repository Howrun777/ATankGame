#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/GameModeBase.h"
#include "Components/TextBlock.h"

#include "TankStageStartWidget.generated.h"

class UButton;
class UImage;
class UBorder;
class UTexture2D;

// 单人模式使用的 Tank 选项（与多人死斗一致：图标 + 蓝图类）
USTRUCT(BlueprintType)
struct FTankOptionSingle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TankSelect")
	UTexture2D* Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TankSelect")
	TSubclassOf<APawn> TankClass = nullptr;
};

UCLASS()
class BATTLEBLASTER_API UTankStageStartWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 【关键】：记住是谁打开了我，以便返回
	UPROPERTY()
	UUserWidget* ParentUI = nullptr;

	// 指定单人闯关模式对应的 GameMode 蓝图
	UPROPERTY(EditAnywhere, Category = "Game Settings")
	TSubclassOf<AGameModeBase> CampaignGameModeClass;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ================= 绑定的 UI 控件 =================
	UPROPERTY(meta = (BindWidget))
	UButton* Btn_Campaign; // 单人闯关按钮

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_Defense;  // 单人守卫按钮

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_Back;     // 返回按钮

	// ================= 单人坦克选择（单槽位） =================
	// 与 UMG 里 Overlay_1 下的命名一致：TankImage_1、HoverFrame_1
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* TankImage_1;

	UPROPERTY(meta = (BindWidgetOptional))
	UBorder* HoverFrame_1;

	// 闯关记录（与 WBP 中 Text_PassingRecord 命名一致，显示历史最高关卡）
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_PassingRecord;

	// 可选坦克列表（在蓝图里配置图标和对应 Tank 蓝图类）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TankSelect")
	TArray<FTankOptionSingle> TankOptions;

	// 当前选中的坦克在 TankOptions 中的下标
	UPROPERTY()
	int32 SelectedTankIndex = 0;

	// 滚轮/摇杆切换冷却
	UPROPERTY(EditAnywhere, Category = "TankSelect|Input")
	float SwitchCooldown = 0.25f;
	UPROPERTY(EditAnywhere, Category = "TankSelect|Input")
	float AxisDeadZone = 0.3f;

	float MouseWheelCooldownTimer = 0.0f;
	float JoystickSwitchTimer = 0.0f;

	void UpdateSingleTankImage();
	void HandleTankSelectionInput(float DeltaTime);

	// 从 GameInstance 同步闯关记录到 UI
	void RefreshPassingRecord();

private:
	UFUNCTION()
	void OnCampaignClicked();

	UFUNCTION()
	void OnDefenseClicked();

	UFUNCTION()
	void OnBackClicked();

	// 进入闯关/守卫前把当前选中的坦克写入 GameInstance
	void SaveSelectedTankToGameInstance();
};
