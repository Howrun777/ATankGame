#include "Modes/Network/NetworkTeamDeathmatchGameState.h"

#include "Net/UnrealNetwork.h"

ANetworkTeamDeathmatchGameState::ANetworkTeamDeathmatchGameState()
{
	WinningTeamId = -1;
}

void ANetworkTeamDeathmatchGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetworkTeamDeathmatchGameState, TeamScores);
	DOREPLIFETIME(ANetworkTeamDeathmatchGameState, WinningTeamId);
}

int32 ANetworkTeamDeathmatchGameState::GetTeamScore(int32 TeamId) const
{
	return TeamScores.IsValidIndex(TeamId) ? TeamScores[TeamId] : 0;
}

void ANetworkTeamDeathmatchGameState::InitializeTeamScores(int32 TeamCount, int32 InTargetScore)
{
	const int32 ClampedTeamCount = FMath::Max(1, TeamCount);
	TeamScores.Init(0, ClampedTeamCount);
	WinningTeamId = -1;
	InitializeDeathmatchScores(ClampedTeamCount, InTargetScore);
	MirrorTeamScoresToPlayerScores();
	MarkTeamScoreStateDirty();
}

void ANetworkTeamDeathmatchGameState::AddTeamScore(int32 TeamId, int32 Delta)
{
	if (!TeamScores.IsValidIndex(TeamId))
	{
		return;
	}

	TeamScores[TeamId] = FMath::Max(0, TeamScores[TeamId] + Delta);
	MirrorTeamScoresToPlayerScores();
	MarkTeamScoreStateDirty();
}

void ANetworkTeamDeathmatchGameState::SetWinningTeamId(int32 TeamId)
{
	WinningTeamId = TeamId;
	SetWinnerSlotId(TeamId);
	MarkTeamScoreStateDirty();
}

void ANetworkTeamDeathmatchGameState::OnRep_TeamScoreState()
{
	BroadcastScoreStateChanged();
}

void ANetworkTeamDeathmatchGameState::MirrorTeamScoresToPlayerScores()
{
	if (PlayerScores.Num() != TeamScores.Num())
	{
		PlayerScores.SetNum(TeamScores.Num());
	}

	for (int32 TeamId = 0; TeamId < TeamScores.Num(); ++TeamId)
	{
		PlayerScores[TeamId] = TeamScores[TeamId];
	}
}

void ANetworkTeamDeathmatchGameState::MarkTeamScoreStateDirty()
{
	++ScoreStateRevision;
	ForceNetUpdate();
	BroadcastScoreStateChanged();
}
