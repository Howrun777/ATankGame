
#include "Modes/FreeForAll/UI/MutiBattleMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "GameFramework/InputDeviceSubsystem.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Core/BattleBlasterGameInstance.h"
#include "Modes/MainMenu/UI/SelectMapWidget.h"
#include "InputCoreTypes.h"

// ================== 每帧执行 ==================
void UMutiBattleMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// ================= 【处理背景渐隐】 =================
	if (bIsFading && BGImage_Overlay)
	{
		// 按时间流逝扣减透明度
		CurrentFadeAlpha -= (InDeltaTime * FadeSpeed);

		if (CurrentFadeAlpha <= 0.0f)
		{
			// 渐隐结束，彻底隐藏顶层，关闭开关
			CurrentFadeAlpha = 0.0f;
			bIsFading = false;
			BGImage_Overlay->SetVisibility(ESlateVisibility::Hidden);
		}

		// 实时应用透明度
		BGImage_Overlay->SetOpacity(CurrentFadeAlpha);
	}

	// 1. 手柄图标：仅显示"当前连接手柄数量"，和人数设置分开
	int32 ConnectedCount = GetConnectedDeviceCount();
	UpdateDeviceIcons(ConnectedCount);

	// 2. Tank 槽数量 = 当前 UI 设置的人数（CurrentPlayerCount）
	int32 ConfiguredCount = FMath::Clamp(CurrentPlayerCount, 1, 4);

	// 如果设置人数变化了，就刷新槽位
	if (ConfiguredCount != ActivePlayerCount)
	{
		ActivePlayerCount = ConfiguredCount;

		// 只在"菜单 GameMode"那边统一调用 CreatePlayer/RemovePlayer，
		// 这里就不再负责创建 LocalPlayer，避免耦合太多。
		// 如果你仍想在这里保险，可以保留一行 EnsureLocalPlayers(ActivePlayerCount);

		InitPlayerTankState(ActivePlayerCount);
		UpdateAllTankImages();
		LastSwitchTimestamp.SetNumZeroed(ActivePlayerCount);
	}
	// ================= 新增：控制悬停发光框的显示 =================
