#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Shared/State/TankGameState.h"
#include "Modes/Stage/TankStageGameState.h"
#include "Shared/State/TankPlayerState.h"
#include "Modes/Stage/TankStagePlayerState.h"
#include "Shared/Pawns/Tank.h"
#include "Shared/UI/ScreenMessage.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include "TankStageGameMode.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

class UTankStageOverWidget;

class APlayerStart;

UCLASS()
class BATTLEBLASTER_API ATankStageGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ATankStageGameMode();

protected:
	virtual void BeginPlay() override;

public:
	// 坦克死亡事件处理（绑定到 ATank::OnKilled 委托）
	// ATank::HandleDeath 已完成表现层销毁（HandleDestruction）
	// ATankPlayerState::ProcessDeath 已完成 KDA 结算（无操作，PVE模式）
	// 本方法只负责 PVE 复活流程管理
	UFUNCTION()
	void HandleTankKilled(class ATank* DeadTank, class ATank* KillerTank);

	// 塔死亡事件处理（绑定到 ATower::HealthComp->OnDeath 委托）
	UFUNCTION()
	void HandleTowerDestroyed(class UHealthComponent* InHealthComp, class AController* InstigatedBy, AActor* DamageCauser);

	// Respawn player at current spawn point
	void RespawnPlayer();

	// Get current player tank
	ATank* GetPlayerTank() const { return _PlayerTank; }

	// 用于 WBP_PassWidget 显示机会次数（爱心）
	int32 GetMaxDeathCount() const;
	int32 GetRemainingLives() const;

	// Game over delay
	UPROPERTY(EditAnywhere, Category = "Game Setup")
	float GameOverDelay = 3.0f;

	// Countdown delay before game starts
	UPROPERTY(EditAnywhere, Category = "Game Setup")
	int32 CountdownDelay = 3;

	// Respawn delay after player death
	UPROPERTY(EditAnywhere, Category = "Game Setup")
	float RespawnDelay = 2.0f;

	// Maximum death count before game over
	UPROPERTY(EditAnywhere, Category = "Game Setup")
	int32 MaxDeathCount = 3;

	// 无敌时间（复活后）
	UPROPERTY(EditDefaultsOnly, Category = "Game Setup")
	float InvincibleTime = 3.0f;

	// 复活时生命值百分比 (0.0-1.0)
	UPROPERTY(EditAnywhere, Category = "Respawn", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RespawnHealthPercent = 0.5f;

	// 复活时弹药百分比 (0.0-1.0)
	UPROPERTY(EditAnywhere, Category = "Respawn", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RespawnAmmoPercent = 0.5f;

	// 复活 Niagara 特效
	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	UNiagaraSystem* RespawnNiagaraVFX;

	// Current player death count
	UPROPERTY(VisibleAnywhere, Category = "Game Stats")
	int32 PlayerDeathCount = 0;

private:
	ATank* _PlayerTank;
	int32 TowerCount;           // 剩余塔楼数量
	int32 CountdownSeconds;
	bool IsVictory = false;

	// Player spawn point (respawn here after death)
	AActor* CurrentPlayerStart = nullptr;
	// Player tank class (for respawn)
	TSubclassOf<ATank> CurrentTankClass;

	// Control variables
	bool PlayerCanControl = false;
	bool HasShownControlMessage = false;

	FTimerHandle CountdownTimerHandle;
	FTimerHandle EndTimerHandle;
	FTimerHandle RespawnTimerHandle;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UScreenMessage> ScreenMessageClass;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UTankStageOverWidget> GameOverWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Game Setup")
	TSubclassOf<class ATank> TankClass;

	UScreenMessage* ScreenMessageWidget;
	UTankStageOverWidget* GameOverWidget;

	// 本局开始时间（用于计算游戏时长）
	float GameStartTime = 0.0f;

	// Check if all towers are destroyed
	void CheckVictoryCondition();

	// Select random player start
	AActor* SelectRandomPlayerStart();

	// Cache selected tank class from GameInstance
	void CacheSelectedTankClass();

	// Apply difficulty multiplier to all towers in level
	void ApplyDifficultyToTowers();

	void OnCountdownTimerTimeout();
	void OnGameOverTimerTimeOut();
	void OnRespawnTimerTimeout();
	void HandleGameOver();
	void ReturnToSinglePlayerMenu();

	// 保存玩家状态（过关时调用）
	void SavePlayerStateBeforeLevelEnd();

	// 应用玩家携带状态（进入新关卡时调用）
	void ApplyPlayerCarryState();

	// 启用无敌并播放复活特效
	void EnableInvincibilityAndVFX(ATank* Tank);

	// 结束无敌
	void EndInvincibility(ATank* Tank, UNiagaraComponent* SpawnedSystem);
};
