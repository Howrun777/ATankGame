#pragma once

#include "CoreMinimal.h"
#include "Shared/Controllers/TankPlayerController.h"
#include "NetworkPlayerControllerBase.generated.h"

class ANetworkDeathmatchGameState;
class ANetworkMOBAGameState;
class ANetworkTeamDeathmatchGameState;
class UCppShowScoresWidget;
class UNetworkDeathmatchGameOverWidget;
class UNetworkJoinMessageWidget;
class UNetworkMOBAStateWidget;
class UNetworkTeamScoresWidget;

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

	UFUNCTION(BlueprintCallable, Category = "Network|UI")
	void ShowNetworkJoinMessage(const FString& Message);

	UFUNCTION(Client, Reliable)
	void ClientShowNetworkJoinMessage(const FString& Message);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|UI")
	TSubclassOf<UCppShowScoresWidget> ScoresWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Network|UI")
	UCppShowScoresWidget* ScoresWidget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|UI")
	TSubclassOf<UNetworkTeamScoresWidget> TeamScoresWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Network|UI")
	UNetworkTeamScoresWidget* TeamScoresWidget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|UI")
	TSubclassOf<UNetworkMOBAStateWidget> MOBAStateWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Network|UI")
	UNetworkMOBAStateWidget* MOBAStateWidget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|UI")
	TSubclassOf<UNetworkDeathmatchGameOverWidget> DeathmatchGameOverWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Network|UI")
	UNetworkDeathmatchGameOverWidget* DeathmatchGameOverWidget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|UI")
	TSubclassOf<UNetworkJoinMessageWidget> JoinMessageWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Network|UI")
	UNetworkJoinMessageWidget* JoinMessageWidget = nullptr;

private:
	UPROPERTY()
	ANetworkDeathmatchGameState* BoundDeathmatchGameState = nullptr;

	UPROPERTY()
	ANetworkMOBAGameState* BoundMOBAGameState = nullptr;

	FTimerHandle NetworkScoreUIRetryTimerHandle;
	int32 NetworkScoreUIRetryCount = 0;

	UFUNCTION()
	void HandleDeathmatchScoreStateChanged();

	UFUNCTION()
	void HandleMOBAStateChanged();

	void BindDeathmatchScoreState(ANetworkDeathmatchGameState* DeathmatchGameState);
	void UnbindDeathmatchScoreState();
	void BindMOBAState(ANetworkMOBAGameState* MOBAGameState);
	void UnbindMOBAState();
	void RemoveScoreWidgets();
};
