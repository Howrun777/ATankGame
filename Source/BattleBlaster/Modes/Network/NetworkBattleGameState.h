#pragma once

#include "CoreMinimal.h"
#include "Shared/State/TankGameState.h"
#include "NetworkBattleGameState.generated.h"

UCLASS()
class BATTLEBLASTER_API ANetworkBattleGameState : public ATankGameState
{
	GENERATED_BODY()

public:
	ANetworkBattleGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Network|Match")
	int32 ConnectedPlayerCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Network|Match")
	int32 MaxNetworkPlayers = 4;

	void SetConnectedPlayerCount(int32 NewCount);
	void SetMaxNetworkPlayers(int32 NewMaxPlayers);
};
