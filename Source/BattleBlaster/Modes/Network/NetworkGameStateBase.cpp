#include "Modes/Network/NetworkGameStateBase.h"

#include "Net/UnrealNetwork.h"

ANetworkGameStateBase::ANetworkGameStateBase()
{
	ConnectedPlayerCount = 0;
	MaxNetworkPlayers = 4;
}

void ANetworkGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetworkGameStateBase, ConnectedPlayerCount);
	DOREPLIFETIME(ANetworkGameStateBase, MaxNetworkPlayers);
}

void ANetworkGameStateBase::SetConnectedPlayerCount(int32 NewCount)
{
	ConnectedPlayerCount = NewCount;
}

void ANetworkGameStateBase::SetMaxNetworkPlayers(int32 NewMaxPlayers)
{
	MaxNetworkPlayers = NewMaxPlayers;
}
