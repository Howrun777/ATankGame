// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AIBotPlayerController.generated.h"

class ATank;
class ATower;
class ATurret;

// ================= AI 战斗状态机 =================
UENUM(BlueprintType)
enum class EAICombatState : uint8
{
	Idle          UMETA(DisplayName = "Idle"),           // 空闲/巡逻
	Chase         UMETA(DisplayName = "Chase"),          // 追击目标
	Strafe        UMETA(DisplayName = "Strafe"),         // 侧翼机动（环绕目标移动）
	KeepDistance  UMETA(DisplayName = "KeepDistance"),   // 保持距离
	Flee          UMETA(DisplayName = "Flee"),            // 逃跑/规避
	TakeCover     UMETA(DisplayName = "TakeCover"),       // 寻找掩体
	Ambush        UMETA(DisplayName = "Ambush")           // 伏击/隐藏
};

// ================= 战术移动类型 =================
UENUM(BlueprintType)
enum class ETacticalMoveType : uint8
{
	None          UMETA(DisplayName = "None"),
	CircleLeft    UMETA(DisplayName = "CircleLeft"),     // 向左环绕
	CircleRight   UMETA(DisplayName = "CircleRight"),    // 向右环绕
	ForwardStrafe UMETA(DisplayName = "ForwardStrafe"),  // 前进侧滑
	BackwardStrafe UMETA(DisplayName = "BackwardStrafe"),// 后退侧滑
	RandomStrafe  UMETA(DisplayName = "RandomStrafe")    // 随机侧滑
};

// ================= AI 难度等级 =================
UENUM(BlueprintType)
enum class EAIDifficulty : uint8
{
	Easy      UMETA(DisplayName = "Easy (简单)"),
	Normal    UMETA(DisplayName = "Normal (普通)"),
	Hard      UMETA(DisplayName = "Hard (困难)"),
	Insane    UMETA(DisplayName = "Insane (噩梦/无上限)")
};


UCLASS()
class BATTLEBLASTER_API AAIBotPlayerController : public AAIController
{
	GENERATED_BODY()

public:
	// 当前 AI 难度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Difficulty")
	EAIDifficulty CurrentDifficulty = EAIDifficulty::Insane;

	// 应用难度设置（可以在 BeginPlay 调用，也可以在蓝图里随时调用以动态改难度）
	UFUNCTION(BlueprintCallable, Category = "AI|Difficulty")
	void ApplyDifficultySettings(EAIDifficulty NewDifficulty);

	// 构造函数
	AAIBotPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// 【核心修复】：监听 AI 附身和脱离事件，保证复活后拿到新的坦克引用
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	// AI 行为
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

	// 尝试寻找最近的敌人并转向
	void RotateTurretTowardsEnemy();

	// 开火
	void AttemptFire();

	// ================= 核心：状态机更新 =================
	void UpdateCombatState();
	void ExecuteCurrentState(float DeltaTime);

	// ================= 战术机动 =================
	void ExecuteTacticalMovement(float DeltaTime);
	ETacticalMoveType ChooseTacticalMove(float DeltaTime);
	void PerformCircleMovement(ETacticalMoveType CircleDir, float DeltaTime);
	void PerformStrafeMovement(ETacticalMoveType StrafeType, float DeltaTime);

	// ================= 威胁评估 =================
	float CalculateTargetThreat(AActor* Target);
	AActor* SelectBestTarget();

	// ================= 阵营检查（团队模式） =================
	bool IsEnemy(AActor* Target) const;

	// ================= 闪避与掩体 =================
	bool ShouldDodge();
	FVector CalculateDodgeDirection();
	void ExecuteDodge();

	// ================= 血量感知行为 =================
	bool ShouldFlee() const;
	bool ShouldBeAggressive() const;
	FVector GetFleeDestination();

	// ================= 预测瞄准 =================
	FVector CalculatePredictedAimPoint();
	bool IsAimingAtTarget() const;

