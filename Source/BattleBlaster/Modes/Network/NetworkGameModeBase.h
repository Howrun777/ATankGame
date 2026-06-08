#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Shared/AI/AIBotPlayerController.h"
#include "NetworkGameModeBase.generated.h"

class APlayerStart;
class ATank;
class ANetworkGameStateBase;
class ANetworkPlayerStateBase;

UCLASS()
class BATTLEBLASTER_API ANetworkGameModeBase : public AGameMode
{
	GENERATED_BODY()

public:
	ANetworkGameModeBase();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	virtual bool UsesTeamDamageRules() const;
	virtual bool AreTeamIdsHostile(int32 AttackerTeamId, int32 VictimTeamId) const;
	virtual bool CanTankDamageTank(const ATank* AttackerTank, const ATank* VictimTank) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Network|Spawn")
	TSubclassOf<ATank> TankClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Network|Players", meta = (ClampMin = "1", ClampMax = "8"))
	int32 MaxNetworkPlayers = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Network|AI", meta = (ClampMin = "0", ClampMax = "7"))
	int32 AICount = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Network|AI")
	TSubclassOf<AAIBotPlayerController> AIControllerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|AI")
	EAIDifficulty NetworkAIDifficulty = EAIDifficulty::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Network|Spawn")
	bool bUseTaggedPlayerStarts = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Spawn", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float InitialAmmoRatio = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Network|Respawn", meta = (ClampMin = "0.0"))
	float RespawnDelay = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Network|Respawn", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RespawnHealthPercent = 1.0f;

protected:
	UPROPERTY()
	TArray<ATank*> ActiveTanks;

	UFUNCTION()
	void HandleTankKilled(ATank* DeadTank, ATank* KillerTank);

	virtual int32 ChooseTeamIdForSlot(int32 SlotId) const;
	virtual bool ShouldRespawnPlayer(ANetworkPlayerStateBase* PlayerState) const;
	virtual void HandleNetworkTankKilled(ATank* DeadTank, ATank* KillerTank);
	virtual void CheckNetworkGameOver();

	int32 AllocateSlotId() const;
	void InitializePlayerIdentity(AController* Controller, int32 SlotId, bool bAIPlayer) const;
	AActor* FindSpawnPointForSlot(int32 SlotId) const;
	void SpawnTankForController(AController* Controller);
	void InitializeSpawnedTank(ATank* NewTank, AController* Controller, ANetworkPlayerStateBase* NetworkPS, bool bResetResources);
	int32 CalculateInitialAmmoForTank(const ATank* Tank) const;
	void ScheduleRespawn(AController* Controller);
	virtual void RespawnPlayer(AController* Controller);
	void RefreshConnectedPlayerCount() const;
	void SpawnConfiguredAIPlayers();
	void SpawnAIForSlot(int32 SlotId);
	int32 GetMaxHumanPlayerSlots() const;
	bool IsSlotOccupied(int32 SlotId) const;

	ANetworkGameStateBase* GetNetworkGameState() const;
};