// 只要鼠标悬停在坦克图片上，就把对应的发光框显示出来 (幽灵状态)，否则隐藏 (Hidden)
	if (HoverFrame_1 && TankImage_1) HoverFrame_1->SetVisibility(TankImage_1->IsHovered() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	if (HoverFrame_2 && TankImage_2) HoverFrame_2->SetVisibility(TankImage_2->IsHovered() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	if (HoverFrame_3 && TankImage_3) HoverFrame_3->SetVisibility(TankImage_3->IsHovered() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	if (HoverFrame_4 && TankImage_4) HoverFrame_4->SetVisibility(TankImage_4->IsHovered() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	// 【新增】：单独处理玩家1的鼠标滚轮悬停选择
	HandleMouseWheelTargeting(InDeltaTime);
	// 4. 处理玩家 Tank 选择输入
	HandleTankSelectionInput(InDeltaTime);
}
void UMutiBattleMenuWidget::UpdateBackgroundImage()
{
	if (!BGImage || !BGImage_Overlay || BackgroundImages.Num() == 0) return;

	int32 TargetIndex = CurrentPlayerCount - 2;
	if (!BackgroundImages.IsValidIndex(TargetIndex) || !BackgroundImages[TargetIndex]) return;

	UTexture2D* NewTexture = BackgroundImages[TargetIndex];

	// 如果新图片和底层现在的图片是同一张，直接退出
	if (BGImage->GetBrush().GetResourceObject() == NewTexture) return;

	// ================= 【双缓冲交接】 =================
	// 1. 把旧图交给覆膜
	BGImage_Overlay->SetBrush(BGImage->GetBrush());
	BGImage_Overlay->SetOpacity(1.0f);
	BGImage_Overlay->SetVisibility(ESlateVisibility::HitTestInvisible);

	// 2. 底图换新图
	BGImage->SetBrushFromTexture(NewTexture);

	// 3. 启动渐隐
	CurrentFadeAlpha = 1.0f;
	bIsFading = true;
}

// ================== 检测手柄数量（委托 GameInstance 精确过滤）  ==================
int32 UMutiBattleMenuWidget::GetConnectedDeviceCount()
{
	UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (GI)
	{
		// 强制刷新：每次菜单 Tick 都重新检测手柄数量
		int32 Count = GI->GetConnectedGamepadCount(/* bForceRefresh */ true);
		return FMath::Clamp(Count, 1, 4);
	}

	// 回退：原始实现（仅在 GameInstance 不可用时使用）
	IPlatformInputDeviceMapper& DeviceMapper = IPlatformInputDeviceMapper::Get();
	TArray<FInputDeviceId> ConnectedDevices;
	DeviceMapper.GetAllConnectedInputDevices(ConnectedDevices);
	int32 RawDeviceCount = ConnectedDevices.Num();
	int32 EstimatedGamepadCount = FMath::Max(0, RawDeviceCount - 1);
	return FMath::Clamp(FMath::Max(1, EstimatedGamepadCount), 1, 4);
}

void UMutiBattleMenuWidget::HandleMouseWheelTargeting(float DeltaTime)
{
	// 没坦克选或者没人玩，直接退出
	if (TankOptions.Num() == 0 || ActivePlayerCount <= 0) return;

	// 冷却倒计时
	if (MouseWheelCooldownTimer > 0.0f)
	{
		MouseWheelCooldownTimer -= DeltaTime;
		return;
	}

	APlayerController* PC0 = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC0) return;

	// 读取鼠标滚轮的值
	float MouseWheel = PC0->GetInputAnalogKeyState(EKeys::MouseWheelAxis);
	if (FMath::Abs(MouseWheel) < 0.1f) return;

	// ================= 核心：判断鼠标悬停在哪个【坦克图片】上 =================
	int32 TargetPlayerIndex = -1;

	// 这里直接检测底层的 Image 是否被鼠标悬停
	if (TankImage_1 && TankImage_1->IsHovered()) TargetPlayerIndex = 0;
	else if (TankImage_2 && TankImage_2->IsHovered()) TargetPlayerIndex = 1;
	else if (TankImage_3 && TankImage_3->IsHovered()) TargetPlayerIndex = 2;
	else if (TankImage_4 && TankImage_4->IsHovered()) TargetPlayerIndex = 3;

	// 如果鼠标没有悬停在任何有效的玩家槽位上，就不做任何事
	if (TargetPlayerIndex == -1 || TargetPlayerIndex >= ActivePlayerCount) return;

	// ================= 执行选择逻辑 =================
	int32 Direction = (MouseWheel > 0.0f) ? +1 : -1;
	const int32 TankCount = TankOptions.Num();

	// 修改目标玩家的索引
	PlayerTankIndices[TargetPlayerIndex] = (PlayerTankIndices[TargetPlayerIndex] + Direction + TankCount) % TankCount;

	// 刷新该玩家的图片
	UpdateTankImageForPlayer(TargetPlayerIndex);

	// 重置滚轮冷却时间
	MouseWheelCooldownTimer = SwitchCooldown;
}

// ================== 更新图片显示 ==================
void UMutiBattleMenuWidget::UpdateDeviceIcons(int32 DeviceCount)
{
	// 规则：
	// - 只显示“当前选择人数”的槽位（2/3/4）
	// - 槽位索引 < 实际手柄数 => 显示玩家手柄图（DeviceIconTextures[0]）
	// - 槽位索引 >= 实际手柄数 => 显示AI图（DeviceIconTextures[1]）
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

void UMutiBattleMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 1. 绑定按钮点击事件到我们写的 C++ 函数上
	if (Btn_PlayerMinus) Btn_PlayerMinus->OnClicked.AddDynamic(this, &UMutiBattleMenuWidget::OnPlayerMinusClicked);
	if (Btn_PlayerPlus)  Btn_PlayerPlus->OnClicked.AddDynamic(this, &UMutiBattleMenuWidget::OnPlayerPlusClicked);

	if (Btn_ScoreMinus)  Btn_ScoreMinus->OnClicked.AddDynamic(this, &UMutiBattleMenuWidget::OnScoreMinusClicked);
	if (Btn_ScorePlus)   Btn_ScorePlus->OnClicked.AddDynamic(this, &UMutiBattleMenuWidget::OnScorePlusClicked);

	if (Btn_Confirm)     Btn_Confirm->OnClicked.AddDynamic(this, &UMutiBattleMenuWidget::OnConfirmClicked);
	if (Btn_Back)        Btn_Back->OnClicked.AddDynamic(this, &UMutiBattleMenuWidget::OnBackClicked);

	// 2. 初始化UI显示
	UpdateDisplay();
	// 初始化时就显示对应的背景
	UpdateBackgroundImage();

	// 3. 根据当前设备数量初始化玩家 Tank 状态
	const int32 DeviceCount = GetConnectedDeviceCount();      // 复用你已有方法
	ActivePlayerCount = FMath::Clamp(DeviceCount, 1, 4);      // 至少1人，最多4人

	InitPlayerTankState(ActivePlayerCount);
	UpdateAllTankImages();
}

void UMutiBattleMenuWidget::UpdateDisplay()
{
	// 将数字转为 Text 并更新到屏幕上
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
void UMutiBattleMenuWidget::OnPlayerMinusClicked()
{
	CurrentPlayerCount--;
	// 限制人数在 2 到 4 之间
	CurrentPlayerCount = FMath::Clamp(CurrentPlayerCount, 2, 4);
	UpdateDisplay();
	// 人数减少后刷新背景
	UpdateBackgroundImage();
	UpdateDeviceIcons(GetConnectedDeviceCount());
}

void UMutiBattleMenuWidget::OnPlayerPlusClicked()
{
	CurrentPlayerCount++;
	CurrentPlayerCount = FMath::Clamp(CurrentPlayerCount, 2, 4);
	UpdateDisplay();
	// 人数增加后刷新背景
	UpdateBackgroundImage();
	UpdateDeviceIcons(GetConnectedDeviceCount());
}

// ---------------- 分数加减逻辑 ----------------
void UMutiBattleMenuWidget::OnScoreMinusClicked()
{
	CurrentScore--;
	// 限制分数最低为 1
	CurrentScore = FMath::Max(1, CurrentScore);
	UpdateDisplay();
}

void UMutiBattleMenuWidget::OnScorePlusClicked()
{
	CurrentScore++;
	// 假设最高分限制为 99
	CurrentScore = FMath::Min(99, CurrentScore);
	UpdateDisplay();
}

// ---------------- 确认与返回 ----------------
void UMutiBattleMenuWidget::OnConfirmClicked()
{
	// 1. 保存数据到 GameInstance
	UBattleBlasterGameInstance* GameInst = Cast<UBattleBlasterGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (GameInst)
	{
		GameInst->TargetPlayerCount = CurrentPlayerCount;
		GameInst->TargetMatchScore = CurrentScore;

		// 保存实际连接的手柄数量和AI控制信息
		GameInst->ConnectedGamepadCount = GetConnectedDeviceCount();

		// 计算需要AI控制的玩家索引
		GameInst->AIControlledPlayerIndices.Empty();
		int32 DeviceCount = GameInst->ConnectedGamepadCount;
		int32 ConfiguredCount = FMath::Clamp(CurrentPlayerCount, 2, 4);

		for (int32 i = DeviceCount; i < ConfiguredCount; i++)
		{
			GameInst->AIControlledPlayerIndices.Add(i);
		}

		UE_LOG(LogTemp, Display, TEXT("Confirm: ConnectedGamepads=%d, TargetPlayers=%d, AICount=%d"),
			GameInst->ConnectedGamepadCount, ConfiguredCount, GameInst->AIControlledPlayerIndices.Num());

		// ====== 保存每个玩家选中的Tank蓝图 ======
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

			// 如果没配 TankClass 就用默认PawnClass，让GameMode处理
			GameInst->SelectedTankClasses[i] = SelectedClass;
		}
	}

	// 2. 打开共用的地图选择界面
	if (MapSelectWidgetClass)
	{
		// 创建地图 UI
		USelectMapWidget* MapUI = CreateWidget<USelectMapWidget>(GetWorld(), MapSelectWidgetClass);
		if (MapUI)
		{
			// 告诉地图 UI："上一级是我" (以后它点返回就能回到死斗菜单)
			MapUI->ParentUI = this;

			// 告诉地图 UI："一会进地图的时候，套用死斗模式规则！"
			MapUI->TargetGameModeClass = this->MultiplayerGameModeClass;

			MapUI->AddToViewport();
			this->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UMutiBattleMenuWidget::OnBackClicked()
{
	// 【修改】不只是移除自己，还要创建上一个菜单
	if (PreviousMenuClass)
	{
		UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), PreviousMenuClass);
		if (Widget)
		{
			Widget->AddToViewport();
			this->RemoveFromParent(); // 移除当前设置页面
		}
	}
	else
	{
		// 如果忘了设置上一级菜单，至少把自己关掉，避免卡死
		RemoveFromParent();
	}
}

void UMutiBattleMenuWidget::InitPlayerTankState(int32 InPlayerCount)
{
	// 保证有可供选择的 Tank，否则直接初始化为0
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
		// 默认都从第0个 Tank 开始
		PlayerTankIndices[i] = 0;
		PlayerSwitchTimers[i] = 0.0f;
	}
}

void UMutiBattleMenuWidget::HandleTankSelectionInput(float DeltaTime)
{
	// 没有可选 Tank 或者没有有效玩家就直接返回
	if (TankOptions.Num() == 0 || ActivePlayerCount <= 0)
	{
		return;
	}

	for (int32 PlayerIndex = 0; PlayerIndex < ActivePlayerCount; ++PlayerIndex)
	{
		HandleSinglePlayerInput(PlayerIndex, DeltaTime);
	}
}

void UMutiBattleMenuWidget::HandleSinglePlayerInput(int32 PlayerIndex, float DeltaTime)
{
	if (!TankOptions.IsValidIndex(0))
	{
		return;
	}

	// 更新这个玩家的冷却计时器
	if (PlayerSwitchTimers.IsValidIndex(PlayerIndex))
	{
		PlayerSwitchTimers[PlayerIndex] += DeltaTime;
	}

	// 没过冷却就不处理
	if (!PlayerSwitchTimers.IsValidIndex(PlayerIndex) ||
		PlayerSwitchTimers[PlayerIndex] < SwitchCooldown)
	{
		return;
	}

	// 获取对应 PlayerController（本地最多4个）
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), PlayerIndex);
	if (!PC)
	{
		return;
	}

	// 读取左摇杆Y轴
	float AxisY = PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY);

	// 死区判断：绝对值太小不响应
	if (FMath::Abs(AxisY) < AxisDeadZone)
	{
		return;
	}

	// 注册 DeviceId → PlayerIndex 映射（设备来源追踪）
	ULocalPlayer* LP = PC->GetLocalPlayer();
	if (LP)
	{
		FPlatformUserId UserId = LP->GetPlatformUserId();
		FInputDeviceId ActiveDevice = IPlatformInputDeviceMapper::Get().GetPrimaryInputDeviceForUser(UserId);
		NotifyPlayerInputDevice(PlayerIndex, ActiveDevice);
	}

	// 判断方向：上(-1) 或 下(+1)
	int32 Direction = (AxisY > 0.0f) ? +1 : -1;

	// 修改对应玩家的 Tank 下标（循环）
	if (!PlayerTankIndices.IsValidIndex(PlayerIndex))
	{
		return;
	}

	const int32 TankCount = TankOptions.Num();
	int32& CurrentIndex = PlayerTankIndices[PlayerIndex];

	CurrentIndex = (CurrentIndex + Direction + TankCount) % TankCount;

	// 重置冷却计时器
	PlayerSwitchTimers[PlayerIndex] = 0.0f;

	// 刷新该玩家对应的 Tank 图片
	UpdateTankImageForPlayer(PlayerIndex);
}

