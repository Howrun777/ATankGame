// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TankPlayerState.h"
#include "TankBattlePlayerState.generated.h"

/**
 * TankBattlePlayerState - 多人死斗(BattleBlasterGameMode)专用玩家状态
 * 继承自TankPlayerState基类，额外添加一些死斗模式特有的功能
 */
UCLASS()
class BATTLEBLASTER_API ATankBattlePlayerState : public ATankPlayerState
{
	GENERATED_BODY()

public:
	ATankBattlePlayerState();

	// ================= 死斗模式特有状态 =================

	// 是否处于无敌状态
	UPROPERTY(VisibleAnywhere, Category = "Battle State")
	bool bIsInvincible;

	// 复活时间（用于显示等）
	UPROPERTY(VisibleAnywhere, Category = "Battle State")
	float RespawnTime;

	// ================= 死斗模式特有函数 =================

	// 设置无敌状态
	void SetInvincible(bool bInvincible) { bIsInvincible = bInvincible; }

	// 获取是否无敌
	bool IsInvincible() const { return bIsInvincible; }

	// 重置状态（用于新游戏）
	virtual void ResetForNewGame() override;
};
