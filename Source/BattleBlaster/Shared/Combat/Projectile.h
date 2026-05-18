#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"




#include "Projectile.generated.h"

UCLASS()
class BATTLEBLASTER_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProjectile();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ProjectileMesh;

	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMovementComp;

	/* @brief 拖尾粒子效果的组件实例（Niagara粒子组件）
	 *       - UNiagaraComponent是Niagara粒子系统的**实例化组件**，挂载在Actor上用于实时渲染拖尾粒子（如子弹/炮弹拖尾）
	 *       - 该组件与Actor生命周期绑定，是运行时实际显示粒子效果的载体
	 */
	UPROPERTY(VisibleAnywhere)
	UNiagaraComponent* TrailParticles;

	/* @brief 命中粒子效果的模板资源（Niagara粒子系统）
	 *       - UNiagaraSystem是粒子效果的**模板资源**（非实例），本身不渲染，需通过Spawn等方式实例化后才能显示
	 *       - 用于在命中目标（如地面、敌人、物体）时生成对应的粒子效果（如火花、爆炸、烟雾）
	 */
	UPROPERTY(EditAnywhere)
	UNiagaraSystem* HitParticles;

	UPROPERTY(EditAnywhere)
	USoundBase* LaunchSound;

	UPROPERTY(EditAnywhere)
	USoundBase* HitSound;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UCameraShakeBase> HitCameraShakeClass;

	UPROPERTY(EditAnywhere)
	float Damage = 25.0f;

	UPROPERTY(ReplicatedUsing = OnRep_BoostVisuals)
	bool bBoostVisualsEnabled = false;

	UPROPERTY(ReplicatedUsing = OnRep_PierceMode)
	bool bCanPierce = false;
	
	UPROPERTY(EditAnywhere, Replicated, Category = "Combat|Pierce")
	int32 MaxPenetrationCount = -1;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayHitEffects(FVector EffectLocation, FRotator EffectRotation);

	UFUNCTION()
	void OnRep_BoostVisuals();

	UFUNCTION()
	void OnRep_PierceMode();

/*
【核心功能】UPrimitiveComponent 组件"物理碰撞命中（Hit）"事件的回调函数
遵循UE内置的 OnComponentHit 委托签名，当当前Actor的Primitive组件（如胶囊体、静态网格、碰撞体）
与其他Actor/组件发生「实质性物理碰撞」（非触发器/重叠事件）时，UE引擎会自动调用此函数
开发者可在函数内编写碰撞后的业务逻辑（如扣血、播放音效、施加物理力、生成特效等）
【触发前提】
1. 目标Primitive组件需开启 bGenerateHitEvents；
2. 函数需绑定到组件的 OnComponentHit 委托；
3. 碰撞为"阻挡型"（非Trigger）
*/
	UFUNCTION()
	void OnHit(
		UPrimitiveComponent * HitComponent,
		// 【自身碰撞组件】触发本次碰撞的「当前Actor」的Primitive组件（如角色的胶囊体、武器的碰撞体）
		AActor * OtherActor,
		// 【碰撞对方Actor】与自身发生碰撞的「另一方」所属的Actor（可能为空，必须先判空）
		UPrimitiveComponent * OtherComp,
		// 【对方碰撞组件】OtherActor 上实际发生碰撞的Primitive组件（如敌人的静态网格体、场景的碰撞盒）
		FVector NormalImpulse,
		// 【碰撞冲量法线】碰撞产生的冲量方向（单位向量），代表碰撞力的作用方向（用于计算反弹/击退）
		const FHitResult & Hit
		// 【碰撞详细结果】只读结构体，包含碰撞点、表面法线、物理材质、碰撞类型等核心信息
	);

	// ================= 强化版视觉表现 (Damage Boost) =================
	// 强化时的炮弹模型（比如更大、更尖锐的模型）
	UPROPERTY(EditAnywhere, Category = "Combat|Boost")
	UStaticMesh* BoostedProjectileMesh;
	// 强化时的拖尾特效（比如变成红色的粗大尾焰）
	UPROPERTY(EditAnywhere, Category = "Combat|Boost")
	UNiagaraSystem* BoostedTrailParticles;
	// 【新增】：强化时的命中特效
	UPROPERTY(EditAnywhere, Category = "Combat|Boost")
	UNiagaraSystem* BoostedHitParticles;

	// 【新增】：强化时的发射音效
	UPROPERTY(EditAnywhere, Category = "Combat|Boost")
	USoundBase* BoostedLaunchSound;

	// 【新增】：强化时的命中音效
	UPROPERTY(EditAnywhere, Category = "Combat|Boost")
	USoundBase* BoostedHitSound;
	// 供外部（Tank）调用的换装函数
	void EnableBoostVisuals();

	// ================== 子弹穿透相关 ==================
	UPROPERTY()
	int32 CurrentPenetrationCount = 0;
	
	// 启用子弹穿透模式
	void EnablePierceMode(bool bInfinitePierce = true, int32 InMaxPenetrationCount = -1);
	// 设置炮弹生命周期（防止无限存在）
	void SetProjectileLifeSpan(float InLifeSpan);
	
};
