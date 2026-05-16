#pragma once
#include "CoreMinimal.h"
#include "Shared/World/DestructibleProp.h"
#include "ExplosiveBarrel.generated.h"

class UNiagaraSystem;
class USoundBase;
class USphereComponent;

UCLASS()
class BATTLEBLASTER_API AExplosiveBarrel : public ADestructibleProp
{
	GENERATED_BODY()

public:
	AExplosiveBarrel();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void HandleDestruction() override;
	// 【核心控制】伤害衰减系数
    // 0.0 = 完全无衰减，范围内全吃满伤害
	// 1.0 = 线性衰减（默认，距离越远伤害匀速降低）
	// 0.0~1.0 = 衰减变慢，伤害更均匀（半程位置仍能吃到高伤害）
	UPROPERTY(EditAnywhere, Category = "Explosion")
	float DamageFalloff = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Explosion")
	float ExplosionRadius = 150.0f; // 爆炸范围

	UPROPERTY(EditAnywhere, Category = "Explosion")
	float ExplosionDamage = 100.0f; // 爆炸伤害

	UPROPERTY(EditAnywhere, Category = "Effects")
	UNiagaraSystem* ExplosionEffect;

	UPROPERTY(EditAnywhere, Category = "Effects")
	USoundBase* ExplosionSound;

	// 爆炸范围可视化球体（编辑器可见，游戏隐藏）
	UPROPERTY(VisibleAnywhere, Category = "Explosion")
	USphereComponent* ExplosionRadiusVisualizer;
};
