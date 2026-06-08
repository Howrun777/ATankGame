#include "Core/Networking/BattleBlasterSessionSubsystem.h"

#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Modes/Network/NetworkDeathmatchGameMode.h"
#include "Modes/Network/NetworkMOBAGameMode.h"
#include "Modes/Network/NetworkTeamDeathmatchGameMode.h"
#include "Modes/Network/NetworkTeamMOBAGameMode.h"

void UBattleBlasterSessionSubsystem::HostNetworkGame(const FNetworkMatchSettings& Settings)
{
	HostListenServerWithOptions(Settings.MapName, BuildTravelOptions(Settings));
}

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

void UBattleBlasterSessionSubsystem::JoinNetworkGame(const FString& Address, int32 Port)
{
	if (Port <= 0)
	{
		JoinByIp(Address);
		return;
	}

	JoinByIpAndPort(Address, FString::FromInt(FMath::Clamp(Port, 1, 65535)));
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

FString UBattleBlasterSessionSubsystem::BuildTravelOptions(const FNetworkMatchSettings& Settings) const
{
	const UClass* GameModeClass = ResolveNetworkGameModeClass(Settings.ModeType);
	const FString ModeName = GetModeOptionName(Settings.ModeType);
	const int32 SafePort = FMath::Clamp(Settings.Port, 1, 65535);
	const int32 SafeMaxPlayers = FMath::Clamp(Settings.MaxPlayers, 1, 8);
	const int32 SafeAICount = FMath::Clamp(Settings.AICount, 0, FMath::Max(0, SafeMaxPlayers - 1));
	const int32 SafeTargetScore = FMath::Clamp(Settings.TargetScore, 1, 99);
	const int32 SafeTeamCount = ResolveTeamCount(Settings);

	return FString::Printf(TEXT("listen?game=%s?Port=%d?NetworkMode=%s?MaxPlayers=%d?AICount=%d?TargetScore=%d?TeamCount=%d"),
		*GameModeClass->GetPathName(),
		SafePort,
		*ModeName,
		SafeMaxPlayers,
		SafeAICount,
		SafeTargetScore,
		SafeTeamCount);
}

UClass* UBattleBlasterSessionSubsystem::ResolveNetworkGameModeClass(ENetworkGameModeType ModeType) const
{
	switch (ModeType)
	{
	case ENetworkGameModeType::TeamDeathmatch:
		return LoadNetworkGameModeBlueprintOrFallback(
			TEXT("/Game/Blueprints/NetworkMode/BP_NetworkTeamDeathmatchGameMode.BP_NetworkTeamDeathmatchGameMode_C"),
			ANetworkTeamDeathmatchGameMode::StaticClass());
	case ENetworkGameModeType::MOBA:
		return LoadNetworkGameModeBlueprintOrFallback(
			TEXT("/Game/Blueprints/NetworkMode/BP_NetworkMOBAGameMode.BP_NetworkMOBAGameMode_C"),
			ANetworkMOBAGameMode::StaticClass());
	case ENetworkGameModeType::TeamMOBA:
		return LoadNetworkGameModeBlueprintOrFallback(
			TEXT("/Game/Blueprints/NetworkMode/BP_NetworkTeamMOBAGameMode.BP_NetworkTeamMOBAGameMode_C"),
			ANetworkTeamMOBAGameMode::StaticClass());
	case ENetworkGameModeType::Deathmatch:
	default:
		return LoadNetworkGameModeBlueprintOrFallback(
			TEXT("/Game/Blueprints/NetworkMode/BP_NetworkDeathmatchGameMode.BP_NetworkDeathmatchGameMode_C"),
			ANetworkDeathmatchGameMode::StaticClass());
	}
}

UClass* UBattleBlasterSessionSubsystem::LoadNetworkGameModeBlueprintOrFallback(
	const TCHAR* BlueprintClassPath,
	UClass* FallbackClass) const
{
	if (UClass* LoadedClass = StaticLoadClass(AGameModeBase::StaticClass(), nullptr, BlueprintClassPath, nullptr, LOAD_NoWarn))
	{
		return LoadedClass;
	}

	return FallbackClass;
}

FString UBattleBlasterSessionSubsystem::GetModeOptionName(ENetworkGameModeType ModeType) const
{
	switch (ModeType)
	{
	case ENetworkGameModeType::TeamDeathmatch:
		return TEXT("TeamDeathmatch");
	case ENetworkGameModeType::MOBA:
		return TEXT("MOBA");
	case ENetworkGameModeType::TeamMOBA:
		return TEXT("TeamMOBA");
	case ENetworkGameModeType::Deathmatch:
	default:
		return TEXT("Deathmatch");
	}
}

int32 UBattleBlasterSessionSubsystem::ResolveTeamCount(const FNetworkMatchSettings& Settings) const
{
	if (Settings.TeamCount > 0)
	{
		return FMath::Clamp(Settings.TeamCount, 1, 8);
	}

	switch (Settings.ModeType)
	{
	case ENetworkGameModeType::TeamDeathmatch:
	case ENetworkGameModeType::TeamMOBA:
		return 2;
	case ENetworkGameModeType::MOBA:
		return FMath::Clamp(Settings.MaxPlayers, 1, 8);
	case ENetworkGameModeType::Deathmatch:
	default:
		return 1;
	}
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
