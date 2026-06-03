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

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Network|Deathmatch", meta = (ClampMin = "1"))
	int32 TargetScore = 7;

protected:
	virtual bool ShouldRespawnPlayer(ANetworkPlayerStateBase* PlayerState) const override;
	virtual void HandleNetworkTankKilled(ATank* DeadTank, ATank* KillerTank) override;
	virtual void CheckNetworkGameOver() override;

	ANetworkDeathmatchGameState* GetDeathmatchGameState() const;

private:
	FTimerHandle MatchElapsedTimerHandle;

	void AddDeathmatchScore(ATank* DeadTank, ATank* KillerTank);
	void HandleDeathmatchEnded(int32 WinnerSlotId);
	void UpdateMatchElapsedTime();
};