void UMutiBattleMenuWidget::UpdateTankImageForPlayer(int32 PlayerIndex)
{
	if (!TankOptions.IsValidIndex(0) ||
		!PlayerTankIndices.IsValidIndex(PlayerIndex) ||
		!TankOptions.IsValidIndex(PlayerTankIndices[PlayerIndex]))
	{
		return;
	}

	// 选中的 Tank 图标
	UTexture2D* Icon = TankOptions[PlayerTankIndices[PlayerIndex]].Icon;

	// 找到对应的 Image 控件
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
		// 没配置图标就隐藏
		TargetImage->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UMutiBattleMenuWidget::UpdateAllTankImages()
{
	for (int32 i = 0; i < ActivePlayerCount; ++i)
	{
		UpdateTankImageForPlayer(i);
	}

	// 超出 ActivePlayerCount 的槽位隐藏
	if (ActivePlayerCount < 4)
	{
		if (ActivePlayerCount <= 0 && TankImage_1) TankImage_1->SetVisibility(ESlateVisibility::Hidden);
		if (ActivePlayerCount <= 1 && TankImage_2) TankImage_2->SetVisibility(ESlateVisibility::Hidden);
		if (ActivePlayerCount <= 2 && TankImage_3) TankImage_3->SetVisibility(ESlateVisibility::Hidden);
		if (ActivePlayerCount <= 3 && TankImage_4) TankImage_4->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UMutiBattleMenuWidget::EnsureLocalPlayers(int32 WantedPlayers)
{
	UWorld* World = GetWorld();
	if (!World) return;

	const int32 CurrentControllers = UGameplayStatics::GetNumLocalPlayerControllers(World);

	for (int32 i = CurrentControllers; i < WantedPlayers; ++i)
	{
		UGameplayStatics::CreatePlayer(World, -1, true);
	}
}

void UMutiBattleMenuWidget::OnTankSelectAxisInput(int32 PlayerIndex, float AxisValue)
{
	if (TankOptions.Num() <= 0) return;

	// 只允许当前活跃玩家范围内
	if (PlayerIndex < 0 || PlayerIndex >= ActivePlayerCount) return;

	// 死区（IMC里也建议做DeadZone，这里再保险）
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

	// 初始化时间戳数组
	if (LastSwitchTimestamp.Num() != ActivePlayerCount)
	{
		LastSwitchTimestamp.SetNumZeroed(ActivePlayerCount);
	}

	const float Now = World->GetTimeSeconds();

	// 冷却：Triggered 会连续触发
	if ((Now - LastSwitchTimestamp[PlayerIndex]) < SwitchCooldown)
	{
		return;
	}

	// 确保索引数组大小正确
	if (PlayerTankIndices.Num() != ActivePlayerCount)
	{
		PlayerTankIndices.SetNumZeroed(ActivePlayerCount);
	}

	const int32 TankCount = TankOptions.Num();
	int32& Index = PlayerTankIndices[PlayerIndex];

	// 约定：你在 IMC 里给 Gamepad LeftY 加 Scalar=-1 后：
	// 上推/滚轮上 => AxisValue > 0
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

void UMutiBattleMenuWidget::NotifyPlayerInputDevice(int32 PlayerIndex, FInputDeviceId DeviceId)
{
	UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (GI)
	{
		GI->RegisterPlayerDeviceMapping(PlayerIndex, DeviceId);
	}
}
