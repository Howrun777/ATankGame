#include "Modes/Network/NetworkBattlePlayerController.h"

void ANetworkBattlePlayerController::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Display, TEXT("NetworkBattlePlayerController BeginPlay: Local=%d, NetMode=%d"),
		IsLocalController() ? 1 : 0,
		static_cast<int32>(GetNetMode()));
}
