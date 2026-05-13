
#include "PauseMenuWidget.h"



#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "MainMenuGameMode.h" 
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameModeBase.h" // 获取当前GameMode需要
#include "SelectMapWidget.h"            // 引入我们做的地图选择UI头文件
#include "Engine/LocalPlayer.h"
#include "TankStageGameMode.h" // 用于判断当前是否单人模式
#include "TankMOBAGameMode.h"  // 用于判断当前是否MOBA模式
#include "Widgets/SWidget.h"


bool UPauseMenuWidget::Initialize()
{
	if (!Super::Initialize()) return false;

	// 绑定点击事件
	if (Btn_Resume)   Btn_Resume->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnResumeClicked);
	if (Btn_Restart)  Btn_Restart->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnRestartClicked);
	if (Btn_MainMenu) Btn_MainMenu->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnMainMenuClicked);

	// 【新增】：绑定切换地图
	if (Btn_ChangeMap) Btn_ChangeMap->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnChangeMapClicked);

	return true;
}
static void RemoveExtraLocalPlayersIfSinglePlayer(UWorld* World)
{
	if (!IsValid(World)) return;

	AGameModeBase* GM = UGameplayStatics::GetGameMode(World);
	if (!IsValid(GM)) return;

	// 只在“单人模式”重启时清理分屏玩家，避免误伤多人死斗的 2~4P
	if (!GM->IsA(ATankStageGameMode::StaticClass()))
	{
		return;
	}

	UGameInstance* GI = World->GetGameInstance();
	if (!IsValid(GI)) return;

	// 倒序删除，只保留 LocalPlayers[0]
	const TArray<ULocalPlayer*>& LocalPlayers = GI->GetLocalPlayers();
	for (int32 i = LocalPlayers.Num() - 1; i > 0; --i)
	{
		if (IsValid(LocalPlayers[i]))
		{
			GI->RemoveLocalPlayer(LocalPlayers[i]);
		}
	}
}
void UPauseMenuWidget::Setup()
{
	this->AddToViewport(9999);

	// 在MOBA模式或单人闯关模式下禁用更换地图按钮
	AGameModeBase* CurrentGM = UGameplayStatics::GetGameMode(GetWorld());
	if (CurrentGM && (CurrentGM->IsA(ATankMOBAGameMode::StaticClass()) || CurrentGM->IsA(ATankStageGameMode::StaticClass())))
	{
		// MOBA模式或单人闯关模式：隐藏更换地图按钮
		if (Btn_ChangeMap)
		{
			Btn_ChangeMap->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else
	{
		// 其他模式：显示更换地图按钮
		if (Btn_ChangeMap)
		{
			Btn_ChangeMap->SetVisibility(ESlateVisibility::Visible);
		}
	}

	APlayerController* PC = GetOwningPlayer();
	if (PC)
	{
		// 【修复】：使用 GameAndUI 模式，确保暂停时还能接收再次按下暂停键的指令
		FInputModeGameAndUI InputData;
		TSharedRef<SWidget> FocusWidget = this->TakeWidget();
		InputData.SetWidgetToFocus(FocusWidget);
		InputData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		// 不要屏蔽游戏输入，否则可能卡死
		InputData.SetHideCursorDuringCapture(false);

		PC->SetInputMode(InputData);
		PC->SetShowMouseCursor(true);

		UGameplayStatics::SetGamePaused(GetWorld(), true);
	}
}

void UPauseMenuWidget::Teardown()
{
	// 1. 移除 UI
	this->RemoveFromParent();
	// 2. 恢复输入模式
	APlayerController* PC = GetOwningPlayer();
	if (PC)
	{
		FInputModeGameOnly InputData;
		PC->SetInputMode(InputData);
		PC->SetShowMouseCursor(false);
		// 3. 恢复游戏
		UGameplayStatics::SetGamePaused(GetWorld(), false);
	}
}

void UPauseMenuWidget::OnResumeClicked()
{
	Teardown(); // 执行关闭菜单逻辑即可
}

void UPauseMenuWidget::OnRestartClicked()
{
	// 1) 先退出暂停（否则重开后可能还停在 paused / 输入模式异常）
	Teardown();

	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	// 2) 如果当前是单人模式，重开前清理掉多余 LocalPlayer，避免“继承分屏状态”
	RemoveExtraLocalPlayersIfSinglePlayer(World);

	// 3) 获取当前关卡名
	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, /*bRemovePrefixString=*/true);

	// 4) 关键：把“当前正在运行的 GameMode”强制写进 OpenLevel 的 Options
	FString OptionsString;
	if (AGameModeBase* CurrentGM = UGameplayStatics::GetGameMode(World))
	{
		// 形如：?game=/Game/Blueprints/BP_TankStageGameMode.BP_TankStageGameMode_C
		OptionsString = FString::Printf(TEXT("?game=%s"), *CurrentGM->GetClass()->GetPathName());
	}

	// 5) bAbsolute=true：绝对应用我们传入的 Options（不会被地图默认模式覆盖）
	UGameplayStatics::OpenLevel(World, FName(*CurrentLevelName), /*bAbsolute=*/true, OptionsString);
}

void UPauseMenuWidget::OnChangeMapClicked()
{
	// 确保我们在蓝图里配置了地图UI的类
	if (MapSelectWidgetClass)
	{
		// 创建地图选择 UI
		USelectMapWidget* MapUI = CreateWidget<USelectMapWidget>(GetWorld(), MapSelectWidgetClass);
		if (MapUI)
		{
			// 1. 【完美返回机制】：告诉地图 UI，它的上一级是当前的暂停菜单
			MapUI->ParentUI = this;

			// 2. 【核心魔法：保持模式不变】：
			// 获取当前世界正在运行的 GameMode
			AGameModeBase* CurrentGameMode = UGameplayStatics::GetGameMode(GetWorld());
			if (CurrentGameMode)
			{
				// 直接把当前模式的 Class 塞给地图 UI！
				// 这样不论你是从单人还是多人进来的，切地图后还是这个模式！
				MapUI->TargetGameModeClass = CurrentGameMode->GetClass();
			}

			// 3. 显示地图UI，层级设高一点防遮挡
			MapUI->AddToViewport(10000);

			// 4. 把当前的暂停菜单隐藏起来
			this->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("请在暂停菜单蓝图中配置 MapSelectWidgetClass !"));
	}
}
void UPauseMenuWidget::OnMainMenuClicked()
{
	// 1. 先清理暂停菜单（恢复输入、取消游戏暂停）
	Teardown();

	// 2. 安全校验：获取有效游戏世界和GameInstance
	UWorld* CurrentWorld = GetWorld();
	if (!IsValid(CurrentWorld))
	{
		UE_LOG(LogTemp, Error, TEXT("返回主菜单失败：无效的游戏世界上下文"));
		return;
	}
	UGameInstance* GameInstance = CurrentWorld->GetGameInstance();
	if (!IsValid(GameInstance))
	{
		UE_LOG(LogTemp, Error, TEXT("返回主菜单失败：无效的GameInstance"));
		return;
	}

	// ==================== 核心：清理多余本地玩家，关闭分屏 ====================
	// 获取所有本地玩家
	TArray<ULocalPlayer*> AllLocalPlayers = GameInstance->GetLocalPlayers();
	// 倒序遍历，只保留索引0的主玩家，删除其他所有分屏玩家
	for (int32 i = AllLocalPlayers.Num() - 1; i > 0; i--)
	{
		ULocalPlayer* ExtraPlayer = AllLocalPlayers[i];
		if (IsValid(ExtraPlayer))
		{
			GameInstance->RemoveLocalPlayer(ExtraPlayer);
			UE_LOG(LogTemp, Log, TEXT("清理多余分屏玩家：索引%d"), i);
		}
	}

	// ==================== 强制指定游戏模式，加载主菜单 ====================
	FString LoadOptions;
	// --- 情况1：你的MainMenuGameMode是C++类，用这个
	FString GameModeClassName = AMainMenuGameMode::StaticClass()->GetName();
	LoadOptions = FString::Printf(TEXT("?GameMode=%s"), *GameModeClassName);

	// --- 情况2：你的MainMenuGameMode是蓝图类，注释上面一行，用这个
	// LoadOptions = TEXT("?GameMode=BP_MainMenuGameMode_C"); // 蓝图类名必须加_C后缀

	// 加载主菜单关卡
	UGameplayStatics::OpenLevel(
		CurrentWorld,
		FName("MainMenuMap"), // 确保和你的主菜单关卡名完全一致
		true,
		LoadOptions
	);

	UE_LOG(LogTemp, Log, TEXT("正在加载主菜单，已清理所有分屏玩家"));
}