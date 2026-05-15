#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TurretProjectile.generated.h"

class UNiagaraSystem;
class USoundBase;
class UNiagaraComponent;

UCLASS()
class BATTLEBLASTER_API ATurretProjectile : public AActor
{
	GENERATED_BODY()

public:
	ATurretProjectile();

	// 初始化投射物
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void InitializeProjectile(AActor* InTargetActor, float InDamage, float InSpeed, int32 InCampIndex);

	// 获取伤害
	float GetDamage() const { return Damage; }

	// 获取阵营索引
	int32 GetCampIndex() const { return CampIndex; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// 击中目标
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// 目标 actor
	UPROPERTY()
	AActor* TargetActor;

	// 伤害值
	UPROPERTY()
	float Damage;

	// 速度
	UPROPERTY()
	float Speed;

	// 阵营索引
	UPROPERTY()
	int32 CampIndex;

	// 存活时间
	float LifeTime;

	// 最大存活时间（5秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float MaxLifeTime;

	// 碰撞组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ProjectileMesh;

	// 移动组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UProjectileMovementComponent* ProjectileMovement;

	// 发射音效
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	USoundBase* LaunchSound;

	// 发射特效（ muzzle flash）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UNiagaraSystem* LaunchEffect;

	// 拖尾特效
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNiagaraComponent* TrailComponent;

	// 击中音效
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	USoundBase* HitSound;

	// 击中特效
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UNiagaraSystem* HitEffect;

};
