// Fill out your copyright notice in the Description page of Project Settings.

#include "TankMOBAPlayerState.h"

ATankMOBAPlayerState::ATankMOBAPlayerState()
{
	CampIndex = 0;
	bIsDead = false;
	bIsEliminated = false;
	RespawnTimeRemaining = 0.0f;
	CurrentRespawnDelay = 2.0f;
	bIsWaitingForRespawn = false;
	TurretDestroyedCount = 0;
}

FLinearColor ATankMOBAPlayerState::GetCampColor() const
{
	switch (CampIndex)
	{
		case 0: return FLinearColor(1.0f, 0.0f, 0.0f); // 红色
		case 1: return FLinearColor(0.0f, 0.0f, 1.0f); // 蓝色
		case 2: return FLinearColor(0.0f, 1.0f, 0.0f); // 绿色
		case 3: return FLinearColor(1.0f, 1.0f, 0.0f); // 黄色
		default: return FLinearColor::White;
	}
}

void ATankMOBAPlayerState::ResetForNewGame()
{
	Super::ResetForNewGame();

	CampIndex = 0;
	bIsDead = false;
	bIsEliminated = false;
	RespawnTimeRemaining = 0.0f;
	CurrentRespawnDelay = 2.0f;
	bIsWaitingForRespawn = false;
	TurretDestroyedCount = 0;
}

void ATankMOBAPlayerState::InitializeMOBAState(int32 InCampIndex)
{
	CampIndex = InCampIndex;
	PlayerIndex = InCampIndex; // 【核心修复】：同时设置 PlayerIndex，HandleTankKilled 通过它来匹配死者
	bIsDead = false;
	bIsEliminated = false;
	RespawnTimeRemaining = 0.0f;
	CurrentRespawnDelay = 2.0f;
	bIsWaitingForRespawn = false;
	TurretDestroyedCount = 0;
}

float ATankMOBAPlayerState::CalculateRespawnDelay(float GameTime, float InitialDelay, float MaxDelay, float GrowthInterval, float GrowthAmount) const
{
	// 每隔 GrowthInterval 秒，复活延迟增长 GrowthAmount 秒
	float GrowthSteps = (GrowthInterval > 0.0f) ? FMath::FloorToFloat(GameTime / GrowthInterval) : 0.0f;
	float NewDelay = InitialDelay + GrowthSteps * GrowthAmount;
	return FMath::Min(NewDelay, MaxDelay);
}

void ATankMOBAPlayerState::HandleKillConfirmed(ATank* Victim)
{
	// MOBA 模式：每个玩家独立阵营（CampIndex = PlayerIndex），
	// 击杀跨阵营玩家不涉及"阵营积分"概念（无团队分数），
	// 所有 KDA 增减已由基类 ProcessDeath 处理，这里不需要额外逻辑。
}
