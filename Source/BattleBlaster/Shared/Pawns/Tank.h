#pragma once

#include "CoreMinimal.h"
#include "Shared/Pawns/BasePawn.h"
#include "Shared/UI/HUDWidget.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "EnhancedInputComponent.h"
#include "Shared/Buffs/TankBuffComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Tank.generated.h"

// 前向声明
class UInputMappingContext;
class UCameraComponent;
class UUserWidget;
class ATankPlayerController;
class UHealthComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnTankDeathSignature,
	class ATank*, DeadTank,
	class ATank*, KillerTank
);

UCLASS()
class BATTLEBLASTER_API ATank : public ABasePawn
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	ATank();
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void Tick(float DeltaTime) override;
	virtual void HandleDestruction() override;
	virtual void Fire() override;
	virtual UPawnMovementComponent* GetMovementComponent() const override;

	// ==============================================
	// 1. 核心组件 (Core Components)
	// ==============================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UTankBuffComponent* BuffComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UCameraComponent* CameraComp;

	/** 炮管开镜相机，挂载到炮管/发射点 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Aim")
	UCameraComponent* ScopeCameraComp;

	/** 移动组件（支持 AI 控制器 MoveTo） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Movement")
	UFloatingPawnMovement* PawnMovementComponent;

	// ==============================================
	// 2. 输入系统 (Input)
	// ==============================================
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	int32 MappingPriority = 0;

	UPROPERTY(EditAnywhere, Category = "Input|Movement")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input|Movement")
	UInputAction* TurnAction;

	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	UInputAction* FireAction;

	UPROPERTY(EditAnywhere, Category = "Input|Combat")
	UInputAction* TurretTurnAction;

	UPROPERTY(EditAnywhere, Category = "Input|Aim")
	UInputAction* IA_Aim_Toggle;

	UPROPERTY(EditAnywhere, Category = "Input|Aim")
	UInputAction* IA_Aim_Hold;

	// ==============================================
	// 3. 相机与瞄准 (Camera & Aim)
	// ==============================================
	/** 键鼠点击切换：开镜状态（松开后保持） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aim")
	bool bAimToggleOn = false;

	/** 手柄按住：扳机按下状态 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aim")
	bool bAimHoldPressed = false;

	/** 当前是否处于开镜状态 (bAimToggleOn || bAimHoldPressed) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aim")
	bool bIsAiming = false;

	/** 开镜时显示的 UMG 瞄准镜 UI */
	UPROPERTY(EditAnywhere, Category = "Aim|UI")
	TSubclassOf<UUserWidget> ScopeWidgetClass;

	UPROPERTY()
	UUserWidget* ScopeWidgetInstance = nullptr;

	// ==============================================
	// 4. 移动系统 (Movement)
	// ==============================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float Speed = 450.0f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float TurnRate = 90.0f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float CurrentTurnSpeed = 0.0f;

	/** 贴墙滑动：避免轻微擦墙完全卡死 */
	UPROPERTY(EditAnywhere, Category = "Movement|Feel")
	bool bEnableWallSlide = true;

	/** 贴墙滑动时的速度比例 (0-1) */
	UPROPERTY(EditAnywhere, Category = "Movement|Feel", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WallSlideSpeedScale = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Correction")
	float ClientCorrectionInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Correction")
	float ClientCorrectionDistanceThreshold = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Correction")
	float ClientCorrectionAngleThreshold = 30.0f;

	// ==============================================
	// 5. 战斗系统 (Combat)
	// ==============================================
	UPROPERTY(EditAnywhere, Category = "Combat")
	int32 MaxAmmo = 40;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	int32 CurrentAmmo = 20;

	UPROPERTY(EditAnywhere, Category = "Combat|Reward")
	int32 AmmoReward = 10;

	UPROPERTY(EditAnywhere, Category = "Combat|Reward")
	float HealthReward = 25.0f;

	/** 记录上次被谁打了（追溯击杀者） */
	UPROPERTY()
	AActor* LastHitPlayer = nullptr;

	/** 记录最后一次挨打的时间 */
	float LastHitTime = 0.0f;

	// ==============================================
	// 6. Buff 系统 (Buffs)
	// ==============================================
	/** 记录坦克的初始原始移速 */
	UPROPERTY()
	float BaseSpeed;

	/** 标记目前是否有无限子弹 Buff */
	UPROPERTY()
	bool bHasInfiniteAmmo = false;

	/** 开无限子弹前缓存的真实子弹数量 */
	UPROPERTY()
	int32 CachedAmmo = 0;

	/** 标记当前是否有伤害翻倍 Buff */
	UPROPERTY()
	bool bHasDamageBoost = false;

	/** 是否拥有"子弹穿透" Buff */
	UPROPERTY()
	bool bHasBulletPierce = false;

	/** 是否拥有"双发弹道" Buff */
	UPROPERTY()
	bool bHasDoubleShot = false;

	/** 是否处于"穿墙" Buff 状态 */
	UPROPERTY()
	bool bIsGhostMode = false;

	// ==============================================
	// 8. 团队与 AI (Team & AI)
	// ==============================================
	/** 比赛槽位缓存。PlayerState::SlotId 是权威数据源。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Team")
	int32 SlotId = -1;

	/** 队伍/阵营缓存。PlayerState::TeamId 是权威数据源。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Team")
	int32 TeamId = -1;

	// ==============================================
	// 9. 状态与控制 (State & Control)
	// ==============================================
	/** Tank Pawn 的存活状态（与 PlayerState::IsAlive 保持同步） */
	bool IsAlive = true;

	/** 玩家控制器缓存（通过 GetTankPlayerController() 获取，不直接存储） */
	UPROPERTY()
	ATankPlayerController* TankPC;

	// 声明两个处理函数，准备与HealthComponent绑定
	UFUNCTION()
	void HandleHealthChanged(UHealthComponent* InHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
	void HandleDeath(UHealthComponent* InHealthComp, class AController* InstigatedBy, AActor* DamageCauser);

	// 公共接口 (Public API)
	// ==============================================

	// Buff 组件获取
	UTankBuffComponent* GetBuffComponent() const
	{
		return FindComponentByClass<UTankBuffComponent>();
	}

	// ========== PlayerState 数据访问（跨 Pawn 保留）==========
	// 获取比赛槽位（从 PlayerState 读取）
	int32 GetSlotId() const;

	// 获取队伍/阵营 ID（从 PlayerState 读取）
	int32 GetTeamId() const;

	// 获取存活状态（从 PlayerState 读取）
	bool GetIsAlive() const;

	// 设置存活状态（同步到 PlayerState）
	void SetIsAlive(bool bAlive);

	// 获取出生点位置（从 PlayerState 读取）
	FVector GetHomeSpawnLocation() const;

	// 获取出生点旋转（从 PlayerState 读取）
	FRotator GetHomeSpawnRotation() const;

	// 检查是否有出生点（从 PlayerState 读取）
	bool HasSpawnLocation() const;

	// 获取弹药（从 PlayerState 读取）
	int32 GetAmmo() const;

	// 设置弹药（同步到 PlayerState）
	void SetAmmo(int32 NewAmmo);

	// 获取 PlayerController（缓存版本）
	ATankPlayerController* GetTankPlayerController() const
	{
		return TankPC;
	}

	// 输入响应
	void MoveInput(const FInputActionValue& Value);
	void TurnInput(const FInputActionValue& Value);
	void TurretTurnInput(const FInputActionValue& Value);

	void ApplyMoveInput(float InputValue);
	void ApplyMoveInput(float InputValue, float DeltaSeconds);
	void ApplyTurnInput(float InputValue);
	void ApplyTurnInput(float InputValue, float DeltaSeconds);
	void ApplyTurretTurnInput(float InputValue);
	void ApplyTurretTurnInput(float InputValue, float DeltaSeconds);
	void ApplyFire(const FTransform& RequestedMuzzleTransform);
	FTransform GetValidatedFireTransform(const FTransform& RequestedMuzzleTransform) const;

	UFUNCTION(Server, Unreliable)
	void ServerMoveInput(float InputValue, float ClientDeltaSeconds);

	UFUNCTION(Server, Unreliable)
	void ServerTurnInput(float InputValue, float ClientDeltaSeconds);

	UFUNCTION(Server, Unreliable)
	void ServerTurretTurnInput(float InputValue, float ClientDeltaSeconds);

	UFUNCTION(Server, Reliable)
	void ServerFire(FTransform RequestedMuzzleTransform);

	UFUNCTION(Client, Unreliable)
	void ClientCorrectTankTransform(FVector ServerLocation, FRotator ServerRotation, FRotator ServerTurretRotation);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastHandleDestruction();

	void OnAimToggle(const FInputActionValue& Value);
	void OnAimHoldStarted(const FInputActionValue& Value);
	void OnAimHoldCompleted(const FInputActionValue& Value);

	// 瞄准与相机
	void UpdateAimView();

	// 移动（AI 专用）
	void MoveAI(const FVector2D& MoveInput);
	void MoveWithAI(float ForwardInput, float RightInput);

	// 战斗与奖励
	void SetPlayerEnabled(bool Enabled);
	void HandleKillReward();

	// 团队与 AI
	void SetSlotId(int32 NewSlotId);
	void SetTeamId(int32 NewTeamId);
	void NotifyAttacked(AActor* Attacker);

	// ========== 死亡与结算 ==========
	// 处理Tank死亡并返回凶手Tank指针（供 GameMode 后续处理胜负/阵营判定）
	ATank* ExecuteDeathAndReturnKiller();

	// Tank 死亡广播委托（GameMode 监听此委托处理胜负判定）
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnTankDeathSignature OnKilled;

private:
	// 【缓存】最近一次死亡的凶手 Tank（由 HandleDeath 设置，ExecuteDeathAndReturnKiller 读取）
	UPROPERTY()
	class ATank* CachedKiller;

	float ClientCorrectionAccumulator = 0.0f;
};
