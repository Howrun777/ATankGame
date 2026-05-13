#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"


#include "MutiPlayerMenuWidget.generated.h"

UCLASS()
class BATTLEBLASTER_API UMutiPlayerMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 【新增】下一个页面：死斗设置菜单类
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Navigation")
	TSubclassOf<UUserWidget> BattleSetupMenuClass;

	// 团队死斗开始菜单类（WBP_TeamBattleMenuWidget）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Navigation")
	TSubclassOf<UUserWidget> TeamBattleMenuClass;

	// MOBASetupWidget 菜单类
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Navigation")
	TSubclassOf<class UMOBASetupWidget> MOBASetupWidgetClass;

	// 【新增】上一个页面：主菜单类
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Navigation")
	TSubclassOf<UUserWidget> MainMenuClass;

protected:
	virtual void NativeConstruct() override;

	// 对应你的截图中的按钮名字
	UPROPERTY(meta = (BindWidget))
	UButton* ButMutiBattle; // 多人死斗

	UPROPERTY(meta = (BindWidget))
	UButton* ButTeamWork;   // 团队协作

	UPROPERTY(meta = (BindWidget))
	UButton* ButMOBA;       // MOBAMode

	UPROPERTY(meta = (BindWidget))
	UButton* ButBack;       // 返回

	UFUNCTION()
	void OnMutiBattleClicked();

	UFUNCTION()
	void OnTeamWorkClicked();

	UFUNCTION()
	void OnMOBAClicked();

	UFUNCTION()
	void OnBackClicked();
};