#include "Modes/Network/NetworkMOBAGameState.h"

#include "Net/UnrealNetwork.h"

ANetworkMOBAGameState::ANetworkMOBAGameState()
{
	WinningTeamId = -1;
	MatchElapsedSeconds = 0;
	MOBAStateRevision = 0;
}

void ANetworkMOBAGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetworkMOBAGameState, AliveCoreCountsByTeam);
	DOREPLIFETIME(ANetworkMOBAGameState, bTeamEliminated);
	DOREPLIFETIME(ANetworkMOBAGameState, WinningTeamId);
	DOREPLIFETIME(ANetworkMOBAGameState, MatchElapsedSeconds);
	DOREPLIFETIME(ANetworkMOBAGameState, MOBAStateRevision);
}

bool ANetworkMOBAGameState::HasCoreAliveForTeam(int32 TeamId) const
{
	return AliveCoreCountsByTeam.IsValidIndex(TeamId) && AliveCoreCountsByTeam[TeamId] > 0;
}

bool ANetworkMOBAGameState::IsTeamEliminated(int32 TeamId) const
{
	return bTeamEliminated.IsValidIndex(TeamId) && bTeamEliminated[TeamId];
}

void ANetworkMOBAGameState::InitializeMOBAState(int32 TeamCount)
{
	const int32 ClampedTeamCount = FMath::Max(1, TeamCount);
	AliveCoreCountsByTeam.Init(0, ClampedTeamCount);
	bTeamEliminated.Init(false, ClampedTeamCount);
	WinningTeamId = -1;
	MatchElapsedSeconds = 0;
	MOBAStateRevision = 0;
	SetMatchOver(false);
	MarkMOBAStateDirty();
}

void ANetworkMOBAGameState::RegisterCoreForTeam(int32 TeamId)
{
	if (!AliveCoreCountsByTeam.IsValidIndex(TeamId))
	{
		return;
	}

	++AliveCoreCountsByTeam[TeamId];
	MarkMOBAStateDirty();
}

void ANetworkMOBAGameState::MarkCoreDestroyedForTeam(int32 TeamId)
{
	if (!AliveCoreCountsByTeam.IsValidIndex(TeamId))
	{
		return;
	}

	AliveCoreCountsByTeam[TeamId] = FMath::Max(0, AliveCoreCountsByTeam[TeamId] - 1);
	MarkMOBAStateDirty();
}

void ANetworkMOBAGameState::SetTeamEliminated(int32 TeamId, bool bEliminated)
{
	if (!bTeamEliminated.IsValidIndex(TeamId) || bTeamEliminated[TeamId] == bEliminated)
	{
		return;
	}

	bTeamEliminated[TeamId] = bEliminated;
	MarkMOBAStateDirty();
}

void ANetworkMOBAGameState::SetWinningTeamId(int32 TeamId)
{
	WinningTeamId = TeamId;
	SetMatchOver(TeamId >= 0);
	MarkMOBAStateDirty();
}

void ANetworkMOBAGameState::IncrementMatchElapsedSeconds()
{
	if (IsMatchOver())
	{
		return;
	}

	++MatchElapsedSeconds;
	MarkMOBAStateDirty();
}

void ANetworkMOBAGameState::OnRep_MOBAState()
{
	BroadcastMOBAStateChanged();
}

void ANetworkMOBAGameState::MarkMOBAStateDirty()
{
	++MOBAStateRevision;
	ForceNetUpdate();
	BroadcastMOBAStateChanged();
}

void ANetworkMOBAGameState::BroadcastMOBAStateChanged()
{
	OnMOBAStateChanged.Broadcast();
}
