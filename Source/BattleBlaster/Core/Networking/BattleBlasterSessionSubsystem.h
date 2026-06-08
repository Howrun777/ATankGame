#pragma once

#include "CoreMinimal.h"
#include "Core/Networking/BattleBlasterNetworkTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BattleBlasterSessionSubsystem.generated.h"

UCLASS()
class BATTLEBLASTER_API UBattleBlasterSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Network|Session")
	void HostNetworkGame(const FNetworkMatchSettings& Settings);

	UFUNCTION(BlueprintCallable, Category = "Network|Session")
	void HostListenServer(FName MapName);

	UFUNCTION(BlueprintCallable, Category = "Network|Session")
	void HostListenServerWithOptions(FName MapName, const FString& Options);

	UFUNCTION(BlueprintCallable, Category = "Network|Session")
	void JoinNetworkGame(const FString& Address, int32 Port = 7777);

	UFUNCTION(BlueprintCallable, Category = "Network|Session")
	void JoinByIp(const FString& Address);

	UFUNCTION(BlueprintCallable, Category = "Network|Session")
	void JoinByIpAndPort(const FString& IP, const FString& Port);

	UFUNCTION(BlueprintPure, Category = "Network|Session")
	FString BuildTravelOptions(const FNetworkMatchSettings& Settings) const;

private:
	UClass* ResolveNetworkGameModeClass(ENetworkGameModeType ModeType) const;
	UClass* LoadNetworkGameModeBlueprintOrFallback(const TCHAR* BlueprintClassPath, UClass* FallbackClass) const;
	FString GetModeOptionName(ENetworkGameModeType ModeType) const;
	int32 ResolveTeamCount(const FNetworkMatchSettings& Settings) const;
	void RemoveExtraLocalPlayers();
};
