#include "TankStageStartWidget.h"
#include "BattleBlasterGameInstance.h"
#include "MainMenuGameMode.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

void UTankStageStartWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Campaign) Btn_Campaign->OnClicked.AddDynamic(this, &UTankStageStartWidget::OnCampaignClicked);
	if (Btn_Defense)  Btn_Defense->OnClicked.AddDynamic(this, &UTankStageStartWidget::OnDefenseClicked);
	if (Btn_Back)     Btn_Back->OnClicked.AddDynamic(this, &UTankStageStartWidget::OnBackClicked);

	// 同步闯关记录（历史最高关卡）
	RefreshPassingRecord();

	// 初始化单人坦克选择：默认选第一个，并刷新图片
	if (TankOptions.Num() > 0)
	{
		SelectedTankIndex = 0;
		UpdateSingleTankImage();
	}
}

void UTankStageStartWidget::RefreshPassingRecord()
{
	if (!Text_PassingRecord) return;

	UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (GI)
	{
		// 先加载存档，确保历史记录是最新的
		GI->LoadGameData();
		Text_PassingRecord->SetText(FText::AsNumber(GI->GetBestLevelRecord()));
	}
	else
	{
		Text_PassingRecord->SetText(FText::AsNumber(0));
	}
}

void UTankStageStartWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 鼠标悬浮时显示选中效果（HoverFrame_1）
	if (HoverFrame_1 && TankImage_1)
	{
		HoverFrame_1->SetVisibility(
			TankImage_1->IsHovered() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}

	// 滚轮 + 手柄左摇杆上下切换坦克
	HandleTankSelectionInput(InDeltaTime);
}

void UTankStageStartWidget::UpdateSingleTankImage()
{
	if (!TankImage_1 || TankOptions.Num() == 0) return;
	if (!TankOptions.IsValidIndex(SelectedTankIndex)) return;

	UTexture2D* Icon = TankOptions[SelectedTankIndex].Icon;
	if (Icon)
	{
		TankImage_1->SetBrushFromTexture(Icon);
		TankImage_1->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		TankImage_1->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UTankStageStartWidget::HandleTankSelectionInput(float DeltaTime)
{
	if (TankOptions.Num() == 0) return;

	const int32 TankCount = TankOptions.Num();
	APlayerController* PC0 = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC0) return;

	// 1) 鼠标滚轮：仅在鼠标悬停在坦克图片上时切换
	if (MouseWheelCooldownTimer > 0.0f)
		MouseWheelCooldownTimer -= DeltaTime;
		else if (TankImage_1 && TankImage_1->IsHovered())
	{
		float MouseWheel = PC0->GetInputAnalogKeyState(EKeys::MouseWheelAxis);
		if (FMath::Abs(MouseWheel) >= 0.1f)
		{
			int32 Direction = (MouseWheel > 0.0f) ? 1 : -1; // 滚轮向上 => 下一个（与多人死斗一致）
			SelectedTankIndex = (SelectedTankIndex + Direction + TankCount) % TankCount;
			UpdateSingleTankImage();
			MouseWheelCooldownTimer = SwitchCooldown;
			return;
		}
	}

	// 2) 手柄左摇杆上下
	if (JoystickSwitchTimer < SwitchCooldown)
	{
		JoystickSwitchTimer += DeltaTime;
		return;
	}
	float AxisY = PC0->GetInputAnalogKeyState(EKeys::Gamepad_LeftY);
	if (FMath::Abs(AxisY) < AxisDeadZone) return;

	int32 Direction = (AxisY > 0.0f) ? 1 : -1; // 摇杆上 => 负值，选上一个
	SelectedTankIndex = (SelectedTankIndex + Direction + TankCount) % TankCount;
	UpdateSingleTankImage();
	JoystickSwitchTimer = 0.0f;
}

void UTankStageStartWidget::SaveSelectedTankToGameInstance()
{
	UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (!GI) return;

	GI->SelectedTankClasses.Empty();
	GI->SelectedTankClasses.SetNum(1);
	if (TankOptions.IsValidIndex(SelectedTankIndex))
		GI->SelectedTankClasses[0] = TankOptions[SelectedTankIndex].TankClass;
	else
		GI->SelectedTankClasses[0] = nullptr;
}

void UTankStageStartWidget::OnCampaignClicked()
{
	if (CampaignGameModeClass)
	{
		SaveSelectedTankToGameInstance();

		UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
		if (GI)
		{
			// 加载存档获取历史最高记录
			GI->LoadGameData();

			// 重置当前关卡为1，从头开始闯关
			GI->ResetCurrentLevel();

			// 设置返回菜单类型为单人闯关选择界面
			GI->SetReturnToMenuType(EReturnToMenuType::SinglePlayerMenu);
		}

		// 获取随机关卡名称（从 GameInstance 的 CampaignLevelNames 数组中随机选择）
		FName LevelName = FName("Level_1"); // default fallback
		if (GI && GI->CampaignLevelNames.Num() > 0)
		{
			int32 RandomIdx = FMath::RandRange(0, GI->CampaignLevelNames.Num() - 1);
			LevelName = GI->CampaignLevelNames[RandomIdx];
			UE_LOG(LogTemp, Display, TEXT("Campaign: Loading random level %s (Level %d)"), *LevelName.ToString(), GI->CurrentLevelIndex);
		}
		else if (GI)
		{
			// 如果没有配置 CampaignLevelNames，则使用 Level_X 格式
			FString LevelNameString = FString::Printf(TEXT("Level_%d"), GI->CurrentLevelIndex);
			LevelName = FName(*LevelNameString);
			UE_LOG(LogTemp, Display, TEXT("Campaign: Loading level %s"), *LevelName.ToString());
		}

		FString LoadOptions = FString::Printf(TEXT("?game=%s"), *CampaignGameModeClass->GetPathName());
		UGameplayStatics::OpenLevel(GetWorld(), LevelName, true, LoadOptions);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("请在单人菜单UI蓝图中配置 CampaignGameModeClass !"));
	}
}

void UTankStageStartWidget::OnDefenseClicked()
{
	// 守卫模式：同样保存当前选中的坦克
	SaveSelectedTankToGameInstance();
	UE_LOG(LogTemp, Warning, TEXT("守卫模式暂未开放！"));
}

void UTankStageStartWidget::OnBackClicked()
{
	// 如果有 ParentUI（从主菜单打开的情况），显示父界面
	if (ParentUI)
	{
		ParentUI->SetVisibility(ESlateVisibility::Visible);
		RemoveFromParent();
	}
	else
	{
		// 没有 ParentUI，说明是从游戏返回到单人选择界面的
		// 此时点击返回应该回到主菜单 (MainMenuWidget)
		UWorld* World = GetWorld();
		if (!World) return;

		// 先移除当前界面
		RemoveFromParent();

		// 获取 PlayerController
		APlayerController* PC = World->GetFirstPlayerController();
		if (!PC) return;

		// 设置返回类型为主菜单（重要！）
		UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(World->GetGameInstance());
		if (GI)
		{
			GI->SetReturnToMenuType(EReturnToMenuType::MainMenu);
		}

		// 重新加载主菜单关卡，会创建 MainMenuWidget
		FString LoadOptions = FString::Printf(TEXT("?GameMode=%s"), *AMainMenuGameMode::StaticClass()->GetName());
		UGameplayStatics::OpenLevel(World, FName(TEXT("MainMenuLevel_1")), true, LoadOptions);
	}
}
