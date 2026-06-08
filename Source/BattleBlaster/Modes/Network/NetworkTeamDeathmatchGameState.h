#pragma once

#include "CoreMinimal.h"
#include "Modes/Network/NetworkDeathmatchGameState.h"
#include "NetworkTeamDeathmatchGameState.generated.h"

UCLASS()
class BATTLEBLASTER_API ANetworkTeamDeathmatchGameState : public ANetworkDeathmatchGameState
{
	GENERATED_BODY()

public:
	ANetworkTeamDeathmatchGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_TeamScoreState, Category = "Network|Team Deathmatch")
	TArray<int32> TeamScores;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_TeamScoreState, Category = "Network|Team Deathmatch")
	int32 WinningTeamId = -1;

	UFUNCTION(BlueprintPure, Category = "Network|Team Deathmatch")
	int32 GetTeamScore(int32 TeamId) const;

	void InitializeTeamScores(int32 TeamCount, int32 InTargetScore);
	void AddTeamScore(int32 TeamId, int32 Delta);
	void SetWinningTeamId(int32 TeamId);

private:
	UFUNCTION()
	void OnRep_TeamScoreState();

	void MirrorTeamScoresToPlayerScores();
	void MarkTeamScoreStateDirty();
};
