#pragma once

#include "CoreMinimal.h"
#include "Modes/Network/NetworkGameModeBase.h"
#include "NetworkTeamDeathmatchGameMode.generated.h"

class ANetworkTeamDeathmatchGameState;

UCLASS()
class BATTLEBLASTER_API ANetworkTeamDeathmatchGameMode : public ANetworkGameModeBase
{
	GENERATED_BODY()

public:
	ANetworkTeamDeathmatchGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual bool UsesTeamDamageRules() const override { return true; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Network|Team Deathmatch", meta = (ClampMin = "1"))
	int32 TargetScore = 7;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Network|Team Deathmatch", meta = (ClampMin = "2", ClampMax = "4"))
	int32 TeamCount = 2;

protected:
	virtual int32 ChooseTeamIdForSlot(int32 SlotId) const override;
	virtual bool ShouldRespawnPlayer(ANetworkPlayerStateBase* PlayerState) const override;
	virtual void HandleNetworkTankKilled(ATank* DeadTank, ATank* KillerTank) override;
	virtual void CheckNetworkGameOver() override;

	ANetworkTeamDeathmatchGameState* GetTeamDeathmatchGameState() const;

private:
	FTimerHandle MatchElapsedTimerHandle;

	void AddTeamDeathmatchScore(ATank* DeadTank, ATank* KillerTank);
	void HandleTeamDeathmatchEnded(int32 WinnerTeamId);
	void UpdateMatchElapsedTime();
};
