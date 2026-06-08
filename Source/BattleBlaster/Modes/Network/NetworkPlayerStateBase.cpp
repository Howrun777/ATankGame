#include "Modes/Network/NetworkPlayerStateBase.h"

#include "Net/UnrealNetwork.h"

ANetworkPlayerStateBase::ANetworkPlayerStateBase()
{
	bIsReady = false;
	bIsAIPlayer = false;
}

void ANetworkPlayerStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetworkPlayerStateBase, bIsReady);
	DOREPLIFETIME(ANetworkPlayerStateBase, bIsAIPlayer);
}

void ANetworkPlayerStateBase::SetReady(bool bReady)
{
	bIsReady = bReady;
}

void ANetworkPlayerStateBase::SetAIPlayer(bool bAIPlayer)
{
	bIsAIPlayer = bAIPlayer;
}
