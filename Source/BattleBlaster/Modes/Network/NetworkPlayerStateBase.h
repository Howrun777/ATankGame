#pragma once

#include "CoreMinimal.h"
#include "Shared/State/TankPlayerState.h"
#include "NetworkPlayerStateBase.generated.h"

UCLASS()
class BATTLEBLASTER_API ANetworkPlayerStateBase : public ATankPlayerState
{
	GENERATED_BODY()

public:
	ANetworkPlayerStateBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Network|Lobby")
	bool bIsReady = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Network|Lobby")
	bool bIsAIPlayer = false;

	UFUNCTION(BlueprintCallable, Category = "Network|Lobby")
	void SetReady(bool bReady);

	UFUNCTION(BlueprintPure, Category = "Network|Lobby")
	bool IsReady() const { return bIsReady; }

	UFUNCTION(BlueprintCallable, Category = "Network|Lobby")
	void SetAIPlayer(bool bAIPlayer);

	UFUNCTION(BlueprintPure, Category = "Network|Lobby")
	bool IsAIPlayer() const { return bIsAIPlayer; }
};
