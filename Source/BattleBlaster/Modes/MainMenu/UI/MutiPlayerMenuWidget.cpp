#include "Modes/MainMenu/UI/MutiPlayerMenuWidget.h"
#include "Modes/MOBA/UI/MOBASetupWidget.h"

void UMutiPlayerMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ButMutiBattle) ButMutiBattle->OnClicked.AddDynamic(this, &UMutiPlayerMenuWidget::OnMutiBattleClicked);
	if (ButTeamWork)   ButTeamWork->OnClicked.AddDynamic(this, &UMutiPlayerMenuWidget::OnTeamWorkClicked);
	if (ButMOBA)       ButMOBA->OnClicked.AddDynamic(this, &UMutiPlayerMenuWidget::OnMOBAClicked);
	if (ButBack)       ButBack->OnClicked.AddDynamic(this, &UMutiPlayerMenuWidget::OnBackClicked);
}

void UMutiPlayerMenuWidget::OnMutiBattleClicked()
{
	// 跳转到死斗设置页面 (MutiBattleMenuWidget)
	if (BattleSetupMenuClass)
	{
		UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), BattleSetupMenuClass);
		if (Widget)
		{
			Widget->AddToViewport();
			this->RemoveFromParent();
		}
	}
}

void UMutiPlayerMenuWidget::OnTeamWorkClicked()
{
	// 跳转到团队死斗设置页面 (TeamBattleMenuWidget)
	if (TeamBattleMenuClass)
	{
		UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), TeamBattleMenuClass);
		if (Widget)
		{
			Widget->AddToViewport();
			this->RemoveFromParent();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("未配置 TeamBattleMenuClass，请在多人菜单蓝图中指定 WBP_TeamBattleMenuWidget"));
	}
}

void UMutiPlayerMenuWidget::OnMOBAClicked()
{
	// 跳转到 MOBASetupWidget 页面
	if (MOBASetupWidgetClass)
	{
		UMOBASetupWidget* MOBASetupUI = CreateWidget<UMOBASetupWidget>(GetWorld(), MOBASetupWidgetClass);
		if (MOBASetupUI)
		{
			MOBASetupUI->AddToViewport();
			this->RemoveFromParent();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("未配置 MOBASetupWidgetClass，请在蓝图中指定 WBP_MOBASetupWidget"));
	}
}

void UMutiPlayerMenuWidget::OnBackClicked()
{
	// 返回主菜单
	if (MainMenuClass)
	{
		UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), MainMenuClass);
		if (Widget)
		{
			Widget->AddToViewport();
			this->RemoveFromParent();
		}
	}
}
