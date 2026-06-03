#include "Modes/TeamBattle/UI/TeamBattleMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "GameFramework/InputDeviceSubsystem.h"
#include "InputCoreTypes.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Core/BattleBlasterGameInstance.h"
#include "Modes/MainMenu/UI/SelectMapWidget.h"

void UTeamBattleMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_ScoreMinus) Btn_ScoreMinus->OnClicked.AddDynamic(this, &UTeamBattleMenuWidget::OnScoreMinusClicked);
	if (Btn_ScorePlus)  Btn_ScorePlus->OnClicked.AddDynamic(this, &UTeamBattleMenuWidget::OnScorePlusClicked);
	if (Btn_Confirm)    Btn_Confirm->OnClicked.AddDynamic(this, &UTeamBattleMenuWidget::OnConfirmClicked);
	if (Btn_Back)      Btn_Back->OnClicked.AddDynamic(this, &UTeamBattleMenuWidget::OnBackClicked);

	UpdateScoreDisplay();
	RefreshConnectedDeviceCount();
	EnsureLocalPlayers(GetDesiredLocalPlayerCount());
	InitPlayerTankState();
	UpdateAllTankImages();
}

void UTeamBattleMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	DeviceCountRefreshTimer += InDeltaTime;
	if (DeviceCountRefreshTimer >= DeviceCountRefreshInterval)
	{
		RefreshConnectedDeviceCount();
		EnsureLocalPlayers(GetDesiredLocalPlayerCount());
	}

	if (HoverFrame_1 && TankImage_1) HoverFrame_1->SetVisibility(TankImage_1->IsHovered() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	if (HoverFrame_2 && TankImage_2) HoverFrame_2->SetVisibility(TankImage_2->IsHovered() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	if (HoverFrame_3 && TankImage_3) HoverFrame_3->SetVisibility(TankImage_3->IsHovered() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	if (HoverFrame_4 && TankImage_4) HoverFrame_4->SetVisibility(TankImage_4->IsHovered() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);

	HandleMouseWheelTargeting(InDeltaTime);
	HandleTankSelectionInput(InDeltaTime);
}

int32 UTeamBattleMenuWidget::GetConnectedDeviceCount()
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

void UTeamBattleMenuWidget::RefreshConnectedDeviceCount()
{
	CachedConnectedDeviceCount = GetConnectedDeviceCount();
	DeviceCountRefreshTimer = 0.0f;
	UpdateDeviceIcons(CachedConnectedDeviceCount);
}

int32 UTeamBattleMenuWidget::GetDesiredLocalPlayerCount() const
{
	const int32 PlayerSlots = FMath::Clamp(TeamBattlePlayerCount, 1, 4);
	return PlayerSlots == 3 ? 4 : PlayerSlots;
}

void UTeamBattleMenuWidget::UpdateDeviceIcons(int32 DeviceCount)
{
	UImage* Slots[4] = { Image_Gamepad_1, Image_Gamepad_2, Image_Gamepad_3, Image_Gamepad_4 };
	for (int32 i = 0; i < 4; i++)
	{
		UImage* WidgetSlot = Slots[i];
		if (!WidgetSlot) continue;

		WidgetSlot->SetVisibility(ESlateVisibility::Visible);
		const bool bIsAI = (i >= DeviceCount);
		const int32 IconIndex = bIsAI ? 1 : 0;
		if (DeviceIconTextures.IsValidIndex(IconIndex) && DeviceIconTextures[IconIndex])
		{
			WidgetSlot->SetBrushFromTexture(DeviceIconTextures[IconIndex]);
		}
	}
}

void UTeamBattleMenuWidget::UpdateScoreDisplay()
{
	if (Text_ScoreCount)
	{
		Text_ScoreCount->SetText(FText::AsNumber(CurrentScore));
	}
}

void UTeamBattleMenuWidget::InitPlayerTankState()
{
	const int32 TankCount = TankOptions.Num();
	if (TankCount == 0)
	{
		PlayerTankIndices.Empty();
		PlayerSwitchTimers.Empty();
		return;
	}
	PlayerTankIndices.SetNum(TeamBattlePlayerCount);
	PlayerSwitchTimers.SetNum(TeamBattlePlayerCount);
	LastSwitchTimestamp.SetNumZeroed(TeamBattlePlayerCount);
	for (int32 i = 0; i < TeamBattlePlayerCount; ++i)
	{
		PlayerTankIndices[i] = 0;
		PlayerSwitchTimers[i] = 0.0f;
	}
}

void UTeamBattleMenuWidget::UpdateTankImageForPlayer(int32 SlotId)
{
	if (!TankOptions.IsValidIndex(0)
		|| !PlayerTankIndices.IsValidIndex(SlotId)
		|| !TankOptions.IsValidIndex(PlayerTankIndices[SlotId]))
	{
		return;
	}
	UTexture2D* Icon = TankOptions[PlayerTankIndices[SlotId]].Icon;
	UImage* TargetImage = nullptr;
	switch (SlotId)
	{
	case 0: TargetImage = TankImage_1; break;
	case 1: TargetImage = TankImage_2; break;
	case 2: TargetImage = TankImage_3; break;
	case 3: TargetImage = TankImage_4; break;
	default: break;
	}
	if (!TargetImage) return;
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

void UTeamBattleMenuWidget::UpdateAllTankImages()
{
	for (int32 i = 0; i < TeamBattlePlayerCount; ++i)
	{
		UpdateTankImageForPlayer(i);
	}
}

void UTeamBattleMenuWidget::HandleMouseWheelTargeting(float DeltaTime)
{
	if (TankOptions.Num() == 0) return;
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
	if (TargetPlayerIndex == -1) return;

	int32 Direction = (MouseWheel > 0.0f) ? +1 : -1;
	const int32 TankCount = TankOptions.Num();
	PlayerTankIndices[TargetPlayerIndex] = (PlayerTankIndices[TargetPlayerIndex] + Direction + TankCount) % TankCount;
	UpdateTankImageForPlayer(TargetPlayerIndex);
	MouseWheelCooldownTimer = SwitchCooldown;
}

void UTeamBattleMenuWidget::HandleTankSelectionInput(float DeltaTime)
{
	if (TankOptions.Num() == 0) return;
	for (int32 SlotId = 0; SlotId < TeamBattlePlayerCount; ++SlotId)
	{
		HandleSinglePlayerInput(SlotId, DeltaTime);
	}
}

void UTeamBattleMenuWidget::HandleSinglePlayerInput(int32 SlotId, float DeltaTime)
{
	if (!TankOptions.IsValidIndex(0)) return;
	if (PlayerSwitchTimers.IsValidIndex(SlotId))
	{
		PlayerSwitchTimers[SlotId] += DeltaTime;
	}
	if (!PlayerSwitchTimers.IsValidIndex(SlotId) || PlayerSwitchTimers[SlotId] < SwitchCooldown)
	{
		return;
	}
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), SlotId);
	if (!PC) return;
	float AxisY = PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY);
	if (FMath::Abs(AxisY) < AxisDeadZone) return;

	// 注册 DeviceId → SlotId 映射
	ULocalPlayer* LP = PC->GetLocalPlayer();
	if (LP)
	{
		FPlatformUserId UserId = LP->GetPlatformUserId();
		FInputDeviceId ActiveDevice = IPlatformInputDeviceMapper::Get().GetPrimaryInputDeviceForUser(UserId);
		NotifyPlayerInputDevice(SlotId, ActiveDevice);
	}

	int32 Direction = (AxisY > 0.0f) ? +1 : -1;
	if (!PlayerTankIndices.IsValidIndex(SlotId)) return;
	const int32 TankCount = TankOptions.Num();
	int32& CurrentIndex = PlayerTankIndices[SlotId];
	CurrentIndex = (CurrentIndex + Direction + TankCount) % TankCount;
	PlayerSwitchTimers[SlotId] = 0.0f;
	UpdateTankImageForPlayer(SlotId);
}

