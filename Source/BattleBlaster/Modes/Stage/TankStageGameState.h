// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shared/State/TankGameState.h"
#include "TankStageGameState.generated.h"

/**
 * TankStageGameState - 单人闯关(TankStageGameMode)专用游戏状态
 * 继承自TankGameState基类，管理单人闯关模式的游戏状态
 */
UCLASS()
class BATTLEBLASTER_API ATankStageGameState : public ATankGameState
{
	GENERATED_BODY()

public:
	ATankStageGameState();

	// ================= 单人闯关模式特有状态 =================

	// 当前关卡ID
	UPROPERTY(VisibleAnywhere, Category = "Stage Info")
	int32 CurrentStageId;

	// 剩余塔楼数量
	UPROPERTY(VisibleAnywhere, Category = "Stage Info")
	int32 RemainingTowerCount;

	// 当前波次
	UPROPERTY(VisibleAnywhere, Category = "Stage Info")
	int32 CurrentWave;

	// 总波次数
	UPROPERTY(VisibleAnywhere, Category = "Stage Info")
	int32 TotalWaves;

	// 是否通关
	UPROPERTY(VisibleAnywhere, Category = "Stage Info")
	bool bIsVictory;

	// 游戏结束延迟（秒）
	UPROPERTY(VisibleAnywhere, Category = "Stage Info")
	float GameOverDelay;

	// 初始生命次数
	UPROPERTY(VisibleAnywhere, Category = "Stage Info")
	int32 InitialLives;

	// ================= 单人闯关模式特有函数 =================

	// 设置剩余塔楼数量
	void SetRemainingTowerCount(int32 Count) { RemainingTowerCount = Count; }

	// 获取剩余塔楼数量
	int32 GetRemainingTowerCount() const { return RemainingTowerCount; }

	// 减少塔楼数量
	void DecreaseTowerCount() { RemainingTowerCount = FMath::Max(0, RemainingTowerCount - 1); }

	// 设置当前波次
	void SetCurrentWave(int32 Wave) { CurrentWave = Wave; }

	// 获取当前波次
	int32 GetCurrentWave() const { return CurrentWave; }

	// 设置通关状态
	void SetVictory(bool bVictory) { bIsVictory = bVictory; }

	// 获取是否通关
	bool IsVictory() const { return bIsVictory; }

	// 获取当前关卡ID
	int32 GetCurrentStageId() const { return CurrentStageId; }

	// 设置当前关卡ID
	void SetCurrentStageId(int32 StageId) { CurrentStageId = StageId; }

	// 获取总波次数
	int32 GetTotalWaves() const { return TotalWaves; }

	// 设置总波次数
	void SetTotalWaves(int32 Waves) { TotalWaves = Waves; }

	// 重置状态（用于新游戏）
	virtual void ResetForNewGame() override;
};
