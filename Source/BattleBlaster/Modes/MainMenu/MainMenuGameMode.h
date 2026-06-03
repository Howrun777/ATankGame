#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MainMenuGameMode.generated.h"

UCLASS()
class BATTLEBLASTER_API AMainMenuGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AMainMenuGameMode();

protected:
	// 重写 BeginPlay
	virtual void BeginPlay() override;

	// 重写 EndPlay，在 GameMode 销毁前清理 Widget
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 定义一个变量，用来在编辑器里选择你的 WBP_MainMenuWidget 蓝图
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> MainMenuWidgetClass;

	// 单人闯关模式返回时使用的 Widget（选择Tank界面）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> SinglePlayerSelectWidgetClass;

	// 用于保存创建出来的 UI 实例指针
	UPROPERTY()
	class UUserWidget* CurrentWidget;
};

