// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shared/State/TankGameState.h"
#include "TankBattleGameState.generated.h"

/**
 * TankBattleGameState - 多人死斗(BattleBlasterGameMode)专用游戏状态
 * 继承自TankGameState基类，管理死斗模式的游戏状态
 */
UCLASS()
class BATTLEBLASTER_API ATankBattleGameState : public ATankGameState
{
	GENERATED_BODY()

public:
	ATankBattleGameState();

	// ================= 死斗模式特有状态 =================

	// 获胜者索引 (-1:进行中, 0:P0胜, 1:P1胜...)
	UPROPERTY(VisibleAnywhere, Category = "Battle Info")
	int32 WinnerIndex;

	// 目标获胜分数
	UPROPERTY(VisibleAnywhere, Category = "Battle Info")
	int32 TargetScore;

	// 玩家数量
	UPROPERTY(VisibleAnywhere, Category = "Battle Info")
	int32 PlayerCount;

	// 游戏结束延迟（秒）
	UPROPERTY(VisibleAnywhere, Category = "Battle Info")
	float GameOverDelay;

	// ================= 玩家数据 (从GameMode迁移) =================

	// 玩家分数数组
	UPROPERTY(VisibleAnywhere, Category = "Player Data")
	TArray<int32> PlayerScores;

	// ================= 死斗模式特有函数 =================

	// 设置获胜者
	void SetWinner(int32 InWinnerIndex) { WinnerIndex = InWinnerIndex; }

	// 获取获胜者索引
	int32 GetWinnerIndex() const { return WinnerIndex; }

	// 检查是否有获胜者
	bool HasWinner() const { return WinnerIndex >= 0; }

	// 获取目标分数
	int32 GetTargetScore() const { return TargetScore; }

	// 设置目标分数
	void SetTargetScore(int32 Score) { TargetScore = Score; }

	// 重置状态（用于新游戏）
	virtual void ResetForNewGame() override;

	// ================= 数据操作函数 =================

	// 初始化玩家数据数组
	void InitializePlayerData(int32 InPlayerCount, int32 InTargetScore);

	// 增加玩家分数
	void AddPlayerScore(int32 SlotId, int32 Amount = 1);

	// 获取玩家分数
	int32 GetPlayerScore(int32 SlotId) const;
};