void UTeamBattleMenuWidget::EnsureLocalPlayers(int32 WantedPlayers)
{
	UWorld* World = GetWorld();
	if (!World) return;

	const int32 TargetPlayers = FMath::Clamp(WantedPlayers, 1, 4);

	for (int32 Index = UGameplayStatics::GetNumLocalPlayerControllers(World) - 1; Index >= TargetPlayers; --Index)
	{
		if (APlayerController* ExtraPC = UGameplayStatics::GetPlayerController(World, Index))
		{
			UGameplayStatics::RemovePlayer(ExtraPC, true);
		}
	}

	const int32 BeforeCount = UGameplayStatics::GetNumLocalPlayerControllers(World);
	for (int32 Index = BeforeCount; Index < TargetPlayers; ++Index)
	{
		UGameplayStatics::CreatePlayer(World, Index, true);
	}

	const int32 AfterCount = UGameplayStatics::GetNumLocalPlayerControllers(World);
	if (BeforeCount != AfterCount)
	{
		UE_LOG(LogTemp, Display, TEXT("TeamBattle menu LocalPlayers: target=%d current=%d"), TargetPlayers, AfterCount);
	}
}

void UTeamBattleMenuWidget::OnScoreMinusClicked()
{
	CurrentScore = FMath::Max(1, CurrentScore - 1);
	UpdateScoreDisplay();
}

