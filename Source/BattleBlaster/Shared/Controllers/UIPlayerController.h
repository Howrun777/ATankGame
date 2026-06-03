#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "UIPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UMutiBattleMenuWidget;
class UTeamBattleMenuWidget;
class UMOBASetupWidget;

UCLASS()
class BATTLEBLASTER_API AUIPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UPROPERTY(EditAnywhere, Category = "Input|Menu")
	UInputMappingContext* MenuMappingContext = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input|Menu")
	UInputAction* IA_TankSelectAxis = nullptr;

private:
	int32 GetLocalPlayerSlot() const;
	void HandleTankSelectAxis(const FInputActionValue& Value);

	UMutiBattleMenuWidget* GetMenuWidgetCached() const;

	mutable TWeakObjectPtr<UMutiBattleMenuWidget> CachedMenuWidget;

	// 【新增】注册 LocalPlayerIndex → DeviceId 映射到 GameInstance
	void RegisterDeviceMappingToGameInstance();

	// 【新增】模板方法：查找当前可见的指定类型 Widget
	template<typename WidgetType>
	WidgetType* FindWidget() const;
};
