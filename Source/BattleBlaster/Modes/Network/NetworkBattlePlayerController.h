#pragma once

#include "CoreMinimal.h"
#include "Shared/Controllers/TankPlayerController.h"
#include "NetworkBattlePlayerController.generated.h"

UCLASS()
class BATTLEBLASTER_API ANetworkBattlePlayerController : public ATankPlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
};
