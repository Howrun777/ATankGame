#pragma once

#include "CoreMinimal.h"
#include "Shared/Controllers/TankPlayerController.h"
#include "NetworkPlayerControllerBase.generated.h"

class ANetworkDeathmatchGameState;
class UCppShowScoresWidget;
class UNetworkDeathmatchGameOverWidget;

UCLASS()
class BATTLEBLASTER_API ANetworkPlayerControllerBase : public ATankPlayerController
{
	GENERATED_BODY()

public:
	// Network mode keeps the shared combat UI behavior in ATankPlayerController.
	// This subclass is the network-specific extension point for default assets,
	// owner-only UI RPCs, and future lobby/scoreboard features.
	ANetworkPlayerControllerBase();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Network|UI")
	void InitializeNetworkScoreUI();

	UFUNCTION(BlueprintCallable, Category = "Network|UI")
	void RefreshNetworkScoreUI();

	UFUNCTION(BlueprintCallable, Category = "Network|UI")
	void ShowNetworkDeathmatchGameOver(int32 WinnerSlotId);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|UI")
	TSubclassOf<UCppShowScoresWidget> ScoresWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Network|UI")
	UCppShowScoresWidget* ScoresWidget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|UI")
	TSubclassOf<UNetworkDeathmatchGameOverWidget> DeathmatchGameOverWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Network|UI")
	UNetworkDeathmatchGameOverWidget* DeathmatchGameOverWidget = nullptr;

private:
	UPROPERTY()
	ANetworkDeathmatchGameState* BoundDeathmatchGameState = nullptr;

	FTimerHandle NetworkScoreUIRetryTimerHandle;
	int32 NetworkScoreUIRetryCount = 0;

	UFUNCTION()
	void HandleDeathmatchScoreStateChanged();

	void BindDeathmatchScoreState(ANetworkDeathmatchGameState* DeathmatchGameState);
	void UnbindDeathmatchScoreState();
};
