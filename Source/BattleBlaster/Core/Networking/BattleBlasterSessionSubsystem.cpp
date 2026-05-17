#include "Core/Networking/BattleBlasterSessionSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void UBattleBlasterSessionSubsystem::HostListenServer(FName MapName)
{
	UWorld* World = GetWorld();
	if (!World || MapName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("HostListenServer failed: invalid World or MapName."));
		return;
	}

	UGameplayStatics::OpenLevel(World, MapName, true, TEXT("listen"));
}

void UBattleBlasterSessionSubsystem::JoinByIp(const FString& Address)
{
	UWorld* World = GetWorld();
	if (!World || Address.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinByIp failed: invalid World or Address."));
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinByIp failed: missing local PlayerController."));
		return;
	}

	PC->ClientTravel(Address, TRAVEL_Absolute);
}
