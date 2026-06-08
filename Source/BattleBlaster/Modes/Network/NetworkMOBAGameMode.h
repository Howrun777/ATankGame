#pragma once

#include "CoreMinimal.h"
#include "Modes/Network/NetworkGameModeBase.h"
#include "NetworkMOBAGameMode.generated.h"

class ANetworkMOBAGameState;

UCLASS()
class BATTLEBLASTER_API ANetworkMOBAGameMode : public ANetworkGameModeBase
{
	GENERATED_BODY()

public:
	ANetworkMOBAGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual bool UsesTeamDamageRules() const override { return true; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Network|MOBA", meta = (ClampMin = "1", ClampMax = "8"))
	int32 TeamCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Network|MOBA")
	bool bAssumeCoreAliveUntilRegistered = true;

	UFUNCTION(BlueprintCallable, Category = "Network|MOBA")
	void RegisterCoreForTeam(int32 TeamId);

	UFUNCTION(BlueprintCallable, Category = "Network|MOBA")
	void NotifyCoreDestroyedForTeam(int32 TeamId);

protected:
	virtual int32 ChooseTeamIdForSlot(int32 SlotId) const override;
	virtual bool ShouldRespawnPlayer(ANetworkPlayerStateBase* PlayerState) const override;
	virtual void HandleNetworkTankKilled(ATank* DeadTank, ATank* KillerTank) override;
	virtual void CheckNetworkGameOver() override;
	virtual void RespawnPlayer(AController* Controller) override;

	ANetworkMOBAGameState* GetMOBAGameState() const;
	bool HasCoreAliveForTeam(int32 TeamId) const;

private:
	FTimerHandle MatchElapsedTimerHandle;

	void UpdateMatchElapsedTime();
};
