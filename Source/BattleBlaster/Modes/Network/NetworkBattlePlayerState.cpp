#include "Modes/Network/NetworkBattlePlayerState.h"

#include "Net/UnrealNetwork.h"

ANetworkBattlePlayerState::ANetworkBattlePlayerState()
{
	bIsReady = false;
}

void ANetworkBattlePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetworkBattlePlayerState, bIsReady);
}

void ANetworkBattlePlayerState::SetReady(bool bReady)
{
	bIsReady = bReady;
}
