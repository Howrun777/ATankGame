// Fill out your copyright notice in the Description page of Project Settings.

#include "MOBASetupWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "GameFramework/InputDeviceSubsystem.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "BattleBlasterGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/SWidget.h"
#include "InputCoreTypes.h"
#include "Engine/LocalPlayer.h"

// ================== 每帧执行 ==================
void UMOBASetupWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// ================= 【处理背景渐隐】 =================
	if (bIsFading && BGImage_Overlay)
	{
		CurrentFadeAlpha -= (InDeltaTime * FadeSpeed);

		if (CurrentFadeAlpha <= 0.0f)
		{
			CurrentFadeAlpha = 0.0f;
			bIsFading = false;
			BGImage_Overlay->SetVisibility(ESlateVisibility::Hidden);
		}

		BGImage_Overlay->SetOpacity(CurrentFadeAlpha);
	}

	// 1. 手柄图标：仅显示"当前连接手柄数量"
	int32 ConnectedCount = GetConnectedDeviceCount();
	UpdateDeviceIcons(ConnectedCount);

	// 2. Tank 槽数量 = 当前 UI 设置的人数
	int32 ConfiguredCount = FMath::Clamp(CurrentPlayerCount, 1, 4);

	if (ConfiguredCount != ActivePlayerCount)
	{
		ActivePlayerCount = ConfiguredCount;
		InitPlayerTankState(ActivePlayerCount);
		UpdateAllTankImages();
		LastSwitchTimestamp.SetNumZeroed(ActivePlayerCount);
	}

	// ================= 控制悬停发光框的显示 =================
	if (HoverFrame_1 && TankImage_1) HoverFrame_1->SetVisibility(TankImage_1->IsHovered() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	if (HoverFrame_2 && TankImage_2) HoverFrame_2->SetVisibility(TankImage_2->IsHovered() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	if (HoverFrame_3 && TankImage_3) HoverFrame_3->SetVisibility(TankImage_3->IsHovered() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	if (HoverFrame_4 && TankImage_4) HoverFrame_4->SetVisibility(TankImage_4->IsHovered() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);

	// 玩家1的鼠标滚轮悬停选择
	HandleMouseWheelTargeting(InDeltaTime);

	// 4. 处理玩家 Tank 选择输入
	HandleTankSelectionInput(InDeltaTime);
}

void UMOBASetupWidget::UpdateBackgroundImage()
{
	if (!BGImage || !BGImage_Overlay || BackgroundImages.Num() == 0) return;

	int32 TargetIndex = CurrentPlayerCount - 2;
	if (!BackgroundImages.IsValidIndex(TargetIndex) || !BackgroundImages[TargetIndex]) return;

	UTexture2D* NewTexture = BackgroundImages[TargetIndex];

	if (BGImage->GetBrush().GetResourceObject() == NewTexture) return;

	// ================= 【双缓冲交接】 =================
	BGImage_Overlay->SetBrush(BGImage->GetBrush());
	BGImage_Overlay->SetOpacity(1.0f);
	BGImage_Overlay->SetVisibility(ESlateVisibility::HitTestInvisible);

	BGImage->SetBrushFromTexture(NewTexture);

	CurrentFadeAlpha = 1.0f;
	bIsFading = true;
}

// ================== 检测手柄数量（委托 GameInstance 精确过滤）  ==================
int32 UMOBASetupWidget::GetConnectedDeviceCount()
{
	UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (GI)
	{
		int32 Count = GI->GetConnectedGamepadCount(/* bForceRefresh */ true);
		return FMath::Clamp(Count, 1, 4);
	}

	IPlatformInputDeviceMapper& DeviceMapper = IPlatformInputDeviceMapper::Get();
	TArray<FInputDeviceId> ConnectedDevices;
	DeviceMapper.GetAllConnectedInputDevices(ConnectedDevices);
	int32 RawDeviceCount = ConnectedDevices.Num();
	int32 EstimatedGamepadCount = FMath::Max(0, RawDeviceCount - 1);
	return FMath::Clamp(FMath::Max(1, EstimatedGamepadCount), 1, 4);
}

void UMOBASetupWidget::HandleMouseWheelTargeting(float DeltaTime)
{
	if (TankOptions.Num() == 0 || ActivePlayerCount <= 0) return;

	if (MouseWheelCooldownTimer > 0.0f)
	{
		MouseWheelCooldownTimer -= DeltaTime;
		return;
	}

	APlayerController* PC0 = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC0) return;

	float MouseWheel = PC0->GetInputAnalogKeyState(EKeys::MouseWheelAxis);
	if (FMath::Abs(MouseWheel) < 0.1f) return;

	int32 TargetPlayerIndex = -1;

	if (TankImage_1 && TankImage_1->IsHovered()) TargetPlayerIndex = 0;
	else if (TankImage_2 && TankImage_2->IsHovered()) TargetPlayerIndex = 1;
	else if (TankImage_3 && TankImage_3->IsHovered()) TargetPlayerIndex = 2;
	else if (TankImage_4 && TankImage_4->IsHovered()) TargetPlayerIndex = 3;

	if (TargetPlayerIndex == -1 || TargetPlayerIndex >= ActivePlayerCount) return;

	int32 Direction = (MouseWheel > 0.0f) ? +1 : -1;
	const int32 TankCount = TankOptions.Num();

	PlayerTankIndices[TargetPlayerIndex] = (PlayerTankIndices[TargetPlayerIndex] + Direction + TankCount) % TankCount;

	UpdateTankImageForPlayer(TargetPlayerIndex);

	MouseWheelCooldownTimer = SwitchCooldown;
}

// ================== 更新设备图标显示 ==================
void UMOBASetupWidget::UpdateDeviceIcons(int32 DeviceCount)
{
	const int32 ConfiguredCount = FMath::Clamp(CurrentPlayerCount, 2, 4);

	UImage* Slots[4] = { Image_Gamepad_1, Image_Gamepad_2, Image_Gamepad_3, Image_Gamepad_4 };
	for (int32 i = 0; i < 4; i++)
	{
		UImage* WidgetSlot = Slots[i];
		if (!WidgetSlot) continue;

		if (i < ConfiguredCount)
		{
			WidgetSlot->SetVisibility(ESlateVisibility::Visible);

			const bool bIsAI = (i >= DeviceCount);
			const int32 IconIndex = bIsAI ? 1 : 0;
			if (DeviceIconTextures.IsValidIndex(IconIndex) && DeviceIconTextures[IconIndex])
			{
				WidgetSlot->SetBrushFromTexture(DeviceIconTextures[IconIndex]);
			}
		}
		else
		{
			WidgetSlot->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UMOBASetupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 1. 绑定按钮点击事件
	if (Btn_PlayerMinus) Btn_PlayerMinus->OnClicked.AddDynamic(this, &UMOBASetupWidget::OnPlayerMinusClicked);
	if (Btn_PlayerPlus)  Btn_PlayerPlus->OnClicked.AddDynamic(this, &UMOBASetupWidget::OnPlayerPlusClicked);

	if (Btn_ScoreMinus)  Btn_ScoreMinus->OnClicked.AddDynamic(this, &UMOBASetupWidget::OnScoreMinusClicked);
	if (Btn_ScorePlus)   Btn_ScorePlus->OnClicked.AddDynamic(this, &UMOBASetupWidget::OnScorePlusClicked);

	if (Btn_Confirm)     Btn_Confirm->OnClicked.AddDynamic(this, &UMOBASetupWidget::OnConfirmClicked);
	if (Btn_Back)        Btn_Back->OnClicked.AddDynamic(this, &UMOBASetupWidget::OnBackClicked);

	// 2. 初始化UI显示
	UpdateDisplay();
	UpdateBackgroundImage();

	// 3. 初始化玩家 Tank 状态
	const int32 DeviceCount = GetConnectedDeviceCount();
	ActivePlayerCount = FMath::Clamp(DeviceCount, 1, 4);

	InitPlayerTankState(ActivePlayerCount);
	UpdateAllTankImages();
}

void UMOBASetupWidget::NativeDestruct()
{
	// 移除所有按钮的事件绑定，防止访问已销毁的对象
	if (Btn_PlayerMinus) Btn_PlayerMinus->OnClicked.RemoveAll(this);
	if (Btn_PlayerPlus)  Btn_PlayerPlus->OnClicked.RemoveAll(this);
	if (Btn_ScoreMinus)  Btn_ScoreMinus->OnClicked.RemoveAll(this);
	if (Btn_ScorePlus)   Btn_ScorePlus->OnClicked.RemoveAll(this);
	if (Btn_Confirm)     Btn_Confirm->OnClicked.RemoveAll(this);
	if (Btn_Back)        Btn_Back->OnClicked.RemoveAll(this);

	Super::NativeDestruct();
}

void UMOBASetupWidget::UpdateDisplay()
{
	if (Text_PlayerCount)
	{
		Text_PlayerCount->SetText(FText::AsNumber(CurrentPlayerCount));
	}
	if (Text_ScoreCount)
	{
		Text_ScoreCount->SetText(FText::AsNumber(CurrentScore));
	}
}

// ---------------- 人数加减逻辑 ----------------
void UMOBASetupWidget::OnPlayerMinusClicked()
{
	CurrentPlayerCount--;
	CurrentPlayerCount = FMath::Clamp(CurrentPlayerCount, 2, 4);
	UpdateDisplay();
	UpdateBackgroundImage();
	UpdateDeviceIcons(GetConnectedDeviceCount());
}

void UMOBASetupWidget::OnPlayerPlusClicked()
{
	CurrentPlayerCount++;
	CurrentPlayerCount = FMath::Clamp(CurrentPlayerCount, 2, 4);
	UpdateDisplay();
	UpdateBackgroundImage();
	UpdateDeviceIcons(GetConnectedDeviceCount());
}

// ---------------- 分数加减逻辑 ----------------
void UMOBASetupWidget::OnScoreMinusClicked()
{
	CurrentScore--;
	CurrentScore = FMath::Max(1, CurrentScore);
	UpdateDisplay();
}

void UMOBASetupWidget::OnScorePlusClicked()
{
	CurrentScore++;
	CurrentScore = FMath::Min(99, CurrentScore);
	UpdateDisplay();
}

// ---------------- 确认与返回（核心改动） ----------------
void UMOBASetupWidget::OnConfirmClicked()
{
	// 1. 保存数据到 GameInstance
	UBattleBlasterGameInstance* GameInst = Cast<UBattleBlasterGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (GameInst)
	{
		GameInst->TargetPlayerCount = CurrentPlayerCount;
		GameInst->TargetMatchScore = CurrentScore;

		GameInst->ConnectedGamepadCount = GetConnectedDeviceCount();

		GameInst->AIControlledPlayerIndices.Empty();
		int32 DeviceCount = GameInst->ConnectedGamepadCount;
		int32 ConfiguredCount = FMath::Clamp(CurrentPlayerCount, 2, 4);

		for (int32 i = DeviceCount; i < ConfiguredCount; i++)
		{
			GameInst->AIControlledPlayerIndices.Add(i);
		}

		UE_LOG(LogTemp, Display, TEXT("MOBASetup Confirm: ConnectedGamepads=%d, TargetPlayers=%d, AICount=%d"),
			GameInst->ConnectedGamepadCount, ConfiguredCount, GameInst->AIControlledPlayerIndices.Num());

		// 保存每个玩家选中的Tank蓝图
		const int32 SaveCount = FMath::Clamp(ActivePlayerCount, 1, 4);
		GameInst->SelectedTankClasses.Empty();
		GameInst->SelectedTankClasses.SetNum(SaveCount);

		for (int32 i = 0; i < SaveCount; ++i)
		{
			TSubclassOf<APawn> SelectedClass = nullptr;

			if (PlayerTankIndices.IsValidIndex(i) &&
				TankOptions.IsValidIndex(PlayerTankIndices[i]))
			{
				SelectedClass = TankOptions[PlayerTankIndices[i]].TankClass;
			}

			GameInst->SelectedTankClasses[i] = SelectedClass;
		}
	}

	// 2. 根据人数自动计算地图索引并跳转
	// 2人->索引0, 3人->索引1, 4人->索引2
	int32 MapIndex = CurrentPlayerCount - 2;

	if (MOBAMaps.IsValidIndex(MapIndex))
	{
		FName LevelToLoad = MOBAMaps[MapIndex].LevelName;

		FString OptionsString = TEXT("");

		if (MultiplayerGameModeClass)
		{
			FString ClassPath = MultiplayerGameModeClass->GetPathName();
			OptionsString = FString::Printf(TEXT("?game=%s"), *ClassPath);
		}

		UE_LOG(LogTemp, Warning, TEXT("MOBASetup: 人数=%d, 跳转地图: %s, 模式: %s"),
			CurrentPlayerCount, *LevelToLoad.ToString(), *OptionsString);

		UGameplayStatics::OpenLevel(GetWorld(), LevelToLoad, true, OptionsString);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MOBASetup: 地图索引 %d 无效，请检查 MOBAMaps 配置！"), MapIndex);
	}
}

void UMOBASetupWidget::OnBackClicked()
{
	if (PreviousMenuClass)
	{
		UUserWidget* Widget = CreateWidget<UUserWidget>(GetOwningPlayer(), PreviousMenuClass);
		if (Widget)
		{
			Widget->AddToViewport();
			this->RemoveFromParent();
		}
	}
	else
	{
		RemoveFromParent();
	}
}

void UMOBASetupWidget::InitPlayerTankState(int32 InPlayerCount)
{
	const int32 TankCount = TankOptions.Num();
	if (TankCount == 0)
	{
		PlayerTankIndices.Empty();
		PlayerSwitchTimers.Empty();
		return;
	}

	PlayerTankIndices.SetNum(InPlayerCount);
	PlayerSwitchTimers.SetNum(InPlayerCount);

	for (int32 i = 0; i < InPlayerCount; ++i)
	{
		PlayerTankIndices[i] = 0;
		PlayerSwitchTimers[i] = 0.0f;
	}
}

void UMOBASetupWidget::HandleTankSelectionInput(float DeltaTime)
{
	if (TankOptions.Num() == 0 || ActivePlayerCount <= 0)
	{
		return;
	}

	for (int32 PlayerIndex = 0; PlayerIndex < ActivePlayerCount; ++PlayerIndex)
	{
		HandleSinglePlayerInput(PlayerIndex, DeltaTime);
	}
}

void UMOBASetupWidget::HandleSinglePlayerInput(int32 PlayerIndex, float DeltaTime)
{
	if (!TankOptions.IsValidIndex(0))
	{
		return;
	}

	if (PlayerSwitchTimers.IsValidIndex(PlayerIndex))
	{
		PlayerSwitchTimers[PlayerIndex] += DeltaTime;
	}

	if (!PlayerSwitchTimers.IsValidIndex(PlayerIndex) ||
		PlayerSwitchTimers[PlayerIndex] < SwitchCooldown)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), PlayerIndex);
	if (!PC)
	{
		return;
	}

	float AxisY = PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY);

	if (FMath::Abs(AxisY) < AxisDeadZone)
	{
		return;
	}

	// 注册 DeviceId → PlayerIndex 映射
	ULocalPlayer* LP = PC->GetLocalPlayer();
	if (LP)
	{
		FPlatformUserId UserId = LP->GetPlatformUserId();
		FInputDeviceId ActiveDevice = IPlatformInputDeviceMapper::Get().GetPrimaryInputDeviceForUser(UserId);
		NotifyPlayerInputDevice(PlayerIndex, ActiveDevice);
	}

	int32 Direction = (AxisY > 0.0f) ? +1 : -1;

	if (!PlayerTankIndices.IsValidIndex(PlayerIndex))
	{
		return;
	}

	const int32 TankCount = TankOptions.Num();
	int32& CurrentIndex = PlayerTankIndices[PlayerIndex];

	CurrentIndex = (CurrentIndex + Direction + TankCount) % TankCount;

	PlayerSwitchTimers[PlayerIndex] = 0.0f;

	UpdateTankImageForPlayer(PlayerIndex);
}

