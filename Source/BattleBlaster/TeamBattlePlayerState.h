// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TankPlayerState.h"
#include "TeamBattlePlayerState.generated.h"

/**
 * TeamBattlePlayerState - 团队死斗(TeamBattleGameMode)专用玩家状态
 * 继承自TankPlayerState基类，添加团队相关的阵营信息
 */
UCLASS()
class BATTLEBLASTER_API ATeamBattlePlayerState : public ATankPlayerState
{
	GENERATED_BODY()

public:
	ATeamBattlePlayerState();

	// ================= 团队模式特有状态 =================

	// 阵营定义：0=红色，1=蓝色
	UPROPERTY(VisibleAnywhere, Category = "Team Info")
	uint8 TeamCamp;

	// 个人对团队的贡献分（可选）
	UPROPERTY(VisibleAnywhere, Category = "Team Info")
	int32 TeamScoreContribution;

	// 是否处于无敌状态
	UPROPERTY(VisibleAnywhere, Category = "Battle State")
	bool bIsInvincible;

	// 复活时间
	UPROPERTY(VisibleAnywhere, Category = "Battle State")
	float RespawnTime;

	// ================= 团队模式特有函数 =================

	// 设置阵营
	void SetTeamCamp(uint8 InCamp) { TeamCamp = InCamp; }

	// 获取阵营
	uint8 GetTeamCamp() const { return TeamCamp; }

	// 设置无敌状态
	void SetInvincible(bool bInvincible) { bIsInvincible = bInvincible; }

	// 获取是否无敌
	bool IsInvincible() const { return bIsInvincible; }

	// 重置状态（用于新游戏）
	virtual void ResetForNewGame() override;

protected:
	// 同阵营击杀不加分，跨阵营击杀为凶手阵营 +1
	virtual void HandleKillConfirmed(ATank* Victim) override;
};
