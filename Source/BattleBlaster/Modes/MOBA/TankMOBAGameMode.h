// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Modes/MOBA/UI/MOBAGameOverWidget.h"
#include "TankMOBAGameMode.generated.h"

class ATank;
class ATankMOBAGameState;
class ATankMOBAPlayerState;
class ATankPlayerController;
class ATurret;
class UNiagaraSystem;
class USoundBase;
class UBattleBlasterGameInstance;
class UMOBATopStateUI;

UCLASS()
class BATTLEBLASTER_API ATankMOBAGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ATankMOBAGameMode();

	// ================= 配置参数 =================

	// 坦克蓝图类
	UPROPERTY(EditDefaultsOnly, Category = "Game Setup")
	TSubclassOf<class ATank> TankClass;

	// MOBA 顶部状态 UI 类
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UMOBATopStateUI> TopStateUIClass;

	// MOBA 顶部状态 UI 实例
	UPROPERTY()
	UMOBATopStateUI* TopStateUIInstance;

	/** 结算界面延迟（秒），与多人死斗一致 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|MOBA")
	float GameOverDelay = 3.0f;

	/** MOBA 结算 Widget 蓝图（如 WBP_MOBAGameOverWidget） */
	UPROPERTY(EditDefaultsOnly, Category = "UI|MOBA")
	TSubclassOf<UMOBAGameOverWidget> MOBAGameOverWidgetClass;

	UPROPERTY()
	UMOBAGameOverWidget* MOBAGameOverWidgetInstance;

	// ================= 游戏初始化 =================

	// 玩家数量
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MOBA")
	int32 PlayerCount;

	// 目标玩家数量
	int32 TargetPlayerCount = 2;

	// 视口玩家数量（用于分屏）
	int32 ViewportPlayerCount = 0;

	// 实际连接的手柄数量
	int32 ConnectedGamepadCount = 1;

	// 记录每个玩家索引是否被AI控制 (true=AI, false=真实玩家)
	UPROPERTY()
	TArray<bool> bIsPlayerAIControlled;

	// 存储所有活跃的坦克实例
	UPROPERTY()
	TArray<ATank*> ActiveTanks;

	// 存储所有找到的出生点 (Tag: P0, P1, P2...)
	UPROPERTY()
	TArray<AActor*> PlayerStarts;

	// ================= 复活参数（蓝图可调节） =================

	// 初始复活延迟（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA|Respawn")
	float InitialRespawnDelay;

	// 最大复活延迟（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA|Respawn")
	float MaxRespawnDelay;

	// 复活延迟增长间隔（秒，每隔这么久复活延迟增加一次）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA|Respawn")
	float RespawnDelayGrowthInterval;

	// 复活延迟每次增长量（秒，每次增长这么多）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA|Respawn")
	float RespawnDelayGrowthAmount;

	// 复活特效
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA|Respawn")
	UNiagaraSystem* RespawnEffect;

	// 复活特效高度偏移（相对于Tank根组件）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA|Respawn")
	float RespawnEffectHeight;

	// 复活音效
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA|Respawn")
	USoundBase* RespawnSound;

	// ================= 复活属性配置（所有玩家共享） =================

	// 复活后恢复的生命值百分比（0.0-1.0）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA|Respawn")
	float RespawnHealthPercent;

	// 复活后恢复的弹药百分比（0.0-1.0）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA|Respawn")
	float RespawnAmmoPercent;

	// ================= 防御塔攻击/治疗参数（蓝图可调节） =================

	// 攻击防御塔伤害值
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA|Tower")
	float TowerDamage;

	// 攻击己方防御塔治疗值（占伤害的百分比）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MOBA|Tower")
	float TowerHealPercent;

	// ================= 玩家生成 =================

	// 生成玩家Tank
	void HandleStartingNewPlayer(APlayerController* NewPlayer);

	// 获取玩家出生点
	AActor* GetPlayerStartForIndex(int32 SlotId);

	// ================= 玩家死亡与复活 =================

	// 坦克死亡事件处理（绑定到 ATank::OnKilled 委托）
	// KDA 已在 ATankPlayerState::ProcessDeath 内部完成，
	// 本方法只处理：核心塔存活检查 / 复活计时 / 胜负判定 / UI 刷新
	UFUNCTION()
	void HandleTankKilled(class ATank* DeadTank, class ATank* KillerTank);

	// 开始玩家复活计时
	void StartPlayerRespawn(ATankMOBAPlayerState* MOBAState);

	// 复活玩家
	void RespawnPlayer(ATankMOBAPlayerState* MOBAState);

	// 玩家阵亡并进入观战
	void EliminatePlayer(ATankMOBAPlayerState* MOBAState);

	// 检查并更新复活倒计时
	void UpdateRespawnTimers(float DeltaTime);

	// 通知所有玩家防御塔被摧毁
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	void NotifyAllPlayersTowerDestroyed(int32 CampIndex, bool bIsCoreTurret);

	// 通知所有玩家主防御塔被摧毁（某个阵营无法复活）
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	void NotifyAllPlayersCoreDestroyed(int32 CampIndex);

	// 获取当前游戏时间
	float GetCurrentGameTime() const { return MatchTime; }

	// 获取当前活跃的阵营数量
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	int32 GetActiveCampCount() const { return ActiveTanks.Num(); }

	// 隐藏指定阵营的 CoreTurret UI 图片
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	void HideCoreTurretImage(int32 CampIndex);

protected:
	// 游戏时间计时器
	float MatchTime;

	// 游戏时间计时器句柄
	FTimerHandle GameTimerHandle;

	FTimerHandle BindPawnTimerHandle;

	// 复活计时器句柄
	FTimerHandle RespawnTimerHandle;

	// 虚拟 void BeginPlay() override;
	virtual void BeginPlay() override;

	// 虚拟 void Tick(float DeltaTime) override;
	virtual void Tick(float DeltaTime) override;

	// 虚拟 void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 游戏状态
	ATankMOBAGameState* MOBAGameState;

	// 玩家阵营映射（PlayerController索引 -> 阵营索引）
	TMap<int32, int32> PlayerToCampMap;

	// 更新游戏时间
	void UpdateGameTimer();

	// 游戏结束检查
	// 条件：场上只剩1个核心塔 且 除获胜阵营外所有玩家都已被淘汰
	// 【2026-04-01 修订】：只在玩家被淘汰时调用，不再在核心塔被摧毁时直接判定
	void CheckGameOver();

	// 检查所有玩家的淘汰状态
	void CheckAllPlayersEliminated();

	/** 显示 MOBA 结算全屏 UI */
	void ShowMOBAGameOver();

	/** 是否已安排结算界面定时器 */
	bool bMOBAGameOverUIScheduled = false;

	FTimerHandle MOBAGameOverTimerHandle;
};
