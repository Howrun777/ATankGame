#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "NetworkBattleGameMode.generated.h"

class APlayerStart;
class ATank;
class ANetworkBattleGameState;
class ANetworkBattlePlayerState;

UCLASS()
class BATTLEBLASTER_API ANetworkBattleGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ANetworkBattleGameMode();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Network|Spawn")
	TSubclassOf<ATank> TankClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Network|Players", meta = (ClampMin = "1", ClampMax = "8"))
	int32 MaxNetworkPlayers = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Network|Spawn")
	bool bUseTaggedPlayerStarts = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Network|Respawn", meta = (ClampMin = "0.0"))
	float RespawnDelay = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Network|Respawn", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RespawnHealthPercent = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Network|Respawn", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RespawnAmmoPercent = 1.0f;

protected:
	UPROPERTY()
	TArray<ATank*> ActiveTanks;

	UFUNCTION()
	void HandleTankKilled(ATank* DeadTank, ATank* KillerTank);

	int32 AllocateSlotId() const;
	void InitializePlayerIdentity(APlayerController* PlayerController, int32 SlotId) const;
	AActor* FindSpawnPointForSlot(int32 SlotId) const;
	void SpawnTankForPlayer(APlayerController* PlayerController);
	void InitializeSpawnedTank(ATank* NewTank, APlayerController* PlayerController, ANetworkBattlePlayerState* NetworkPS, bool bResetResources);
	void ScheduleRespawn(APlayerController* PlayerController);
	void RespawnPlayer(APlayerController* PlayerController);
	void RefreshConnectedPlayerCount() const;

	ANetworkBattleGameState* GetNetworkGameState() const;
};