	// ================= 辅助函数 =================
	float GetDistanceToTarget() const;
	float GetAngleToTarget() const;
	bool HasLineOfSightToTarget();

	// ================= 目标刷新（按固定频率） =================
	void RefreshTargetFromAttackList();

public:
	// 随机移动的目标方向
	FVector2D TargetMoveInput = FVector2D::ZeroVector;

	// 下一次改变移动方向的时间
	float NextDirectionChangeTime = 0.0f;

	// 射击间隔
	float FireInterval = 2.0f;
	float NextFireTime = 0.0f;

	// 坦克引用
	UPROPERTY()
	ATank* ControlledTank = nullptr;

	// AI 行为参数
	float DirectionChangeInterval = 2.0f;
	float MinFireInterval = 1.0f;
	float MaxFireInterval = 3.0f;

	// ================= AI 目标追击与攻击参数（暴露到蓝图） =================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float AttackRange = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float AIFireRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float ChaseSpeedScale = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float StopChaseDistance = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float TurretTurnSpeed = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat", meta = (ClampMin = "0.0", ClampMax = "45.0"))
	float AimErrorAngle = 10.0f;

	// 目标 Actor（用于 MoveToActor）
	UPROPERTY()
	AActor* CurrentTarget = nullptr;

	// ================= 攻击列表机制 =================
	UPROPERTY()
	TArray<AActor*> AttackTargetList;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Combat", meta = (DisplayName = "攻击列表过滤器类型"))
	TArray<TSubclassOf<AActor>> AttackFilterTypes;

	UFUNCTION(BlueprintCallable, Category = "AI|Combat")
	void AddTargetToAttackList(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "AI|Combat")
	void RemoveTargetFromAttackList(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "AI|Combat")
	void OnAttackedBy(AActor* Attacker);

	UFUNCTION(BlueprintCallable, Category = "AI|Combat")
	AActor* SelectNearestTargetFromList();

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetTarget(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void StopChasing();

	UFUNCTION(BlueprintCallable, Category = "AI")
	void ResetAIState();

	// ================= 新增：高级AI参数 =================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Advanced")
	EAICombatState CurrentCombatState = EAICombatState::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Advanced", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FleeHealthThreshold = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Advanced", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AggressiveHealthThreshold = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Advanced")
	float MinKeepDistance = 300.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Advanced")
	float MaxKeepDistance = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Advanced")
	float StrafeRadius = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Advanced")
	float StrafeSpeed = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Advanced", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DodgeChance = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Advanced")
	bool bEnablePredictiveAiming = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Advanced")
	float PredictionTime = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Advanced")
	float StateChangeCooldown = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Advanced")
	bool bEnableTacticalMovement = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Advanced")
	bool bEnableDodge = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Advanced")
	bool bPrioritizeLowHealthTargets = true;

	FVector LastKnownThreatDirection = FVector::ZeroVector;
	float LastThreatUpdateTime = 0.0f;

	ETacticalMoveType CurrentTacticalMove = ETacticalMoveType::None;

	float TacticalMoveDuration = 0.0f;
	float CurrentTacticalMoveTime = 0.0f;

	int32 ConsecutiveDodgeCount = 0;
	float LastDodgeTime = 0.0f;

	float AggressionLevel = 0.0f;
	float AggressionIncreaseRate = 0.05f;

	float LastStateChangeTime = 0.0f;

private:
	AActor* FindNearestTarget();
	bool PassesFilter(AActor* Actor) const;
	void StartChase(AActor* Target);

	bool bIsChasing = false;

	FAIRequestID CurrentMoveRequestID;

	FVector PreviousTargetLocation;
	float PreviousTargetUpdateTime;

	FVector EstimatedTargetVelocity;

	FTimerHandle TargetQueryTimerHandle;

	UPROPERTY(EditAnywhere, Category = "AI|Combat", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float TargetQueryInterval = 0.5f;
};