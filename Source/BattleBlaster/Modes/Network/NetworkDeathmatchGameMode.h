#pragma once

#include "CoreMinimal.h"
#include "Modes/Network/NetworkGameModeBase.h"
#include "NetworkDeathmatchGameMode.generated.h"

class ANetworkDeathmatchGameState;

UCLASS()
class BATTLEBLASTER_API ANetworkDeathmatchGameMode : public ANetworkGameModeBase
{
	GENERATED_BODY()

public:
	ANetworkDeathmatchGameMode();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Network|Deathmatch", meta = (ClampMin = "1"))
	int32 TargetScore = 7;

protected:
	virtual bool ShouldRespawnPlayer(ANetworkPlayerStateBase* PlayerState) const override;
	virtual void HandleNetworkTankKilled(ATank* DeadTank, ATank* KillerTank) override;
	virtual void CheckNetworkGameOver() override;

	ANetworkDeathmatchGameState* GetDeathmatchGameState() const;

private:
	void AddDeathmatchScore(ATank* DeadTank, ATank* KillerTank);
};
