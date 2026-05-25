#pragma once

#include "CoreMinimal.h"
#include "Shared/Controllers/TankPlayerController.h"
#include "NetworkBattlePlayerController.generated.h"

UCLASS()
class BATTLEBLASTER_API ANetworkBattlePlayerController : public ATankPlayerController
{
	GENERATED_BODY()

public:
	// Network mode keeps the shared combat UI behavior in ATankPlayerController.
	// This subclass is the network-specific extension point for default assets,
	// owner-only UI RPCs, and future lobby/scoreboard features.
	ANetworkBattlePlayerController();

	virtual void BeginPlay() override;
};
