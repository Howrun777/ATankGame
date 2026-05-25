// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shared/State/TankPlayerState.h"
#include "TankMOBAPlayerState.generated.h"

/**
 * TankMOBAPlayerState - MOBA模式的玩家状态
 * 继承自TankPlayerState，管理阵营、死亡、复活等
 */
UCLASS()
class BATTLEBLASTER_API ATankMOBAPlayerState : public ATankPlayerState
{
	GENERATED_BODY()

public:
	ATankMOBAPlayerState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ================= 阵营系统 =================

	// 获取阵营索引 (0=红色,1=蓝色,2=绿色,3=黄色)
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	int32 GetCampIndex() const { return CampIndex; }

	// 设置阵营索引
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	void SetCampIndex(int32 Index)
	{
		CampIndex = Index;
		SetTeamId(Index);
	}

	// 获取阵营颜色（用于UI显示）
	FLinearColor GetCampColor() const;

	// ================= 死亡与复活 =================

	// 是否已死亡
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	bool IsDead() const { return bIsDead; }

	// 设置死亡状态
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	void SetDead(bool bDead) { bIsDead = bDead; }

	// 是否被淘汰（主防御塔被摧毁，无法复活）
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	bool IsEliminated() const { return bIsEliminated; }

	// 设置淘汰状态
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	void SetEliminated(bool bEliminated) { bIsEliminated = bEliminated; }

	// 获取复活剩余时间（秒，精确到0.1）
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	float GetRespawnTimeRemaining() const { return RespawnTimeRemaining; }

	// 设置复活剩余时间
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	void SetRespawnTimeRemaining(float Time) { RespawnTimeRemaining = Time; }

	// 获取当前复活等待时间
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	float GetCurrentRespawnDelay() const { return CurrentRespawnDelay; }

	// 设置当前复活等待时间
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	void SetCurrentRespawnDelay(float Delay) { CurrentRespawnDelay = Delay; }

	// 是否正在等待复活
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	bool IsWaitingForRespawn() const { return bIsWaitingForRespawn; }

	// 设置是否等待复活
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	void SetWaitingForRespawn(bool bWaiting) { bIsWaitingForRespawn = bWaiting; }

	// 获取杀敌数
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	int32 GetTurretDestroyedCount() const { return TurretDestroyedCount; }

	// 增加防御塔摧毁数
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	void AddTurretDestroyed() { TurretDestroyedCount++; }

	// 重置状态（用于新游戏）
	virtual void ResetForNewGame() override;

	// 初始化MOBA状态
	void InitializeMOBAState(int32 InCampIndex);

	// 计算复活延迟（每隔 GrowthInterval 秒增长 GrowthAmount 秒）
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	float CalculateRespawnDelay(float GameTime, float InitialDelay, float MaxDelay, float GrowthInterval, float GrowthAmount) const;

protected:
	// MOBA 模式：每个玩家独立阵营，不存在跨阵营加分，KDA 由基类 ProcessDeath 处理
	virtual void HandleKillConfirmed(ATank* Victim) override;

protected:
	// 阵营索引 (0=红色,1=蓝色,2=绿色,3=黄色)
	UPROPERTY(VisibleAnywhere, Replicated, Category = "MOBA")
	int32 CampIndex;

	// 是否已死亡
	UPROPERTY(VisibleAnywhere, Replicated, Category = "MOBA")
	bool bIsDead;

	// 是否被淘汰（主防御塔被摧毁，无法复活）
	UPROPERTY(VisibleAnywhere, Replicated, Category = "MOBA")
	bool bIsEliminated;

	// 复活剩余时间（秒）
	UPROPERTY(VisibleAnywhere, Replicated, Category = "MOBA")
	float RespawnTimeRemaining;

	// 当前复活等待时间（秒）
	UPROPERTY(VisibleAnywhere, Replicated, Category = "MOBA")
	float CurrentRespawnDelay;

	// 是否正在等待复活
	UPROPERTY(VisibleAnywhere, Replicated, Category = "MOBA")
	bool bIsWaitingForRespawn;

	// 摧毁的防御塔数量
	UPROPERTY(VisibleAnywhere, Replicated, Category = "MOBA")
	int32 TurretDestroyedCount;
};
