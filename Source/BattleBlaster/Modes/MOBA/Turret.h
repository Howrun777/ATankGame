// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shared/World/DestructibleProp.h"
#include "Turret.generated.h"

class ATankMOBAGameState;
class ATurretProjectile;
class USoundBase;

/**
 * Turret - 防御塔基类
 * 继承自DestructibleProp，添加了攻击功能
 */
UCLASS()
class BATTLEBLASTER_API ATurret : public ADestructibleProp
{
	GENERATED_BODY()

public:
	ATurret();

	// ================= 阵营 =================

	// 所属阵营索引 (0=红色,1=蓝色,2=绿色,3=黄色)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA Turret")
	int32 CampIndex;

	// 是否为主防御塔（CoreTurret）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA Turret")
	bool bIsCoreTurret;

	// ================= 伤害免疫 =================

	// 主防御塔是否处于伤害免疫状态
	UFUNCTION(BlueprintCallable, Category = "MOBA Turret")
	bool IsDamageImmune() const;

	// 检查并更新伤害免疫状态
	UFUNCTION(BlueprintCallable, Category = "MOBA Turret")
	void UpdateDamageImmunity();

	// 覆盖伤害处理，实现伤害免疫和治疗
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	// ================= 攻击参数 =================

	// 攻击检测间隔（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA Turret")
	float AttackInterval;

	// 视野范围（攻击范围）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA Turret", meta = (ClampMin = "100", ClampMax = "5000"))
	float VisionRadius;

	// 子弹/追踪弹速度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA Turret")
	float ProjectileSpeed;

	// 每次攻击的伤害值
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA Turret")
	float AttackDamage;

	// 蓝图可配置：己方Tank攻击时的治疗百分比
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA Turret")
	float HealPercent;

	// ================= 发射点 =================

	// 发射点组件（可拖拽调整位置）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA Turret")
	class USceneComponent* MuzzlePoint;

	// 发射点相对于Actor的偏移位置（用于蓝图调整）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA Turret")
	FVector MuzzleOffset;

	// ================= 攻击范围显示 =================

	// 是否在编辑器中显示攻击范围
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA Turret|Visualization")
	bool bShowVisionRangeInEditor;

	// 是否在游戏中显示攻击范围（通过按键切换）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA Turret|Visualization")
	bool bCanToggleVisionRangeInGame;

	// 攻击范围显示组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MOBA Turret|Visualization")
	class USphereComponent* VisionRangeComp;

	// 视野范围显示材质（可选，用于更清晰的显示）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA Turret|Visualization")
	UMaterialInterface* VisionRangeMaterial;

	// 切换攻击范围显示（游戏中调用）
	UFUNCTION(BlueprintCallable, Category = "MOBA Turret|Visualization")
	void ToggleVisionRange();

	// 设置攻击范围显示
	UFUNCTION(BlueprintCallable, Category = "MOBA Turret|Visualization")
	void SetVisionRangeVisible(bool bVisible);

	// ================= 攻击状态 =================

	// 当前是否正在攻击
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MOBA Turret")
	bool bIsAttacking;

	// 当前锁定的目标
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MOBA Turret")
	AActor* CurrentTarget;

	// ================= 攻击函数 =================

	// 开启攻击
	UFUNCTION(BlueprintCallable, Category = "MOBA Turret")
	void StartAttacking();

	// 停止攻击
	UFUNCTION(BlueprintCallable, Category = "MOBA Turret")
	void StopAttacking();

	// 定时检测并攻击
	UFUNCTION()
	void DetectAndAttack();

	// 发射投射物
	UFUNCTION(BlueprintCallable, Category = "MOBA Turret")
	void FireProjectile();

	// 获取发射点位置
	FVector GetMuzzleLocation() const;

	// 设置目标
	void SetTarget(AActor* NewTarget);

	// 检查目标是否在攻击
	bool CanAttackTarget(AActor* Target) const;

	// 检查是否应该攻击目标（阵营判断）
	bool ShouldAttackTarget(AActor* Target) const;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	// 攻击定时器句柄
	FTimerHandle AttackTimerHandle;

	// 游戏状态引用
	ATankMOBAGameState* MOBAGameState;

	// 蓝图可配置：投射物类
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA Turret")
	TSubclassOf<ATurretProjectile> ProjectileClass;

	// 蓝图可配置：发射点Socket名称（备选方案）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA Turret")
	FName ProjectileMuzzleSocket;

	// 初始化视野范围显示
	void InitializeVisionRangeVisualization();

	// Resolve the MOBA camp for a target. Returns -1 while the target has no stable camp yet.
	int32 ResolveTargetCampIndex(AActor* Target) const;

	// 更新发射点位置
	void UpdateMuzzleLocation();

	// 覆盖破坏处理，通知游戏状态
	virtual void HandleDestruction() override;

	// ================= 废墟与音效（蓝图可配置） =================

	// 普通炮塔（bIsCoreTurret=false）被摧毁后替换的废墟网格体
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA Turret|Destruction")
	UStaticMesh* TurretRuinsMesh;

	// 核心炮塔（bIsCoreTurret=true）被摧毁后替换的废墟网格体
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA Turret|Destruction")
	UStaticMesh* CoreTurretRuinsMesh;

	// 炮塔销毁时播放的音效
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA Turret|Destruction")
	USoundBase* DestructionSound;

	// 死亡时播放的 Niagara 特效（可为空）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA Turret|Destruction")
	class UNiagaraSystem* DeathEffect;

	// Niagara 特效的生成位置（可拖拽调整）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA Turret|Destruction")
	class USceneComponent* DeathEffectSpawnPoint;
};
