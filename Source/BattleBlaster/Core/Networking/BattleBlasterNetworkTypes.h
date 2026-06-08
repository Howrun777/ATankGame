#pragma once

#include "CoreMinimal.h"
#include "BattleBlasterNetworkTypes.generated.h"

class UTexture2D;
class UWorld;

UENUM(BlueprintType)
enum class EBattleBlasterNetworkConnectionType : uint8
{
	LAN UMETA(DisplayName = "LAN"),
	DedicatedServer UMETA(DisplayName = "Dedicated Server")
};

UENUM(BlueprintType)
enum class ENetworkGameModeType : uint8
{
	Deathmatch UMETA(DisplayName = "Deathmatch"),
	TeamDeathmatch UMETA(DisplayName = "Team Deathmatch"),
	MOBA UMETA(DisplayName = "MOBA"),
	TeamMOBA UMETA(DisplayName = "Team MOBA")
};

USTRUCT(BlueprintType)
struct BATTLEBLASTER_API FNetworkMapInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Map")
	FString MapDisplayName = TEXT("Network Test Map");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Map")
	TSoftObjectPtr<UWorld> LevelAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Map")
	FName LevelName = FName(TEXT("NetworkBattleTestMap"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Map")
	UTexture2D* MapThumbnail = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Map", meta = (ClampMin = "1", ClampMax = "8"))
	int32 MinPlayers = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Map", meta = (ClampMin = "1", ClampMax = "8"))
	int32 MaxPlayers = 8;
};

USTRUCT(BlueprintType)
struct BATTLEBLASTER_API FNetworkMatchSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Match")
	EBattleBlasterNetworkConnectionType ConnectionType = EBattleBlasterNetworkConnectionType::LAN;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Match")
	ENetworkGameModeType ModeType = ENetworkGameModeType::Deathmatch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Match")
	FName MapName = FName(TEXT("NetworkBattleTestMap"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Match", meta = (ClampMin = "1", ClampMax = "65535"))
	int32 Port = 7777;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Match", meta = (ClampMin = "1", ClampMax = "8"))
	int32 MaxPlayers = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Match", meta = (ClampMin = "0", ClampMax = "16"))
	int32 AICount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Match", meta = (ClampMin = "1", ClampMax = "99"))
	int32 TargetScore = 7;

	// Zero means the subsystem chooses the mode's default team count.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Match", meta = (ClampMin = "0", ClampMax = "8"))
	int32 TeamCount = 0;
};
