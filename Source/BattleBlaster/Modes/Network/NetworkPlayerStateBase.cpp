#include "Modes/Network/NetworkPlayerStateBase.h"

#include "Net/UnrealNetwork.h"

ANetworkPlayerStateBase::ANetworkPlayerStateBase()
{
	bIsReady = false;
}

void ANetworkPlayerStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetworkPlayerStateBase, bIsReady);
}

void ANetworkPlayerStateBase::SetReady(bool bReady)
{
	bIsReady = bReady;
}
