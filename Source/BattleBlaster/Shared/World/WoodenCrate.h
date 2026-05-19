#pragma once
#include "CoreMinimal.h"
#include "Shared/World/DestructibleProp.h"
#include "WoodenCrate.generated.h"

class UNiagaraSystem;
class USoundBase;

UCLASS()
class BATTLEBLASTER_API AWoodenCrate : public ADestructibleProp
{
	GENERATED_BODY()

protected:
	virtual void HandleDestruction() override;
	virtual void ApplyDestructionState() override;
	virtual void PlayDestructionEffects(const FVector& EffectLocation) override;

	UPROPERTY(EditAnywhere, Category = "Effects")
	UNiagaraSystem* BreakEffect; // 破碎特效（木屑飞溅）

	UPROPERTY(EditAnywhere, Category = "Effects")
	USoundBase* BreakSound; // 破碎音效

	UPROPERTY(EditAnywhere, Category = "Effects")
	float TimeToDisappear = 3.0f; // 特效播放完后，多久彻底销毁
};
