#include "MainMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TankStageStartWidget.h"
#include "MutiBattleMenuWidget.h" 
#include "GameSettingsMenuWidget.h"
void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 绑定回调函数
	if (BtnSinglePlayer)
		BtnSinglePlayer->OnClicked.AddDynamic(this, &UMainMenuWidget::OnSinglePlayerClicked);

	if (BtnTwoPlayers)
		BtnTwoPlayers->OnClicked.AddDynamic(this, &UMainMenuWidget::OnMultiPlayersClicked);

	if (BtnSettings)
		BtnSettings->OnClicked.AddDynamic(this, &UMainMenuWidget::OnSettingsClicked);

	if (BtnQuitGame)
		BtnQuitGame->OnClicked.AddDynamic(this, &UMainMenuWidget::OnQuitClicked);
}

void UMainMenuWidget::OnSinglePlayerClicked()
{
	// 【完美修改】：打开单人模式选择菜单 UI
	if (SinglePlayerMenuClass)
	{
		// 创建单人菜单 UI
		UTankStageStartWidget* SingleUI = CreateWidget<UTankStageStartWidget>(GetWorld(), SinglePlayerMenuClass);
		if (SingleUI)
		{
			// 1. 告诉单人菜单：“你的上一级是主菜单”
			SingleUI->ParentUI = this;

			// 2. 显示单人菜单
			SingleUI->AddToViewport();

			// 3. 把主菜单自己隐藏起来（不要销毁，等人家点返回时还要露脸呢！）
			this->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("请在主菜单蓝图里配置 SinglePlayerMenuClass !"));
	}
}

void UMainMenuWidget::OnMultiPlayersClicked()
{
	// 【修改】不再直接进游戏，而是打开多人菜单 UI
	if (MultiplayerMenuClass)
	{
		UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), MultiplayerMenuClass);
		if (Widget)
		{
			Widget->AddToViewport();
			// 移除自己（主菜单）
			this->RemoveFromParent();
		}
	}
}

void UMainMenuWidget::OnSettingsClicked()
{
	// 打开设置菜单
	if (SettingsMenuClass)
	{
		UGameSettingsMenuWidget* SettingsWidget = CreateWidget<UGameSettingsMenuWidget>(GetWorld(), SettingsMenuClass);
		if (SettingsWidget)
		{
			// 设置返回目标为主菜单
			SettingsWidget->ParentUI = this;

			// 显示设置菜单
			SettingsWidget->AddToViewport();

			// 隐藏主菜单
			this->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("请在主菜单蓝图里配置 SettingsMenuClass !"));
	}
}

void UMainMenuWidget::OnQuitClicked()
{
	// 退出游戏逻辑
	UKismetSystemLibrary::QuitGame(GetWorld(), GetOwningPlayer(), EQuitPreference::Quit, false);
}
