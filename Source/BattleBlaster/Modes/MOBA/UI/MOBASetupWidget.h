// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/GameModeBase.h"
#include "Modes/MainMenu/UI/SelectMapWidget.h"
#include "Modes/FreeForAll/UI/MutiBattleMenuWidget.h"
#include "Modes/MainMenu/UI/MutiPlayerMenuWidget.h"
#include "MOBASetupWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;
class UTexture2D;

UCLASS()
class BATTLEBLASTER_API UMOBASetupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 上一级菜单类
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Navigation")
	TSubclassOf<UUserWidget> PreviousMenuClass;
	UPROPERTY(EditAnywhere, Category = "UI GameMode")
	TSubclassOf<AGameModeBase> MultiplayerGameModeClass;

	// MOBAMaps 地图数组：索引0=2人地图, 索引1=3人地图, 索引2=4人地图
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	TArray<FMapInfo> MOBAMaps;

	UFUNCTION()
	void OnTankSelectAxisInput(int32 SlotId, float AxisValue);

	UFUNCTION(BlueprintCallable, Category = "TankSelect|DeviceMapping")
	void NotifyPlayerInputDevice(int32 SlotId, FInputDeviceId DeviceId);

	UPROPERTY()
	int32 UIConfiguredPlayerCount = 1;

	// ================= 背景图片管理 =================
	UPROPERTY(EditDefaultsOnly, Category = "UI Setup|Backgrounds")
	TArray<UTexture2D*> BackgroundImages;

protected:
	// ================= 设备图标（手柄 / AI） =================
	UPROPERTY(EditDefaultsOnly, Category = "UI Setup|DeviceIcons")
	TArray<TObjectPtr<UTexture2D>> DeviceIconTextures;

	// 绑定背景图片控件
	UPROPERTY(meta = (BindWidget))
	class UImage* BGImage;

	UPROPERTY(meta = (BindWidget))
	class UImage* BGImage_Overlay;

	bool bIsFading = false;
	float CurrentFadeAlpha = 0.0f;
	UPROPERTY(EditDefaultsOnly, Category = "UI Setup|Backgrounds")
	float FadeSpeed = 7.0f;

	void UpdateBackgroundImage();

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ================== Tank 选择相关控件 ==================
	UPROPERTY(meta = (BindWidget))
	UImage* TankImage_1;

	UPROPERTY(meta = (BindWidget))
	UImage* TankImage_2;

	UPROPERTY(meta = (BindWidget))
	UImage* TankImage_3;

	UPROPERTY(meta = (BindWidget))
	UImage* TankImage_4;

	// ================= 新增：绑定高亮边框 =================
	UPROPERTY(meta = (BindWidget))
	class UBorder* HoverFrame_1;
	UPROPERTY(meta = (BindWidget))
	class UBorder* HoverFrame_2;
	UPROPERTY(meta = (BindWidget))
	class UBorder* HoverFrame_3;
	UPROPERTY(meta = (BindWidget))
	class UBorder* HoverFrame_4;

	// 鼠标滚轮选择
	void HandleMouseWheelTargeting(float DeltaTime);
	float MouseWheelCooldownTimer = 0.0f;

	// ================== Tank 选项配置 ==================
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TankSelect")
	TArray<FTankOption> TankOptions;

	UPROPERTY()
	TArray<int32> PlayerTankIndices;

	UPROPERTY()
	int32 ActivePlayerCount = 1;

	// ========== 输入参数 ==========
	UPROPERTY(EditAnywhere, Category = "TankSelect|Input")
	float AxisDeadZone = 0.3f;
	UPROPERTY(EditAnywhere, Category = "TankSelect|Input")
	float SwitchCooldown = 0.25f;

	UPROPERTY()
	TArray<float> PlayerSwitchTimers;

	// ================== 内部辅助方法 ==================
	void InitPlayerTankState(int32 InPlayerCount);
	void UpdateTankImageForPlayer(int32 SlotId);
	void UpdateAllTankImages();
	void HandleTankSelectionInput(float DeltaTime);
	void HandleSinglePlayerInput(int32 SlotId, float DeltaTime);

	UPROPERTY()
	TArray<float> LastSwitchTimestamp;

	void EnsureLocalPlayers(int32 WantedPlayers);

	// ==================== 绑定按钮控件 ====================
	UPROPERTY(meta = (BindWidget))
	UButton* Btn_Confirm;
	UPROPERTY(meta = (BindWidget))
	UButton* Btn_Back;
	UPROPERTY(meta = (BindWidget))
	UButton* Btn_PlayerMinus;
	UPROPERTY(meta = (BindWidget))
	UButton* Btn_PlayerPlus;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_PlayerCount;
	UPROPERTY(meta = (BindWidget))
	UButton* Btn_ScoreMinus;
	UPROPERTY(meta = (BindWidget))
	UButton* Btn_ScorePlus;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_ScoreCount;

	// ================= 绑定手柄图片 =================
	UPROPERTY(meta = (BindWidget))
	UImage* Image_Gamepad_1;
	UPROPERTY(meta = (BindWidget))
	UImage* Image_Gamepad_2;
	UPROPERTY(meta = (BindWidget))
	UImage* Image_Gamepad_3;
	UPROPERTY(meta = (BindWidget))
	UImage* Image_Gamepad_4;

	// ==================== 内部数据逻辑 ====================
	int32 CurrentPlayerCount = 2;
	int32 CurrentScore = 7;

	void UpdateDisplay();

	// ==================== 按钮点击事件 ====================
	UFUNCTION()
	void OnConfirmClicked();

	UFUNCTION()
	void OnBackClicked();

	UFUNCTION()
	void OnPlayerMinusClicked();

	UFUNCTION()
	void OnPlayerPlusClicked();

	UFUNCTION()
	void OnScoreMinusClicked();

	UFUNCTION()
	void OnScorePlusClicked();

	// ================= 手柄检测逻辑 =================
	static constexpr float DeviceCountRefreshInterval = 0.5f;
	float DeviceCountRefreshTimer = 0.0f;
	int32 CachedConnectedDeviceCount = 1;

	void RefreshConnectedDeviceCount();
	int32 GetConnectedDeviceCount();
	void UpdateDeviceIcons(int32 DeviceCount);
};
