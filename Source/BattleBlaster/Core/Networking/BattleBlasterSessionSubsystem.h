#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BattleBlasterSessionSubsystem.generated.h"

UCLASS()
class BATTLEBLASTER_API UBattleBlasterSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Network|Session")
	void HostListenServer(FName MapName);

	UFUNCTION(BlueprintCallable, Category = "Network|Session")
	void JoinByIp(const FString& Address);
};
