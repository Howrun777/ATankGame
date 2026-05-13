
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Blueprint/UserWidget.h"

#include "TestGameMode.generated.h"

UCLASS()
class BATTLEBLASTER_API ATestGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override; // 对应蓝图的 Event BeginPlay

protected:
	// 1. 暴露一个变量给蓝图，让你选是用哪个 UI 蓝图 (比如 WBP_MainMenu)
	UPROPERTY(EditDefaultsOnly, Category = "TestUI")
	TSubclassOf<UUserWidget> TestUIWidgetClass;

	// 2. 存储生成的 UI 指针
	UPROPERTY()
	UUserWidget* TestUIWidgetInstance;
};
