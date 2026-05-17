#include "Modes/Network/NetworkBattleGameState.h"

#include "Net/UnrealNetwork.h"

ANetworkBattleGameState::ANetworkBattleGameState()
{
	ConnectedPlayerCount = 0;
	MaxNetworkPlayers = 4;
}

void ANetworkBattleGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetworkBattleGameState, ConnectedPlayerCount);
	DOREPLIFETIME(ANetworkBattleGameState, MaxNetworkPlayers);
}

void ANetworkBattleGameState::SetConnectedPlayerCount(int32 NewCount)
{
	ConnectedPlayerCount = NewCount;
}

void ANetworkBattleGameState::SetMaxNetworkPlayers(int32 NewMaxPlayers)
{
	MaxNetworkPlayers = NewMaxPlayers;
}
