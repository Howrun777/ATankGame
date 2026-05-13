// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TankGameState.h"
#include "TeamBattleGameState.generated.h"

/**
 * TeamBattleGameState - 团队死斗(TeamBattleGameMode)专用游戏状态
 * 继承自TankGameState基类，管理团队死斗模式的游戏状态
 */
UCLASS()
class BATTLEBLASTER_API ATeamBattleGameState : public ATankGameState
{
	GENERATED_BODY()

public:
	ATeamBattleGameState();

	// ================= 团队模式阵营定义 =================

	enum class ETeamCamp : uint8
	{
		Red = 0,   // 红队
		Blue = 1   // 蓝队
	};

	// ================= 团队模式特有状态 =================

	// 阵营分数：索引0=红色阵营，索引1=蓝色阵营
	UPROPERTY(VisibleAnywhere, Category = "Team Info")
	TArray<int32> TeamScores;

	// 获胜阵营 (-1:进行中, 0:红色胜, 1:蓝色胜)
	UPROPERTY(VisibleAnywhere, Category = "Team Info")
	int32 WinnerCampIndex;

	// 目标获胜分数
	UPROPERTY(VisibleAnywhere, Category = "Team Info")
	int32 TargetScore;

	// 玩家数量
	UPROPERTY(VisibleAnywhere, Category = "Team Info")
	int32 PlayerCount;

	// 游戏结束延迟（秒）
	UPROPERTY(VisibleAnywhere, Category = "Team Info")
	float GameOverDelay;

	// ================= 团队模式特有函数 =================

	// 添加团队分数
	void AddTeamScore(ETeamCamp Camp, int32 Points);

	// 获取团队分数
	int32 GetTeamScore(ETeamCamp Camp) const;

	// 获取红队分数
	int32 GetRedTeamScore() const { return TeamScores.IsValidIndex(0) ? TeamScores[0] : 0; }

	// 获取蓝队分数
	int32 GetBlueTeamScore() const { return TeamScores.IsValidIndex(1) ? TeamScores[1] : 0; }

	// 设置获胜阵营
	void SetWinnerCamp(int32 InWinnerCampIndex) { WinnerCampIndex = InWinnerCampIndex; }

	// 获取获胜阵营
	int32 GetWinnerCampIndex() const { return WinnerCampIndex; }

	// 检查是否有获胜者
	bool HasWinner() const { return WinnerCampIndex >= 0; }

	// 获取目标分数
	int32 GetTargetScore() const { return TargetScore; }

	// 设置目标分数
	void SetTargetScore(int32 Score) { TargetScore = Score; }

	// ================= 数据操作函数 =================

	// 初始化玩家数据
	void InitializePlayerData(int32 InPlayerCount);

	// 重置状态（用于新游戏）
	virtual void ResetForNewGame() override;
};
