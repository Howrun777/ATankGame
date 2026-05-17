#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TeamBattleMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;
class UTexture2D;
class UBorder;

// 团队模式坦克选项（与 MutiBattle 的 FTankOption 结构一致，独立命名避免 USTRUCT 冲突）
USTRUCT(BlueprintType)
struct FTeamBattleTankOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TankSelect")
	UTexture2D* Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TankSelect")
	TSubclassOf<APawn> TankClass = nullptr;
};

/**
 * 团队死斗开始菜单：与多人死斗类似，但固定 2v2 四人，无人数设置；
 * 手柄不足 4 个时由 AI 补充；确认后进入地图选择并传递 BP_TeamBattleGameMode。
 */
UCLASS()
class BATTLEBLASTER_API UTeamBattleMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 上一级菜单（多人模式选择菜单，用于返回） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Navigation")
	TSubclassOf<UUserWidget> PreviousMenuClass;

	/** 地图选择 UI 类（WBP_SelectMapWidget） */
	UPROPERTY(EditAnywhere, Category = "Navigation")
	TSubclassOf<class USelectMapWidget> MapSelectWidgetClass;

	/** 团队死斗使用的 GameMode（BP_TeamBattleGameMode） */
	UPROPERTY(EditAnywhere, Category = "UI GameMode")
	TSubclassOf<AGameModeBase> TeamBattleGameModeClass;

	UFUNCTION()
	void OnTankSelectAxisInput(int32 SlotId, float AxisValue);

	UFUNCTION(BlueprintCallable, Category = "TankSelect|DeviceMapping")
	void NotifyPlayerInputDevice(int32 SlotId, FInputDeviceId DeviceId);

protected:
	// ================= 设备图标（手柄 / AI） =================
	UPROPERTY(EditDefaultsOnly, Category = "UI Setup|DeviceIcons")
	TArray<TObjectPtr<UTexture2D>> DeviceIconTextures;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* BGImage;
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* BGImage_Overlay;

	// ================= Tank 选择 =================
	UPROPERTY(meta = (BindWidget))
	UImage* TankImage_1;
	UPROPERTY(meta = (BindWidget))
	UImage* TankImage_2;
	UPROPERTY(meta = (BindWidget))
	UImage* TankImage_3;
	UPROPERTY(meta = (BindWidget))
	UImage* TankImage_4;

	UPROPERTY(meta = (BindWidgetOptional))
	UBorder* HoverFrame_1;
	UPROPERTY(meta = (BindWidgetOptional))
	UBorder* HoverFrame_2;
	UPROPERTY(meta = (BindWidgetOptional))
	UBorder* HoverFrame_3;
	UPROPERTY(meta = (BindWidgetOptional))
	UBorder* HoverFrame_4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TankSelect")
	TArray<FTeamBattleTankOption> TankOptions;

	UPROPERTY()
	TArray<int32> PlayerTankIndices;
	UPROPERTY()
	TArray<float> PlayerSwitchTimers;
	UPROPERTY()
	TArray<float> LastSwitchTimestamp;

	UPROPERTY(EditAnywhere, Category = "TankSelect|Input")
	float AxisDeadZone = 0.3f;
	UPROPERTY(EditAnywhere, Category = "TankSelect|Input")
	float SwitchCooldown = 0.25f;
	float MouseWheelCooldownTimer = 0.0f;

	// ================= 按钮与显示 =================
	UPROPERTY(meta = (BindWidget))
	UButton* Btn_Confirm;
	UPROPERTY(meta = (BindWidget))
	UButton* Btn_Back;
	UPROPERTY(meta = (BindWidget))
	UButton* Btn_ScoreMinus;
	UPROPERTY(meta = (BindWidget))
	UButton* Btn_ScorePlus;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_ScoreCount;

	UPROPERTY(meta = (BindWidget))
	UImage* Image_Gamepad_1;
	UPROPERTY(meta = (BindWidget))
	UImage* Image_Gamepad_2;
	UPROPERTY(meta = (BindWidget))
	UImage* Image_Gamepad_3;
	UPROPERTY(meta = (BindWidget))
	UImage* Image_Gamepad_4;

	// 团队模式固定 4 人
	static constexpr int32 TeamBattlePlayerCount = 4;
	int32 CurrentScore = 7;

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	static constexpr float DeviceCountRefreshInterval = 0.5f;
	float DeviceCountRefreshTimer = 0.0f;
	int32 CachedConnectedDeviceCount = 1;

	void RefreshConnectedDeviceCount();
	int32 GetConnectedDeviceCount();
	void UpdateDeviceIcons(int32 DeviceCount);
	void UpdateScoreDisplay();
	void InitPlayerTankState();
	void UpdateTankImageForPlayer(int32 SlotId);
	void UpdateAllTankImages();
	void HandleTankSelectionInput(float DeltaTime);
	void HandleSinglePlayerInput(int32 SlotId, float DeltaTime);
	void HandleMouseWheelTargeting(float DeltaTime);
	void EnsureLocalPlayers(int32 WantedPlayers);

	UFUNCTION()
	void OnConfirmClicked();
	UFUNCTION()
	void OnBackClicked();
	UFUNCTION()
	void OnScoreMinusClicked();
	UFUNCTION()
	void OnScorePlusClicked();
};
