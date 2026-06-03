#include "Core/Networking/BattleBlasterSessionSubsystem.h"

#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void UBattleBlasterSessionSubsystem::HostListenServer(FName MapName)
{
	HostListenServerWithOptions(MapName, TEXT("listen"));
}

void UBattleBlasterSessionSubsystem::HostListenServerWithOptions(FName MapName, const FString& Options)
{
	UWorld* World = GetWorld();
	if (!World || MapName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("HostListenServer failed: invalid World or MapName."));
		return;
	}

	RemoveExtraLocalPlayers();

	const FString TravelOptions = Options.IsEmpty() ? TEXT("listen") : Options;
	UGameplayStatics::OpenLevel(World, MapName, true, TravelOptions);
}

void UBattleBlasterSessionSubsystem::JoinByIp(const FString& Address)
{
	UWorld* World = GetWorld();
	if (!World || Address.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinByIp failed: invalid World or Address."));
		return;
	}

	RemoveExtraLocalPlayers();

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinByIp failed: missing local PlayerController."));
		return;
	}

	PC->ClientTravel(Address, TRAVEL_Absolute);
}

void UBattleBlasterSessionSubsystem::JoinByIpAndPort(const FString& IP, const FString& Port)
{
	const FString TrimmedIP = IP.TrimStartAndEnd();
	const FString TrimmedPort = Port.TrimStartAndEnd();
	if (TrimmedIP.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinByIpAndPort failed: empty IP."));
		return;
	}

	const FString Address = TrimmedPort.IsEmpty()
		? TrimmedIP
		: FString::Printf(TEXT("%s:%s"), *TrimmedIP, *TrimmedPort);

	JoinByIp(Address);
}

void UBattleBlasterSessionSubsystem::RemoveExtraLocalPlayers()
{
	UWorld* World = GetWorld();
	if (!World) return;

	for (int32 Index = UGameplayStatics::GetNumLocalPlayerControllers(World) - 1; Index > 0; --Index)
	{
		if (APlayerController* ExtraPC = UGameplayStatics::GetPlayerController(World, Index))
		{
			UGameplayStatics::RemovePlayer(ExtraPC, true);
		}
	}

	if (UGameViewportClient* Viewport = World->GetGameViewport())
	{
		Viewport->SetForceDisableSplitscreen(true);
	}
}
