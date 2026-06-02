#pragma once

#include "CoreMinimal.h"
#include "Modes/Network/NetworkGameStateBase.h"
#include "NetworkDeathmatchGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNetworkDeathmatchScoreStateChanged);

UCLASS()
class BATTLEBLASTER_API ANetworkDeathmatchGameState : public ANetworkGameStateBase
{
	GENERATED_BODY()

public:
	ANetworkDeathmatchGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable, Category = "Network|Deathmatch")
	FOnNetworkDeathmatchScoreStateChanged OnScoreStateChanged;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ScoreState, Category = "Network|Deathmatch")
	TArray<int32> PlayerScores;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ScoreState, Category = "Network|Deathmatch")
	int32 TargetScore = 7;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ScoreState, Category = "Network|Deathmatch")
	int32 WinnerSlotId = -1;

	UFUNCTION(BlueprintPure, Category = "Network|Deathmatch")
	int32 GetPlayerScore(int32 SlotId) const;

	UFUNCTION(BlueprintPure, Category = "Network|Deathmatch")
	bool IsMatchOver() const { return WinnerSlotId >= 0; }

	void InitializeDeathmatchScores(int32 MaxPlayers, int32 InTargetScore);
	void AddPlayerScore(int32 SlotId, int32 Delta);
	void SetWinnerSlotId(int32 SlotId);

private:
	UFUNCTION()
	void OnRep_ScoreState();

	void BroadcastScoreStateChanged();
};
