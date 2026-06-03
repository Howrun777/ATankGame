#include "Modes/Network/NetworkDeathmatchGameState.h"

#include "Net/UnrealNetwork.h"

ANetworkDeathmatchGameState::ANetworkDeathmatchGameState()
{
	TargetScore = 7;
	WinnerSlotId = -1;
	MatchElapsedSeconds = 0;
	ScoreStateRevision = 0;
}

void ANetworkDeathmatchGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetworkDeathmatchGameState, PlayerScores);
	DOREPLIFETIME(ANetworkDeathmatchGameState, TargetScore);
	DOREPLIFETIME(ANetworkDeathmatchGameState, WinnerSlotId);
	DOREPLIFETIME(ANetworkDeathmatchGameState, MatchElapsedSeconds);
	DOREPLIFETIME(ANetworkDeathmatchGameState, ScoreStateRevision);
}

int32 ANetworkDeathmatchGameState::GetPlayerScore(int32 SlotId) const
{
	return PlayerScores.IsValidIndex(SlotId) ? PlayerScores[SlotId] : 0;
}

void ANetworkDeathmatchGameState::InitializeDeathmatchScores(int32 MaxPlayers, int32 InTargetScore)
{
	PlayerScores.Init(0, FMath::Max(0, MaxPlayers));
	TargetScore = FMath::Max(1, InTargetScore);
	WinnerSlotId = -1;
	MatchElapsedSeconds = 0;
	ScoreStateRevision = 0;
	SetMatchOver(false);
	MarkScoreStateDirty();
}

void ANetworkDeathmatchGameState::AddPlayerScore(int32 SlotId, int32 Delta)
{
	if (!PlayerScores.IsValidIndex(SlotId))
	{
		return;
	}

	PlayerScores[SlotId] = FMath::Max(0, PlayerScores[SlotId] + Delta);
	MarkScoreStateDirty();
}

void ANetworkDeathmatchGameState::SetWinnerSlotId(int32 SlotId)
{
	WinnerSlotId = SlotId;
	SetMatchOver(SlotId >= 0);
	MarkScoreStateDirty();
}

void ANetworkDeathmatchGameState::IncrementMatchElapsedSeconds()
{
	if (IsMatchOver())
	{
		return;
	}

	++MatchElapsedSeconds;
	MarkScoreStateDirty();
}

void ANetworkDeathmatchGameState::MarkScoreStateDirty()
{
	++ScoreStateRevision;
	ForceNetUpdate();
	BroadcastScoreStateChanged();
}

void ANetworkDeathmatchGameState::OnRep_ScoreState()
{
	BroadcastScoreStateChanged();
}

void ANetworkDeathmatchGameState::BroadcastScoreStateChanged()
{
	OnScoreStateChanged.Broadcast();
}
