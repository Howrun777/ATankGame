#include "Modes/Network/NetworkBattlePlayerController.h"

#include "Shared/UI/BulletsWidget.h"
#include "Shared/UI/HUDWidget.h"
#include "Shared/UI/KDAWidget.h"
#include "UObject/ConstructorHelpers.h"

ANetworkBattlePlayerController::ANetworkBattlePlayerController()
{
	// These C++ defaults are fallbacks only. The production network mode should
	// use BP_NetworkBattlePlayerController to assign widgets per mode.
	static ConstructorHelpers::FClassFinder<UHUDWidget> DefaultHUDWidgetClass(TEXT("/Game/Blueprints/Controller/WBP_HUD"));
	if (DefaultHUDWidgetClass.Succeeded())
	{
		HUDWidgetClass = DefaultHUDWidgetClass.Class;
	}

	static ConstructorHelpers::FClassFinder<UBulletsWidget> DefaultAmmoWidgetClass(TEXT("/Game/Blueprints/Controller/BP_BulletsWidget"));
	if (DefaultAmmoWidgetClass.Succeeded())
	{
		AmmoWidgetClass = DefaultAmmoWidgetClass.Class;
	}

	static ConstructorHelpers::FClassFinder<UKDAWidget> DefaultKDAWidgetClass(TEXT("/Game/Blueprints/Controller/WBP_KDAWidget"));
	if (DefaultKDAWidgetClass.Succeeded())
	{
		KDAWidgetClass = DefaultKDAWidgetClass.Class;
	}

}

void ANetworkBattlePlayerController::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Display, TEXT("NetworkBattlePlayerController BeginPlay: Local=%d, NetMode=%d"),
		IsLocalController() ? 1 : 0,
		static_cast<int32>(GetNetMode()));
}
