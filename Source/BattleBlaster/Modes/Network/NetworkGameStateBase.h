#pragma once

#include "CoreMinimal.h"
#include "Shared/State/TankGameState.h"
#include "NetworkGameStateBase.generated.h"

UCLASS()
class BATTLEBLASTER_API ANetworkGameStateBase : public ATankGameState
{
	GENERATED_BODY()

public:
	ANetworkGameStateBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Network|Match")
	int32 ConnectedPlayerCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Network|Match")
	int32 MaxNetworkPlayers = 4;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Network|Match")
	bool bIsMatchOver = false;

	UFUNCTION(BlueprintPure, Category = "Network|Match")
	bool IsMatchOver() const { return bIsMatchOver; }

	void SetConnectedPlayerCount(int32 NewCount);
	void SetMaxNetworkPlayers(int32 NewMaxPlayers);
	void SetMatchOver(bool bOver);
};
