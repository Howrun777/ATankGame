// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TankPlayerState.h"
#include "TankStagePlayerState.generated.h"

/**
 * TankStagePlayerState - 单人闯关(TankStageGameMode)专用玩家状态
 * 继承自TankPlayerState基类，添加单人模式特有的生命和关卡信息
 */
UCLASS()
class BATTLEBLASTER_API ATankStagePlayerState : public ATankPlayerState
{
	GENERATED_BODY()

public:
	ATankStagePlayerState();

	// ================= 单人闯关模式特有状态 =================

	// 剩余生命次数
	UPROPERTY(VisibleAnywhere, Category = "Stage Info")
	int32 RemainingLives;

	// 当前关卡得分
	UPROPERTY(VisibleAnywhere, Category = "Stage Info")
	int32 StageScore;

	// 本关卡击杀敌人总数
	UPROPERTY(VisibleAnywhere, Category = "Stage Info")
	int32 EnemyKillCount;

	// 通关时间（秒）
	UPROPERTY(VisibleAnywhere, Category = "Stage Info")
	float StageTime;

	// 是否处于无敌状态
	UPROPERTY(VisibleAnywhere, Category = "Battle State")
	bool bIsInvincible;

	// 当前关卡ID
	UPROPERTY(VisibleAnywhere, Category = "Stage Info")
	int32 CurrentStageId;

	// ================= 单人闯关模式特有函数 =================

	// 使用一条命
	void UseLife() { RemainingLives = FMath::Max(0, RemainingLives - 1); }

	// 获取剩余生命
	int32 GetRemainingLives() const { return RemainingLives; }

	// 添加分数
	void AddStageScore(int32 Points) { StageScore += Points; }

	// 添加击杀
	void AddEnemyKill() { EnemyKillCount++; Super::AddKill(); }

	// 设置无敌状态
	void SetInvincible(bool bInvincible) { bIsInvincible = bInvincible; }

	// 获取是否无敌
	bool IsInvincible() const { return bIsInvincible; }

	// 重置状态（用于新游戏）
	virtual void ResetForNewGame() override;

protected:
	// PVE 模式：无人头奖励，无需阵营计分
	virtual void HandleKillConfirmed(ATank* Victim) override;
};