void UMOBASetupWidget::UpdateTankImageForPlayer(int32 PlayerIndex)
{
	if (!TankOptions.IsValidIndex(0) ||
		!PlayerTankIndices.IsValidIndex(PlayerIndex) ||
		!TankOptions.IsValidIndex(PlayerTankIndices[PlayerIndex]))
	{
		return;
	}

	UTexture2D* Icon = TankOptions[PlayerTankIndices[PlayerIndex]].Icon;

	UImage* TargetImage = nullptr;
	switch (PlayerIndex)
	{
	case 0: TargetImage = TankImage_1; break;
	case 1: TargetImage = TankImage_2; break;
	case 2: TargetImage = TankImage_3; break;
	case 3: TargetImage = TankImage_4; break;
	default: break;
	}

	if (!TargetImage)
	{
		return;
	}

	if (Icon)
	{
		TargetImage->SetBrushFromTexture(Icon);
		TargetImage->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		TargetImage->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UMOBASetupWidget::UpdateAllTankImages()
{
	for (int32 i = 0; i < ActivePlayerCount; ++i)
	{
		UpdateTankImageForPlayer(i);
	}

	if (ActivePlayerCount < 4)
	{
		if (ActivePlayerCount <= 0 && TankImage_1) TankImage_1->SetVisibility(ESlateVisibility::Hidden);
		if (ActivePlayerCount <= 1 && TankImage_2) TankImage_2->SetVisibility(ESlateVisibility::Hidden);
		if (ActivePlayerCount <= 2 && TankImage_3) TankImage_3->SetVisibility(ESlateVisibility::Hidden);
		if (ActivePlayerCount <= 3 && TankImage_4) TankImage_4->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UMOBASetupWidget::EnsureLocalPlayers(int32 WantedPlayers)
{
	UWorld* World = GetWorld();
	if (!World) return;

	const int32 CurrentControllers = UGameplayStatics::GetNumLocalPlayerControllers(World);

	for (int32 i = CurrentControllers; i < WantedPlayers; ++i)
	{
		UGameplayStatics::CreatePlayer(World, -1, true);
	}
}

void UMOBASetupWidget::OnTankSelectAxisInput(int32 PlayerIndex, float AxisValue)
{
	if (TankOptions.Num() <= 0) return;

	if (PlayerIndex < 0 || PlayerIndex >= ActivePlayerCount) return;

	if (FMath::Abs(AxisValue) < AxisDeadZone) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// 注册 DeviceId → PlayerIndex 映射
	APlayerController* PC = UGameplayStatics::GetPlayerController(World, PlayerIndex);
	if (PC)
	{
		ULocalPlayer* LP = PC->GetLocalPlayer();
		if (LP)
		{
			FPlatformUserId UserId = LP->GetPlatformUserId();
			FInputDeviceId ActiveDevice = IPlatformInputDeviceMapper::Get().GetPrimaryInputDeviceForUser(UserId);
			NotifyPlayerInputDevice(PlayerIndex, ActiveDevice);
		}
	}

	if (LastSwitchTimestamp.Num() != ActivePlayerCount)
	{
		LastSwitchTimestamp.SetNumZeroed(ActivePlayerCount);
	}

	const float Now = World->GetTimeSeconds();

	if ((Now - LastSwitchTimestamp[PlayerIndex]) < SwitchCooldown)
	{
		return;
	}

	if (PlayerTankIndices.Num() != ActivePlayerCount)
	{
		PlayerTankIndices.SetNumZeroed(ActivePlayerCount);
	}

	const int32 TankCount = TankOptions.Num();
	int32& Index = PlayerTankIndices[PlayerIndex];

	if (AxisValue > 0.0f)
	{
		Index = (Index - 1 + TankCount) % TankCount;
	}
	else
	{
		Index = (Index + 1) % TankCount;
	}

	LastSwitchTimestamp[PlayerIndex] = Now;

	UpdateTankImageForPlayer(PlayerIndex);
}

void UMOBASetupWidget::NotifyPlayerInputDevice(int32 PlayerIndex, FInputDeviceId DeviceId)
{
	UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (GI)
	{
		GI->RegisterPlayerDeviceMapping(PlayerIndex, DeviceId);
	}
}