void UTeamBattleMenuWidget::OnScorePlusClicked()
{
	CurrentScore = FMath::Min(99, CurrentScore + 1);
	UpdateScoreDisplay();
}

void UTeamBattleMenuWidget::OnConfirmClicked()
{
	UBattleBlasterGameInstance* GameInst = Cast<UBattleBlasterGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (GameInst)
	{
		GameInst->TargetPlayerCount = TeamBattlePlayerCount;
		GameInst->TargetMatchScore = CurrentScore;
		RefreshConnectedDeviceCount();
		GameInst->ConnectedGamepadCount = CachedConnectedDeviceCount;

		GameInst->AIControlledPlayerIndices.Empty();
		int32 DeviceCount = GameInst->ConnectedGamepadCount;
		for (int32 i = DeviceCount; i < TeamBattlePlayerCount; i++)
		{
			GameInst->AIControlledPlayerIndices.Add(i);
		}

		GameInst->SelectedTankClasses.Empty();
		GameInst->SelectedTankClasses.SetNum(TeamBattlePlayerCount);
		for (int32 i = 0; i < TeamBattlePlayerCount; ++i)
		{
			TSubclassOf<APawn> SelectedClass = nullptr;
			if (PlayerTankIndices.IsValidIndex(i) && TankOptions.IsValidIndex(PlayerTankIndices[i]))
			{
				SelectedClass = TankOptions[PlayerTankIndices[i]].TankClass;
			}
			GameInst->SelectedTankClasses[i] = SelectedClass;
		}

		UE_LOG(LogTemp, Display, TEXT("TeamBattle Confirm: ConnectedGamepads=%d, AICount=%d"),
			GameInst->ConnectedGamepadCount, GameInst->AIControlledPlayerIndices.Num());
	}

	EnsureLocalPlayers(GetDesiredLocalPlayerCount());

	if (MapSelectWidgetClass)
	{
		USelectMapWidget* MapUI = CreateWidget<USelectMapWidget>(GetWorld(), MapSelectWidgetClass);
		if (MapUI)
		{
			MapUI->ParentUI = this;
			MapUI->TargetGameModeClass = TeamBattleGameModeClass;
			MapUI->AddToViewport();
			this->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UTeamBattleMenuWidget::OnBackClicked()
{
	if (PreviousMenuClass)
	{
		UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), PreviousMenuClass);
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

void UTeamBattleMenuWidget::OnTankSelectAxisInput(int32 SlotId, float AxisValue)
{
	if (TankOptions.Num() <= 0) return;
	if (SlotId < 0 || SlotId >= TeamBattlePlayerCount) return;
	if (FMath::Abs(AxisValue) < AxisDeadZone) return;
	UWorld* World = GetWorld();
	if (!World) return;

	// 注册 DeviceId → SlotId 映射
	APlayerController* PC = UGameplayStatics::GetPlayerController(World, SlotId);
	if (PC)
	{
		ULocalPlayer* LP = PC->GetLocalPlayer();
		if (LP)
		{
			FPlatformUserId UserId = LP->GetPlatformUserId();
			FInputDeviceId ActiveDevice = IPlatformInputDeviceMapper::Get().GetPrimaryInputDeviceForUser(UserId);
			NotifyPlayerInputDevice(SlotId, ActiveDevice);
		}
	}

	if (LastSwitchTimestamp.Num() != TeamBattlePlayerCount)
	{
		LastSwitchTimestamp.SetNumZeroed(TeamBattlePlayerCount);
	}
	const float Now = World->GetTimeSeconds();
	if ((Now - LastSwitchTimestamp[SlotId]) < SwitchCooldown) return;
	if (PlayerTankIndices.Num() != TeamBattlePlayerCount)
	{
		PlayerTankIndices.SetNumZeroed(TeamBattlePlayerCount);
	}
	const int32 TankCount = TankOptions.Num();
	int32& Index = PlayerTankIndices[SlotId];
	if (AxisValue > 0.0f)
	{
		Index = (Index - 1 + TankCount) % TankCount;
	}
	else
	{
		Index = (Index + 1) % TankCount;
	}
	LastSwitchTimestamp[SlotId] = Now;
	UpdateTankImageForPlayer(SlotId);
}

void UTeamBattleMenuWidget::NotifyPlayerInputDevice(int32 SlotId, FInputDeviceId DeviceId)
{
	UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (GI)
	{
		GI->RegisterPlayerDeviceMapping(SlotId, DeviceId);
	}
}
