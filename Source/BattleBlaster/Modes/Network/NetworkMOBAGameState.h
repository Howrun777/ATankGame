#pragma once

#include "CoreMinimal.h"
#include "Modes/Network/NetworkGameStateBase.h"
#include "NetworkMOBAGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNetworkMOBAStateChanged);

UCLASS()
class BATTLEBLASTER_API ANetworkMOBAGameState : public ANetworkGameStateBase
{
	GENERATED_BODY()

public:
	ANetworkMOBAGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable, Category = "Network|MOBA")
	FOnNetworkMOBAStateChanged OnMOBAStateChanged;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_MOBAState, Category = "Network|MOBA")
	TArray<int32> AliveCoreCountsByTeam;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_MOBAState, Category = "Network|MOBA")
	TArray<bool> bTeamEliminated;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_MOBAState, Category = "Network|MOBA")
	int32 WinningTeamId = -1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_MOBAState, Category = "Network|MOBA")
	int32 MatchElapsedSeconds = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_MOBAState, Category = "Network|MOBA")
	int32 MOBAStateRevision = 0;

	UFUNCTION(BlueprintPure, Category = "Network|MOBA")
	bool HasCoreAliveForTeam(int32 TeamId) const;

	UFUNCTION(BlueprintPure, Category = "Network|MOBA")
	bool IsTeamEliminated(int32 TeamId) const;

	void InitializeMOBAState(int32 TeamCount);
	void RegisterCoreForTeam(int32 TeamId);
	void MarkCoreDestroyedForTeam(int32 TeamId);
	void SetTeamEliminated(int32 TeamId, bool bEliminated);
	void SetWinningTeamId(int32 TeamId);
	void IncrementMatchElapsedSeconds();

private:
	UFUNCTION()
	void OnRep_MOBAState();

	void MarkMOBAStateDirty();
	void BroadcastMOBAStateChanged();
};
