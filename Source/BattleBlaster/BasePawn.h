
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/CapsuleComponent.h"
#include "Projectile.h"
#include "HealthComponent.h"

#include "NiagaraFunctionLibrary.h"//粒子特效

#include "BasePawn.generated.h"

UCLASS()
class BATTLEBLASTER_API ABasePawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ABasePawn();

	UPROPERTY(EditAnywhere, Category = "Combat")
	float Fire_Interval = 0.1f;
	UPROPERTY(VisibleAnywhere)
	float Fire_LastTime;

	UPROPERTY(VisibleAnywhere)
	UCapsuleComponent* CapsuleComp;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* BaseMesh;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* TurretMesh;

	UPROPERTY(VisibleAnywhere)
	USceneComponent* ProjectileSpawnPoint;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere)
	UNiagaraSystem* DeathParticles;

	UPROPERTY(EditAnywhere)
	USoundBase* DeathSound;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UCameraShakeBase> DeathCameraShakeClass;

	// 【新增】生命值组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UHealthComponent* HealthComp;

	/**
 * 基类Pawn的炮塔旋转核心函数
 * 功能：计算炮塔指向目标位置的旋转角度，并让炮塔网格体仅绕Z轴（水平方向）旋转朝向目标
 * 适用场景：坦克/炮台等需要炮塔水平瞄准目标的Pawn类
 * @param LookAtTarget 目标位置的世界空间坐标（如鼠标点击点、敌方位置）
	*/
	void RotateTurret(FVector LookAtTarget);

	virtual void Fire();

	virtual void HandleDestruction();
};

